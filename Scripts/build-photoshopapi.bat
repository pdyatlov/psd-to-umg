@echo off
REM =============================================================================
REM  build-photoshopapi.bat
REM -----------------------------------------------------------------------------
REM  One-time bootstrap: rebuild PhotoshopAPI + transitive deps from source
REM  using CMake + vcpkg, then copy the resulting static libs and public headers
REM  into Source/ThirdParty/PhotoshopAPI/Win64/{lib,include}.
REM
REM  This script is DOCUMENTATION / REBUILD-FROM-SOURCE only. It does NOT run
REM  during normal Unreal builds. A fresh clone of PSD2UMG already ships with
REM  the vendored libs committed under Source/ThirdParty/PhotoshopAPI/Win64/.
REM
REM  Prerequisites:
REM    - Git
REM    - CMake 3.20+
REM    - vcpkg (cloned locally, VCPKG_ROOT env var set)
REM    - Visual Studio 2022 with C++ desktop workload (for MSVC toolchain)
REM
REM  Triplet: x64-windows-static-md
REM    Static libraries, dynamic CRT (/MD) -- matches UE's CRT. Do NOT use
REM    x64-windows-static (that uses /MT and will fail to link against UE).
REM
REM  Usage:
REM    Set VCPKG_ROOT to your vcpkg clone, then run:
REM      Scripts\build-photoshopapi.bat
REM =============================================================================

setlocal enableextensions enabledelayedexpansion

REM If launched directly (double-click), re-launch ourselves piping all output
REM to a log file and a console that stays open. The %PSAPI_LOGGED% guard
REM prevents infinite recursion.
if "%PSAPI_LOGGED%"=="" (
    set "PSAPI_LOG=%~dp0..\build-photoshopapi.log"
    set "PSAPI_LOGGED=1"
    call "%~f0" %* > "!PSAPI_LOG!" 2>&1
    set "RC=!ERRORLEVEL!"
    echo.
    echo === Build script finished with exit code !RC! ===
    echo Full log: !PSAPI_LOG!
    echo.
    echo --- last 40 lines ---
    powershell -NoProfile -Command "Get-Content -Tail 40 '!PSAPI_LOG!'"
    echo.
    pause
    exit /b !RC!
)

REM PhotoshopAPI bundles its own vcpkg as a git submodule, so VCPKG_ROOT is not used.

set "REPO_ROOT=%~dp0.."
set "VENDOR_ROOT=%REPO_ROOT%\Source\ThirdParty\PhotoshopAPI"
set "VENDOR_INCLUDE=%VENDOR_ROOT%\Win64\include"
set "VENDOR_LIB=%VENDOR_ROOT%\Win64\lib"
REM Short work dir to avoid Windows MAX_PATH=260 issues with deeply-nested submodules
REM (PhotoshopAPI -> compressed-image -> pybind11_image_util -> doctest -> ...).
set "WORK_DIR=C:\psapi-build"
set "PSAPI_TAG=master"
REM TODO: pin PSAPI_TAG to a specific release tag once upstream tags stabilize.

echo === PSD2UMG :: PhotoshopAPI bootstrap ===
echo   REPO_ROOT    = %REPO_ROOT%
echo   VCPKG_ROOT   = %VCPKG_ROOT%
echo   WORK_DIR     = %WORK_DIR%
echo   VENDOR_LIB   = %VENDOR_LIB%
echo   VENDOR_INC   = %VENDOR_INCLUDE%
echo.

if exist "%WORK_DIR%" rmdir /s /q "%WORK_DIR%"
mkdir "%WORK_DIR%" || exit /b 1

echo === [1/3] Cloning PhotoshopAPI @ %PSAPI_TAG% (with submodules) ===
REM core.longpaths=true bypasses Windows MAX_PATH=260 inside git checkout/submodule.
git -c core.longpaths=true clone -c core.longpaths=true --recurse-submodules --branch %PSAPI_TAG% https://github.com/EmilDohne/PhotoshopAPI.git "%WORK_DIR%\PhotoshopAPI" || exit /b 1

echo === Applying PSD2UMG patches to cloned source (before build) ===
REM All patches: copy our vendored (already-patched) header over the upstream clone
REM so CMake compiles the patched version into the .lib and robocopy carries it back.

REM SmartObjectLayer.h: cache RGBA channels in constructor so evaluate_channel()
REM returns the PSD's pre-rendered composite instead of throwing when the linked
REM .ai file is absent.
echo   Patching SmartObjectLayer.h...
copy /y "%VENDOR_INCLUDE%\PhotoshopAPI\LayeredFile\LayerTypes\SmartObjectLayer.h" ^
     "%WORK_DIR%\PhotoshopAPI\PhotoshopAPI\src\LayeredFile\LayerTypes\SmartObjectLayer.h" || (
    echo WARNING: SmartObjectLayer.h patch copy failed — smart object pixel data may be unavailable
)

REM Layer.h: adds unparsed_tagged_blocks() (Phase 12 — layer effects extraction) and
REM get_channel(ChannelID) (Phase 13 — fill-layer pixel bake via AdjustmentLayer/ShapeLayer).
echo   Patching Layer.h...
copy /y "%VENDOR_INCLUDE%\PhotoshopAPI\LayeredFile\LayerTypes\Layer.h" ^
     "%WORK_DIR%\PhotoshopAPI\PhotoshopAPI\src\LayeredFile\LayerTypes\Layer.h" || (
    echo WARNING: Layer.h patch copy failed — layer effects and fill-layer pixel extraction will break
)

REM ShapeLayer.h: no extra patch needed beyond Layer.h (get_channel moved to base).
REM Copy anyway so robocopy round-trips the canonical vendored version.
echo   Patching ShapeLayer.h...
copy /y "%VENDOR_INCLUDE%\PhotoshopAPI\LayeredFile\LayerTypes\ShapeLayer.h" ^
     "%WORK_DIR%\PhotoshopAPI\PhotoshopAPI\src\LayeredFile\LayerTypes\ShapeLayer.h" || (
    echo WARNING: ShapeLayer.h patch copy failed — vendored copy may drift from upstream
)

pushd "%WORK_DIR%\PhotoshopAPI"

echo === [2/3] CMake configure (uses bundled vcpkg submodule) ===
REM PhotoshopAPI's CMakeLists.txt wires up its own bundled vcpkg at thirdparty/vcpkg.
REM Do NOT pass an external -DCMAKE_TOOLCHAIN_FILE — it overrides the bundled one
REM and breaks blosc2/simdutf/mio/compressed-image targets. Disable Python bindings
REM (pybind11 is a submodule we don't need) and tests/examples to keep the build lean.
cmake -B build -S . ^
    -DPSAPI_BUILD_PYTHON=OFF ^
    -DPSAPI_BUILD_TESTS=OFF ^
    -DPSAPI_BUILD_EXAMPLES=OFF ^
    -DPSAPI_BUILD_BENCHMARKS=OFF ^
    -DPSAPI_BUILD_DOCS=OFF ^
    -DCMAKE_BUILD_TYPE=Release || (popd & exit /b 1)

echo === [3/3] CMake build (Release) ===
cmake --build build --config Release || (popd & exit /b 1)
popd

echo === Copying libs, dlls, and headers into vendor tree ===
set "VENDOR_BIN=%VENDOR_ROOT%\Win64\bin"
if not exist "%VENDOR_LIB%" mkdir "%VENDOR_LIB%"
if not exist "%VENDOR_INCLUDE%" mkdir "%VENDOR_INCLUDE%"
if not exist "%VENDOR_BIN%" mkdir "%VENDOR_BIN%"

REM PhotoshopAPI's own .lib + all transitive .libs from build tree (recursive).
robocopy "%WORK_DIR%\PhotoshopAPI\build" "%VENDOR_LIB%" *.lib /S /NFL /NDL /NJH /NJS /NC /NS /NP

REM Vcpkg deps are built dynamic (default x64-windows triplet) — copy DLLs too.
REM Build.cs picks them up via RuntimeDependencies.
robocopy "%WORK_DIR%\PhotoshopAPI\build\vcpkg_installed\x64-windows\bin" "%VENDOR_BIN%" *.dll /NFL /NDL /NJH /NJS /NC /NS /NP

REM PhotoshopAPI source headers live co-located with .cpp under PhotoshopAPI/src/.
REM Copy *.h only (skip .cpp) into Win64/include/PhotoshopAPI/.
robocopy "%WORK_DIR%\PhotoshopAPI\PhotoshopAPI\src" "%VENDOR_INCLUDE%\PhotoshopAPI" *.h *.hpp /E /NFL /NDL /NJH /NJS /NC /NS /NP

REM Umbrella header lives in PhotoshopAPI/include/ — relocate it inside the
REM PhotoshopAPI subdir so its quoted #includes resolve relative to itself.
if exist "%WORK_DIR%\PhotoshopAPI\PhotoshopAPI\include\PhotoshopAPI.h" (
    copy /y "%WORK_DIR%\PhotoshopAPI\PhotoshopAPI\include\PhotoshopAPI.h" "%VENDOR_INCLUDE%\PhotoshopAPI\PhotoshopAPI.h" >nul
)

REM Vcpkg public headers (Eigen, OpenImageIO, fmt, ...).
robocopy "%WORK_DIR%\PhotoshopAPI\build\vcpkg_installed\x64-windows\include" "%VENDOR_INCLUDE%" /E /NFL /NDL /NJH /NJS /NC /NS /NP

REM Bundled submodule headers PhotoshopAPI sources reference via angle brackets.
robocopy "%WORK_DIR%\PhotoshopAPI\thirdparty\compressed-image\compressed_image\include\compressed" "%VENDOR_INCLUDE%\compressed" /E /NFL /NDL /NJH /NJS /NC /NS /NP
robocopy "%WORK_DIR%\PhotoshopAPI\thirdparty\mio\include\mio" "%VENDOR_INCLUDE%\mio" /E /NFL /NDL /NJH /NJS /NC /NS /NP

REM compressed-image's bundled c-blosc2 + nlohmann json headers.
robocopy "%WORK_DIR%\PhotoshopAPI\thirdparty\compressed-image\thirdparty\c-blosc2\include" "%VENDOR_INCLUDE%" /E /NFL /NDL /NJH /NJS /NC /NS /NP
robocopy "%WORK_DIR%\PhotoshopAPI\thirdparty\compressed-image\thirdparty\json\single_include\nlohmann" "%VENDOR_INCLUDE%\nlohmann" /E /NFL /NDL /NJH /NJS /NC /NS /NP

REM simdutf bundled headers (PhotoshopAPI's UnicodeString.h includes "simdutf.h").
robocopy "%WORK_DIR%\PhotoshopAPI\thirdparty\simdutf\include" "%VENDOR_INCLUDE%" /E /NFL /NDL /NJH /NJS /NC /NS /NP

echo === Patching Util/Logger.h to use __VA_OPT__ branch on MSVC ===
REM Upstream gates the __VA_OPT__ branch on !_MSC_VER, but UE 5.7 builds the
REM legacy-preprocessor branch and rejects empty __VA_ARGS__ with a trailing
REM comma. Force the conforming branch by replacing "#ifdef _MSC_VER" with
REM "#if 0" inside Logger.h.
powershell -NoProfile -Command "$f='%VENDOR_INCLUDE%\PhotoshopAPI\Util\Logger.h'; (Get-Content $f -Raw) -replace '#ifdef _MSC_VER','#if 0  /* PSD2UMG patch: force __VA_OPT__ branch */' | Set-Content -NoNewline $f"

echo === Collecting LICENSE files ===
set "VENDOR_LICENSES=%VENDOR_ROOT%\LICENSES"
if not exist "%VENDOR_LICENSES%" mkdir "%VENDOR_LICENSES%"

REM PhotoshopAPI's own license
if exist "%WORK_DIR%\PhotoshopAPI\LICENSE" copy /y "%WORK_DIR%\PhotoshopAPI\LICENSE" "%VENDOR_LICENSES%\PhotoshopAPI-LICENSE.txt" >nul

REM vcpkg writes per-package "copyright" files at:
REM   build\vcpkg_installed\x64-windows-static-md\share\<pkg>\copyright
for /d %%P in ("%WORK_DIR%\PhotoshopAPI\build\vcpkg_installed\x64-windows-static-md\share\*") do (
    if exist "%%P\copyright" copy /y "%%P\copyright" "%VENDOR_LICENSES%\%%~nxP-LICENSE.txt" >nul
)

REM Submodule licenses (blosc2, simdutf, mio, compressed-image, etc.) live under thirdparty\
for /d %%S in ("%WORK_DIR%\PhotoshopAPI\thirdparty\*") do (
    if exist "%%S\LICENSE" copy /y "%%S\LICENSE" "%VENDOR_LICENSES%\%%~nxS-LICENSE.txt" >nul
    if exist "%%S\LICENSE.txt" copy /y "%%S\LICENSE.txt" "%VENDOR_LICENSES%\%%~nxS-LICENSE.txt" >nul
    if exist "%%S\COPYING" copy /y "%%S\COPYING" "%VENDOR_LICENSES%\%%~nxS-COPYING.txt" >nul
)

echo.
echo === DONE ===
echo Vendored libs at:    %VENDOR_LIB%
echo Vendored headers at: %VENDOR_INCLUDE%
echo License files at:    %VENDOR_LICENSES%
echo.
echo Review LICENSES\ before committing — verify every .lib in Win64\lib\
echo has a corresponding LICENSE file. Anything missing must be added by hand.
echo.
endlocal
exit /b 0

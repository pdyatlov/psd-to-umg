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

if "%VCPKG_ROOT%"=="" (
    echo [ERROR] VCPKG_ROOT is not set. Point it at your local vcpkg clone.
    exit /b 1
)

set "REPO_ROOT=%~dp0.."
set "VENDOR_ROOT=%REPO_ROOT%\Source\ThirdParty\PhotoshopAPI"
set "VENDOR_INCLUDE=%VENDOR_ROOT%\Win64\include"
set "VENDOR_LIB=%VENDOR_ROOT%\Win64\lib"
set "WORK_DIR=%TEMP%\psd2umg-photoshopapi-build"
set "PSAPI_TAG=main"
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

echo === [1/5] Cloning PhotoshopAPI @ %PSAPI_TAG% ===
git clone --depth 1 --branch %PSAPI_TAG% https://github.com/EmilDohne/PhotoshopAPI.git "%WORK_DIR%\PhotoshopAPI" || exit /b 1

echo === [2/5] vcpkg install (x64-windows-static-md) ===
pushd "%WORK_DIR%\PhotoshopAPI"
"%VCPKG_ROOT%\vcpkg.exe" install --triplet x64-windows-static-md || (popd & exit /b 1)

echo === [3/5] CMake configure ===
cmake -B build -S . ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows-static-md ^
    -DPSAPI_BUILD_STATIC=ON ^
    -DPSAPI_BUILD_TESTS=OFF ^
    -DPSAPI_BUILD_EXAMPLES=OFF ^
    -DPSAPI_BUILD_BENCHMARKS=OFF ^
    -DCMAKE_BUILD_TYPE=Release || (popd & exit /b 1)

echo === [4/5] CMake build (Release) ===
cmake --build build --config Release || (popd & exit /b 1)
popd

echo === [5/5] Copying libs and headers into vendor tree ===
if not exist "%VENDOR_LIB%" mkdir "%VENDOR_LIB%"
if not exist "%VENDOR_INCLUDE%" mkdir "%VENDOR_INCLUDE%"

REM Copy PhotoshopAPI's own .lib + all transitive .libs from vcpkg installed tree.
robocopy "%WORK_DIR%\PhotoshopAPI\build" "%VENDOR_LIB%" *.lib /S /NFL /NDL /NJH /NJS /NC /NS /NP
robocopy "%WORK_DIR%\PhotoshopAPI\build\vcpkg_installed\x64-windows-static-md\lib" "%VENDOR_LIB%" *.lib /NFL /NDL /NJH /NJS /NC /NS /NP

REM Copy PhotoshopAPI public headers and the vcpkg include tree.
robocopy "%WORK_DIR%\PhotoshopAPI\PhotoshopAPI\include" "%VENDOR_INCLUDE%\PhotoshopAPI" /E /NFL /NDL /NJH /NJS /NC /NS /NP
robocopy "%WORK_DIR%\PhotoshopAPI\build\vcpkg_installed\x64-windows-static-md\include" "%VENDOR_INCLUDE%" /E /NFL /NDL /NJH /NJS /NC /NS /NP

echo.
echo === DONE ===
echo Reminder: drop LICENSE files for every vendored library into
echo   %VENDOR_ROOT%\LICENSES\
echo before committing. See LICENSES\README.md for the required list.
echo.
endlocal
exit /b 0

# Phase 2: C++ PSD Parser — Research

**Researched:** 2026-04-08
**Domain:** UE 5.7 editor plugin, C++20, third-party native static-lib linkage, PSD parsing
**Confidence:** MEDIUM-HIGH (PhotoshopAPI public API verified from source headers; UE 5.7 UFactory coexist-mode behavior is the main open risk)

## Summary

PhotoshopAPI v0.9.0 (April 2026, BSD-3-Clause) is the correct and only viable native C++ PSD parser for this phase. It is C++20, CMake-built, uses vcpkg for its 5 top-level dependencies (OpenImageIO, fmt, libdeflate, stduuid, eigen3 — which transitively pull blosc2, zlib-ng, simdutf, etc.), and exposes a clean `LayeredFile<T>` API that supports everything Phase 2 requires: layer-tree walking, image pixel extraction, and (as of v0.9.0) editable text layer inspection including fonts, styles, and runs. RTTI and exceptions are both required by the library and must be enabled on the consuming UE module.

The biggest integration risk is **not** the library — it is the locked "coexist mode" decision for `UPsdImportFactory`. Unreal's factory system picks exactly one factory per import (highest `ImportPriority` whose `FactoryCanImport` returns true), so running `UPsdImportFactory` alongside `UTextureFactory` to get both a `UTexture2D` and a parsed `FPsdDocument` does NOT work out of the box. The planner must address this explicitly — see "Risks" below.

**Primary recommendation:** Vendor PhotoshopAPI as a pre-built fat static lib + headers under `Source/ThirdParty/PhotoshopAPI/Win64/`, build the `PSD2UMG` module with `bUseRTTI = true` and `bEnableExceptions = true`, use PIMPL to keep PhotoshopAPI types out of public headers, and implement the factory by having `UPsdImportFactory` invoke `UTextureFactory` internally rather than relying on UE to run both.

## User Constraints (from CONTEXT.md)

### Locked Decisions

- **Win64 only** for Phase 2. `PlatformAllowList` in `PSD2UMG.uplugin` stays `["Win64"]` (note: current .uplugin still has `"Mac"` — must be removed in Phase 2).
- **PhotoshopAPI pre-built static libs vendored** at `Source/ThirdParty/PhotoshopAPI/Win64/{lib,include}/` with transitive deps linked in. CMake bootstrap lives at `Scripts/build-photoshopapi.bat` and does NOT run during UBT builds.
- License files committed under `Source/ThirdParty/PhotoshopAPI/LICENSES/`.
- PhotoshopAPI exposed via separate `ModuleType.External` module `Source/ThirdParty/PhotoshopAPI/PhotoshopAPI.Build.cs`. `PSD2UMG` module declares it as `PrivateDependencyModuleName` only — no PhotoshopAPI symbols in public headers.
- `bUseRTTI = true` and `bEnableExceptions = true` on `PSD2UMG` module. All PhotoshopAPI calls wrapped in `try/catch`; exceptions converted to `FPsdParseDiagnostics` and never escape.
- `FPsdDocument` / `FPsdLayer` / `FPsdTextRun` shape fixed (see CONTEXT.md — do not redesign).
- Text extraction: **single run only** in Phase 2. Multi-run flattened; warning emitted.
- `UPsdImportFactory` registered **alongside** `UTextureFactory` in "coexist mode" — both pipelines run; flat `UTexture2D` still created.
- `UImportSubsystem::OnAssetReimport` delegate hook from Phase 1 is **removed** in favor of the factory.
- Test strategy: checked-in `Source/PSD2UMG/Tests/Fixtures/MultiLayer.psd` + UE Automation Spec `FPsdParserSpec`.
- `LogPSD2UMG` log category + `FPsdParseDiagnostics { Severity, LayerName, Message }` struct.
- `FPsdParser::ParseFile(const FString& Path, FPsdDocument& OutDoc, FPsdParseDiagnostics& OutDiag) -> bool`.
- No Message Log integration in Phase 2 — Output Log only.

### Claude's Discretion

- File/class layout under `Source/PSD2UMG/Private/Parser/` and `Public/Parser/`.
- Whether `FPsdParser` is a free-function namespace (`PSD2UMG::Parser`) or a class with static methods — recommendation below: **namespace** for stateless ops.
- Chunking of PRSR-01 vendoring into atomic plan tasks (CMake script vs lib drop ordering).
- Whether `bUseRTTI` requires module isolation tricks.
- pt → px conversion math (assume 72 DPI unless PSD header says otherwise).
- sRGB → `FLinearColor` conversion details.

### Deferred Ideas (OUT OF SCOPE for Phase 2)

- Mac platform support (permanently dropped; future decimal phase if ever).
- Multi-run text styling → Phase 4.
- Blend modes, layer effects, masks → Phase 5.
- Message Log integration for parse diagnostics → later.
- Settings toggle for native vs flat-texture import → rejected; coexist is steady state.
- Golden-output JSON diff testing → Automation Spec assertions suffice.
- Replacing `UTextureFactory` → explicitly rejected.

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| PRSR-01 | PhotoshopAPI integrated as pre-built static lib via Build.cs (Win64) with all transitive deps | See "PhotoshopAPI Library" and "UE5 ThirdParty Module Pattern" |
| PRSR-02 | `FPsdParser::ParseFile()` returns `FPsdDocument` with layer tree (names, types, bounds, visibility, opacity) | `LayeredFile<T>::layers()`, `Layer<T>::name/center_x/center_y/width/height/opacity/visible` |
| PRSR-03 | Image layer RGBA extracted as `TArray<uint8>` with correct dimensions | `ImageLayer<T>::evaluate_image_data()` returns `unordered_map<ChannelID, vector<T>>` |
| PRSR-04 | Text layer content, font name, size, color, alignment | `TextLayer::text()`, `primary_font_name()`, `style_normal_font_size()`, `style_normal_fill_color()`, paragraph justification mixin |
| PRSR-05 | Group/folder preserved with correct parent-child hierarchy | `GroupLayer<T>` inherits `Layer<T>`; iterate via `layers()` or `flat_layers()` |
| PRSR-06 | `UPsdImportFactory` intercepts .psd imports | See "UE5 UFactory Pattern" — risk flagged re: coexist mode |
| PRSR-07 | No Python dependency at runtime | Already satisfied by Phase 1 (PythonScriptPlugin removed) — verified in current `PSD2UMG.Build.cs` |

## PhotoshopAPI Library

**Source:** https://github.com/EmilDohne/PhotoshopAPI
**Version:** v0.9.0 (released April 2026 per GitHub)
**License:** BSD-3-Clause — MIT-compatible (permissive, only requires attribution; license file must be redistributed)
**Language:** C++20
**Build:** CMake + vcpkg (`CMakePresets.json`, `vcpkg.json`)
**Platforms:** Win64 / Linux / macOS, 64-bit only, AVX2 recommended
**Compiler:** MSVC (Visual Studio 2022 recommended for C++20), GCC, Clang

### Dependencies (from `vcpkg.json`)

| Top-level dep | Version | Purpose | Notes |
|---|---|---|---|
| openimageio | 3.0.9.1 | Image I/O / color conversion | **Heavyweight** — pulls its own transitive chain (libtiff, libpng, boost subset, etc.) |
| fmt | 11.0.2#1 | String formatting | Small |
| libdeflate | 1.22#0 | Zlib-compatible fast deflate | Small |
| stduuid | 1.2.3#0 | UUID generation | Header-only |
| eigen3 | 3.4.0#4 | Linear algebra | Header-only |

> **IMPORTANT:** The original roadmap mentions `blosc2`, `zlib-ng`, `simdutf`, `fmt`, `OpenImageIO` as expected transitive deps. `vcpkg.json` shows `openimageio`, `fmt`, `libdeflate`, `stduuid`, `eigen3`. The `blosc2` / `zlib-ng` / `simdutf` items are pulled in transitively (through OpenImageIO and PhotoshopAPI's internal thirdparty folder), not as top-level vcpkg entries. The planner should treat the **authoritative list** as whatever the `vcpkg install` step produces in `vcpkg_installed/x64-windows-static/lib/` after a bootstrap run — enumerate after building, not guess.

### RTTI & Exceptions

The library is modern C++20 and uses exceptions for error paths (verified via text in `TextLayer` and `LayeredFile` headers that return `std::optional` for soft failures but `throw` for format errors). It also uses `std::variant` / `std::shared_ptr` polymorphism for the `Layer<T>` base class — `dynamic_cast` / RTTI is required to downcast from `Layer<T>` to `ImageLayer<T>` / `GroupLayer<T>` / `TextLayer`.

**Confidence:** HIGH — verified by inspecting `LayeredFile.h` `find_layer_as<>()` template which is a type-cast helper, implying RTTI-based downcasting. Also consistent with the locked decision in CONTEXT.md.

### Relevant Public API (verified from headers)

**`LayeredFile<T>`** — `PhotoshopAPI/src/LayeredFile/LayeredFile.h`
```cpp
// T is bpp8_t, bpp16_t, or bpp32_t (bit depth)
static LayeredFile<T> read(const std::filesystem::path& path);
std::vector<std::shared_ptr<Layer<T>>>& layers() noexcept;  // root-level
std::shared_ptr<Layer<T>> find_layer(std::string path) const;
std::vector<std::shared_ptr<Layer<T>>> flat_layers();
// canvas width/height accessors exist on LayeredFile
```

**`Layer<T>`** — base class, `LayerTypes/Layer.h`
```cpp
const std::string& name() const noexcept;
virtual uint32_t width() const noexcept;
virtual uint32_t height() const noexcept;
virtual float center_x() const noexcept;
virtual float center_y() const noexcept;
float top_left_x() const noexcept;
float top_left_y() const noexcept;
float opacity() const noexcept;   // 0.0 .. 1.0 (normalized)
bool visible() const noexcept;
```
**Bounds derivation:** Layer does NOT expose a `bounds()` rect directly. Compute as:
```cpp
int32 MinX = FMath::RoundToInt(Layer.top_left_x());
int32 MinY = FMath::RoundToInt(Layer.top_left_y());
int32 MaxX = MinX + static_cast<int32>(Layer.width());
int32 MaxY = MinY + static_cast<int32>(Layer.height());
FIntRect Bounds(MinX, MinY, MaxX, MaxY);
```

**Layer type detection:** Use `std::dynamic_pointer_cast<ImageLayer<T>>(layerPtr)` / `GroupLayer<T>` / `TextLayer<T>`. This is the RTTI requirement.

**`ImageLayer<T>`** — `LayerTypes/ImageLayer.h`
```cpp
data_type evaluate_image_data();  // std::unordered_map<Enum::ChannelID, std::vector<T>>
std::vector<T> evaluate_channel(std::variant<int, Enum::ChannelID, Enum::ChannelIDInfo> id);
std::vector<int> channel_indices(bool include_mask);
size_t num_channels(bool include_mask);
```
Returned channel vectors are laid out row-major (width × height). To assemble RGBA for UE: call `evaluate_image_data()`, pull `ChannelID::Red`, `::Green`, `::Blue`, `::Alpha` vectors, and interleave into a `TArray<uint8>` of size `W*H*4` in `RGBA` byte order.

**`GroupLayer<T>`** — children accessor is the same `layers()` pattern as `LayeredFile`:
```cpp
std::vector<std::shared_ptr<Layer<T>>>& layers() noexcept;  // child layers
```
Recurse with `std::dynamic_pointer_cast<GroupLayer<T>>(child)` check.

**`TextLayer`** — `LayerTypes/TextLayer/TextLayer.h` + mixins
```cpp
// Core content (TextLayer.h)
std::optional<std::string> text() const;  // UTF-8 full content

// Font (TextLayerFontMixin.h)
std::optional<std::string> primary_font_name() const;       // convenience → first used
std::optional<std::string> font_postscript_name(size_t i) const;
std::vector<std::string> used_font_names() const;

// Style (TextLayerStyleNormalMixin.h) — "normal" = default/document-wide
std::optional<int32_t>  style_normal_font() const;          // font index into table
std::optional<double>   style_normal_font_size() const;     // in POINTS
std::optional<std::vector<double>> style_normal_fill_color() const;   // RGB[A] doubles 0..1
std::optional<std::vector<double>> style_normal_stroke_color() const;

// Runs (for Phase 4)
std::vector<size_t> style_run_lengths() const;
std::vector<size_t> paragraph_run_lengths() const;
```

**Alignment/justification:** Lives in the paragraph mixin (`TextLayerParagraphNormalMixin.h`). I was unable to verify the exact method name from a header fetch, but the pattern is consistent with `style_normal_*` — expect something like `paragraph_normal_justification()` returning an enum (`Left`, `Center`, `Right`, `Justify*` variants). **Planner should budget a small investigation task** to confirm the exact name by reading the header once the lib is vendored.

**Confidence:** Content/font/size/color HIGH (directly verified). Alignment MEDIUM (inferred from mixin naming pattern; verify during implementation).

### Known UE Integration Gotchas

- **No prior UE integration documented.** Nobody has publicly integrated PhotoshopAPI with Unreal. This plugin will be the reference implementation.
- **OpenImageIO is a monster transitive dep.** It drags in libtiff, libpng, libjpeg-turbo, boost fragments, OpenEXR, etc. The vendored fat-lib approach hides this from UBT but the final `.lib` bundle on Win64 will likely be 50–200 MB total across all static libs. Git repo size impact — planner should note.
- **vcpkg static triplet required.** The CMake bootstrap must use `x64-windows-static` triplet, NOT `x64-windows` (which produces DLLs). UE wants `.lib` files with no DLL dependencies.
- **MSVC runtime mismatch risk.** UE uses `/MD` (dynamic CRT). vcpkg's `x64-windows-static` defaults to `/MT`. These will NOT link together. The correct triplet is `x64-windows-static-md` (static libs, dynamic CRT) — this is the single most common integration failure for UE third-party libs. **This is a concrete task for the bootstrap script.**
- **std::filesystem::path in `LayeredFile::read()`** — safe to call with a `std::filesystem::path(TCHAR_TO_UTF8(*FStringPath))` conversion.
- **C++20 on UE 5.7** — UE 5.7 supports C++20 but the `PSD2UMG` module must set `CppStandard = CppStandardVersion.Cpp20;` in `Build.cs` (currently unset → defaults to engine default which is C++17 in 5.7). **Concrete Build.cs edit required.**

## UE5 ThirdParty Module Pattern

**Source:** https://dev.epicgames.com/documentation/en-us/unreal-engine/integrating-third-party-libraries-into-unreal-engine (UE 5.7), https://georgy.dev/posts/third-party-integration/

### Canonical `ModuleType.External` Build.cs

File: `Source/ThirdParty/PhotoshopAPI/PhotoshopAPI.Build.cs`
```csharp
using System.IO;
using UnrealBuildTool;

public class PhotoshopAPI : ModuleRules
{
    public PhotoshopAPI(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        string RootPath = ModuleDirectory;  // .../Source/ThirdParty/PhotoshopAPI
        string IncludePath = Path.Combine(RootPath, "include");
        string LibPath     = Path.Combine(RootPath, "Win64", "lib");

        PublicSystemIncludePaths.Add(IncludePath);
        // PublicSystemIncludePaths (not PublicIncludePaths) prevents UE's
        // warning-as-error treatment from applying to PhotoshopAPI headers.

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            // Enumerate after vcpkg build; list every .lib explicitly.
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "PhotoshopAPI.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "OpenImageIO.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "OpenImageIO_Util.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "fmt.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libdeflate.lib"));
            // ... all transitive deps here (libtiff, libpng, zlib, etc.)
        }

        PublicDefinitions.Add("WITH_PHOTOSHOPAPI=1");
    }
}
```

Key points:
- `Type = ModuleType.External` — no C++ source compiled for this module.
- `PublicSystemIncludePaths` instead of `PublicIncludePaths` — suppresses UE's strict warning rules for the external headers. This matters because PhotoshopAPI headers will trigger warnings under `-Werror`.
- `PublicAdditionalLibraries` — enumerate every `.lib`. No glob. UBT does not walk directories.

### Consuming Module Edits (`Source/PSD2UMG/PSD2UMG.Build.cs`)

```csharp
public class PSD2UMG : ModuleRules
{
    public PSD2UMG(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;  // NEW — required for PhotoshopAPI
        bUseRTTI = true;                          // NEW — required for dynamic_pointer_cast
        bEnableExceptions = true;                 // NEW — PhotoshopAPI throws

        PublicDependencyModuleNames.AddRange(new string[] { "Core" });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "CoreUObject", "Engine", "Slate", "SlateCore",
            "DeveloperSettings", "UnrealEd", "UMGEditor", "UMG",
            "AssetRegistry",
            "PhotoshopAPI",   // NEW
        });
    }
}
```

**`bUseRTTI` + PIMPL insulation:** Turning on RTTI for the entire `PSD2UMG` module is acceptable (it's a small editor module). To avoid leaking PhotoshopAPI headers into the rest of the engine via PCH, use PIMPL:

```cpp
// Public/Parser/PsdParser.h  — no PhotoshopAPI types, no RTTI leakage
#pragma once
#include "PsdDocument.h"
#include "PsdParseDiagnostics.h"

namespace PSD2UMG::Parser
{
    bool ParseFile(const FString& Path, FPsdDocument& OutDoc, FPsdParseDiagnostics& OutDiag);
}
```
```cpp
// Private/Parser/PsdParser.cpp  — includes PhotoshopAPI headers, compiled with RTTI+exceptions
#include "Parser/PsdParser.h"
THIRD_PARTY_INCLUDES_START
#include <LayeredFile/LayeredFile.h>
#include <LayeredFile/LayerTypes/ImageLayer.h>
#include <LayeredFile/LayerTypes/GroupLayer.h>
#include <LayeredFile/LayerTypes/TextLayer/TextLayer.h>
THIRD_PARTY_INCLUDES_END

namespace PSD2UMG::Parser { ... }
```

Wrap every PhotoshopAPI include in `THIRD_PARTY_INCLUDES_START` / `THIRD_PARTY_INCLUDES_END` macros — these are UE-provided and disable strict warnings locally.

**Recommendation (discretion item):** Use a free-function namespace `PSD2UMG::Parser` rather than a class with static methods. The operation is stateless and adding `class FPsdParser { static ... }` is idiomatic Java, not idiomatic UE/C++.

## UE5 UFactory Pattern — AND THE COEXIST-MODE RISK

**Source:** https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Editor/UnrealEd/UFactory, https://www.gmpreussner.com/reference/adding-new-asset-types-to-ue4, forum thread on multi-factory dispatch.

### Canonical minimal UFactory subclass

```cpp
// Public/Import/PsdImportFactory.h
#pragma once
#include "Factories/Factory.h"
#include "PsdImportFactory.generated.h"

UCLASS(hidecategories=Object)
class UPsdImportFactory : public UFactory
{
    GENERATED_BODY()
public:
    UPsdImportFactory();

    virtual bool FactoryCanImport(const FString& Filename) override;
    virtual UObject* FactoryCreateBinary(
        UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags,
        UObject* Context, const TCHAR* Type,
        const uint8*& Buffer, const uint8* BufferEnd,
        FFeedbackContext* Warn) override;
};
```
```cpp
// Private/Import/PsdImportFactory.cpp
UPsdImportFactory::UPsdImportFactory()
{
    bEditorImport = true;
    bCreateNew = false;
    SupportedClass = UTexture2D::StaticClass();   // or UWidgetBlueprint in Phase 3
    Formats.Add(TEXT("psd;Photoshop Document"));
    ImportPriority = DefaultImportPriority + 1;    // beat UTextureFactory
}
```
- Factory classes must live in an Editor-type module and derive from `UFactory` with `UCLASS()` — UE discovers them automatically via reflection. No manual registration.
- `FactoryCanImport(filename)` — return true if extension matches. The factory dispatch loop calls this on every registered factory that claims the format.
- `ImportPriority`: integer; **higher wins**. `UTextureFactory` uses `DefaultImportPriority` (0), so `DefaultImportPriority + 1` (or any positive value) makes `UPsdImportFactory` win.

### **RISK: Coexist mode is NOT a standard UE behavior**

The locked decision in CONTEXT.md says:
> `UPsdImportFactory` is registered as an additional factory for `.psd`. **It does NOT replace `UTextureFactory`.** The native pipeline runs alongside — UE still creates a flat `UTexture2D` from the PSD as before.

**This is not how UE's factory dispatch works.** From the docs and forum thread:
> `ImportPriority` determines the order in which factories are tried when importing or reimporting an object. Factories with higher priority values will go first. If a higher-priority factory returns `nullptr` (indicating it cannot handle that specific file), the engine can then attempt the next factory in the priority order.

UE picks **exactly one** factory per imported file — the highest-priority one whose `FactoryCanImport` returns true and whose `FactoryCreateBinary` does not return nullptr. It does NOT run both factories and produce two assets. If `UPsdImportFactory` wins, the flat `UTexture2D` from `UTextureFactory` **will not be created**.

**Three viable resolutions** (planner must pick one and flag it with the user):

1. **Recommended: `UPsdImportFactory` manually invokes `UTextureFactory` internally.** Inside `FactoryCreateBinary`, construct a transient `UTextureFactory`, call its `FactoryCreateBinary` with the same buffer to produce the flat `UTexture2D`, then separately run PhotoshopAPI parsing to produce the `FPsdDocument`, then return the texture as the primary asset and use `GetAdditionalImportedObjects()` to report the parsed document (or just log it for Phase 2). This is a clean pattern and preserves both outputs.

2. **Higher-priority wrapper factory.** Same as (1) conceptually — `UPsdImportFactory` is the only registered factory for `.psd`, it internally does both jobs. Cleaner mental model.

3. **Abandon the "flat texture still created" part of the locked decision.** Parse-only in Phase 2; no flat `UTexture2D` created during import. Phase 3 creates `UTexture2D` assets explicitly from the parsed layer pixel data anyway (TEX-01), so the flat texture from `UTextureFactory` may be redundant.

**The research recommendation is option (1) — it is the cleanest implementation of the user's stated intent.** The planner should confirm with the user during the plan-phase step that option 1 matches their mental model of "coexist mode."

### Automation Spec Pattern

**Source:** UE 5.7 `Misc/AutomationTest.h` conventions.

```cpp
// Tests/PsdParserSpec.cpp  — only compiled WITH_DEV_AUTOMATION_TESTS
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Parser/PsdParser.h"
#include "Misc/Paths.h"

BEGIN_DEFINE_SPEC(FPsdParserSpec, "PSD2UMG.Parser",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
    FString FixturePath;
    FPsdDocument Doc;
    FPsdParseDiagnostics Diag;
END_DEFINE_SPEC(FPsdParserSpec)

void FPsdParserSpec::Define()
{
    BeforeEach([this]()
    {
        FixturePath = FPaths::Combine(
            FPaths::ProjectPluginsDir(),
            TEXT("PSD2UMG/Source/PSD2UMG/Tests/Fixtures/MultiLayer.psd"));
        PSD2UMG::Parser::ParseFile(FixturePath, Doc, Diag);
    });

    Describe("ParseFile", [this]()
    {
        It("parses canvas size", [this]()
        {
            TestEqual(TEXT("Canvas width"), Doc.CanvasSize.X, 512);
            TestEqual(TEXT("Canvas height"), Doc.CanvasSize.Y, 512);
        });

        It("preserves group hierarchy", [this]()
        {
            TestEqual(TEXT("Root count"), Doc.RootLayers.Num(), 3);
            // ... further assertions
        });
    });
}
#endif
```

- `BEGIN_DEFINE_SPEC` / `END_DEFINE_SPEC` — UE's Spec-style pattern (preferred over `IMPLEMENT_SIMPLE_AUTOMATION_TEST` for multi-assertion fixtures).
- `EAutomationTestFlags::EditorContext | EngineFilter` — marks as editor-only test, appears in Session Frontend.
- Category string `"PSD2UMG.Parser"` shows up in Session Frontend → Automation → filter tree.
- Fixture resolved via `FPaths::ProjectPluginsDir()` + relative path.
- **Test files must live in a `Tests/` subfolder and be compiled only under `WITH_DEV_AUTOMATION_TESTS`** (wrap the entire file).
- Run via Window → Developer Tools → Session Frontend → Automation → search "PSD2UMG" → Start Tests.

**Confidence:** HIGH — this is the canonical UE 5.x spec test pattern.

## pt → px and Color Conversion

### Point size to pixel size

PSD stores text size as `double` in the `TySh` descriptor, in **points at 72 DPI** (not document DPI). PhotoshopAPI's `style_normal_font_size()` returns this double directly.

At 72 DPI, **1 pt == 1 px**. Phase 2 conversion is therefore identity:
```cpp
float SizePx = static_cast<float>(Layer.style_normal_font_size().value_or(12.0));
```

Phase 4's `TEXT-01` says "Photoshop point size × 0.75 = UMG font size (72→96 DPI)" — that is a separate display-DPI conversion and is Phase 4's problem, not Phase 2's. Phase 2 should store the raw point value (interpreted as pixels at 72 DPI) in `FPsdTextRun::SizePx` and let Phase 4 scale it.

**Recommendation:** Document in a code comment that `SizePx` holds the value in points-at-72-DPI (which equals pixels-at-72-DPI), and that the 0.75 scaling to UMG font size happens downstream.

### sRGB → FLinearColor

PhotoshopAPI returns text fill color as `std::vector<double>` of length 3 or 4, in **0..1 sRGB space** (not linear; values correspond directly to Photoshop's color picker RGB).

Correct conversion:
```cpp
auto FillOpt = TextLayer->style_normal_fill_color();
FLinearColor Color = FLinearColor::Black;
if (FillOpt.has_value() && FillOpt->size() >= 3)
{
    // Double sRGB 0..1 -> FColor byte sRGB -> FLinearColor linear
    const FColor SrgbByte(
        static_cast<uint8>(FMath::Clamp((*FillOpt)[0] * 255.0, 0.0, 255.0)),
        static_cast<uint8>(FMath::Clamp((*FillOpt)[1] * 255.0, 0.0, 255.0)),
        static_cast<uint8>(FMath::Clamp((*FillOpt)[2] * 255.0, 0.0, 255.0)),
        255);
    Color = FLinearColor::FromSRGBColor(SrgbByte);
}
```

**Why not `FLinearColor(r, g, b)` directly?** That constructor treats the inputs as already-linear values. Photoshop colors are gamma-encoded sRGB, so they must go through the sRGB→linear transfer function. `FLinearColor::FromSRGBColor(FColor)` does this correctly.

For image layer RGBA (PRSR-03): Unreal's `UTexture2D::Source.Init()` with `TSF_BGRA8` expects **sRGB bytes** (no linearization needed — the texture itself carries the sRGB flag). So image pixel data from `evaluate_image_data()` is copied straight into a `TArray<uint8>` with no color-space conversion. Only text colors need the sRGB→linear transform.

**Confidence:** HIGH — `FLinearColor::FromSRGBColor` is the documented canonical conversion.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---|---|---|---|
| PSD binary format parsing | Custom parser against Adobe spec | PhotoshopAPI `LayeredFile<T>::read()` | 200+ page Adobe spec, compression variants (RLE, ZIP, ZIP-prediction), Photoshop-version quirks |
| Text `TySh` descriptor parsing | Manual byte walker | PhotoshopAPI `TextLayer` + mixins | The `TySh` block is a nested Photoshop "descriptor" format with its own type system; untouchable without prior infrastructure |
| Layer compositing / masks | Custom merge code | **Skip entirely in Phase 2** | Deferred to Phase 5 |
| pt→px font math | Custom DPI detection | Identity at 72 DPI for Phase 2, defer DPI scaling to Phase 4 | TEXT-01 is a Phase 4 requirement, not Phase 2 |
| sRGB conversion | Custom gamma function | `FLinearColor::FromSRGBColor(FColor)` | Engine-provided, correct |
| RGBA interleaving | ~~Don't hand-roll~~ | **Do hand-roll** — a simple loop is correct; PhotoshopAPI gives separate channels | Trivial, 5 lines |

## Common Pitfalls

### Pitfall 1: vcpkg triplet produces `/MT` libs that won't link to UE
**What goes wrong:** Default `x64-windows-static` vcpkg triplet compiles with `/MT` (static CRT). UE uses `/MD` (dynamic CRT). Linker produces "LNK2038: mismatch detected for 'RuntimeLibrary'" errors.
**Why it happens:** vcpkg's static triplet is optimized for self-contained executables, not for embedding in a host that uses dynamic CRT.
**How to avoid:** Use the `x64-windows-static-md` triplet (static libs, dynamic CRT) when running the bootstrap script. Document this in `Scripts/build-photoshopapi.bat`.
**Warning sign:** Link errors mentioning `RuntimeLibrary` or `MT_StaticRelease` vs `MD_DynamicRelease`.

### Pitfall 2: UE strict warnings on PhotoshopAPI headers
**What goes wrong:** UE compiles with `-Werror` equivalents; PhotoshopAPI headers emit warnings (unused parameter, sign comparison, etc.) that become errors.
**How to avoid:** Always include PhotoshopAPI headers between `THIRD_PARTY_INCLUDES_START` / `THIRD_PARTY_INCLUDES_END`. Use `PublicSystemIncludePaths` (not `PublicIncludePaths`) in the External Build.cs.
**Warning sign:** Build breaks on PhotoshopAPI header files with `warning treated as error`.

### Pitfall 3: PhotoshopAPI exceptions escaping into UE code
**What goes wrong:** A corrupt PSD triggers `std::runtime_error` from PhotoshopAPI; if it escapes `FPsdParser::ParseFile` it hits UE code compiled without exceptions and crashes the editor.
**How to avoid:** Wrap every PhotoshopAPI call in `try { ... } catch (const std::exception& e) { OutDiag.Entries.Add({Error, {}, UTF8_TO_TCHAR(e.what())}); return false; } catch (...) { ... }`. Explicitly test with a deliberately malformed PSD in the Automation Spec.
**Warning sign:** Editor crash on malformed PSD; stack shows `__CxxFrameHandler`.

### Pitfall 4: Coexist-mode factory assumption
See "RISK" section above. Symptom: registering both factories and expecting both to run — only one runs.

### Pitfall 5: Missing `CppStandard = Cpp20`
**What goes wrong:** PhotoshopAPI uses C++20 concepts (e.g., `concepts::bit_depth<T>`); UE 5.7's default C++17 rejects them.
**How to avoid:** Set `CppStandard = CppStandardVersion.Cpp20;` in `PSD2UMG.Build.cs` — first thing, before anything else.
**Warning sign:** Compile errors citing `concept` or `requires` keywords.

### Pitfall 6: `.uplugin` still allows Mac
Current `PSD2UMG.uplugin` has `"PlatformAllowList": ["Win64", "Mac"]`. Phase 2 is Win64-only. Mac must be removed from the allow list, otherwise a Mac build will try to link Win64 `.lib` files and fail.

### Pitfall 7: PSD fixture in git
**What goes wrong:** A large binary fixture bloats the repo.
**How to avoid:** Keep `MultiLayer.psd` under ~50 KB (a minimal 512×512 multi-layer PSD with 1 image + 1 text + 1 group can easily fit). Do NOT use git-lfs unless size exceeds ~1 MB — lfs is overkill and adds CI friction.

## Recommended Sequencing

Build Phase 2 in this order — each step is independently verifiable:

1. **Build.cs + uplugin prep (no parsing yet).** Add `CppStandard = Cpp20`, `bUseRTTI`, `bEnableExceptions` to `PSD2UMG.Build.cs`. Remove `"Mac"` from `PSD2UMG.uplugin`. Verify plugin still compiles and loads. **Gate: editor still opens cleanly.**

2. **CMake bootstrap script.** Create `Scripts/build-photoshopapi.bat` that clones PhotoshopAPI at tag v0.9.0, runs `cmake --preset` with `x64-windows-static-md` triplet, builds, and copies `.lib` + headers + license files into `Source/ThirdParty/PhotoshopAPI/{include,Win64/lib,LICENSES}`. Run it once locally. Commit the outputs. **Gate: `Source/ThirdParty/PhotoshopAPI/` populated and committed.**

3. **ThirdParty module `PhotoshopAPI.Build.cs`.** Create `Source/ThirdParty/PhotoshopAPI/PhotoshopAPI.Build.cs` with `ModuleType.External`, enumerate every `.lib`. Add `"PhotoshopAPI"` to `PSD2UMG.Build.cs` private deps. **Gate: plugin compiles with the new dep but no PhotoshopAPI code is called yet.**

4. **`FPsdDocument` / `FPsdLayer` / `FPsdTextRun` types + `FPsdParseDiagnostics` + `LogPSD2UMG`.** Pure header work, no PhotoshopAPI includes. Public under `Source/PSD2UMG/Public/Parser/` and `Public/Diagnostics/`. **Gate: types compile in isolation.**

5. **`PSD2UMG::Parser::ParseFile` — image + group only.** Implement the PIMPL file that includes PhotoshopAPI, walks `LayeredFile::layers()`, handles `ImageLayer` and `GroupLayer` (recursive), assembles RGBA. Skip text layers (return `EPsdLayerType::Unknown` for now). Wrap everything in try/catch. **Gate: a simple 2-image-layer PSD round-trips through `ParseFile`.**

6. **Hand-craft `MultiLayer.psd` fixture + Automation Spec `FPsdParserSpec`.** Start with image/group assertions only. **Gate: spec passes via Session Frontend.**

7. **Text layer extraction.** Add `TextLayer` branch: content, primary_font_name, style_normal_font_size, style_normal_fill_color, alignment. This is where the "investigate paragraph mixin for alignment getter name" task lives. Flatten multi-run with diagnostic warning. Extend spec with text assertions. **Gate: spec passes on the full fixture.**

8. **`UPsdImportFactory`** — implement with the recommended "factory internally invokes UTextureFactory" pattern (see Risks section 1). Log the parsed `FPsdDocument` summary via `LogPSD2UMG`. **Gate: dragging the fixture into Content Browser produces a flat `UTexture2D` AND logs the layer tree.**

9. **Remove `FPSD2UMGModule::OnPSDImport` delegate hook** (retire the Phase 1 stub now that the factory owns the integration point). **Gate: ShutdownModule no longer touches `OnAssetReimport`.**

10. **Final verification pass:** run Automation Spec, open the test fixture via drag-and-drop, confirm log output, confirm plugin still loads cleanly on a fresh editor restart.

## Risks

1. **COEXIST-MODE FACTORY** (HIGH) — the locked decision conflicts with UE's one-factory-per-import dispatch model. Planner must resolve at plan time using one of the three resolutions above. **Recommended resolution: Option 1 (factory internally invokes UTextureFactory).**

2. **PhotoshopAPI alignment getter API** (LOW) — exact method name on the paragraph mixin is unverified. Easy to resolve at implementation time by reading the header; budget 15 minutes.

3. **vcpkg build reproducibility** (MEDIUM) — the bootstrap script must pin PhotoshopAPI to tag `v0.9.0` and pin the vcpkg baseline, otherwise future re-runs produce different `.lib` contents. Document the pinned SHAs in the script.

4. **Git repo size** (MEDIUM) — static libs for OpenImageIO + transitive chain could total 100+ MB. Planner should measure after the first bootstrap and decide whether to use git-lfs for the `.lib` files (NOT for the PSD fixture). Alternative: commit a release-mode-only subset.

5. **BSD-3 vs MIT compatibility** (LOW) — MIT plugin can link BSD-3 libs freely; just must preserve BSD-3 license text in `LICENSES/`. No copyleft concerns.

6. **OpenImageIO transitive chain overreach** (LOW-MEDIUM) — PhotoshopAPI may only need a sliver of OpenImageIO. The vcpkg build will pull the full monster. Investigate whether the bootstrap script can disable OIIO features via vcpkg feature flags to shrink the final `.lib` set. Optimization, not blocker.

## Open Questions (require user/planner decision)

1. **Coexist-mode implementation choice.** Which of the three resolutions above does the user actually want? Research strongly recommends Option 1, but the locked decision language implies the user believes both factories run natively — they don't. **This must be clarified before planning the factory task.**

2. **Git-lfs for static libs?** Depends on final `.lib` bundle size — answer after step 2 of the sequencing, not before.

3. **Minimum MSVC toolchain?** UE 5.7 on Windows requires VS 2022. PhotoshopAPI v0.9.0 requires C++20. VS 2022 17.6+ has full enough C++20 support. Recommend documenting `VS 2022 17.6+` in the bootstrap script prerequisites.

## Sources

### Primary (HIGH confidence)
- PhotoshopAPI repo — https://github.com/EmilDohne/PhotoshopAPI
- `LayeredFile.h` — https://github.com/EmilDohne/PhotoshopAPI/blob/master/PhotoshopAPI/src/LayeredFile/LayeredFile.h
- `Layer.h` — https://github.com/EmilDohne/PhotoshopAPI/blob/master/PhotoshopAPI/src/LayeredFile/LayerTypes/Layer.h
- `ImageLayer.h` — https://github.com/EmilDohne/PhotoshopAPI/blob/master/PhotoshopAPI/src/LayeredFile/LayerTypes/ImageLayer.h
- `TextLayer.h` — https://github.com/EmilDohne/PhotoshopAPI/blob/master/PhotoshopAPI/src/LayeredFile/LayerTypes/TextLayer/TextLayer.h
- `TextLayerFontMixin.h` / `TextLayerStyleNormalMixin.h` — same directory
- `vcpkg.json` — https://github.com/EmilDohne/PhotoshopAPI/blob/master/vcpkg.json
- UE 5.7 `UFactory` docs — https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Editor/UnrealEd/UFactory
- UE 5.7 Integrating Third-Party Libraries — https://dev.epicgames.com/documentation/en-us/unreal-engine/integrating-third-party-libraries-into-unreal-engine
- Current repo files: `Source/PSD2UMG/PSD2UMG.Build.cs`, `PSD2UMG.uplugin`, `Source/PSD2UMG/Private/PSD2UMG.cpp`
- `.planning/phases/02-c-psd-parser/02-CONTEXT.md`, `.planning/REQUIREMENTS.md`, `CLAUDE.md`

### Secondary (MEDIUM confidence)
- Georgy Tech Blog — third-party integration — https://georgy.dev/posts/third-party-integration/
- gmpreussner — Adding New Asset Types to UE4 — https://www.gmpreussner.com/reference/adding-new-asset-types-to-ue4
- UE forum: multiple factories same extension — https://forums.unrealengine.com/t/how-do-i-handle-when-factorycreatefile-has-multiple-types-but-only-one-extension/1114773

### Tertiary (LOW — verify at implementation time)
- Exact method name for paragraph alignment/justification getter on PhotoshopAPI `TextLayer` (inferred from mixin naming; verify by reading `TextLayerParagraphNormalMixin.h` once the lib is vendored)
- Exact total `.lib` file list for PhotoshopAPI + transitive deps (depends on vcpkg resolution at build time)

## Metadata

**Confidence breakdown:**
- PhotoshopAPI public API (image/group/text core): HIGH — directly verified from headers
- PhotoshopAPI alignment API: MEDIUM — inferred from mixin pattern
- ThirdParty Build.cs pattern: HIGH — canonical UE 5.x pattern
- UFactory coexist behavior: HIGH — verified multiple sources that UE picks one factory; LOW that the user's "coexist" intent is achievable as described
- Automation Spec pattern: HIGH — canonical
- Color/DPI math: HIGH — engine-provided helpers

**Research date:** 2026-04-08
**Valid until:** 2026-05-08 (PhotoshopAPI is stable at v0.9.0; UE 5.7 APIs stable)

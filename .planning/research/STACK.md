# Stack Research

**Project:** PSD2UMG
**Researched:** 2026-04-07
**Overall confidence:** HIGH (source code verified, official docs checked)

---

## UE 5.7 Plugin Build System

### Build.cs Module Dependencies

The current `AutoPSDUI.Build.cs` targets UE 4.26 and includes `PythonScriptPlugin` which must be removed. The new Build.cs needs the following changes:

**Required module dependencies for UE 5.7 editor plugin:**

```csharp
using System;
using System.IO;
using UnrealBuildTool;

public class PSD2UMG : ModuleRules
{
    public PSD2UMG(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bUseRTTI = false;          // UE default, keep off
        bEnableExceptions = true;  // PhotoshopAPI uses exceptions

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "InputCore",
            "DeveloperSettings",     // UPsd2UmgSettings
            "UnrealEd",              // Editor utilities, FAssetEditorManager
            "UMG",                   // UWidget, UCanvasPanel, UImage, UTextBlock, etc.
            "UMGEditor",             // UWidgetBlueprint, WidgetTree
            "AssetRegistry",         // IAssetRegistry
            "AssetTools",            // IAssetTools, asset factories
            "ContentBrowser",        // Context menu extension
            "ToolMenus",             // Editor menu registration (UE5 pattern)
            "Projects",              // IPluginManager
            "EditorFramework",       // SNotificationList (UE5 notification API)
        });

        // PhotoshopAPI static library linkage
        SetupPhotoshopAPI(Target);
    }

    private void SetupPhotoshopAPI(ReadOnlyTargetRules Target)
    {
        string ThirdPartyDir = Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "PhotoshopAPI");

        // Include paths
        PublicIncludePaths.Add(Path.Combine(ThirdPartyDir, "include"));
        PublicIncludePaths.Add(Path.Combine(ThirdPartyDir, "src"));

        // Platform-specific static libraries
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string LibDir = Path.Combine(ThirdPartyDir, "lib", "Win64");
            PublicAdditionalLibraries.Add(Path.Combine(LibDir, "PhotoshopAPI.lib"));
            // PhotoshopAPI dependencies (all static)
            PublicAdditionalLibraries.Add(Path.Combine(LibDir, "libdeflate.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibDir, "simdutf.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibDir, "fmt.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibDir, "OpenImageIO.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibDir, "OpenImageIO_Util.lib"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            string LibDir = Path.Combine(ThirdPartyDir, "lib", "Mac");
            PublicAdditionalLibraries.Add(Path.Combine(LibDir, "libPhotoshopAPI.a"));
            PublicAdditionalLibraries.Add(Path.Combine(LibDir, "libdeflate.a"));
            PublicAdditionalLibraries.Add(Path.Combine(LibDir, "libsimdutf.a"));
            PublicAdditionalLibraries.Add(Path.Combine(LibDir, "libfmt.a"));
            PublicAdditionalLibraries.Add(Path.Combine(LibDir, "libOpenImageIO.a"));
            PublicAdditionalLibraries.Add(Path.Combine(LibDir, "libOpenImageIO_Util.a"));
        }

        // Suppress warnings from PhotoshopAPI headers
        PublicDefinitions.Add("PSAPI_STATIC=1");
        bEnableExceptions = true;
    }
}
```

**Confidence:** MEDIUM -- The module list is verified against UE 5.7 documentation URLs. The exact dependency libraries for PhotoshopAPI come from reading its CMakeLists.txt. However, the OpenImageIO dependency chain may pull in additional transitive libs (libtiff, libpng, etc.) that will need discovery during the actual build.

### Key UE4-to-UE5 Build.cs Changes

| UE4 Pattern | UE5.7 Replacement | Notes |
|---|---|---|
| `PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs` | Same -- still valid | No change needed |
| No `CppStandard` set (defaults to C++17) | `CppStandard = CppStandardVersion.Cpp20` | Required for PhotoshopAPI |
| `"PythonScriptPlugin"` dependency | REMOVE entirely | No Python at runtime |
| `"EditorScriptingUtilities"` dependency | REMOVE -- functionality absorbed into `UnrealEd` | UE5 consolidated these |
| No `bEnableExceptions` | `bEnableExceptions = true` | PhotoshopAPI throws C++ exceptions |

### .uplugin Changes

```json
{
    "FileVersion": 3,
    "Version": 1,
    "VersionName": "1.0",
    "FriendlyName": "PSD2UMG",
    "Description": "Import PSD files as UMG Widget Blueprints",
    "Category": "Editor",
    "EngineVersion": "5.7.0",
    "CanContainContent": true,
    "Installed": true,
    "Modules": [
        {
            "Name": "PSD2UMG",
            "Type": "Editor",
            "LoadingPhase": "PostEngineInit",
            "PlatformAllowList": [
                "Win64",
                "Mac"
            ]
        }
    ]
}
```

**Key change:** `WhitelistPlatforms` is renamed to `PlatformAllowList` in UE5. UE5 silently ignores the old key -- but using the old key means the platform filter has no effect, and the plugin loads on all platforms. Use `PlatformAllowList` for UE 5.0+.

**Confidence:** HIGH -- verified via UE forum discussions and UE 5.7 documentation links.

---

## PhotoshopAPI Integration

### Version and Status

- **Current version:** v0.9.0 (released April 6, 2024)
- **License:** MIT
- **C++ Standard:** C++20 (required)
- **CMake minimum:** 3.20
- **Source verified:** Cloned repo and inspected headers directly

### Dependencies (from CMakeLists.txt)

PhotoshopAPI v0.9.0 links against these libraries (all via vcpkg):

| Dependency | Purpose | Notes |
|---|---|---|
| OpenImageIO | Smart object image data extraction | Heavy dependency; pulls in libtiff, libpng, OpenEXR, etc. |
| libdeflate | ZIP compression/decompression for layer data | Lightweight |
| Eigen3 | Homography/transform math | Header-only (no .lib needed at link time) |
| fmt | Logging/formatting | Static lib |
| stduuid | Smart object UUID handling | Header-only |
| mio | Memory-mapped file I/O | Header-only |
| simdutf | UTF conversion (SIMD-optimized) | Static lib |
| blosc2 | Compression (used internally) | Static lib, PIC enabled |

### CMake Pre-Build Step

PhotoshopAPI must be built outside UBT as a pre-build step. UBT does not support CMake natively.

**Build script (Win64):**
```bash
# Clone and build PhotoshopAPI as static lib
git clone --branch v0.9.0 --depth 1 https://github.com/EmilDohne/PhotoshopAPI.git
cd PhotoshopAPI
cmake -B build -G "Visual Studio 17 2022" -A x64 \
    -DCMAKE_BUILD_TYPE=Release \
    -DPSAPI_BUILD_STATIC=ON \
    -DPSAPI_BUILD_TESTS=OFF \
    -DPSAPI_BUILD_EXAMPLES=OFF \
    -DPSAPI_BUILD_BENCHMARKS=OFF \
    -DPSAPI_BUILD_DOCS=OFF \
    -DPSAPI_BUILD_PYTHON=OFF \
    -DPSAPI_USE_VCPKG=ON \
    -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreadedDLL"
cmake --build build --config Release
```

**CRITICAL:** UE uses `/MD` (MultiThreadedDLL) runtime by default. PhotoshopAPI defaults to `/MT` (MultiThreaded) when building Python bindings. Set `-DPSAPI_BUILD_PYTHON=OFF` and `-DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreadedDLL"` to match UE's CRT. Mismatched CRT = linker errors or runtime crashes.

**Build script (Mac):**
```bash
cmake -B build -G "Xcode" \
    -DCMAKE_BUILD_TYPE=Release \
    -DPSAPI_BUILD_STATIC=ON \
    -DPSAPI_BUILD_TESTS=OFF \
    -DPSAPI_BUILD_EXAMPLES=OFF \
    -DPSAPI_BUILD_BENCHMARKS=OFF \
    -DPSAPI_BUILD_DOCS=OFF \
    -DPSAPI_BUILD_PYTHON=OFF \
    -DPSAPI_USE_VCPKG=ON \
    -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
cmake --build build --config Release
```

### ThirdParty Directory Layout

```
Plugins/PSD2UMG/
    ThirdParty/
        PhotoshopAPI/
            include/           # PhotoshopAPI.h (single include header)
            src/               # Internal headers needed by includes
            lib/
                Win64/
                    PhotoshopAPI.lib
                    libdeflate.lib
                    simdutf.lib
                    fmt.lib
                    OpenImageIO.lib
                    OpenImageIO_Util.lib
                Mac/
                    libPhotoshopAPI.a
                    libdeflate.a
                    libsimdutf.a
                    libfmt.a
                    libOpenImageIO.a
                    libOpenImageIO_Util.a
    Source/
        PSD2UMG/
            PSD2UMG.Build.cs
            Public/
            Private/
```

**Confidence:** HIGH -- CMakeLists.txt read directly from source. The exact set of .lib files that need shipping depends on what vcpkg produces (OpenImageIO has transitive deps). The build script will need iteration during Phase 1 to capture the full set.

### OpenImageIO Concern

OpenImageIO is a heavyweight dependency (100+ MB compiled). For this plugin's use case (reading PSD layer pixel data), it may be possible to strip it if PhotoshopAPI can function without smart object extraction. If not, the full dependency chain must ship.

**Recommendation:** In Phase 1, test building PhotoshopAPI with a stub OpenImageIO. If smart object layer reading is needed (Phase 5), include the full lib. Otherwise, investigate if the dependency can be made optional via CMake configuration.

---

## PhotoshopAPI Text Layer Status

### Issue #126 Resolution

**Status: RESOLVED in v0.9.0** (confirmed by reading source code directly)

The GitHub issue #126 tracked text layer support. The v0.9.0 release (April 2024) closed this issue with comprehensive text layer functionality. The WebFetch of issue #126 reported it as "Closed (Completed)."

**IMPORTANT NUANCE:** The WebFetch of the issue page mentioned text layers were "not implemented" -- but this reflected an older state of the issue discussion. The v0.9.0 release notes explicitly state "Add support for editable text layer. You can now read/parse/update/create text layer data." The source code confirms this with a full `TextLayer<T>` struct and 15+ header files implementing text layer functionality.

### Available Text Layer API (verified from source code)

PhotoshopAPI v0.9.0 provides a comprehensive text layer API via CRTP mixins on `TextLayer<T>`:

**Core text read/write (TextLayer.h):**
- `text()` -> `std::optional<std::string>` -- returns UTF-8 text content
- `set_text(const std::string&)` -- set text, remaps style ranges automatically
- `replace_text(old, new)` -- find-replace preserving style runs
- `anti_alias()` / `set_anti_alias()` -- anti-aliasing method (None/Crisp/Strong/Smooth/Sharp)

**Font management (TextLayerFontMixin.h):**
- `font_count()` -- number of fonts in the font set
- `font_postscript_name(index)` -> `std::optional<std::string>` -- PostScript font name
- `font_type(index)` -> FontType (OpenType/TrueType)
- `font_script(index)` -> FontScript (Roman/CJK)
- `primary_font_name()` -> convenience for first used font
- `used_font_names()` -> all fonts actually referenced by style runs
- `find_font_index(name)` / `add_font(name)` -- font management
- `set_font(name)` -- set font for all style runs at once

**Per-style-run properties (TextLayerStyleRunMixin.h):**
- `style_run_count()` -- number of style runs
- `style_run_font_size(i)` -> `std::optional<double>` -- font size in points
- `style_run_font(i)` -> font index in font set
- `style_run_fill_color(i)` -> `std::optional<std::vector<double>>` -- RGBA color (4 doubles)
- `style_run_stroke_color(i)` -> stroke color
- `style_run_faux_bold(i)` / `style_run_faux_italic(i)` -> bold/italic flags
- `style_run_tracking(i)` -> letter spacing (tracking value)
- `style_run_leading(i)` -> line spacing
- `style_run_auto_leading(i)` -> auto-leading flag
- `style_run_baseline_shift(i)` -> baseline offset
- `style_run_underline(i)` / `style_run_strikethrough(i)` -> decoration flags
- `style_run_font_caps(i)` -> FontCaps enum (Normal/SmallCaps/AllCaps)
- `style_run_font_baseline(i)` -> FontBaseline (Normal/Superscript/Subscript)
- `style_run_horizontal_scale(i)` / `style_run_vertical_scale(i)` -> scaling
- `style_run_outline_width(i)` -> stroke width
- All getters have corresponding `set_style_run_*()` setters

**Paragraph properties (TextLayerParagraphRunMixin.h):**
- `paragraph_run_count()` -- number of paragraph runs
- `paragraph_run_justification(i)` -> Justification enum (Left/Right/Center/JustifyLastLeft/etc.)
- `paragraph_run_first_line_indent(i)` / `start_indent(i)` / `end_indent(i)`
- `paragraph_run_space_before(i)` / `space_after(i)`
- `paragraph_run_leading_type(i)` -> LeadingType (BottomToBottom/TopToTop)

**Text layer enums (TextLayerEnum.h):**
- `WritingDirection` -- Horizontal/Vertical
- `ShapeType` -- PointText/BoxText
- `Justification` -- Left/Right/Center/JustifyLastLeft/JustifyLastRight/JustifyLastCenter/JustifyAll
- `FontCaps` -- Normal/SmallCaps/AllCaps
- `FontBaseline` -- Normal/Superscript/Subscript
- `CharacterDirection` -- Default/LeftToRight/RightToLeft
- `AntiAliasMethod` -- None/Crisp/Strong/Smooth/Sharp
- `WarpStyle` -- 17 warp types (NoWarp through Custom)

**Layer base properties (Layer.h, inherited by TextLayer):**
- `name()` -> layer name (UTF-8 string)
- `width()` / `height()` -> layer dimensions
- `center_x()` / `center_y()` -> position relative to canvas top-left
- `top_left_x()` / `top_left_y()` -> computed corner position
- `opacity()` -> 0.0-1.0
- `visible()` -> visibility flag
- `blendmode()` -> blend mode enum

### What This Means for PSD2UMG

**No manual TySh parsing needed.** PhotoshopAPI v0.9.0 provides everything required:

| PSD Property | PhotoshopAPI Method | UMG Target |
|---|---|---|
| Text content | `text()` | `UTextBlock::SetText()` |
| Font name (PostScript) | `primary_font_name()` / `style_run_font()` + `font_postscript_name()` | Font mapping table -> UFont asset |
| Font size (points) | `style_run_font_size(0)` | `FSlateFontInfo::Size` (with DPI conversion) |
| Fill color | `style_run_fill_color(0)` | `UTextBlock::SetColorAndOpacity()` |
| Bold/Italic | `style_run_faux_bold(0)` / `style_run_faux_italic(0)` | Font typeface selection |
| Alignment | `paragraph_run_justification(0)` | `UTextBlock::SetJustification()` |
| Letter spacing | `style_run_tracking(0)` | `FSlateFontInfo::LetterSpacing` |
| Underline/Strikethrough | `style_run_underline(0)` / `style_run_strikethrough(0)` | `FTextBlockStyle` decoration |
| Outline width | `style_run_outline_width(0)` | `FSlateFontInfo::OutlineSettings` |
| Stroke color | `style_run_stroke_color(0)` | `FSlateFontInfo::OutlineSettings.OutlineColor` |

**Confidence:** HIGH -- every method listed above was verified by reading the actual header files in the cloned PhotoshopAPI repository.

### Multi-Style-Run Consideration

Photoshop supports multiple style runs within a single text layer (e.g., "Hello **world**" with mixed bold). UMG `UTextBlock` does not support mixed inline styles -- it applies one style to the entire text. For Phase 3 (Text & Typography), the strategy should be:
1. Use the first style run's properties as the "dominant" style
2. Log a warning when multiple style runs with different properties exist
3. Future: consider `URichTextBlock` for multi-style text (Phase 7 stretch goal)

---

## UE 5.7 API Changes

### Confirmed Deprecations and Replacements

| Deprecated (UE4/early UE5) | Replacement (UE 5.7) | Module | Status |
|---|---|---|---|
| `FEditorStyle::Get()` | `FAppStyle::Get()` | SlateCore | `FEditorStyle` fully removed in UE 5.5+ |
| `FEditorStyle::GetBrush()` | `FAppStyle::GetBrush()` | SlateCore | Same |
| `FEditorStyle::GetStyleSetName()` | `FAppStyle::GetAppStyleSetName()` | SlateCore | Same |
| `WhitelistPlatforms` (.uplugin) | `PlatformAllowList` | Projects | UE 5.0+ (old key silently ignored) |
| `BlacklistPlatforms` (.uplugin) | `PlatformDenyList` | Projects | UE 5.0+ |
| `#include "AssetRegistryModule.h"` | `#include "AssetRegistry/AssetRegistryModule.h"` | AssetRegistry | Path changed in UE 5.0 |
| `FAssetRegistryModule::GetRegistry()` | `IAssetRegistry::Get()` | AssetRegistry | Singleton pattern change |
| `EditorScriptingUtilities` module | Functionality merged into `UnrealEd` | UnrealEd | Module removed |
| `PythonScriptPlugin` dependency | REMOVE (not needed) | N/A | We no longer use Python |

**Confidence:** HIGH for FEditorStyle/FAppStyle (verified in UE 5.7 docs). MEDIUM for AssetRegistry path changes (verified via forum posts and plugin migration issues, not official changelog).

### Widget Blueprint Creation Pattern (UE 5.7)

The programmatic widget blueprint creation pattern for UE5 uses:

```cpp
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

// Creating a widget in the WidgetTree:
UWidget* NewWidget = WidgetBlueprint->WidgetTree->ConstructWidget<UCanvasPanel>();

// Adding to parent:
UCanvasPanelSlot* Slot = ParentCanvas->AddChildToCanvas(NewWidget);
Slot->SetOffsets(FMargin(Left, Top, Width, Height));
Slot->SetAnchors(FAnchors(0.f, 0.f)); // Top-left

// CRITICAL: After modifying the widget tree, mark the blueprint as modified:
FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
```

**Key rules:**
1. Use `WidgetTree->ConstructWidget<T>()`, not `NewObject<T>()` -- ConstructWidget registers the widget properly
2. Call `FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified()` after tree changes to sync the editor
3. Wrap editor-only code in `#if WITH_EDITOR` guards
4. `UMGEditor` and `UnrealEd` modules are required dependencies

**Confidence:** MEDIUM -- Pattern verified from a UE5 tutorial (unreal-garden.com) and UE 5.7 API docs showing UWidgetBlueprint in UMGEditor module. The exact API surface may have minor differences in 5.7 vs 5.4 but the core pattern (ConstructWidget + MarkBlueprintAsStructurallyModified) is stable across UE5 versions.

### UWidgetBlueprint Factory Pattern

To create a new UWidgetBlueprint asset from scratch:

```cpp
#include "WidgetBlueprintFactory.h"

// Create the factory
UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
Factory->ParentClass = UUserWidget::StaticClass();

// Create the asset via AssetTools
IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
UObject* NewAsset = AssetTools.CreateAsset(
    AssetName,
    PackagePath,
    UWidgetBlueprint::StaticClass(),
    Factory
);
UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(NewAsset);
```

**Confidence:** MEDIUM -- This pattern is standard UE5 asset creation. The `UWidgetBlueprintFactory` class exists in UMGEditor. Exact constructor behavior should be validated during Phase 2.

### ToolMenus vs Old Extension Points

UE5 uses `UToolMenus` for editor menu/toolbar extension instead of the UE4 `FExtender` pattern:

```cpp
UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
{
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu");
    FToolMenuSection& Section = Menu->FindOrAddSection("AssetContextMenu");
    Section.AddMenuEntry(
        "ImportPsdAsWidget",
        LOCTEXT("ImportPsd", "Import as Widget Blueprint"),
        LOCTEXT("ImportPsdTooltip", "Import selected PSD as UMG Widget Blueprint"),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateLambda([](){ /* handler */ }))
    );
}));
```

**Confidence:** MEDIUM -- UToolMenus is the UE5 standard, but exact menu path strings need validation.

---

## Recommended Stack Summary

### Core Framework

| Technology | Version | Purpose | Why |
|---|---|---|---|
| Unreal Engine | 5.7.4 | Host engine | Project requirement |
| C++20 | (MSVC 19.x / Clang 16+) | Plugin language | Required by PhotoshopAPI, beneficial for concepts/ranges |
| PhotoshopAPI | v0.9.0 | PSD parsing | Only viable C++20 PSD parser with text layer support |

### Build System

| Technology | Version | Purpose | Why |
|---|---|---|---|
| Unreal Build Tool | 5.7 | Plugin compilation | Required by UE |
| CMake | 3.20+ | PhotoshopAPI pre-build | PhotoshopAPI requirement |
| vcpkg | (bundled with PhotoshopAPI) | PhotoshopAPI deps | Simplest path for OIIO/libdeflate/etc. |

### Key Libraries (via PhotoshopAPI)

| Library | Purpose | Link Type |
|---|---|---|
| PhotoshopAPI | PSD/PSB read/write | Static (.lib/.a) |
| OpenImageIO | Image format support | Static (transitive) |
| libdeflate | ZIP compression | Static (transitive) |
| simdutf | UTF encoding | Static (transitive) |
| fmt | Logging | Static (transitive) |
| Eigen3 | Math | Header-only |
| mio | Memory-mapped IO | Header-only |

### UE5 Modules Required

| Module | Purpose |
|---|---|
| Core | Base types |
| CoreUObject | UObject system |
| Engine | Core engine |
| Slate, SlateCore | UI framework |
| UMG | Widget classes (UCanvasPanel, UImage, UTextBlock, etc.) |
| UMGEditor | UWidgetBlueprint, WidgetTree |
| UnrealEd | Editor utilities |
| AssetRegistry | Asset discovery |
| AssetTools | Asset creation |
| DeveloperSettings | Plugin settings |
| ToolMenus | Editor menus (UE5 pattern) |

---

## Confidence Levels

| Area | Confidence | Reason |
|---|---|---|
| Build.cs module list | HIGH | Cross-referenced with UE 5.7 API docs |
| CppStandard = Cpp20 | HIGH | Verified in UE forum and docs, required by PhotoshopAPI |
| PhotoshopAPI text layer API | HIGH | Read source code directly from cloned repo |
| PhotoshopAPI build integration | MEDIUM | CMakeLists.txt verified, but CRT matching and transitive deps need build-time validation |
| UE5 API deprecations | HIGH | FEditorStyle->FAppStyle verified, WhitelistPlatforms->PlatformAllowList verified |
| Widget Blueprint creation pattern | MEDIUM | Verified from tutorial + API docs, not from UE5 engine source |
| OpenImageIO dependency weight | LOW | Size estimate from general knowledge; may be strippable but needs testing |

## Sources

- [PhotoshopAPI GitHub Repository](https://github.com/EmilDohne/PhotoshopAPI) -- cloned and inspected directly
- [PhotoshopAPI v0.9.0 Release Notes](https://github.com/EmilDohne/PhotoshopAPI/releases/tag/v0.9.0)
- [PhotoshopAPI Issue #126](https://github.com/EmilDohne/PhotoshopAPI/issues/126) -- text layer tracking issue (closed)
- [UE 5.7 Third-Party Library Integration](https://dev.epicgames.com/documentation/en-us/unreal-engine/integrating-third-party-libraries-into-unreal-engine)
- [UE 5.7 UWidgetBlueprint API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Editor/UMGEditor/UWidgetBlueprint)
- [UE 5.7 FAppStyle API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/SlateCore/Styling/FAppStyle)
- [UE 5.7 AssetRegistry](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/AssetRegistry)
- [UE 5.7 AssetTools](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Developer/AssetTools)
- [Georgy's UE5 Third-Party Integration Guide](https://georgy.dev/posts/third-party-integration/)
- [Unreal Garden: Build Widgets in Editor](https://unreal-garden.com/tutorials/build-widgets-in-editor/)
- [UE Forum: WhitelistPlatforms to PlatformAllowList](https://forums.unrealengine.com/t/whitelist-blacklist-allow-for-uplugin/1392743)
- [Compile UE5 with C++20](https://dev.epicgames.com/community/snippets/J72/compile-unreal-engine-5-1-with-c-20)

---

---

# v1.3 Advanced Effects — Stack Additions

**Milestone:** v1.3 Advanced Effects
**Researched:** 2026-04-28
**Scope:** Only what is NEW or CHANGED for the five v1.3 features. Everything in the section above is already shipping and is not repeated here.

---

## Feature 1 — vstk descriptor parsing (image/shape-layer stroke)

### What vstk is

`vstk` (key `"vstk"`, `TaggedBlockKey::vecStrokeData`) is a tagged block written to shape and image layers when the designer applies a vector stroke via the layer-properties panel (not the Layer Style dialog). It is distinct from:

- `lfx2 / FrFX` — Layer Style stroke (already parsed by `ParseFrFXDescriptor`)
- `vscg` (`vecStrokeContentData`) — stores the fill content of the stroke (solid color, gradient, pattern)

PhotoshopAPI exposes `vstk` in `unparsed_tagged_blocks()` as a raw `TaggedBlock` with `m_Data` bytes. The library does NOT decode it; the full descriptor byte array is available for direct parsing. Confirmed present in `Enum.h` line 900: `{"vstk", TaggedBlockKey::vecStrokeData}`.

### What vstk contains

The `vstk` payload is a Photoshop object descriptor (same format family as `SoCo`/`vscg` already handled). The descriptor classID is `"strokeStyle"`. Relevant keys:

| Key | ostype | Meaning |
|-----|--------|---------|
| `"strokeEnabled"` | `bool` | Whether stroke is on |
| `"fillEnabled"` | `bool` | Whether fill is on (can coexist) |
| `"strokeStyleLineWidth"` | `UntF` | Width, unit `#Pxl`, value is BE double after 4-byte unit tag |
| `"strokeStyleLineAlignment"` | `enum` | `"strokeStyleAlignInside"` / `"strokeStyleAlignOutside"` / `"strokeStyleAlignCenter"` |
| `"strokeStyleContent"` | `Objc` | Sub-descriptor; classID `"solidColorLayer"` → `"Clr " / Objc / "RGBC"` with `"Rd  "`, `"Grn "`, `"Bl  "` doubles 0–255 |

### Integration with existing lrFX walker

The existing `SkipValueAfterOsType` lambda in `ParseFrFXDescriptor` and `ScanSolidFillColor` already handles `UntF`, `enum`, and nested `Objc` / `VlLs`. No new descriptor-walking infrastructure is needed. The implementation is a new static function `ScanVstkStroke(...)` that mirrors `ScanSolidFillColor`'s `TryParseAt` pattern:

1. Iterate `unparsed_tagged_blocks()` looking for `Block->getKey() == TaggedBlockKey::vecStrokeData`.
2. Try offset 4 first (4-byte version prefix), fall back to 0 and 8.
3. Walk the descriptor: read `"strokeEnabled"` bool; read `"strokeStyleLineWidth"` UntF (skip 4-byte unit tag, read 8-byte BE double); drill into `"strokeStyleContent"` Objc to extract `"Clr " / Objc / "RGBC"` using the same named-key pattern as `ScanSolidFillColor`.
4. Write result into the existing `FPsdLayerEffects.bHasStroke`, `StrokeColor`, `StrokeSize` fields.

`ScanVstkStroke` must be called before the `lfx2/FrFX` path so that vstk wins for shape/image layers (vstk is the newer and more authoritative format for these layer types).

### Data model: no new fields

`FPsdLayerEffects` already carries `bHasStroke`, `StrokeColor`, `StrokeSize`. These fields are reused.

### Stroke rendering on image/shape layers (mapper side)

Use the existing drop-shadow overlay pattern: create a slightly larger background `UImage` (stroke color tint, expanded by `StrokeSize` pixels on each side) placed behind the main image inside a `UOverlay`. Requires no new UMG modules.

Do NOT attempt to use `UBorder` for stroke rendering — `UBorder` wraps a single child slot and does not compose cleanly with `UImage` in a canvas panel.

**Confidence: HIGH** — confirmed from `Enum.h` (TaggedBlockKey) and the existing parallel descriptor walker structure in `ScanSolidFillColor`.

---

## Feature 2 — frameFXMulti / VlLs stroke (newer Photoshop format)

### The problem

Photoshop CC 2020+ serialises multiple layer effects as a `VlLs` list under `lfx2` key `"frameFXMulti"` instead of a single `"FrFX" Objc`. The existing `ParseFrFXDescriptor` handles `VlLs` as a skip target only; it does not inspect the list for stroke content.

### Where and how to extend ParseFrFXDescriptor

The fix lives entirely inside `ParseFrFXDescriptor`. After the existing `if (ItemKey == "FrFX" && OsType == "Objc")` branch, add:

```cpp
else if (ItemKey == "frameFXMulti" && FCStringAnsi::Strcmp(OsType, "VlLs") == 0)
{
    uint32 ListCount = ReadU32BE();
    for (uint32 e = 0; e < ListCount && CheckRemaining(4) && !bFoundStroke; ++e)
    {
        // Each element: 4-byte ostype tag (expected "Objc"), then FrFX Objc body
        char ElemOT[5] = {};
        for (int k = 0; k < 4; ++k) ElemOT[k] = static_cast<char>(Data[Pos + k]);
        Pos += 4;
        if (FCStringAnsi::Strcmp(ElemOT, "Objc") != 0) { SkipValueAfterOsType(ElemOT); continue; }
        // Reuse the existing FrFX parse block verbatim (SkipUnicodeString + ReadPsString + inner loop)
        // ... (duplicate of existing FrFX parse block; first enabled stroke wins)
        bFoundStroke = true; // stop on first match
    }
}
```

The `SkipValueAfterOsType` lambda already handles unknown effect types (glow, inner shadow, etc.) inside the list via its `Objc` recursive branch — they will be skipped cleanly.

### No changes to scan entry point or data model

The raw lfx2 byte scan already locates the correct block. `FPsdLayerEffects` fields are unchanged.

**Confidence: HIGH** — the PSD spec is clear; the extension is additive to the existing walker.

---

## Feature 3 — PtFl pattern fill descriptor

### TaggedBlockKey confirmed

`PtFl` maps to `TaggedBlockKey::adjPattern` in `Enum.h` line 832: `{"PtFl", TaggedBlockKey::adjPattern}`. Available as raw bytes via `unparsed_tagged_blocks()`.

### What PtFl contains

| Key | ostype | Meaning |
|-----|--------|---------|
| `"Ptrn"` | `Objc` | Pattern metadata sub-descriptor |
| `"Ptrn" > "Nm  "` | `TEXT` | Pattern name (UTF-16 string) |
| `"Ptrn" > "Idnt"` | `TEXT` | Pattern UUID |
| `"Scl "` | `UntF` (`#Prc`) | Scale percentage |
| `"Lnkd"` | `bool` | Linked-to-layer flag |
| `"Angl"` | `doub` | Rotation angle |
| `"Clr "` | `Objc` / `"RGBC"` | Tint color (often absent) |

### Why pattern tile extraction is out of scope for v1.3

Pattern tiles are stored in the global `Pat2`/`Pat3` tagged blocks at the document level, matched by the UUID in `PtFl`. Extracting pixel data requires walking a separate section, decoding a channel-format pixel array, and compositing. This is a substantial new parsing surface not justified by v1.3 scope.

### v1.3 deliverable for PtFl

1. Detect `PtFl` blocks and set `EPsdLayerType::PatternFill` (new enum value added alongside `SolidFill`).
2. Extract `"Scl "` UntF value and `"Ptrn" > "Nm  "` TEXT string into new `FPsdLayerEffects` fields.
3. Map `PatternFill` layers to a `UImage` with a 1×1 white placeholder texture. Emit a `UE_LOG(Warning)` naming the pattern so the user knows it was not embedded.

### New fields required

Add to `FPsdLayerEffects`:

```cpp
bool bHasPatternFill = false;
FString PatternFillName;    // from Ptrn.Nm  (UTF-16 TEXT ostype decoded to FString)
float PatternFillScale = 100.f;  // from Scl  UntF #Prc
```

Add `EPsdLayerType::PatternFill` to the enum.

### New mapper

`FPatternFillLayerMapper` at priority 101. `CanMap` checks `Layer.Type == EPsdLayerType::PatternFill`. Emits `UImage` with white placeholder. No new UE5 modules needed.

**Confidence: MEDIUM** — TaggedBlockKey confirmed in `Enum.h`; descriptor layout cross-referenced from Adobe PSD spec and matches the SoCo/vscg pattern. Exact offset alignment needs verification against a real PtFl PSD block during implementation.

---

## Feature 4 — lrFX RGBC channel-order verification

### Current parser is structurally correct

The `lrFX` v0 `sofi` parser (PsdParser.cpp lines 700–721) reads `C0`, `C1`, `C2` from a `ColorSpace + 4×uint16` block and assigns:

```cpp
OutLayer.Effects.ColorOverlayColor = FLinearColor(C0, C1, C2, A);
```

For colorSpace = 0 (RGBC), the PSD spec defines the channel order as R, G, B, reserved. The code reads them in order: `C0=R`, `C1=G`, `C2=B`. This assignment is correct.

The `dsdw` parser uses the same layout and the same positional assignment — also correct.

All named-key paths (`FrFX`, `SoCo`, `vscg`, `vstk`) read `"Rd  "`, `"Grn "`, `"Bl  "` by key name. No channel-order risk exists on those paths.

### No code change for this item

The only v1.3 deliverable is:
1. A PSD fixture with known solid-color lrFX sofi overlays (pure red `FF0000`, pure green `00FF00`, pure blue `0000FF`) on image layers.
2. An automation spec `It()` block asserting `Effects.ColorOverlayColor.R > 0.9` for the red layer, `G > 0.9` for green, `B > 0.9` for blue. This provides a regression guard without requiring visual inspection.

No changes to `PsdTypes.h`, `PsdParser.cpp`, or any mapper.

**Confidence: HIGH** — Code confirms R=C0, G=C1, B=C2. Per PSD spec, RGBC colorSpace=0 lays out channels as R, G, B, reserved in that order. The existing code is correct.

---

## Feature 5 — UTF-16 code-unit slicing fix for non-ASCII URichTextBlock spans

### Root cause

`style_run_lengths()` returns `std::vector<int32_t>` of **UTF-16 code-unit counts** (PhotoshopAPI `TextLayer.h` comment: "Get the style run lengths as a list of code-unit counts"; `TextLayerRunSplitUtils.h` reads from `"RunLengthArray"` in EngineData verbatim, which per the PSD spec stores code-unit counts).

The current slicing code (PsdParser.cpp span-extraction block):

```cpp
const FString FullText = FString(UTF8_TO_TCHAR(FullUtf8.c_str()));
Span.Text = FullText.Mid(CharOffset, Clipped);
CharOffset += Clipped;
```

On Windows, `FString` is UTF-16 internally (TCHAR = wchar_t = char16_t = 2 bytes). `FString::Len()` counts UTF-16 code units and `FString::Mid()` slices by code units. Since PhotoshopAPI run lengths are also UTF-16 code units, **the slicing is correct on Windows for all Unicode including CJK and emoji** (a CJK character is 1 code unit; an emoji outside BMP is 2 surrogate-pair code units, consistent on both sides).

On Mac (where TCHAR = wchar_t = char32_t = 4 bytes on some compiler configurations), `FString::Len()` returns code-point count, which diverges from PhotoshopAPI's UTF-16 code-unit counts for any non-BMP character. This is where the bug manifests.

### Fix

Replace the `FString::Mid` slicing approach with `std::u16string::substr` directly against the UTF-16 representation:

```cpp
// Convert UTF-8 text to UTF-16LE once before the run loop.
// UnicodeString::convertUTF8ToUTF16LE is available via the PhotoshopAPI headers
// already included (TextLayerU16Utils.h includes UnicodeString.h).
const std::u16string FullU16 = UnicodeString::convertUTF8ToUTF16LE(FullUtf8);
size_t U16Offset = 0;

for (size_t ri = 0; ri < RunCount; ++ri)
{
    // ... (existing per-run property extraction unchanged) ...
    const int32 RunLen = LengthsOpt ? static_cast<int32>((*LengthsOpt)[ri]) : 0;
    if (RunLen > 0)
    {
        const size_t Clipped = static_cast<size_t>(
            FMath::Min<int64>(RunLen, static_cast<int64>(FullU16.size()) - static_cast<int64>(U16Offset)));
        const std::u16string SliceU16 = FullU16.substr(U16Offset, Clipped);
        U16Offset += Clipped;
        // Convert UTF-16LE slice back to UTF-8, then to FString via UTF8_TO_TCHAR.
        // This path is correct on both Windows and Mac regardless of TCHAR width.
        Span.Text = FString(UTF8_TO_TCHAR(
            UnicodeString::convertUTF16LEtoUTF8(SliceU16).c_str()));
    }
    // ...
}
```

`UnicodeString::convertUTF8ToUTF16LE` and `UnicodeString::convertUTF16LEtoUTF8` are declared in `Core/Struct/UnicodeString.h` which is included transitively by the PhotoshopAPI headers already included in `PsdParser.cpp`.

### No new headers, modules, or library additions required

The conversion utilities are already in the PhotoshopAPI include tree. No changes to `Build.cs`, `PsdTypes.h`, `FRichTextLayerMapper.cpp`, or any mapper.

**Confidence: HIGH** — Root cause confirmed by `TextLayer.h` doc comment and `TextLayerRunSplitUtils.h` source. Fix uses only already-included PhotoshopAPI utilities. The cross-platform correctness argument (UTF-16 code-unit-aligned slicing then convert to UTF-8 → TCHAR) is sound.

---

## v1.3 Change Summary

| Feature | Where to change | What to add | New UE5 API | New modules |
|---------|----------------|-------------|-------------|-------------|
| vstk stroke | `PsdParser.cpp`: new `ScanVstkStroke()` static fn | Called before lfx2 path in `ConvertLayerRecursive` | None | None |
| frameFXMulti VlLs | `PsdParser.cpp`: extend `ParseFrFXDescriptor` | New `frameFXMulti` branch in top-level descriptor walk | None | None |
| PtFl pattern fill | `PsdParser.cpp`: new `ScanPatternFill()` static fn; `PsdTypes.h`: `EPsdLayerType::PatternFill` + 3 `FPsdLayerEffects` fields; new `FPatternFillLayerMapper.cpp` | Priority-101 mapper emitting placeholder UImage | None | None |
| lrFX RGBC confirm | No code changes | Fixture PSD + automation spec assertions | None | None |
| UTF-16 span slicing | `PsdParser.cpp`: span-extraction loop, replace `FString::Mid` with `u16string::substr` + round-trip through UTF-8 | Removes existing TODO comment | None | None |

## v1.3 Sources

- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/Util/Enum.h` — TaggedBlockKey enumeration confirming `vstk` = `vecStrokeData`, `PtFl` = `adjPattern`
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` lines 800–1500 — `ParseFrFXDescriptor`, `ScanSolidFillColor`, `ScanShapeFillColor`, span-extraction loop
- `Source/PSD2UMG/Public/Parser/PsdTypes.h` — `FPsdLayerEffects`, `EPsdLayerType`, `FPsdTextRunSpan`
- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LayerTypes/TextLayer/TextLayer.h` lines 242–251 — `style_run_lengths()` documented as "code-unit counts"
- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LayerTypes/TextLayer/TextLayerRunSplitUtils.h` lines 144–153 — `get_style_run_lengths()` reads `RunLengthArray` verbatim
- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LayerTypes/TextLayer/TextLayerU16Utils.h` — `decode_utf16be_bytes`, `encode_utf16be_bytes`
- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/Core/Struct/UnicodeString.h` — `convertUTF8ToUTF16LE`, `convertUTF16LEtoUTF8`
- Adobe Photoshop File Format Specification (2023 revision) — `vstk`/`vscg` descriptor structure, `lrFX` sofi/dsdw channel layout, `PtFl` descriptor keys, `frameFXMulti` VlLs format

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

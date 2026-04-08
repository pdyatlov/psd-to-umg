# Domain Pitfalls

**Domain:** UE5 Editor Plugin -- PSD to UMG Widget Blueprint Converter (C++ rewrite from UE4/Python baseline)
**Researched:** 2026-04-07

---

## Critical Pitfalls

Mistakes that cause rewrites, editor crashes, or corrupt assets.

---

### Pitfall 1: WhitelistPlatforms Silently Ignored in UE5

**What goes wrong:** The current `.uplugin` uses `"WhitelistPlatforms": ["Win64", "Mac"]`. UE5 silently ignores this field -- it does not error, it does not warn. The result is that the plugin loads on ALL platforms (including unsupported ones like Linux, Android) where the ThirdParty static lib does not exist, causing linker failures or load-time crashes.

**Why it happens:** UE5 renamed `WhitelistPlatforms` to `PlatformAllowList` and `BlacklistPlatforms` to `PlatformDenyList`. The old names are not recognized at all; they are treated as unknown JSON keys and discarded.

**Consequences:** Plugin appears to work on Win64 during development. On distribution, users on unsupported platforms get confusing link errors or module-not-found crashes with no clear cause.

**Prevention:**
```json
"PlatformAllowList": ["Win64", "Mac"]
```
Replace in `.uplugin` immediately in Phase 0. Verify by checking `FModuleDescriptor::PlatformAllowList` is populated after load.

**Detection:** Search `.uplugin` for any `Whitelist` or `Blacklist` strings. If found, they are silently broken.

**Phase:** Phase 0 (UE 5.7 Port)

**Confidence:** HIGH -- confirmed in UE5 documentation and multiple plugin migration reports.

---

### Pitfall 2: Missing ShutdownModule Delegate Cleanup Causes Editor Crash on Plugin Reload

**What goes wrong:** The current `FAutoPSDUIModule::StartupModule()` registers a raw delegate via `ImportSubsystem->OnAssetReimport.AddRaw(this, &FAutoPSDUIModule::OnPSDImport)` but `ShutdownModule()` is empty. If the plugin is hot-reloaded or the module is unloaded, the delegate fires into a dangling `this` pointer, crashing the editor.

**Why it happens:** UE4 plugins rarely got reloaded in practice. UE5 editor has more aggressive module lifecycle management, and Live Coding can trigger partial reloads.

**Consequences:** Editor crash-to-desktop on module reload. Hard to diagnose because the crash occurs in an unrelated reimport event, not during plugin shutdown.

**Prevention:**
```cpp
void FPsd2UmgModule::ShutdownModule()
{
    if (GEditor)
    {
        if (UImportSubsystem* ImportSubsystem = GEditor->GetEditorSubsystem<UImportSubsystem>())
        {
            ImportSubsystem->OnAssetReimport.RemoveAll(this);
        }
    }
}
```
Also store the delegate handle from `AddRaw` and use `Remove(Handle)` for precision.

**Detection:** Enable `-stompmalloc` allocator during development; dangling pointer access will trigger immediate crash instead of silent corruption.

**Phase:** Phase 0 (UE 5.7 Port)

**Confidence:** HIGH -- this is a documented bug in CONCERNS.md and a known UE pattern.

---

### Pitfall 3: UWidgetBlueprint Created Without Compile Step Opens Corrupt in UMG Designer

**What goes wrong:** Creating a `UWidgetBlueprint` programmatically, adding widgets to its `WidgetTree`, and saving it without calling `FKismetEditorUtilities::CompileBlueprint()` or `FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified()` produces a `.uasset` that:
- Opens in UMG Designer but shows no widgets
- Shows widgets but they cannot be selected
- Crashes on next editor restart when the asset registry tries to load the generated class

**Why it happens:** `UWidgetBlueprint` is a Blueprint subclass. The `WidgetTree` is the design-time representation, but the `UWidgetBlueprintGeneratedClass` is what UMG actually uses. Without compilation, the generated class is stale or empty.

**Consequences:** Corrupted Widget Blueprints that look correct in Content Browser but crash or show empty in UMG Designer. Users lose trust in the tool.

**Prevention:** Follow this exact order of operations:
```cpp
// 1. Create package and blueprint asset
UPackage* Package = CreatePackage(*PackagePath);
UWidgetBlueprint* WidgetBP = CastChecked<UWidgetBlueprint>(
    FKismetEditorUtilities::CreateBlueprint(
        UUserWidget::StaticClass(),
        Package,
        AssetName,
        BPTYPE_Normal,
        UWidgetBlueprint::StaticClass(),
        UWidgetBlueprintGeneratedClass::StaticClass()
    )
);

// 2. Build the widget tree using WidgetTree->ConstructWidget<T>()
UCanvasPanel* RootCanvas = WidgetBP->WidgetTree->ConstructWidget<UCanvasPanel>(
    UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
WidgetBP->WidgetTree->RootWidget = RootCanvas;

// 3. Add child widgets via WidgetTree->ConstructWidget, NOT NewObject
UImage* Img = WidgetBP->WidgetTree->ConstructWidget<UImage>(
    UImage::StaticClass(), TEXT("MyImage"));
UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(Img);

// 4. Mark modified BEFORE compile
WidgetBP->Modify();

// 5. Compile the blueprint to regenerate the class
FKismetEditorUtilities::CompileBlueprint(WidgetBP);

// 6. Mark as structurally modified to update editor caches
FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);

// 7. Save the package
FSavePackageArgs SaveArgs;
SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
UPackage::SavePackage(Package, WidgetBP, *PackageFilename, SaveArgs);
```

**Detection:** After generation, open the Widget Blueprint in UMG Designer. If widgets are not visible or selectable, the compile step was skipped or failed.

**Phase:** Phase 2 (Layer Mapping / Widget Blueprint Generation)

**Confidence:** HIGH -- verified through UE5 documentation and community tutorials.

---

### Pitfall 4: Using NewObject Instead of WidgetTree->ConstructWidget for Child Widgets

**What goes wrong:** Creating UMG widgets with `NewObject<UImage>(WidgetBP)` instead of `WidgetBP->WidgetTree->ConstructWidget<UImage>()` produces widgets that exist as UObjects but are NOT registered in the widget tree. They appear to work in code but:
- Are invisible in UMG Designer's hierarchy panel
- Cannot be selected or edited by the designer
- May be garbage collected unexpectedly

**Why it happens:** `WidgetTree` maintains an internal registry of all widgets it owns. `ConstructWidget` both creates the object AND registers it. `NewObject` only creates the object.

**Consequences:** Widget Blueprints that look broken in the editor. Widgets randomly disappear. Extremely difficult to debug.

**Prevention:** NEVER use `NewObject` for UMG widgets in a WidgetBlueprint. Always use:
```cpp
UWidget* Widget = WidgetBP->WidgetTree->ConstructWidget<UWidgetClass>(
    UWidgetClass::StaticClass(), WidgetName);
```

**Detection:** After generation, verify `WidgetBP->WidgetTree->AllWidgets` count matches expected widget count.

**Phase:** Phase 2 (Layer Mapping)

**Confidence:** HIGH -- documented in UE5 source and confirmed by community tutorial.

---

### Pitfall 5: PhotoshopAPI Bit-Depth Templates Force Compile-Time Branching

**What goes wrong:** PhotoshopAPI uses template specialization for bit depths: `LayeredFile<bpp8_t>`, `LayeredFile<bpp16_t>`, `LayeredFile<bpp32_t>`. Opening a PSD requires knowing the bit depth upfront to instantiate the correct template. Developers write code for `bpp8_t` only, then encounter runtime crashes when a designer provides a 16-bit or 32-bit PSD.

**Why it happens:** The library is designed for maximum performance via compile-time type safety. There is no `LayeredFile<auto>` or virtual base class.

**Consequences:** 16-bit and 32-bit PSDs crash the parser or produce garbage data. Since many Photoshop users work in 16-bit for quality, this affects a significant percentage of real-world files.

**Prevention:** Read bit depth from PSD header first, then branch:
```cpp
auto file = NAMESPACE_PSAPI::File(psdPath);
auto header = file.readHeader(); // Peek at header without full parse
switch (header.m_Depth)
{
case NAMESPACE_PSAPI::Enum::BitDepth::BD_8:
    return ParseDocument<NAMESPACE_PSAPI::bpp8_t>(file);
case NAMESPACE_PSAPI::Enum::BitDepth::BD_16:
    return ParseDocument<NAMESPACE_PSAPI::bpp16_t>(file);
case NAMESPACE_PSAPI::Enum::BitDepth::BD_32:
    return ParseDocument<NAMESPACE_PSAPI::bpp32_t>(file);
}
```
Internally, convert all pixel data to 8-bit for texture export (UE textures are typically 8-bit per channel). The parser wrapper should normalize to a common `FPsdDocument` representation regardless of source bit depth.

**Detection:** Include test PSDs at all three bit depths in the test suite.

**Phase:** Phase 1 (C++ PSD Parser)

**Confidence:** HIGH -- documented in PhotoshopAPI README and API design.

---

### Pitfall 6: PhotoshopAPI Transitive Dependencies Not Linked

**What goes wrong:** PhotoshopAPI depends on several transitive libraries (c-blosc2, libdeflate, zlib-ng, possibly Iconv). Pre-building PhotoshopAPI as a static `.lib` does NOT bundle these dependencies into the output. The UE build links PhotoshopAPI.lib but gets unresolved external symbols for blosc2/zlib functions.

**Why it happens:** Static libraries are just archives of `.obj` files from their own source; they do not recursively include their dependencies. This is a fundamental property of static linking that catches many developers.

**Consequences:** Cryptic linker errors during UE build: `LNK2019: unresolved external symbol` for functions like `blosc2_decompress`, `libdeflate_zlib_decompress`, etc.

**Prevention:** When pre-building PhotoshopAPI via CMake, also collect all transitive static libs:
```
ThirdParty/
  PhotoshopAPI/
    include/           # All PhotoshopAPI headers
    lib/
      Win64/
        PhotoshopAPI.lib
        blosc2.lib
        libdeflate.lib
        zlib.lib         # or zlib-ng
      Mac/
        libPhotoshopAPI.a
        libblosc2.a
        ...
```
In `Build.cs`, add ALL of them:
```csharp
string[] Libs = { "PhotoshopAPI.lib", "blosc2.lib", "libdeflate.lib", "zlib.lib" };
foreach (string Lib in Libs)
{
    PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "lib", Platform, Lib));
}
```
Use CMake's `--target install` to get a complete set of outputs. Verify with `dumpbin /DEPENDENTS` on Windows.

**Detection:** Build failure with `LNK2019` errors referencing symbols not from PhotoshopAPI itself.

**Phase:** Phase 1 (C++ PSD Parser -- ThirdParty integration)

**Confidence:** HIGH -- universal static linking behavior, confirmed by UE ThirdParty integration guides.

---

### Pitfall 7: PSD Coordinate System vs UMG Canvas Panel Mismatch

**What goes wrong:** PSD layer bounds are stored as absolute pixel coordinates relative to the document canvas (top, left, bottom, right). Direct use of these as UCanvasPanelSlot positions produces widgets that are offset, scaled wrong, or anchored incorrectly because:

1. **DPI mismatch:** Photoshop defaults to 72 DPI; UMG operates at 96 DPI (Slate's native DPI). If the designer's PSD is at 72 DPI, all positions and sizes need scaling by `72/96 = 0.75`.
2. **Anchor origin:** UMG Canvas Panel anchors default to (0,0) = top-left of the canvas, which matches PSD. BUT if you set anchors to anything other than top-left (e.g., center or stretch), the position/offset semantics change completely.
3. **Negative coordinates:** PSD layers can extend beyond the canvas (negative top/left values). UMG widgets with negative positions work but may be clipped by parent containers.

**Why it happens:** PSD and UMG use superficially similar coordinate systems (origin top-left, Y-down) but the units (pixels at different DPIs) and the anchor system (PSD has none, UMG has a complex anchor/alignment system) are fundamentally different.

**Consequences:** Widgets appear in wrong positions, are wrong size, or overlap incorrectly. The import "works" but the output does not match the mockup.

**Prevention:**
```cpp
// Read PSD document DPI (usually 72 or 96)
float PsdDpi = PsdDocument.GetResolution(); // Default: 72.0f
float DpiScale = PsdDpi / 96.0f; // 72/96 = 0.75

// Convert layer bounds to UMG position/size
FVector2D Position(LayerBounds.Left * DpiScale, LayerBounds.Top * DpiScale);
FVector2D Size((LayerBounds.Right - LayerBounds.Left) * DpiScale,
               (LayerBounds.Bottom - LayerBounds.Top) * DpiScale);

// Set slot with top-left anchor (0,0) -- safest default
UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot);
Slot->SetAnchors(FAnchors(0.0f, 0.0f)); // Top-left
Slot->SetPosition(Position);
Slot->SetSize(Size);
Slot->SetAutoSize(false); // Explicit size, not auto
```

IMPORTANT: The PROJECT.md says "DPI conversion: Photoshop 72 DPI -> UMG 96 DPI (multiply by 0.75)". This means PSD coordinates are LARGER than UMG coordinates at 72 DPI. A 720px-wide element in a 72 DPI PSD becomes 540px in UMG. This is correct for point-based sizing. However, if the designer works at 96 DPI in Photoshop (common for UI work), the scale factor is 1.0 and no conversion is needed. ALWAYS read the actual DPI from the PSD header; do not hardcode 72.

**Detection:** Visual comparison of imported Widget Blueprint against source PSD at 1:1 zoom. Misalignment of more than 1-2 pixels indicates a DPI or coordinate issue.

**Phase:** Phase 2 (Layer Mapping) and Phase 3 (Text & Typography)

**Confidence:** HIGH -- confirmed by UMG documentation (Slate is 96 DPI) and Photoshop documentation (default 72 DPI).

---

## Moderate Pitfalls

---

### Pitfall 8: FEditorStyle Replaced by FAppStyle -- Compilation Failure

**What goes wrong:** Any code referencing `FEditorStyle::Get()`, `FEditorStyle::GetBrush()`, etc. fails to compile in UE5.1+. The class was deprecated and then removed.

**Prevention:** Global find-and-replace:
```
FEditorStyle::Get()        -> FAppStyle::Get()
FEditorStyle::GetBrush()   -> FAppStyle::GetBrush()
FEditorStyle::GetStyleSetName() -> FAppStyle::GetAppStyleSetName()
#include "EditorStyleSet.h" -> #include "Styling/AppStyle.h"
```

**Phase:** Phase 0 (UE 5.7 Port)

**Confidence:** HIGH -- documented deprecation.

---

### Pitfall 9: AssetRegistryModule.h Include Path Changed

**What goes wrong:** The current code uses `#include "AssetRegistryModule.h"` (line 11 of AutoPSDUI.cpp). In UE5, this header was moved. While it may still compile with a deprecation warning, some UE5 versions remove the redirect entirely.

**Prevention:**
```cpp
// Old (UE4):
#include "AssetRegistryModule.h"

// New (UE5):
#include "AssetRegistry/AssetRegistryModule.h"
// Or for direct registry access:
#include "AssetRegistry/IAssetRegistry.h"
```

Also update module access pattern:
```cpp
// Old:
FAssetRegistryModule& Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
IAssetRegistry& Registry = Module.Get();

// New (UE5 preferred):
IAssetRegistry& Registry = IAssetRegistry::GetChecked();
```

**Phase:** Phase 0 (UE 5.7 Port)

**Confidence:** HIGH -- confirmed in multiple UE5 migration reports.

---

### Pitfall 10: PythonScriptPlugin Dependency Must Be Fully Removed

**What goes wrong:** The current plugin declares `PythonScriptPlugin` as both a `.uplugin` dependency and a `Build.cs` `PrivateDependencyModuleNames` entry. In the C++ rewrite, simply removing the Python code is not enough -- if the dependency declarations remain, the plugin will fail to load in projects that do not have PythonScriptPlugin enabled.

**Prevention:** Remove from three locations:
1. `.uplugin` `Plugins` array -- delete the `PythonScriptPlugin` entry
2. `Build.cs` `PrivateDependencyModuleNames` -- remove `"PythonScriptPlugin"`
3. Any `#include` of PythonScriptPlugin headers in C++ files
4. Also remove `EditorScriptingUtilities` dependency if it was only needed for the Python workflow

**Detection:** Try loading the plugin in a blank project with PythonScriptPlugin disabled. If the plugin fails to load, the dependency was not fully removed.

**Phase:** Phase 0 (UE 5.7 Port)

**Confidence:** HIGH -- directly visible in current codebase.

---

### Pitfall 11: ThirdParty Header Include Order Conflicts with UE Macros

**What goes wrong:** PhotoshopAPI headers include standard C++ headers and define their own macros. UE defines macros like `check`, `verify`, `TEXT`, `PI`, etc. in global scope. Including PhotoshopAPI headers after UE headers causes macro conflicts. Including them before UE headers causes missing UE type definitions.

**Prevention:** Always wrap ThirdParty includes:
```cpp
// In a dedicated wrapper header: PsdApiIncludes.h

#pragma once

// Save and undefine conflicting UE macros
#pragma push_macro("check")
#pragma push_macro("verify")
#pragma push_macro("TEXT")
#undef check
#undef verify
#undef TEXT

THIRD_PARTY_INCLUDES_START
// Disable UE warnings for third-party code
#include "Macros.h"  // or whatever PhotoshopAPI's root header is
#include "LayeredFile/LayeredFile.h"
#include "PhotoshopFile/PhotoshopFile.h"
THIRD_PARTY_INCLUDES_END

// Restore UE macros
#pragma pop_macro("TEXT")
#pragma pop_macro("verify")
#pragma pop_macro("check")
```

Also add to `Build.cs`:
```csharp
bEnableUndefinedIdentifierWarnings = false; // Suppress warnings from third-party headers
PublicDefinitions.Add("NOMINMAX"); // Prevent Windows.h min/max conflicts
```

**Detection:** Compilation errors like `'check' : redefinition` or `'TEXT' : macro redefinition`, or mysterious behavior where standard C++ `check` is accidentally calling UE's `check` macro.

**Phase:** Phase 1 (C++ PSD Parser)

**Confidence:** HIGH -- universal UE plugin problem with any non-UE C++ library.

---

### Pitfall 12: PhotoshopAPI Text Layer Support is Newly Shipped and May Have Edge Cases

**What goes wrong:** PhotoshopAPI merged editable text layer support (PR #205) on March 30, 2026 -- only 8 days before this research. The feature is in milestone 0.7.0. While functional, it is new code and:
- Issue #208 reports `Layer` object lacks a `LayerType` enum to distinguish text from image layers
- Edge cases with complex text (mixed fonts in one layer, right-to-left text, vertical text) may not be handled
- The TySh descriptor format in PSD is notoriously complex and poorly documented

**Prevention:**
1. Pin to a specific PhotoshopAPI release tag (0.7.0 or later), not `main` branch
2. Implement a fallback: if text extraction fails, rasterize the text layer as an image (same as any other layer)
3. Write defensive parsing: wrap all text property extraction in try/catch with per-property fallbacks
4. Test with diverse text: CJK characters, mixed formatting runs, paragraph text vs point text

**Detection:** Test PSD with complex text formatting. If font name, size, or color are wrong/missing, the TySh parsing has a gap.

**Phase:** Phase 1 (C++ PSD Parser) and Phase 3 (Text & Typography)

**Confidence:** MEDIUM -- text layer API exists and works for basic cases, but edge case coverage is unverified given its recency.

---

### Pitfall 13: PSD Layer Order is Bottom-to-Top, UMG Z-Order Follows Add Order

**What goes wrong:** In Photoshop, layers in the Layers panel are ordered top-to-bottom visually, but the PSD file format stores them bottom-to-top (the first layer in the file is the bottom-most visually). Developers iterate layers in file order and add them to UMG in that order, resulting in REVERSED z-ordering where background elements appear on top.

**Prevention:** When traversing the PhotoshopAPI layer tree, either:
1. Reverse the iteration order before adding to the Canvas Panel, OR
2. Use `UCanvasPanelSlot::SetZOrder(int32)` to explicitly set z-order based on the layer's visual position

```cpp
// Approach: iterate layers and assign z-order from PSD visual order
int32 ZOrder = 0;
// Iterate in reverse if PhotoshopAPI returns bottom-to-top
for (int32 i = Layers.Num() - 1; i >= 0; --i)
{
    UWidget* Widget = CreateWidgetForLayer(Layers[i]);
    UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(Widget);
    Slot->SetZOrder(ZOrder++);
}
```

IMPORTANT: Verify PhotoshopAPI's actual iteration order empirically with a test PSD that has distinct top/bottom layers. The convention varies between PSD libraries.

**Detection:** Import a PSD with a known layer stack (e.g., background at bottom, text at top). If the result is visually inverted, layer order is wrong.

**Phase:** Phase 2 (Layer Mapping)

**Confidence:** MEDIUM -- PSD spec stores bottom-to-top but PhotoshopAPI may normalize this. Must verify empirically.

---

### Pitfall 14: SavePackage API Changed in UE5

**What goes wrong:** The old `UPackage::SavePackage(Package, Object, TopLevelFlags, *Filename)` 4-parameter overload is deprecated. Using it produces warnings or compilation failures depending on UE5 minor version.

**Prevention:** Use `FSavePackageArgs`:
```cpp
FSavePackageArgs SaveArgs;
SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
SaveArgs.Error = GError;
SaveArgs.bForceByteSwapping = false;
SaveArgs.bWarnOfLongFilename = true;

FString PackageFilename = FPackageName::LongPackageNameToFilename(
    PackagePath, FPackageName::GetAssetPackageExtension());
bool bSaved = UPackage::SavePackage(Package, WidgetBP, *PackageFilename, SaveArgs);
```

**Phase:** Phase 2 (Widget Blueprint Generation)

**Confidence:** HIGH -- documented API change.

---

### Pitfall 15: Plugin Binary Distribution Requires Exact Engine Version Match

**What goes wrong:** A plugin compiled against UE 5.7.3 will NOT load in UE 5.7.4 or 5.8.0. The engine checks module version stamps at load time. Distributing pre-compiled binaries on GitHub works only for the exact engine build they were compiled against.

**Consequences:** Users download the plugin, drop it into their project, and get "module built with different engine version" errors. This is the #1 complaint for GitHub-distributed UE plugins.

**Prevention:**
1. **Source distribution (recommended for GitHub):** Distribute source code + pre-built ThirdParty libs. Users compile the plugin module locally against their engine version. The static ThirdParty libs (PhotoshopAPI, blosc2, etc.) are ABI-stable across UE versions since they do not use UE types.
2. **Binary distribution (for Fab):** Build separate binaries for each supported UE version. Use `RunUAT BuildPlugin` to produce correct binaries.
3. **Hybrid:** Ship source for the UE module, pre-built binaries for ThirdParty dependencies only.

For GitHub release specifically:
```
PSD2UMG/
  Source/           # Full C++ source, compiles on user's engine
  ThirdParty/
    Win64/          # Pre-built static libs (engine-independent)
    Mac/
    include/
  PSD2UMG.uplugin
```

**Detection:** Test the distribution package in a clean project on a different UE patch version.

**Phase:** Phase 8 (Testing, Docs & Release)

**Confidence:** HIGH -- fundamental UE module system behavior, confirmed by Fab technical requirements.

---

### Pitfall 16: C++20 Requirement Creates Build Compatibility Issues

**What goes wrong:** PhotoshopAPI requires C++20 features (concepts, ranges, designated initializers). Setting `CppStandard = CppStandardVersion.Cpp20` in Build.cs is required. However:
1. UE's own headers may emit warnings or behave differently under C++20 mode
2. MSVC `/std:c++20` enables some features that can conflict with UE macro patterns (e.g., `<=>` spaceship operator, `char8_t`)
3. Older UE5 versions (5.0-5.2) had issues with C++20 mode; UE 5.7 should be fine

**Prevention:**
```csharp
CppStandard = CppStandardVersion.Cpp20;
// Only needed for the module that includes PhotoshopAPI headers
// Other modules in the plugin can stay at default C++ standard
```
If warnings are excessive, isolate PhotoshopAPI interaction into a single translation unit and use `THIRD_PARTY_INCLUDES_START/END` wrappers.

**Detection:** Build with `/W4` or `/Wall` and check for C++20-related warnings in UE headers.

**Phase:** Phase 1 (C++ PSD Parser)

**Confidence:** MEDIUM -- UE 5.7 officially supports C++20 but specific interaction with PhotoshopAPI's usage patterns is unverified.

---

## Minor Pitfalls

---

### Pitfall 17: Widget Names Must Be Unique Within a WidgetTree

**What goes wrong:** If two PSD layers have the same name (common with auto-generated layers like "Layer 1", "Layer 1 copy"), calling `WidgetTree->ConstructWidget` with duplicate names causes one widget to overwrite the other silently, or a runtime assertion in debug builds.

**Prevention:** Generate unique widget names by appending layer index or hash:
```cpp
FName WidgetName = MakeUniqueObjectName(WidgetBP->WidgetTree,
    UImage::StaticClass(), FName(*LayerName));
```

**Phase:** Phase 2 (Layer Mapping)

**Confidence:** HIGH -- documented UObject naming requirement.

---

### Pitfall 18: Texture Import Must Complete Before Widget Blueprint References It

**What goes wrong:** The PSD import flow needs to: (1) export layer images as PNGs, (2) import them as UTexture2D assets, (3) create the Widget Blueprint referencing those textures via SlateBrush. If step 3 runs before step 2 fully completes (including async texture processing), the SlateBrush references null textures.

**Prevention:** Use synchronous asset import with `FAssetImportTask::bAutomated = true` and `bSave = true`. Verify each texture is fully loaded before creating brush references:
```cpp
UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TexturePath);
check(Texture != nullptr); // Must exist before Widget Blueprint creation
```
Alternatively, batch all texture imports first, flush the asset registry, then create the Widget Blueprint.

**Detection:** Widget Blueprint opens but shows white/missing texture thumbnails.

**Phase:** Phase 2 (Layer Mapping)

**Confidence:** HIGH -- standard UE asset dependency ordering.

---

### Pitfall 19: GEditor Pointer is Null in Commandlet/CI Context

**What goes wrong:** The current code accesses `GEditor->GetEditorSubsystem<UImportSubsystem>()` directly. In commandlet mode (used in CI/CD, batch processing), `GEditor` is nullptr. This causes a null pointer crash during module startup.

**Prevention:**
```cpp
void FPsd2UmgModule::StartupModule()
{
    if (GEditor)
    {
        if (UImportSubsystem* ImportSubsystem = GEditor->GetEditorSubsystem<UImportSubsystem>())
        {
            ImportSubsystem->OnAssetReimport.AddRaw(this, &FPsd2UmgModule::OnPSDImport);
        }
    }
}
```

**Phase:** Phase 0 (UE 5.7 Port)

**Confidence:** HIGH -- known UE pattern; the plugin type is "Editor" which helps, but commandlets can still load editor modules.

---

### Pitfall 20: Hardcoded Plugin Path Breaks Outside ProjectPluginsDir

**What goes wrong:** The current code uses `FPaths::ProjectPluginsDir() / TEXT("AutoPSDUI/...")`. This path is wrong if the plugin is installed in the Engine's Plugins directory (Fab/Marketplace distribution) or in a subfolder of the project's Plugins directory.

**Prevention:** Use the module's own location:
```cpp
FString PluginBaseDir = IPluginManager::Get().FindPlugin(TEXT("PSD2UMG"))->GetBaseDir();
```
Or for the new C++ architecture, this is less relevant since Python file paths are eliminated. But any references to plugin content (test PSDs, default settings files) should use `IPluginManager`.

**Phase:** Phase 0 (UE 5.7 Port) -- relevant even though Python is being removed, because the pattern may be reused for other content references.

**Confidence:** HIGH -- documented in CONCERNS.md.

---

### Pitfall 21: No Progress Reporting for Large PSD Imports

**What goes wrong:** A 100+ layer PSD with large image layers can take 10-30 seconds to import. Without progress feedback, users think the editor has frozen and force-quit, corrupting partially-written assets.

**Prevention:** Wrap the import pipeline in `FScopedSlowTask`:
```cpp
FScopedSlowTask SlowTask(TotalLayers, LOCTEXT("ImportingPSD", "Importing PSD layers..."));
SlowTask.MakeDialog(true); // Show cancel button

for (const auto& Layer : Layers)
{
    if (SlowTask.ShouldCancel()) { break; }
    SlowTask.EnterProgressFrame(1.0f, FText::FromString(Layer.Name));
    // Process layer...
}
```

**Phase:** Phase 2 (Layer Mapping) or Phase 6 (Editor UI)

**Confidence:** HIGH -- standard UE editor UX pattern.

---

## Phase-Specific Warnings

| Phase | Likely Pitfall | Mitigation |
|-------|---------------|------------|
| Phase 0 (UE 5.7 Port) | WhitelistPlatforms silently ignored (#1), FEditorStyle removed (#8), AssetRegistry include changed (#9), PythonScriptPlugin dep not fully removed (#10), GEditor null in commandlets (#19) | Systematic find-and-replace of all deprecated identifiers. Build + load test in clean UE 5.7 project. |
| Phase 1 (C++ PSD Parser) | Transitive deps not linked (#6), bit-depth templates (#5), macro conflicts (#11), C++20 compat (#16), text layer edge cases (#12) | Build PhotoshopAPI with all deps first, verify linking before any parsing code. Create wrapper header for macro isolation. |
| Phase 2 (Layer Mapping) | Widget Blueprint corruption (#3), NewObject vs ConstructWidget (#4), layer order inversion (#13), SavePackage API (#14), widget name uniqueness (#17), texture import ordering (#18) | Follow exact order-of-operations for WidgetBlueprint creation. Verify with visual comparison test. |
| Phase 3 (Text & Typography) | DPI mismatch (#7), text parsing edge cases (#12) | Read actual DPI from PSD header. Test with diverse fonts and languages. |
| Phase 5 (Anchors) | Anchor semantics change position meaning (#7) | Use top-left anchor as safe default; only apply complex anchors with explicit user override suffixes. |
| Phase 8 (Release) | Binary version mismatch (#15), platform packaging (#1) | Ship source + pre-built ThirdParty. Test in multiple UE versions. |

---

## Sources

- [Georgy's Tech Blog: Third-Party Integration](https://georgy.dev/posts/third-party-integration/) -- Build.cs patterns
- [Unreal Garden: Build Widgets in Editor](https://unreal-garden.com/tutorials/build-widgets-in-editor/) -- WidgetTree programmatic API
- [UE5 Official: Integrating Third-Party Libraries](https://dev.epicgames.com/documentation/en-us/unreal-engine/integrating-third-party-libraries-into-unreal-engine)
- [UE5 Official: PlatformAllowList](https://docs.unrealengine.com/5.0/en-US/API/Runtime/Projects/FModuleDescriptor/PlatformAllowList/)
- [PhotoshopAPI GitHub](https://github.com/EmilDohne/PhotoshopAPI) -- Library architecture and limitations
- [Joyrok: UMG Layouts Tips](https://joyrok.com/UMG-Layouts-Tips-and-Tricks) -- UMG DPI and coordinate behavior
- [Fab Technical Requirements](https://support.fab.com/s/article/FAB-TECHNICAL-REQUIREMENTS?language=en_US) -- Distribution requirements
- [Epic Dev Community: Guide to Building and Packaging Plugins](https://dev.epicgames.com/community/learning/tutorials/Y4yO/epic-games-store-fab-guide-to-building-and-packaging-unreal-engine-plugins)
- [CesiumGS/cesium-unreal #1314](https://github.com/CesiumGS/cesium-unreal/issues/1314) -- FEditorStyle deprecation example
- [HttpGPT #75](https://github.com/lucoiso/UEHttpGPT/issues/75) -- WhitelistPlatforms vs PlatformAllowList

---

*Concerns audit: 2026-04-07*

# Architecture Patterns

**Domain:** UE5 Editor Plugin — PSD to UMG Widget Blueprint Importer
**Researched:** 2026-04-07

## Recommended Architecture

### Three-Stage Pipeline

```
 .psd file on disk
       |
       v
 +---------------------------+
 | Stage 1: PSD Parsing      |
 | PhotoshopAPI -> FPsdDoc   |
 | (no UE dependency)        |
 +---------------------------+
       |
       v  FPsdDocument (plain C++ value type)
       |
 +---------------------------+
 | Stage 2: Layer Mapping    |
 | IPsdLayerMapper strategy  |
 | FLayerMappingRegistry     |
 | -> FWidgetTreeModel       |
 +---------------------------+
       |
       v  FWidgetTreeModel (plain C++ value type)
       |
 +---------------------------+
 | Stage 3: Widget Building  |
 | FWidgetBlueprintGenerator |
 | UE UMG/UMGEditor APIs     |
 | -> UWidgetBlueprint        |
 +---------------------------+
       |
       v
 UWidgetBlueprint .uasset + UTexture2D .uasset(s)
```

Each stage is independently testable. Stages 1 and 2 have zero UE header dependency, making them unit-testable outside the engine. Stage 3 requires FAutomationTestBase.

### Component Boundaries

| Component | Responsibility | Communicates With |
|-----------|---------------|-------------------|
| `UPsdImportFactory` | UFactory subclass; hooks into UE import pipeline for .psd files; entry point for import and reimport | Stage 1 parser, Stage 3 generator |
| `FPsdParser` | Wraps PhotoshopAPI; produces `FPsdDocument` | PhotoshopAPI (ThirdParty) |
| `FPsdDocument` | Value type: layer tree with names, types, bounds, pixel data, text properties, visibility, opacity | Consumed by Stage 2 |
| `IPsdLayerMapper` | Interface: maps one FPsdLayer to an FWidgetNodeModel | Registered in FLayerMappingRegistry |
| `FLayerMappingRegistry` | Ordered list of mappers; first match wins | Queried by mapping orchestrator |
| `FWidgetTreeModel` | Value type: tree of FWidgetNodeModel (widget class, slot data, brush info, text props) | Consumed by Stage 3 |
| `FWidgetBlueprintGenerator` | Creates UWidgetBlueprint, populates UWidgetTree, creates texture assets | UMG/UMGEditor APIs |
| `FTextureImporter` | Creates UTexture2D assets from raw RGBA pixel data in memory | UE asset/package APIs |
| `UPsd2UmgSettings` | UDeveloperSettings: font map, texture output path, flatten settings | Read by all stages |

### Data Flow

1. User drags `MyUI.psd` into Content Browser (or right-clicks "Import as Widget Blueprint")
2. UE calls `UPsdImportFactory::FactoryCreateFile()` because `.psd` is in Formats list
3. Factory calls `FPsdParser::ParseFile(Filename)` which returns `FPsdDocument`
4. Factory calls mapping orchestrator which walks FPsdDocument tree, queries `FLayerMappingRegistry` for each layer, produces `FWidgetTreeModel`
5. Factory calls `FWidgetBlueprintGenerator::Generate(WidgetTreeModel, Package)` which:
   a. Creates `UWidgetBlueprint` via `UWidgetBlueprintFactory`
   b. Recursively builds UWidget tree via `UWidgetTree::ConstructWidget<>()`
   c. Creates `UTexture2D` assets for image layers via `FTextureImporter`
   d. Sets slot positions, sizes, anchors on each `UCanvasPanelSlot`
   e. Calls `FKismetEditorUtilities::CompileBlueprint()` and saves
6. Factory returns the `UWidgetBlueprint` to the asset import system

---

## Stage 1: UPsdImportFactory — The Import Pipeline Entry Point

### Why UFactory (not UImportSubsystem callback)

The existing codebase hooks `UImportSubsystem::OnAssetPostImport` and intercepts UTexture2D assets created from .psd files. This is fragile: it depends on the .psd first being imported as a texture (which we don't want), and it doesn't participate in UE's standard asset creation workflow.

The correct approach: subclass `UFactory` directly. This makes .psd a first-class import format that produces `UWidgetBlueprint` assets (not textures).

**Confidence:** HIGH — UFactory is the standard UE pattern for file-based asset importers. UWidgetBlueprintFactory already does exactly this for creating blank widget BPs.

### UPsdImportFactory Declaration

```cpp
#pragma once
#include "Factories/Factory.h"
#include "EditorReimportHandler.h"
#include "PsdImportFactory.generated.h"

UCLASS()
class UPsdImportFactory : public UFactory, public FReimportHandler
{
    GENERATED_BODY()
public:
    UPsdImportFactory();

    // UFactory interface
    virtual UObject* FactoryCreateFile(
        UClass* InClass,
        UObject* InParent,           // The package
        FName InName,                // Asset name
        EObjectFlags Flags,
        const FString& Filename,     // Full path to .psd file
        const TCHAR* Parms,
        FFeedbackContext* Warn,
        bool& bOutOperationCanceled
    ) override;

    virtual bool FactoryCanImport(const FString& Filename) override;

    // FReimportHandler interface
    virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
    virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
    virtual EReimportResult::Type Reimport(UObject* Obj) override;
};
```

### Constructor Setup

```cpp
UPsdImportFactory::UPsdImportFactory()
{
    // Tell UE this factory imports files (not creates new assets)
    bCreateNew = false;
    bEditorImport = true;
    bText = false;

    // The class we produce
    SupportedClass = UWidgetBlueprint::StaticClass();

    // Register .psd extension
    Formats.Add(TEXT("psd;Photoshop Document"));

    // High import priority so we win over UTextureFactory for .psd files
    ImportPriority = DefaultImportPriority + 1;
}
```

### Why FactoryCreateFile (not FactoryCreateBinary)

`FactoryCreateFile` receives the filename as a string and lets us handle file I/O ourselves. This is ideal because PhotoshopAPI opens the file directly via its own API (`PhotoshopFile::FromFile(path)`). We never need UE to pre-read the file into a buffer.

`FactoryCreateBinary` would load the entire file into a `const uint8*` buffer first, which is wasteful for large PSD files (100+ MB PSBs).

**Confidence:** HIGH — `FactoryCreateFile` is documented in UE API and explicitly described as "useful if you want to handle data loading yourself."

### Import Priority

Setting `ImportPriority` higher than default ensures `UPsdImportFactory` is selected over `UTextureFactory` when a .psd file is dragged in. Without this, UE's built-in texture factory would claim .psd files first and produce a UTexture2D instead of a UWidgetBlueprint.

---

## Stage 2: Widget Blueprint Creation API (UE 5.7)

### Creating UWidgetBlueprint

The existing codebase in `AutoPSDUILibrary.cpp` already demonstrates the correct pattern. Use `UWidgetBlueprintFactory` internally:

```cpp
UWidgetBlueprint* FWidgetBlueprintGenerator::CreateBlueprint(
    UObject* InParent, FName InName, EObjectFlags Flags)
{
    UWidgetBlueprintFactory* BPFactory = NewObject<UWidgetBlueprintFactory>();
    BPFactory->ParentClass = UUserWidget::StaticClass();

    // Use AssetTools to create — handles package setup, notification, etc.
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(
        "AssetTools").Get();
    UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(
        AssetTools.CreateAsset(InName.ToString(),
            FPackageName::GetLongPackagePath(InParent->GetPathName()),
            nullptr, BPFactory));
    return WBP;
}
```

**Alternative for FactoryCreateFile context:** When called from inside FactoryCreateFile, `InParent` is already the target package. You can also directly use:

```cpp
UWidgetBlueprint* WBP = CastChecked<UWidgetBlueprint>(
    BPFactory->FactoryCreateNew(
        UWidgetBlueprint::StaticClass(),
        InParent,
        InName,
        Flags,
        nullptr,     // Context
        GWarn));
```

This avoids the AssetTools layer (which is redundant when the import system already set up the package).

**Confidence:** HIGH — The existing `CreateWBP()` in AutoPSDUILibrary.cpp proves this works. The `UWidgetBlueprintFactory::FactoryCreateNew` path is used engine-wide.

### Populating UWidgetTree

```cpp
void FWidgetBlueprintGenerator::BuildWidgetTree(
    UWidgetBlueprint* WBP,
    const FWidgetTreeModel& Model)
{
    UWidgetTree* Tree = WBP->WidgetTree;

    // 1. Create root canvas panel
    UCanvasPanel* RootCanvas = Tree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), FName(TEXT("RootCanvas")));
    Tree->RootWidget = RootCanvas;

    // 2. Recursively add children
    for (const FWidgetNodeModel& Child : Model.RootChildren)
    {
        AddWidgetRecursive(Tree, RootCanvas, Child);
    }

    // 3. Mark modified (critical for editor to recognize changes)
    Tree->Modify();
    WBP->Modify();
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);
}
```

**Key rules for UWidgetTree manipulation:**
- Always use `Tree->ConstructWidget<T>()`, never `NewObject<T>()` — ConstructWidget registers the widget with the tree's internal bookkeeping.
- Set `Tree->RootWidget` directly (as the existing code does).
- After all modifications, call `Modify()` on both tree and blueprint, then `MarkBlueprintAsStructurallyModified`.

**Confidence:** HIGH — Verified via existing AutoPSDUILibrary.cpp code, UE documentation, and multiple community examples all using the same pattern.

### Adding Children and Setting CanvasPanelSlot

```cpp
void FWidgetBlueprintGenerator::AddWidgetRecursive(
    UWidgetTree* Tree,
    UPanelWidget* Parent,
    const FWidgetNodeModel& Node)
{
    // Construct the widget
    UWidget* Widget = Tree->ConstructWidget<UWidget>(
        Node.WidgetClass, FName(*Node.Name));

    // AddChild returns the slot, which for UCanvasPanel is UCanvasPanelSlot
    UPanelSlot* Slot = Parent->AddChild(Widget);

    // Configure canvas slot positioning
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
    {
        // Anchors: FAnchors(MinX, MinY, MaxX, MaxY)
        // Single point anchor (no stretching): Min == Max
        CanvasSlot->SetAnchors(FAnchors(
            Node.Anchors.Minimum.X, Node.Anchors.Minimum.Y,
            Node.Anchors.Maximum.X, Node.Anchors.Maximum.Y));

        // For single-point anchors, Offsets = (PosX, PosY, SizeX, SizeY)
        // For stretch anchors, Offsets = (Left, Top, Right, Bottom) margins
        CanvasSlot->SetOffsets(FMargin(
            Node.Position.X,    // Left / PosX
            Node.Position.Y,    // Top / PosY
            Node.Size.X,        // Right / SizeX
            Node.Size.Y));      // Bottom / SizeY

        // Alignment pivot (0,0 = top-left, default)
        CanvasSlot->SetAlignment(Node.Alignment);

        // Z-order from PSD layer ordering
        CanvasSlot->SetZOrder(Node.ZOrder);
    }

    // Apply widget-specific properties (handled by mapper output)
    ApplyWidgetProperties(Widget, Node);

    // Recurse for container widgets (UCanvasPanel children)
    if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
    {
        for (const FWidgetNodeModel& Child : Node.Children)
        {
            AddWidgetRecursive(Tree, Panel, Child);
        }
    }
}
```

### UCanvasPanelSlot API Summary

| Method | Signature | When to Use |
|--------|-----------|-------------|
| `SetAnchors` | `void SetAnchors(FAnchors InAnchors)` | Always — defines anchor point(s) |
| `SetOffsets` | `void SetOffsets(FMargin InOffset)` | Always — position+size or margins depending on anchor mode |
| `SetPosition` | `void SetPosition(FVector2D InPosition)` | Convenience — sets Left/Top of Offsets only |
| `SetSize` | `void SetSize(FVector2D InSize)` | Convenience — sets Right/Bottom of Offsets only |
| `SetAlignment` | `void SetAlignment(FVector2D InAlignment)` | When pivot differs from top-left |
| `SetZOrder` | `void SetZOrder(int32 InZOrder)` | Control draw ordering |
| `SetAutoSize` | `void SetAutoSize(bool InbAutoSize)` | For text/auto-sizing widgets |

**Anchor semantics (critical for PSD import):**
- Single-point anchor (Min == Max): Widget is positioned absolutely. Offsets = (X, Y, Width, Height).
- Stretch anchor (Min != Max): Widget stretches with parent. Offsets = margin distances from anchor edges.
- PSD layers naturally map to single-point anchors at (0,0) with absolute position/size from layer bounds. Anchor heuristics (Phase 5) can upgrade these to stretch anchors based on proximity to canvas edges.

**Confidence:** HIGH — SetAnchors, SetOffsets, SetPosition, SetSize are all documented in UE 5.x API docs and verified through multiple sources.

### Compiling and Saving

```cpp
void FWidgetBlueprintGenerator::CompileAndSave(UWidgetBlueprint* WBP)
{
    // Compile the blueprint (generates the UClass)
    FKismetEditorUtilities::CompileBlueprint(WBP);

    // Save via EditorAssetLibrary (handles package dirtying)
    UEditorAssetLibrary::SaveLoadedAsset(WBP);
}
```

This matches the existing `CompileAndSaveBP()` in AutoPSDUILibrary.cpp. No changes needed.

**Confidence:** HIGH — Existing code already uses this pattern successfully.

---

## Stage 3: Texture Import from Raw RGBA Pixels

### Pattern: Create Persistent UTexture2D from Memory

PhotoshopAPI provides raw pixel data per layer. We need to create saveable UTexture2D assets from this data without writing temp PNG files to disk (eliminating the temp file pipeline the Python version uses).

```cpp
UTexture2D* FTextureImporter::CreateTextureFromPixels(
    const FString& AssetPath,
    int32 Width, int32 Height,
    const TArray<uint8>& PixelData)  // BGRA8 format, Width*Height*4 bytes
{
    // 1. Create package for the texture asset
    UPackage* Package = CreatePackage(*AssetPath);
    Package->FullyLoad();

    FString AssetName = FPaths::GetBaseFilename(AssetPath);

    // 2. Create the UTexture2D in the package
    UTexture2D* Texture = NewObject<UTexture2D>(
        Package, *AssetName,
        RF_Public | RF_Standalone);

    // 3. Initialize Source data (this is what gets serialized to .uasset)
    //    Source.Init takes raw pixel bytes directly
    Texture->Source.Init(
        Width, Height,
        /*NumSlices=*/ 1,
        /*NumMips=*/ 1,
        ETextureSourceFormat::TSF_BGRA8,
        PixelData.GetData());

    // 4. Configure texture properties
    Texture->CompressionSettings = TextureCompressionSettings::TC_EditorIcon;
    Texture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
    Texture->NeverStream = true;
    Texture->SRGB = true;
    Texture->LODGroup = TextureGroup::TEXTUREGROUP_UI;

    // 5. Trigger platform data generation from Source
    Texture->UpdateResource();

    // 6. Register with asset registry and save
    Package->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(Texture);

    FString PackageFilename = FPackageName::LongPackageNameToFilename(
        AssetPath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, Texture, *PackageFilename, SaveArgs);

    return Texture;
}
```

**Critical detail: Source.Init vs PlatformData.** PlatformData is transient (derived/cached data). Source is what gets serialized. If you only populate PlatformData, the texture turns black on editor restart. Always initialize via `Source.Init()`.

**PhotoshopAPI pixel format note:** PhotoshopAPI can provide pixels in various formats (8/16/32 bit). For our use, extract 8-bit RGBA and swizzle to BGRA for `TSF_BGRA8`. PhotoshopAPI's `ImageData` returns channel-separated data that needs interleaving.

**Confidence:** HIGH — `Source.Init()` pattern is well-documented and confirmed in multiple forum threads as the correct approach for persistent textures. The `SavePackage` API changed slightly in UE 5.x (uses `FSavePackageArgs` struct instead of positional parameters).

---

## Reimport Support

### Pattern: UFactory + FReimportHandler Multiple Inheritance

This is the standard UE pattern for reimport. The same factory class handles both initial import and reimport.

```cpp
// In UPsdImportFactory (declared above with FReimportHandler)

bool UPsdImportFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
    UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(Obj);
    if (!WBP) return false;

    // Retrieve stored source path from asset import data
    if (UAssetImportData* ImportData = WBP->AssetImportData)
    {
        ImportData->ExtractFilenames(OutFilenames);
        return OutFilenames.Num() > 0;
    }
    return false;
}

void UPsdImportFactory::SetReimportPaths(
    UObject* Obj, const TArray<FString>& NewReimportPaths)
{
    UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(Obj);
    if (WBP && WBP->AssetImportData && NewReimportPaths.Num() > 0)
    {
        WBP->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
    }
}

EReimportResult::Type UPsdImportFactory::Reimport(UObject* Obj)
{
    UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(Obj);
    if (!WBP || !WBP->AssetImportData) return EReimportResult::Failed;

    const FString SourcePath = WBP->AssetImportData->GetFirstFilename();
    if (!FPaths::FileExists(SourcePath)) return EReimportResult::Failed;

    // Re-run the full pipeline, merging into existing blueprint
    // (See Reimport Strategy below)
    return ReimportIntoExisting(WBP, SourcePath);
}
```

### Storing the Source Path

`UWidgetBlueprint` inherits from `UBlueprint` which has an `AssetImportData` property (of type `UAssetImportData*`). During initial import in `FactoryCreateFile`, store the source .psd path:

```cpp
// In FactoryCreateFile, after creating the WBP:
if (!WBP->AssetImportData)
{
    WBP->AssetImportData = NewObject<UAssetImportData>(WBP);
}
WBP->AssetImportData->Update(Filename);
```

This makes the reimport path available for `CanReimport` and enables the "Reimport" button in the Asset Editor menu.

**Note on UBlueprint::AssetImportData:** This property exists but may not be populated by default. It is present in UE5 and is the standard mechanism for tracking source files. Verify in UE 5.7 headers that this member still exists on UBlueprint (it has been there since UE 4.x).

**Confidence:** MEDIUM — The FReimportHandler pattern is well-established. AssetImportData on UBlueprint is present in UE 4.x/5.x but less commonly used for blueprints (more for meshes/textures). If UWidgetBlueprint doesn't have it, we can store the path in a custom metadata tag via `UMetaData` or a UPROPERTY on a custom subclass. Needs verification in UE 5.7 headers.

### Reimport Strategy: Stable Layer Identity

On reimport, we want to update changed layers without destroying manual widget edits. Strategy:

1. **Layer identity key:** PSD layer name (must be unique within parent group). PhotoshopAPI provides layer names. These are stable across saves if the designer doesn't rename layers.

2. **Diff algorithm:**
   - Parse new PSD into FPsdDocument
   - Walk existing UWidgetTree and build name-to-widget map
   - For each layer in new PSD:
     - If matching widget exists: update properties (position, size, brush, text)
     - If no match: create new widget
   - For each existing widget not in new PSD: optionally remove or flag

3. **Preserve manual edits:** Properties that were manually changed in UMG Designer should not be overwritten. This requires tracking "imported" vs "manual" state, which is complex. Phase 6 simplification: reimport replaces the entire widget tree (destructive but predictable). Non-destructive merge is a Phase 7+ enhancement.

**Confidence:** MEDIUM — The stable-name approach is sound but "preserve manual edits" adds significant complexity. Recommend starting with destructive reimport.

---

## Pluggable Mapper Registry: IPsdLayerMapper

### UE Pattern: IModularFeatures

UE has a built-in extensibility mechanism: `IModularFeatures`. Modules register implementations at startup, and consumers query by feature name. However, this is heavyweight for our use case (designed for cross-module plugin systems).

**Recommendation: Use a simpler custom registry.** The mapper registry is internal to our plugin and doesn't need cross-plugin discovery. A TArray of registered mappers with priority ordering is sufficient.

### Interface Design

```cpp
// IPsdLayerMapper.h
class IPsdLayerMapper
{
public:
    virtual ~IPsdLayerMapper() = default;

    /** Return true if this mapper can handle the given layer. */
    virtual bool CanMap(const FPsdLayer& Layer) const = 0;

    /** Map the layer to a widget node model. Called only if CanMap returned true. */
    virtual FWidgetNodeModel Map(
        const FPsdLayer& Layer,
        const FPsdImportContext& Context) const = 0;

    /** Priority: higher values are checked first. Default mappers use 0-100. */
    virtual int32 GetPriority() const { return 0; }
};
```

### Registry Design

```cpp
// FLayerMappingRegistry.h
class FLayerMappingRegistry
{
public:
    static FLayerMappingRegistry& Get();

    void RegisterMapper(TSharedRef<IPsdLayerMapper> Mapper);
    void UnregisterMapper(TSharedRef<IPsdLayerMapper> Mapper);

    /** Find first mapper that can handle this layer (sorted by priority descending). */
    IPsdLayerMapper* FindMapper(const FPsdLayer& Layer) const;

private:
    TArray<TSharedRef<IPsdLayerMapper>> Mappers;
    bool bSorted = false;
};
```

### Default Mappers (registered at module startup)

| Mapper | Priority | Trigger | Produces |
|--------|----------|---------|----------|
| `FButtonLayerMapper` | 100 | Name starts with `Button_` | UButton with state brushes |
| `FProgressBarLayerMapper` | 100 | Name starts with `Progress_` | UProgressBar |
| `FListViewLayerMapper` | 100 | Name starts with `ListView_` or `TileView_` | UListView / UTileView |
| `FTextLayerMapper` | 50 | Layer is a text layer (PhotoshopAPI type) | UTextBlock |
| `FGroupLayerMapper` | 10 | Layer is a group | UCanvasPanel (recursive) |
| `FImageLayerMapper` | 0 | Fallback: any visible pixel layer | UImage |

Priority ordering ensures prefix-based mappers are checked before type-based fallbacks. Users can register custom mappers at priority > 100 to override defaults.

**Why not IModularFeatures:** IModularFeatures requires the mapper implementations to be in separate modules and uses string-based feature names. Our mappers are all in the same module and benefit from compile-time type safety. If cross-plugin extensibility becomes needed later (Phase 7+), the registry can delegate to IModularFeatures as an additional source.

**Confidence:** HIGH — The strategy/registry pattern is well-understood. The priority-based first-match approach mirrors how UE's own factory system works (ImportPriority on UFactory).

---

## Build Module Dependencies

### Build.cs Module Dependencies (UE 5.7)

```csharp
PrivateDependencyModuleNames.AddRange(new string[]
{
    "CoreUObject",
    "Engine",
    "Slate",
    "SlateCore",
    "DeveloperSettings",
    "UnrealEd",        // UFactory, FKismetEditorUtilities, FReimportHandler
    "UMG",             // UWidget, UCanvasPanel, UCanvasPanelSlot, UImage, UTextBlock
    "UMGEditor",       // UWidgetBlueprint, UWidgetBlueprintFactory, UWidgetTree
    "AssetTools",      // IAssetTools (if using AssetTools.CreateAsset)
    "AssetRegistry",   // FAssetRegistryModule::AssetCreated
    "Kismet",          // FBlueprintEditorUtils
    "InputCore",       // FKey (if needed for input binding)
});
```

**Removed from existing:** `PythonScriptPlugin`, `EditorScriptingUtilities` (no longer needed).

**Added:** `AssetTools` (explicit, was loaded dynamically before), `Kismet` (for FBlueprintEditorUtils).

---

## Suggested Build Order

### Phase 1: PSD Parsing (no UE widget dependency)

Build `FPsdParser` and `FPsdDocument` first. This is pure C++ wrapping PhotoshopAPI. Testable with simple console tests or catch2/doctest outside UE. The `UPsdImportFactory` skeleton should also be created here (constructor + `FactoryCreateFile` that logs and returns nullptr) to verify .psd files are intercepted.

**Deliverable:** Drag .psd into Content Browser, see parsed layer tree in Output Log.

### Phase 2: Widget Blueprint Generation (core pipeline)

Build `FWidgetBlueprintGenerator` and `FTextureImporter`. Implement the recursive widget tree builder. Start with just UCanvasPanel (groups) and UImage (pixel layers). Integrate with `UPsdImportFactory::FactoryCreateFile` to produce actual .uasset output.

**Deliverable:** Drag .psd, get a UWidgetBlueprint with correct hierarchy, positioned images.

### Phase 3: Layer Mapping (pluggable strategies)

Build `IPsdLayerMapper`, `FLayerMappingRegistry`, and all default mappers. Refactor the hardcoded Stage 2 logic into mapper implementations. Add text, button, progress bar, list view mappers.

**Deliverable:** Full widget type coverage matching the Python baseline.

### Rationale for This Order

- Phase 1 first because everything depends on having parsed PSD data.
- Phase 2 before Phase 3 because you need a working end-to-end pipeline (even if hardcoded) to validate the UWidgetBlueprint creation API works. Getting the UE editor integration right is the highest-risk work.
- Phase 3 last because the mapper pattern is a refactoring of working code into a pluggable architecture. It's safer to refactor working code than to build an abstraction layer before you have concrete implementations.

---

## Anti-Patterns to Avoid

### Anti-Pattern 1: Using NewObject Instead of ConstructWidget
**What:** Creating UWidget instances with `NewObject<UWidget>()` instead of `WidgetTree->ConstructWidget<UWidget>()`
**Why bad:** Widgets created with NewObject are not registered in the WidgetTree and will be garbage collected or cause crashes when the blueprint is compiled.
**Instead:** Always use `WidgetTree->ConstructWidget<T>(T::StaticClass(), WidgetName)`.

### Anti-Pattern 2: Populating PlatformData Instead of Source
**What:** Setting up FTexturePlatformData and mip chain on UTexture2D, skipping `Source.Init()`
**Why bad:** PlatformData is transient derived data. The texture appears correct initially but turns black after editor restart because nothing is serialized.
**Instead:** Always call `Texture->Source.Init()` with pixel data. PlatformData will be regenerated from Source automatically.

### Anti-Pattern 3: Writing Temp PNG Files for Texture Import
**What:** The Python baseline exports layer images as PNG to disk, then imports via UTextureFactory.
**Why bad:** Slow (disk I/O), creates temp file cleanup problems, unnecessary encoding/decoding round-trip.
**Instead:** Create UTexture2D directly from raw RGBA pixels in memory (see FTextureImporter pattern above).

### Anti-Pattern 4: Single God-Mapper Instead of Registry
**What:** One giant switch/if-else chain that handles all layer-to-widget mappings.
**Why bad:** Not extensible, grows unwieldy, impossible for users to add custom mappings.
**Instead:** IPsdLayerMapper + FLayerMappingRegistry with first-match-wins priority ordering.

### Anti-Pattern 5: Modifying Widget Properties After Compile
**What:** Calling CompileBlueprint, then modifying widget properties, then not recompiling.
**Why bad:** The compiled UClass and the design-time WidgetTree get out of sync.
**Instead:** Build entire widget tree first, then compile once at the end.

---

## Scalability Considerations

| Concern | Small PSD (10 layers) | Medium PSD (100 layers) | Large PSD (500+ layers) |
|---------|----------------------|------------------------|------------------------|
| Parse time | <100ms | <500ms | 1-3s |
| Texture creation | Negligible | May need batching | Async texture creation or progress dialog |
| Widget tree depth | No issue | No issue | UE WidgetTree may slow in UMG Designer — consider flattening deep nesting |
| Memory (pixel data) | <10MB | <100MB | >500MB — must stream layer pixels, not hold all in memory simultaneously |

**Recommendation for large PSDs:** Process layers one at a time in Stage 3, creating each texture and releasing pixel data before moving to the next layer. Do not hold all layer pixel data in memory simultaneously.

---

## Sources

- [UFactory::FactoryCreateFile — UE API Documentation](https://docs.unrealengine.com/4.26/en-US/API/Editor/UnrealEd/Factories/UFactory/FactoryCreateFile/)
- [UE5 Custom Asset Type Tutorial — Algosyntax](https://store.algosyntax.com/tutorials/unreal-engine/tutorial-ue5-how-to-create-your-custom-asset/)
- [UE5 Reimport Support Tutorial — Algosyntax](https://store.algosyntax.com/tutorials/unreal-engine/tutorial-ue5-add-reimport-support-to-custom-asset/)
- [UWidgetTree::ConstructWidget — UE 5.7 API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/UMG/Blueprint/UWidgetTree/ConstructWidget)
- [UWidgetBlueprint — UE 5.7 API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Editor/UMGEditor/UWidgetBlueprint)
- [UCanvasPanelSlot::SetOffsets — UE 5.6 API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/UMG/Components/UCanvasPanelSlot/SetOffsets)
- [UCanvasPanelSlot::SetAnchors — UE 5.0 API](https://docs.unrealengine.com/5.0/en-US/API/Runtime/UMG/Components/UCanvasPanelSlot/SetAnchors/)
- [FReimportManager::Reimport — UE 5.6 API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Editor/UnrealEd/FReimportManager/Reimport)
- [Add UWidgets to a UserWidget in Editor — Unreal Garden](https://unreal-garden.com/tutorials/build-widgets-in-editor/)
- [Programmatic Widget Construction Gist — SalahAdDin](https://gist.github.com/SalahAdDin/b3814f109d340f1e0f5dcac6e623b8ce)
- [IModularFeatures — UE 5.6 API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Core/Features/IModularFeatures/GetModularFeatureImplementation)
- [UE5 Procedural Texture Persistence — Epic Forums](https://forums.unrealengine.com/t/how-to-save-a-procedurally-generated-texture/409542)
- [UWidgetTree — UE 5.7 API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/UMG/UWidgetTree)

---

*Architecture research: 2026-04-07*

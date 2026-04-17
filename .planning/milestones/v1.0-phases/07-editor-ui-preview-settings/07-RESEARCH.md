# Phase 7: Editor UI, Preview & Settings — Research

**Researched:** 2026-04-10
**Domain:** UE5 Slate editor UI, FReimportHandler, UToolMenus, UWidgetAnimation, CommonUI
**Confidence:** HIGH (all critical APIs verified from local UE 5.7 engine headers)

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Import Preview Dialog (EDITOR-02, EDITOR-03)**
- D-01: Preview dialog appears on every PSD import. It shows the parsed layer tree before creating any assets.
- D-02: Each layer row shows: checkbox (toggle on/off), layer name, and a colored badge showing resolved widget type (UButton, UImage, UTextBlock, etc.). No thumbnails, no dimensions.
- D-03: Widget type badges are read-only — no dropdown to override type. Widget type is determined by naming convention only.
- D-04: Dialog includes an editable output path field, pre-filled from `UPSD2UMGSettings::WidgetBlueprintAssetDir`, with a browse button for per-import override.

**Reimport Strategy (EDITOR-04)**
- D-05: Layer identity matched by PSD layer name (not path, not internal ID). Renaming a layer = new widget.
- D-06: Reimport updates PSD-sourced properties (position, size, image data, text content, brushes) and preserves manual edits (Blueprint logic, bindings, animations, manually-added widgets).
- D-07: Widgets from deleted PSD layers are kept as orphans in the Blueprint.
- D-08: Reimport shows the preview dialog with change annotations: [new], [changed], [unchanged] per layer.

**CommonUI & Animations (CUI-01 through CUI-04)**
- D-09: CommonUI mode is a global toggle (`bUseCommonUI`, default: false). When on, all Button_ layers produce `UCommonButtonBase`.
- D-10: Input action binding `Button_Confirm[IA_Confirm]` resolves the UInputAction asset at import time by searching `InputActionSearchPath`. If not found, log warning and create button without binding.
- D-11: Animation generation: `_show` = 0→1 opacity over 0.3s, `_hide` = 1→0 opacity, `_hover` = widget scale 1.0→1.05 over 0.15s.
- D-12: ScrollBox content height: let UMG handle at runtime.

**Settings & Context Menu (EDITOR-01, EDITOR-05)**
- D-13: New settings: `bShowPreviewDialog` (bool, default: true), `bUseCommonUI` (bool, default: false), `InputActionSearchPath` (FDirectoryPath, default: `/Game/Input/`), `SourceDPI` (int32, default: 72).
- D-14: Right-click context menu on both PSD texture assets and existing WBPs (with PSD source metadata).

### Claude's Discretion
- Dialog layout and Slate widget arrangement
- Change detection algorithm for reimport (hash-based, property comparison, etc.)
- How PSD source path is stored as metadata on Widget Blueprints for reimport
- UWidgetAnimation creation API details (FMovieSceneSequence, etc.)
- CommonUI module dependency management (conditional linking)

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope.
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| EDITOR-01 | Plugin settings in Project Settings → Plugins → PSD2UMG | UDeveloperSettings category grouping syntax verified in existing UPSD2UMGSettings pattern |
| EDITOR-02 | Import preview dialog shows layer tree with checkboxes and widget type badges | SWindow + STreeView + FSlateApplication::AddModalWindow pattern verified |
| EDITOR-03 | User can toggle individual layers on/off before import | SCheckBox in STableRow with cascade cascade toggling on group rows |
| EDITOR-04 | Reimport support with name-based identity, preserves manual edits | FReimportHandler interface verified from EditorReimportHandler.h; UAssetImportData::Update() for source path storage |
| EDITOR-05 | Right-click context menu in Content Browser | UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu") + UContentBrowserAssetContextMenuContext verified |
| CUI-01 | Optional CommonUI mode: Button_ → UCommonButtonBase | UCommonButtonBase::SetTriggeringEnhancedInputAction verified in CommonButtonBase.h |
| CUI-02 | Input action binding via layer name syntax Button_Confirm[IA_Confirm] | Asset registry search for UInputAction; SetTriggeringEnhancedInputAction(UInputAction*) signature confirmed |
| CUI-03 | Animation generation from _show/_hide/_hover variants | UWidgetAnimation + UMovieScene + UMovieSceneFloatTrack + FMovieSceneFloatChannel::AddLinearKey verified |
| CUI-04 | ScrollBox content height auto-calculated from children | Deferred to runtime (D-12) — no import-time action needed |
</phase_requirements>

---

## Summary

Phase 7 introduces editor UX features on top of the existing import pipeline. The core technical areas are: (1) a Slate modal dialog for import preview, (2) FReimportHandler integration for reimport flows, (3) UToolMenus-based Content Browser extender, (4) UWidgetAnimation programmatic creation via the MovieScene API, and (5) optional CommonUI module dependency with UCommonButtonBase.

All critical APIs have been verified directly from UE 5.7 engine headers at `C:/Program Files/Epic Games/UE_5.7/`. No guessing required — exact class names, method signatures, and property names are confirmed. The most technically complex area is programmatic UWidgetAnimation creation, which requires building a UMovieScene with possessables, tracks, sections, and float-channel keyframes — this is underdocumented but the headers reveal all necessary methods.

The recommended PSD-source-path storage strategy is a sub-object `UAssetImportData` created on the `UWidgetBlueprint`'s outer package, using `UAssetImportData::Update()` to set the absolute path. For reimport, `CanReimport` checks for this sub-object's presence and extracts the path via `GetFirstFilename()`. Change detection for reimport can be hash-based (MD5 of each layer's pixel data or a struct hash of text/position properties) — simple and reliable.

**Primary recommendation:** Implement the preview dialog as a standalone `SPsdImportPreviewDialog` Slate widget class (separate from the factory), the reimport handler as a non-UObject class that inherits `FReimportHandler`, and the content browser extender in `StartupModule()` using `UToolMenus::RegisterStartupCallback`. Keep CommonUI dependency conditional using `FModuleManager::IsModuleLoaded(TEXT("CommonUI"))` at call sites.

---

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Slate (built-in) | UE 5.7 | Modal dialog, tree view | The only editor UI toolkit in UE5 |
| ToolMenus (built-in) | UE 5.7 | Content Browser menu extension | Replaced legacy FContentBrowserMenuExtender in UE5 |
| UnrealEd (built-in) | UE 5.7 | FReimportHandler, FReimportManager | Standard asset reimport interface |
| MovieScene (built-in) | UE 5.7 | UWidgetAnimation backbone | Required for widget animation data model |
| MovieSceneTracks (built-in) | UE 5.7 | UMovieSceneFloatTrack + sections | Float keyframe track for opacity/scale |
| UMG (built-in) | UE 5.7 | UWidgetAnimation, FWidgetAnimationBinding | Animation bound to widget tree |
| CommonUI (conditional) | UE 5.7 | UCommonButtonBase | Optional per D-09; guarded at runtime |
| ContentBrowser (built-in) | UE 5.7 | UContentBrowserAssetContextMenuContext | Context data for right-click menu |

### Build.cs Additions Required
```csharp
// Always needed for phase 7:
PrivateDependencyModuleNames.AddRange(new string[]
{
    "ToolMenus",         // UToolMenus::Get()->ExtendMenu
    "ContentBrowser",    // UContentBrowserAssetContextMenuContext
    "MovieScene",        // UMovieScene, FGuid possessable
    "MovieSceneTracks",  // UMovieSceneFloatTrack, MovieSceneFloatSection
    "InputCore",         // FKey (used in CommonUI path)
    "EnhancedInput",     // UInputAction (for CUI-02)
});

// Conditional — only if CommonUI plugin is installed:
// if (Target.bBuildDeveloperTools || true) // always true for editor plugins
// {
//     PrivateDependencyModuleNames.Add("CommonUI");
// }
// Safer: guard with FModuleManager::IsModuleLoaded at runtime, link unconditionally
// if the plugin is available in the project. See Architecture Patterns.
```

---

## Architecture Patterns

### Recommended Project Structure
```
Source/PSD2UMG/
├── Private/
│   ├── UI/
│   │   ├── SPsdImportPreviewDialog.h     // Slate dialog widget
│   │   ├── SPsdImportPreviewDialog.cpp
│   │   └── PsdLayerTreeItem.h            // TSharedPtr tree node struct
│   ├── Reimport/
│   │   ├── FPsdReimportHandler.h         // FReimportHandler subclass
│   │   └── FPsdReimportHandler.cpp
│   ├── ContentBrowser/
│   │   └── FPsdContentBrowserExtensions.cpp  // UToolMenus registration
│   └── Animation/
│       └── FPsdWidgetAnimationBuilder.cpp    // UWidgetAnimation factory helper
```

### Pattern 1: Modal Preview Dialog (EDITOR-02, EDITOR-03)

**What:** Create an `SWindow` via `FSlateApplication::AddModalWindow`. This blocks the calling thread until the window is destroyed. The factory calls this before `FWidgetBlueprintGenerator::Generate()`.

**When to use:** Any blocking editor dialog that must complete before import proceeds.

```cpp
// Source: verified from UE 5.7 FSlateApplication header
TSharedRef<SWindow> DialogWindow = SNew(SWindow)
    .Title(LOCTEXT("ImportPreviewTitle", "PSD2UMG — Import Preview"))
    .SizingRule(ESizingRule::UserSized)
    .AutoCenter(EAutoCenter::PrimaryWorkArea)
    .ClientSize(FVector2D(600, 640))
    .MinWidth(480)
    .MinHeight(520)
    .SupportsMinimize(false)
    .SupportsMaximize(false);

TSharedRef<SPsdImportPreviewDialog> DialogContent =
    SNew(SPsdImportPreviewDialog)
    .Document(Doc)
    .OutputPath(Settings->WidgetBlueprintAssetDir.Path)
    .OnConfirmed_Lambda([&](const FPsdImportParams& Params) {
        OutParams = Params;
        DialogWindow->RequestDestroyWindow();
    })
    .OnCancelled_Lambda([&]() {
        bCancelled = true;
        DialogWindow->RequestDestroyWindow();
    });

DialogWindow->SetContent(DialogContent);

// Blocks until window is destroyed:
GEditor->EditorAddModalWindow(DialogWindow);
// Alternative: FSlateApplication::Get().AddModalWindow(DialogWindow, ParentWindow);
```

**Key insight on GEditor->EditorAddModalWindow vs FSlateApplication::AddModalWindow:** Both work. `GEditor->EditorAddModalWindow` is slightly safer in editor context as it pumps the editor tick properly. `FSlateApplication::Get().AddModalWindow(Window, TSharedPtr<SWidget>())` also works and is the lower-level primitive. Use `GEditor->EditorAddModalWindow` in the factory context.

### Pattern 2: STreeView for Layer Tree (EDITOR-02)

**What:** `STreeView<TSharedPtr<FPsdLayerTreeItem>>` driven by a flat TArray of items with parent pointers.

```cpp
// Tree item struct (no USTRUCT needed — Slate-only)
struct FPsdLayerTreeItem
{
    FString LayerName;
    FString WidgetTypeBadge;      // "Button", "Image", "TextBlock", etc.
    FLinearColor BadgeColor;
    bool bChecked = true;
    EPsdChangeAnnotation ChangeAnnotation = EPsdChangeAnnotation::None; // new/changed/unchanged
    TSharedPtr<FPsdLayerTreeItem> Parent;
    TArray<TSharedPtr<FPsdLayerTreeItem>> Children;
};

// Row generation delegate
SNew(STableRow<TSharedPtr<FPsdLayerTreeItem>>, OwnerTable)
[
    SNew(SHorizontalBox)
    + SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
    [ SNew(SCheckBox)
        .IsChecked_Lambda([Item](){ return Item->bChecked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
        .OnCheckStateChanged_Lambda([Item, WeakThis](ECheckBoxState NewState) {
            // cascade to children
        })
    ]
    // ... other columns per UI-SPEC
]
```

### Pattern 3: FReimportHandler Implementation (EDITOR-04)

**What:** Subclass `FReimportHandler` (not a UObject). Constructor auto-registers with `FReimportManager`; destructor auto-unregisters. The handler is owned as a member of the module class.

**Verified interface** (from `EditorReimportHandler.h`):

```cpp
// Header: Source/Editor/UnrealEd/Public/EditorReimportHandler.h
class FPsdReimportHandler : public FReimportHandler
{
public:
    // Returns true if Obj is a UWidgetBlueprint with PSD source metadata attached
    virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;

    // Updates the source path stored on the asset
    virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;

    // Runs the reimport: parse PSD, show preview dialog with [new]/[changed]/[unchanged],
    // call FWidgetBlueprintGenerator::Update() on user confirm
    virtual EReimportResult::Type Reimport(UObject* Obj) override;
};
```

**Source path storage — recommended approach:** Store a `UAssetImportData` sub-object on the WBP package. `UWidgetBlueprint` does not have a built-in `AssetImportData` property (unlike `UStaticMesh`), so we create and store it ourselves using the asset metadata tag system OR as a direct UObject sub-object.

**Better alternative: Asset Registry Tags.** Use `UObject::GetAssetRegistryTags` + store a custom tag `PSD2UMG.SourcePath` as an `FAssetData` metadata string. This avoids adding a sub-object property to UWidgetBlueprint. The tag is written when the WBP is saved and readable without loading the asset.

Simplest approach: tag on the UWidgetBlueprint package:
```cpp
// Write at generation time (in FWidgetBlueprintGenerator::Generate)
UMetaData* MetaData = WBP->GetOutermost()->GetMetaData();
MetaData->SetValue(WBP, TEXT("PSD2UMG.SourcePsdPath"), *AbsoluteSourcePath);
// Read in CanReimport:
if (UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(Obj))
{
    UMetaData* Meta = WBP->GetOutermost()->GetMetaData();
    FString Path = Meta->GetValue(WBP, TEXT("PSD2UMG.SourcePsdPath"));
    if (!Path.IsEmpty()) { OutFilenames.Add(Path); return true; }
}
```

**Caution:** Package metadata is editor-only and persists across saves. This is the lightest-weight approach that matches FReimportHandler expectations.

### Pattern 4: Content Browser Right-Click Menu (EDITOR-05)

**What:** Register via `UToolMenus::RegisterStartupCallback` to extend `"ContentBrowser.AssetContextMenu"`. Use `UContentBrowserAssetContextMenuContext` (verified from `ContentBrowserMenuContexts.h`) to access `SelectedAssets`.

**Verified from engine source** (`AssetContextMenu.cpp` line 195): the menu name is `"ContentBrowser.AssetContextMenu"`. Asset-class-specific sub-menus are named `"ContentBrowser.AssetContextMenu.{ClassName}"` but we do NOT need a specific class menu — we want to appear for any asset and show/hide based on selection type.

```cpp
// In StartupModule():
UToolMenus::RegisterStartupCallback(
    FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FPsdContentBrowserExtensions::RegisterMenus));

// In RegisterMenus():
UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu");
FToolMenuSection& Section = Menu->AddSection("PSD2UMGSection", LOCTEXT("PSD2UMGSectionLabel", "PSD2UMG"));
Section.AddDynamicEntry("PSD2UMGActions", FNewToolMenuSectionDelegate::CreateLambda(
    [](FToolMenuSection& InSection)
    {
        const UContentBrowserAssetContextMenuContext* Context =
            InSection.FindContext<UContentBrowserAssetContextMenuContext>();
        if (!Context || Context->SelectedAssets.IsEmpty()) return;

        // Check if any selected asset is a PSD texture
        bool bHasPsd = Context->SelectedAssets.ContainsByPredicate([](const FAssetData& A) {
            return A.AssetClassPath == UTexture2D::StaticClass()->GetClassPathName()
                && A.GetTagValueRef<FString>(TEXT("PSD2UMG.SourcePsdPath")).Len() > 0;
            // Or check via asset name suffix / custom tag
        });

        // Check if any selected asset is a WBP with PSD metadata
        bool bHasWbpWithPsd = Context->SelectedAssets.ContainsByPredicate([](const FAssetData& A) {
            return A.AssetClassPath == UWidgetBlueprint::StaticClass()->GetClassPathName()
                && A.GetTagValueRef<FString>(TEXT("PSD2UMG.SourcePsdPath")).Len() > 0;
        });

        if (bHasPsd)
        {
            InSection.AddMenuEntry("PSD2UMG_ImportAsWBP",
                LOCTEXT("ImportAsWBP", "Import as Widget Blueprint"),
                LOCTEXT("ImportAsWBPTip", "Generate a UMG Widget Blueprint from this PSD"),
                FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Import"),
                FUIAction(FExecuteAction::CreateLambda([Context]() { /* invoke import */ })));
        }
        if (bHasWbpWithPsd)
        {
            InSection.AddMenuEntry("PSD2UMG_ReimportFromPsd",
                LOCTEXT("ReimportFromPSD", "Reimport from PSD"),
                LOCTEXT("ReimportFromPSDTip", "Update this Widget Blueprint from its source PSD"),
                FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Refresh"),
                FUIAction(FExecuteAction::CreateLambda([Context]() { /* invoke reimport */ })));
        }
    }));
```

**Note on deprecated GetSelectedObjects:** As of UE 5.5, `GetSelectedObjects()` is deprecated. Use `Context->SelectedAssets` (FAssetData array) and load lazily with `Asset.GetAsset()` only inside the action handler. Do NOT load assets during context menu construction — this is the UE5 rule. The snippet above respects this by only reading tag values, not loading assets.

### Pattern 5: UWidgetAnimation Creation (CUI-03)

**What:** Build UWidgetAnimation at import time and add to `UWidgetBlueprint::Animations` TArray (verified as `TArray<TObjectPtr<UWidgetAnimation>>` in `WidgetBlueprint.h` line 231).

**Verified API chain** (all from UE 5.7 headers):
- `UWidgetBlueprint::Animations` — `TArray<TObjectPtr<UWidgetAnimation>>` (WITH_EDITORONLY_DATA)
- `UWidgetAnimation::MovieScene` — `TObjectPtr<UMovieScene>`
- `UWidgetAnimation::AnimationBindings` — `TArray<FWidgetAnimationBinding>`
- `FWidgetAnimationBinding` fields: `WidgetName` (FName), `SlotWidgetName` (FName), `AnimationGuid` (FGuid), `bIsRootWidget` (bool)
- `UMovieScene::AddPossessable(const FString& Name, UClass* Class)` → returns `FGuid`
- `UMovieScene::AddTrack<UMovieSceneFloatTrack>(const FGuid& ObjectGuid)` → returns `UMovieSceneFloatTrack*`
- `UMovieScenePropertyTrack::SetPropertyNameAndPath(FName, const FString&)`
- `UMovieSceneFloatTrack::CreateNewSection()` → returns `UMovieSceneSection*` (cast to `UMovieSceneFloatSection`)
- `UMovieSceneFloatSection::GetChannel()` → returns `FMovieSceneFloatChannel&`
- `FMovieSceneFloatChannel::AddLinearKey(FFrameNumber, float)` → adds a linear-interpolation keyframe

```cpp
// Source: verified from UE 5.7 engine headers
// TickResolution = 24000 ticks/sec is standard UMG animation resolution

static UWidgetAnimation* CreateOpacityFadeAnimation(
    UWidgetBlueprint* WBP,
    const FString& AnimName,
    const FName& TargetWidgetName,
    float FromOpacity, float ToOpacity, float DurationSec)
{
    UWidgetAnimation* Anim = NewObject<UWidgetAnimation>(
        WBP,
        FName(*AnimName),
        RF_Transactional | RF_Public);

    UMovieScene* MovieScene = NewObject<UMovieScene>(
        Anim, FName(*(AnimName + TEXT("_Scene"))),
        RF_Transactional);
    Anim->MovieScene = MovieScene;

    // Set tick resolution and display rate
    const FFrameRate TickRes(24000, 1);
    const FFrameRate DisplayRate(30, 1);
    MovieScene->SetTickResolutionDirectly(TickRes);
    MovieScene->SetDisplayRate(DisplayRate);

    // Duration in ticks
    const int32 EndTick = FMath::RoundToInt(DurationSec * TickRes.Numerator);
    MovieScene->SetPlaybackRange(FFrameNumber(0), EndTick);

    // Add a possessable for the target widget
    const FGuid WidgetGuid = MovieScene->AddPossessable(TargetWidgetName.ToString(), UWidget::StaticClass());

    // Add a float track for RenderOpacity
    UMovieSceneFloatTrack* OpacityTrack = MovieScene->AddTrack<UMovieSceneFloatTrack>(WidgetGuid);
    OpacityTrack->SetPropertyNameAndPath(TEXT("RenderOpacity"), TEXT("RenderOpacity"));

    // Create a section and add keyframes
    UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(OpacityTrack->CreateNewSection());
    Section->SetRange(TRange<FFrameNumber>(FFrameNumber(0), FFrameNumber(EndTick)));
    OpacityTrack->AddSection(*Section);

    FMovieSceneFloatChannel& Channel = Section->GetChannel();
    Channel.AddLinearKey(FFrameNumber(0), FromOpacity);
    Channel.AddLinearKey(FFrameNumber(EndTick), ToOpacity);

    // Register the animation binding so UMG runtime can resolve widget by name
    FWidgetAnimationBinding Binding;
    Binding.WidgetName = TargetWidgetName;
    Binding.SlotWidgetName = NAME_None;
    Binding.AnimationGuid = WidgetGuid;
    Binding.bIsRootWidget = false;
    Anim->AnimationBindings.Add(Binding);

    // Register the possessable binding in the sequence
    Anim->BindPossessableObject(WidgetGuid, *WBP->WidgetTree, WBP->WidgetTree);

    WBP->Animations.Add(Anim);
    return Anim;
}
```

**For scale (_hover) animation:** Use `UMovieSceneFloatTrack` with property path `"RenderTransform.Scale.X"` and `"RenderTransform.Scale.Y"`, or use `UMovieScene2DTransformTrack` if available. Simpler alternative: use two separate `UMovieSceneFloatTrack` instances for X and Y scale independently with property path `"RenderTransform.Translation.X"` etc. The render transform path in UMG is `"RenderTransform"` of type `FWidgetTransform`. Target property: `RenderTransform.Scale.X` and `RenderTransform.Scale.Y`.

**IMPORTANT WARNING:** `BindPossessableObject` during import time (not playback) is unusual. The safer pattern is to NOT call `BindPossessableObject` and rely solely on `FWidgetAnimationBinding` name resolution at runtime. The UMG runtime's `LocateBoundObjects` uses the `FWidgetAnimationBinding::WidgetName` to find the widget in the tree at playback time. This is the correct approach for import-time animation creation. Do not call `BindPossessableObject` — just populate `AnimationBindings`.

### Pattern 6: CommonUI Conditional Dependency (CUI-01, CUI-02)

**Verified API** from `CommonButtonBase.h`:
- `SetTriggeringEnhancedInputAction(UInputAction* InInputAction)` — sets the Enhanced Input action that triggers this button
- `TriggeringEnhancedInputAction` — the stored `TObjectPtr<UInputAction>` property (line 962)
- Also available: `SetTriggeredInputAction(const FDataTableRowHandle&)` for legacy data table approach

**Conditional module linking strategy:**

```csharp
// In Build.cs — conditional on plugin availability
if (Target.Type == TargetType.Editor)
{
    // CommonUI is available if the plugin is enabled in the project
    // Safest: always add if present, guard with IsModuleLoaded at runtime
    PrivateDependencyModuleNames.Add("CommonUI");
}
```

**Runtime guard:**
```cpp
// In FCommonUIButtonLayerMapper::Map() (new mapper class):
if (!FModuleManager::Get().IsModuleLoaded(TEXT("CommonUI")))
{
    UE_LOG(LogPSD2UMG, Error,
        TEXT("PSD2UMG: bUseCommonUI is enabled but the CommonUI module is not loaded. Falling back to UButton."));
    return nullptr; // FLayerMappingRegistry falls through to FButtonLayerMapper
}
// Safe to use UCommonButtonBase here
UCommonButtonBase* Btn = Tree->ConstructWidget<UCommonButtonBase>(UCommonButtonBase::StaticClass(), FName(*CleanName));
```

**Input action asset lookup for CUI-02:**
```cpp
// Parse "Button_Confirm[IA_Confirm]" → extract "IA_Confirm"
// Then search asset registry:
FAssetRegistryModule& ARModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
TArray<FAssetData> Results;
ARModule.Get().GetAssetsByClass(UInputAction::StaticClass()->GetClassPathName(), Results);
for (const FAssetData& Asset : Results)
{
    if (Asset.AssetName == FName(*InputActionName) &&
        Asset.PackagePath.ToString().StartsWith(Settings->InputActionSearchPath.Path))
    {
        UInputAction* IA = Cast<UInputAction>(Asset.GetAsset());
        Btn->SetTriggeringEnhancedInputAction(IA);
        break;
    }
}
```

### Pattern 7: UDeveloperSettings Category Grouping (EDITOR-01)

**Verified:** The `Category = "PSD2UMG|Group"` pipe-separated syntax works with `UDeveloperSettings`. Each sub-group appears as a collapsible section in the Project Settings detail panel.

```cpp
UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG|General",
    meta=(ToolTip="Show layer preview dialog before every import. Disable for batch import workflows."))
bool bShowPreviewDialog = true;

UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG|CommonUI",
    meta=(ToolTip="When enabled, Button_ layers generate UCommonButtonBase instead of UButton. Requires CommonUI plugin."))
bool bUseCommonUI = false;

UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG|CommonUI",
    meta=(ToolTip="Search path for UInputAction assets referenced in layer names (e.g. Button_Confirm[IA_Confirm])."))
FDirectoryPath InputActionSearchPath;

UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG|Typography",
    meta=(ToolTip="Photoshop source document DPI. Used for pt to px conversion. Default 72 = 1pt equals 1px in UMG.", ClampMin=1))
int32 SourceDPI = 72;
```

### Anti-Patterns to Avoid

- **Using `GetSelectedObjects()` in context menu construction:** Deprecated in UE 5.5. Loading assets during menu construction causes editor hitches. Use `FAssetData` tags and load only inside action handlers.
- **Calling `BindPossessableObject` at import time:** This is a playback-time binding operation. For animation creation, only populate `AnimationBindings` struct — the runtime resolves bindings by widget name.
- **Calling `FReimportManager::Reimport` from within a `FReimportHandler::Reimport`:** Creates infinite recursion. Implement `Reimport` to directly call `FWidgetBlueprintGenerator::Update()`, not the manager.
- **Adding CommonUI to `PublicDependencyModuleNames`:** CommonUI types must not leak through public headers — keep in `PrivateDependencyModuleNames`.
- **Creating `SWindow` with `SizingRule::FixedSize`:** The preview dialog needs `UserSized` to allow resizing on small screens.
- **Using `FSlateApplication::AddWindow` instead of `AddModalWindow`:** Non-modal window returns immediately; the factory would continue before the user confirms. Must use modal.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Source path persistence on assets | Custom UObject property on WBP | UPackage metadata (`GetMetaData()->SetValue`) | Package metadata is the UE pattern for editor-only import tracking; UWidgetBlueprint has no AssetImportData by default |
| Asset browsing dialog | Custom file picker | `IDesktopPlatform::OpenDirectoryDialog` | Standard UE folder picker with native OS integration |
| Reimport registration | Manual polling or delegates | `FReimportHandler` + `FReimportManager` | Auto-integrates with the editor's Asset menu "Reimport" item |
| Content Browser menu | `FContentBrowserMenuExtender_SelectedAssets` (UE4 API) | `UToolMenus::ExtendMenu("ContentBrowser.AssetContextMenu")` | The UE4 delegate extender API is no longer the primary mechanism in UE5 |
| Widget animation ticks | UUserWidget::NativeTick + manual lerp | UWidgetAnimation + UMovieSceneFloatTrack | Timeline-based approach persists in asset, works in designer |
| MD5 change detection | Custom hash impl | `FMD5Hash` + `FAssetImportInfo::FSourceFile::FileHash` | Already in UAssetImportData infrastructure |

**Key insight:** The UE4 `FContentBrowserMenuExtender_SelectedAssets` delegate is still technically present in UE5 but the `UToolMenus` system is the UE5 way. The CONTEXT.md correctly identifies UToolMenus as the pattern to use.

---

## Common Pitfalls

### Pitfall 1: FReimportHandler Not Auto-Unregistering
**What goes wrong:** If FPsdReimportHandler is stack-allocated or destroyed while the module is alive, the destructor calls `FReimportManager::Instance()->UnregisterHandler()` — good. But if the handler is destroyed after `FReimportManager` during engine shutdown, the manager pointer is invalid.
**Why it happens:** Shutdown order is not guaranteed.
**How to avoid:** Store the handler as a `TUniquePtr` member of the module class. Initialize in `StartupModule()`, reset in `ShutdownModule()` before module cleanup completes.
**Warning signs:** Crash in `FReimportManager::UnregisterHandler` during editor shutdown.

### Pitfall 2: AddModalWindow Deadlock in Background Thread
**What goes wrong:** If `FactoryCreateBinary` is somehow called off the game thread, `GEditor->EditorAddModalWindow` will assert or deadlock.
**Why it happens:** UE editor import can be async in some contexts.
**How to avoid:** Guard with `IsInGameThread()` check before showing dialog. If off-thread, skip dialog and proceed with defaults. This is extremely rare for PSD imports triggered from Content Browser.

### Pitfall 3: UWidgetAnimation Without Module Dependencies
**What goes wrong:** `MovieScene` and `MovieSceneTracks` modules not in Build.cs → linker errors on `UMovieScene`, `UMovieSceneFloatTrack`, `AddLinearKey`.
**Why it happens:** These are runtime modules not pulled in transitively by UMG.
**How to avoid:** Explicitly add both to `PrivateDependencyModuleNames`.

### Pitfall 4: CommonUI Module Not Loaded
**What goes wrong:** Plugin compiles against CommonUI headers but the target project doesn't have CommonUI enabled → module not loaded at runtime → crash when `UCommonButtonBase::StaticClass()` is dereferenced.
**Why it happens:** CommonUI is a plugin, not a core module. Must be explicitly enabled in the project.
**How to avoid:** Guard with `FModuleManager::Get().IsModuleLoaded(TEXT("CommonUI"))` before using any CommonUI types. Log error and fall back to `UButton` if not loaded (per D-09 fallback).

### Pitfall 5: Tree View Checkbox Cascade Infinite Loop
**What goes wrong:** Parent checkbox change triggers child checkbox change triggers parent state re-evaluation triggers parent change... infinite recursion.
**Why it happens:** Incorrect wiring of the cascade logic.
**How to avoid:** Set a `bUpdatingChildren` guard flag during cascade. Or use bottom-up: only update parent's indeterminate state after all children are updated, don't trigger additional parent callbacks.

### Pitfall 6: UMovieSceneFloatSection Range Must Cover Keyframes
**What goes wrong:** Section's `TRange<FFrameNumber>` does not cover keyframe positions → keyframes are silently ignored during playback.
**Why it happens:** Sections default to a zero-duration range.
**How to avoid:** Always set `Section->SetRange(TRange<FFrameNumber>(FFrameNumber(0), FFrameNumber(EndTick)))` before adding keyframes.

### Pitfall 7: Package Metadata Not Serialized in Cooked Builds
**What goes wrong:** PSD source path metadata is lost in cooked builds.
**Why it happens:** `UMetaData` is editor-only and stripped at cook time.
**How to avoid:** This is expected and acceptable — reimport is editor-only functionality. No action needed.

---

## Code Examples

### Verified: SWindow Modal Pattern
```cpp
// Source: verified from GEditor in UnrealEd, FSlateApplication in SlateCore
bool bCancelled = false;
FPsdImportParams OutParams;

TSharedRef<SWindow> DialogWindow = SNew(SWindow)
    .Title(LOCTEXT("ImportPreviewTitle", "PSD2UMG \u2014 Import Preview"))
    .SizingRule(ESizingRule::UserSized)
    .ClientSize(FVector2D(600, 640))
    .SupportsMinimize(false)
    .SupportsMaximize(false);

// ... set content ...

GEditor->EditorAddModalWindow(DialogWindow);

if (bCancelled) return Result; // abort import
```

### Verified: FReimportHandler Required Overrides
```cpp
// Source: EditorReimportHandler.h (UE 5.7)
// All three are pure virtual and must be implemented:
virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
virtual EReimportResult::Type Reimport(UObject* Obj) override;
// EReimportResult::Type: Failed | Succeeded | Cancelled
```

### Verified: UContentBrowserAssetContextMenuContext SelectedAssets
```cpp
// Source: ContentBrowserMenuContexts.h (UE 5.7) — line 59
// SelectedAssets is TArray<FAssetData>, NOT TArray<UObject*> — no load required
UPROPERTY(BlueprintReadOnly, Category="Tool Menus")
TArray<FAssetData> SelectedAssets;
// Access tag on asset data without loading:
FString Val = Asset.GetTagValueRef<FString>(TEXT("MyTag"));
```

### Verified: UWidgetBlueprint::Animations
```cpp
// Source: WidgetBlueprint.h (UE 5.7) — line 231
// WITH_EDITORONLY_DATA:
UPROPERTY()
TArray<TObjectPtr<UWidgetAnimation>> Animations;
// Add animation:
WBP->Animations.Add(NewAnimation);
```

### Verified: UCommonButtonBase Input Action Setter
```cpp
// Source: CommonButtonBase.h (UE 5.7) — line 465
COMMONUI_API void SetTriggeringEnhancedInputAction(UInputAction* InInputAction);
// Stored in:
TObjectPtr<UInputAction> TriggeringEnhancedInputAction; // line 962
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `FContentBrowserMenuExtender_SelectedAssets` delegate | `UToolMenus::ExtendMenu` + `AddDynamicEntry` | UE5.0 | Must use UToolMenus; old delegate still works but is not the primary pattern |
| `GetSelectedObjects()` in context menu | `Context->SelectedAssets` (FAssetData, no load) | UE5.5 deprecated | Do NOT load assets in context menu construction |
| `FEditorStyle` | `FAppStyle` | UE5.0 | Already migrated in this project (per CLAUDE.md) |
| `WhitelistPlatforms` | `PlatformAllowList` | UE5.0 | Already migrated |
| IAssetTypeActions for context menu | `UAssetDefinition` + UToolMenus | UE5.2+ | IAssetTypeActions deprecated 5.2; we don't subclass it so no impact |

---

## Open Questions

1. **Scale animation property path for _hover**
   - What we know: RenderTransform is `FWidgetTransform` on `UWidget`; scale is `FVector2D Scale`.
   - What's unclear: The exact property path string for MovieScene to bind to `RenderTransform.Scale.X` — is it `"RenderTransform.Scale.X"` or something else? UMG's property binding for transform uses a dedicated `UMovieSceneWidgetComponent` track in the designer.
   - Recommendation: Use `UMovieSceneFloatTrack` with property path `"RenderTransform.Scale.X"` and `"RenderTransform.Scale.Y"` as two separate tracks. If this fails, use a `UMovieScene2DTransformTrack` (if present) or fall back to per-tick animation via a simpler `UWidgetAnimation` driving the full transform. Test empirically in Wave 1.

2. **Change detection hashing strategy**
   - What we know: Layer name is the stable identity key (D-05). We need to detect changed vs unchanged layers.
   - What's unclear: Whether pixel data hashing is fast enough for large PSDs at reimport time.
   - Recommendation: Hash a struct of `{LayerName, Bounds, Opacity, bVisible, Text.Content, Text.SizePx}` using `GetTypeHash` for quick change detection without pixel comparison. Only hash pixels if the structural properties match (full equality expensive path). Store these hashes as package metadata alongside the source path.

3. **UPsdImportFactory dual-output problem**
   - What we know: `UPsdImportFactory::FactoryCreateBinary` returns `UTexture2D*` (the texture). The WBP is a side-effect. The reimport handler via `FReimportManager::Reimport(WBP)` calls `Reimport(UObject*)` which receives the WBP, not the texture.
   - What's unclear: How to hook the reimport of the WBP back to the PSD factory when `UPsdImportFactory` is declared as handling `UTexture2D`, not `UWidgetBlueprint`.
   - Recommendation: `FPsdReimportHandler` is a separate, standalone handler (not tied to `UPsdImportFactory`'s interface). It reads the source PSD path from metadata and drives the import directly via `PSD2UMG::Parser::ParseFile` + `FWidgetBlueprintGenerator::Update()`. This is correct: the WBP reimport path is fully independent of the texture factory.

---

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| UE 5.7 editor | All | Yes | 5.7 | — |
| ContentBrowser module | EDITOR-05 | Yes | built-in | — |
| ToolMenus module | EDITOR-05 | Yes | built-in | — |
| MovieScene module | CUI-03 | Yes | built-in | — |
| MovieSceneTracks module | CUI-03 | Yes | built-in | — |
| CommonUI plugin | CUI-01, CUI-02 | Conditional | UE 5.7 bundled plugin | Fall back to UButton with log warning |
| EnhancedInput plugin | CUI-02 | Conditional | UE 5.7 bundled plugin | Skip input binding, log warning |

---

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | FAutomationTestBase (UE built-in, BEGIN_DEFINE_SPEC) |
| Config file | None — uses EditorContext + ProductFilter flags |
| Quick run command | `UnrealEditor-Win64-Test.exe [project] -ExecCmds="Automation RunTests PSD2UMG"` |
| Full suite command | Same (all PSD2UMG tests in one suite) |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| EDITOR-01 | Settings class has 4 new properties with correct defaults | unit | `PSD2UMG.Settings` spec | ❌ Wave 0 |
| EDITOR-02 | Preview dialog created, populated with correct layer tree items | integration (editor) | `PSD2UMG.ImportDialog` spec | ❌ Wave 0 |
| EDITOR-03 | Checkbox toggle cascades to children | unit (dialog logic) | `PSD2UMG.ImportDialog.CheckboxCascade` | ❌ Wave 0 |
| EDITOR-04 | Reimport handler CanReimport returns true for WBPs with PSD metadata | unit | `PSD2UMG.Reimport.CanReimport` | ❌ Wave 0 |
| EDITOR-05 | Context menu entries registered (smoke test: menu exists) | manual-only | — | — |
| CUI-01 | With bUseCommonUI=true, Button_ layers produce UCommonButtonBase | integration | `PSD2UMG.CommonUI.ButtonMapper` | ❌ Wave 0 |
| CUI-02 | Input action binding resolves by name search | unit (mock registry) | `PSD2UMG.CommonUI.InputBinding` | ❌ Wave 0 |
| CUI-03 | Animation created on WBP with correct keyframe count | unit | `PSD2UMG.Animation.Create` | ❌ Wave 0 |
| CUI-04 | ScrollBox children placed without manual height — N/A (runtime) | manual-only | — | — |

### Sampling Rate
- **Per task commit:** `PSD2UMG.Settings` + `PSD2UMG.Reimport` (fast unit specs)
- **Per wave merge:** Full `PSD2UMG.*` suite
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `Source/PSD2UMG/Tests/FSettingsSpec.cpp` — covers EDITOR-01 property defaults
- [ ] `Source/PSD2UMG/Tests/FReimportHandlerSpec.cpp` — covers EDITOR-04 CanReimport logic
- [ ] `Source/PSD2UMG/Tests/FWidgetAnimationBuilderSpec.cpp` — covers CUI-03 animation keyframe creation
- [ ] `Source/PSD2UMG/Tests/FCommonUIMapperSpec.cpp` — covers CUI-01, CUI-02 (can be skipped if CommonUI not in test environment)

---

## Project Constraints (from CLAUDE.md)

- **Engine**: UE 5.7.4 — use `FAppStyle` not `FEditorStyle`, `PlatformAllowList` not `WhitelistPlatforms`
- **Language**: C++20 — `CppStandard = CppStandardVersion.Cpp20` in Build.cs
- **No Python at runtime**: All logic in C++
- **PhotoshopAPI linkage**: Pre-built static lib — no changes needed for this phase
- **Editor-only**: Module type "Editor", LoadingPhase "PostEngineInit"
- **Naming**: `F` prefix for non-UObject value types; `U` for UObject; `S` for Slate widgets; PascalCase; boolean members prefixed `b`
- **Error handling**: Never abort import — skip + warn pattern; use `UE_LOG(LogPSD2UMG, Warning/Error, ...)`
- **Settings pattern**: `EditAnywhere, config, BlueprintReadWrite` for UPROPERTY; `GetCategoryName` returns `"Plugins"`, `GetSectionName` returns `"PSD2UMG"`
- **GSD workflow**: No direct repo edits without GSD command

---

## Sources

### Primary (HIGH confidence)
- `/c/Program Files/Epic Games/UE_5.7/Engine/Source/Editor/UnrealEd/Public/EditorReimportHandler.h` — FReimportHandler/FReimportManager interface, all 3 required virtual methods
- `/c/Program Files/Epic Games/UE_5.7/Engine/Source/Editor/UMGEditor/Public/WidgetBlueprint.h` — `TArray<TObjectPtr<UWidgetAnimation>> Animations` (line 231)
- `/c/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/UMG/Public/Animation/WidgetAnimation.h` — `AnimationBindings`, `MovieScene`, `GetBindings()`
- `/c/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/UMG/Public/Animation/WidgetAnimationBinding.h` — `FWidgetAnimationBinding` struct fields
- `/c/Program Files/Epic Games/UE_5.7/Engine/Source/Editor/ContentBrowser/Public/ContentBrowserMenuContexts.h` — `UContentBrowserAssetContextMenuContext`, `SelectedAssets` (TArray<FAssetData>), `GetSelectedObjects` deprecated 5.5
- `/c/Program Files/Epic Games/UE_5.7/Engine/Source/Developer/ToolMenus/Public/ToolMenus.h` — `ExtendMenu`, `RegisterStartupCallback`
- `/c/Program Files/Epic Games/UE_5.7/Engine/Source/Developer/ToolMenus/Public/ToolMenuSection.h` — `AddDynamicEntry`, `AddMenuEntry`
- `/c/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/MovieScene/Public/MovieScene.h` — `AddPossessable`, `AddTrack<T>`, `SetPlaybackRange`, `SetTickResolutionDirectly`
- `/c/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/MovieSceneTracks/Public/Tracks/MovieSceneFloatTrack.h` — `UMovieSceneFloatTrack`, `SetPropertyNameAndPath`
- `/c/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/MovieSceneTracks/Public/Sections/MovieSceneFloatSection.h` — `GetChannel()` → `FMovieSceneFloatChannel&`
- `/c/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/MovieScene/Public/Channels/MovieSceneFloatChannel.h` — `AddLinearKey(FFrameNumber, float)`, `AddCubicKey`, `AddConstantKey`
- `/c/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Engine/Classes/EditorFramework/AssetImportData.h` — `UAssetImportData::Update()`, `GetFirstFilename()`, `SourceData`
- `/c/Program Files/Epic Games/UE_5.7/Engine/Plugins/Runtime/CommonUI/Source/CommonUI/Public/CommonButtonBase.h` — `SetTriggeringEnhancedInputAction(UInputAction*)`, `TriggeringEnhancedInputAction` property
- `/c/Program Files/Epic Games/UE_5.7/Engine/Source/Editor/ContentBrowser/Private/AssetContextMenu.cpp` — Confirmed menu name `"ContentBrowser.AssetContextMenu"` (line 195)

### Secondary (MEDIUM confidence)
- [UE5 IReimportHandler forum tutorial](https://store.algosyntax.com/tutorials/unreal-engine/tutorial-ue5-add-reimport-support-to-custom-asset/) — implementation flow overview (headers verified locally)
- [UE5 Content Browser context menu extension forum](https://forums.unrealengine.com/t/adding-menu-entry-to-content-browser-asset-menu/541290) — UToolMenus pattern confirmation

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all modules/headers verified from UE 5.7 local install
- Architecture: HIGH — FReimportHandler, UToolMenus, SWindow/STreeView all verified from engine headers
- UWidgetAnimation creation: MEDIUM — API chain verified from headers but FMovieSceneFloatChannel.AddLinearKey for render opacity and scale not tested end-to-end; scale animation property path requires empirical validation
- CommonUI conditional linking: MEDIUM — API verified, but project-level plugin availability at test time is unknown
- Pitfalls: HIGH — derived from header analysis and known UE patterns

**Research date:** 2026-04-10
**Valid until:** 2026-10-10 (stable UE5 APIs; 6-month estimate)

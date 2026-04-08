# PSD → UMG Plugin: Development Plan

## Project overview

We are building an Unreal Engine 5.7.4 editor plugin that imports `.psd` files and converts them into fully functional UMG Widget Blueprints. The plugin preserves layer hierarchy, positions, text properties, images, and effects — so a designer's Photoshop mockup becomes a working UI in one click.

**Base project:** Fork of [HakimHua/AutoPSDUI](https://github.com/HakimHua/AutoPSDUI) — an open-source UE4 plugin (11 stars, 7 commits) that does basic PSD → WBP conversion. Written ~82% Python, ~15% C++. We are rewriting it into a production-grade C++ plugin for UE 5.7.

**Target engine:** Unreal Engine 5.7.4  
**Target platforms:** Win64 (primary), Mac (secondary)  
**Language:** C++20 with minimal Python (user scripting only)  
**License:** MIT (same as original)

---

## Architecture

The entire system is a **three-stage pipeline**:

```
.psd file
    │
    ▼
┌─────────────────────────────┐
│  Stage 1: PSD Parser        │  PhotoshopAPI (C++20 library)
│  Input:  raw .psd bytes     │  github.com/EmilDohne/PhotoshopAPI
│  Output: FPsdDocument       │  327 stars, actively maintained
└─────────────┬───────────────┘
              │
              ▼
┌─────────────────────────────┐
│  Stage 2: Layer Mapper      │  Pluggable strategy pattern
│  Input:  FPsdDocument       │  Maps PSD layers → widget descriptors
│  Output: FWidgetTreeModel   │  User can register custom mappers
└─────────────┬───────────────┘
              │
              ▼
┌─────────────────────────────┐
│  Stage 3: Widget Builder    │  UE UMG/Slate API
│  Input:  FWidgetTreeModel   │  Creates UWidgetBlueprint asset
│  Output: .uasset (WBP)     │  Imports textures, sets anchors
└─────────────────────────────┘
```

Each stage is isolated, independently testable, and replaceable (e.g. Stage 1 could be swapped for a Figma API parser later).

---

## Key data structures

```cpp
// Stage 1 output — intermediate PSD representation
struct FPsdLayerInfo
{
    FString Name;
    EPsdLayerType Type;           // Image, Text, Group, SmartObject, Shape, Adjustment
    FIntRect Bounds;              // Position & size in canvas coordinates
    float Opacity;                // 0.0 - 1.0
    EBlendMode BlendMode;
    bool bVisible;
    int32 ZOrder;
    
    // Text-specific (only if Type == Text)
    TOptional<FPsdTextInfo> TextInfo;
    
    // Image-specific (only if Type == Image/Shape/SmartObject)
    TArray<uint8> PixelData;      // RGBA raw pixels
    int32 PixelWidth;
    int32 PixelHeight;
    
    // Layer effects
    TArray<FPsdLayerEffect> Effects;  // Drop Shadow, Stroke, Color Overlay, etc.
    
    // Hierarchy
    TArray<TSharedPtr<FPsdLayerInfo>> Children;
    TWeakPtr<FPsdLayerInfo> Parent;
};

struct FPsdTextInfo
{
    FString Content;
    FString FontFamily;
    float FontSize;                // In Photoshop points (72 DPI)
    FLinearColor Color;
    bool bBold;
    bool bItalic;
    ETextJustify::Type Alignment;
    float LetterSpacing;
    float LineHeight;
    
    // Effects on text
    TOptional<FPsdStrokeEffect> Outline;
    TOptional<FPsdShadowEffect> Shadow;
};

struct FPsdDocument
{
    int32 CanvasWidth;
    int32 CanvasHeight;
    int32 DPI;
    TArray<TSharedPtr<FPsdLayerInfo>> RootLayers;  // Top-level layers
    
    // Flatten the tree into an ordered list
    TArray<TSharedPtr<FPsdLayerInfo>> GetAllLayers() const;
    
    // Find layer by path ("Group1/SubGroup/LayerName")
    TSharedPtr<FPsdLayerInfo> FindLayer(const FString& Path) const;
};

// Stage 2 output — widget tree descriptor
struct FWidgetNodeDescriptor
{
    FString Name;
    EWidgetType WidgetType;       // Image, TextBlock, Button, CanvasPanel, Overlay, HBox, VBox, ScrollBox, etc.
    FVector2D Position;            // Relative to parent, in UMG coordinates
    FVector2D Size;
    FAnchorData Anchors;
    float RenderOpacity;
    
    // Widget-specific properties
    TSharedPtr<FImageWidgetProps> ImageProps;
    TSharedPtr<FTextWidgetProps> TextProps;
    TSharedPtr<FButtonWidgetProps> ButtonProps;
    // ... etc.
    
    TArray<TSharedPtr<FWidgetNodeDescriptor>> Children;
};
```

---

## Phase 0 — Fork & port to UE 5.7 (~1 week)

**Goal:** AutoPSDUI compiles and runs on UE 5.7.4, basic PSD → texture import works.

### Tasks

1. **Fork & branch setup**
   - Fork `HakimHua/AutoPSDUI` to your GitHub
   - Create branch `feature/ue57-port`
   - Restructure as a proper UE plugin in `Plugins/PSD2UMG/`

2. **Update .uplugin descriptor**
   ```json
   {
       "FileVersion": 3,
       "Version": 1,
       "VersionName": "0.1.0",
       "FriendlyName": "PSD2UMG",
       "Description": "Import PSD files as UMG Widget Blueprints",
       "Category": "Editor",
       "CreatedBy": "YOUR_NAME",
       "EngineVersion": "5.7",
       "Modules": [
           {
               "Name": "PSD2UMG",
               "Type": "Editor",
               "LoadingPhase": "PostEngineInit",
               "PlatformAllowList": ["Win64", "Mac"]
           }
       ],
       "Plugins": [
           {
               "Name": "EditorScriptingUtilities",
               "Enabled": true
           }
       ]
   }
   ```
   - Note: `WhitelistPlatforms` → `PlatformAllowList` (UE5 rename)
   - Remove `PythonScriptPlugin` dependency (we're going pure C++)

3. **Fix deprecated API calls**
   - `FEditorStyle` → `FAppStyle` (since UE 5.1)
   - `GEditor->GetEditorSubsystem<>()` patterns
   - `AssetRegistry` module name changes
   - `FAssetData` → check if any constructor changes
   - Blueprint creation APIs: `FKismetEditorUtilities`, `FWidgetBlueprintEditorUtils`

4. **Verify compilation**
   - Plugin compiles with 0 errors, 0 warnings
   - Dropping a .psd into Content Browser still imports it as UTexture2D (default UE behavior)
   - Existing Python scripts may break — that's expected, we'll replace them

### Acceptance criteria
- [ ] Plugin loads in UE 5.7.4 editor without errors
- [ ] Plugin appears in Edit → Plugins
- [ ] .psd drag-and-drop into Content Browser creates a UTexture2D

---

## Phase 1 — Replace PSD parser with C++ PhotoshopAPI (~2-3 weeks)

**Goal:** Remove Python dependency. Parse PSD layers natively in C++ using PhotoshopAPI.

### PhotoshopAPI integration

[PhotoshopAPI](https://github.com/EmilDohne/PhotoshopAPI) is a C++20 library (327 stars, MIT license) that parses PSD/PSB files with full layer support. It handles 8/16/32-bit depths, groups, text layers, masks, and smart objects.

### Tasks

1. **Add PhotoshopAPI as dependency**
   ```
   Plugins/PSD2UMG/
   ├── Source/
   │   ├── PSD2UMG/
   │   │   ├── PSD2UMG.Build.cs
   │   │   ├── Private/
   │   │   └── Public/
   │   └── ThirdParty/
   │       └── PhotoshopAPI/          ← git submodule
   ```
   - Add as git submodule: `git submodule add https://github.com/EmilDohne/PhotoshopAPI.git Source/ThirdParty/PhotoshopAPI`
   - In `PSD2UMG.Build.cs`:
     ```csharp
     PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "../ThirdParty/PhotoshopAPI/include"));
     // Link static lib or compile sources directly
     // PhotoshopAPI uses CMake — may need to pre-build or wrap as UE module
     ```
   - **Alternative approach:** If CMake integration is painful, compile PhotoshopAPI as a static `.lib` externally and reference it in `.Build.cs` via `PublicAdditionalLibraries`

2. **Write FPsdParser class**
   ```cpp
   // PsdParser.h
   class FPsdParser
   {
   public:
       // Parse a PSD file from disk and return intermediate representation
       static TSharedPtr<FPsdDocument> ParseFile(const FString& FilePath);
       
   private:
       // Convert PhotoshopAPI LayeredFile tree into our FPsdDocument
       static void ConvertLayerTree(
           const NAMESPACE_PSAPI::LayeredFile<NAMESPACE_PSAPI::bpp8_t>& LayeredFile,
           FPsdDocument& OutDocument
       );
       
       // Recursively convert a single layer
       static TSharedPtr<FPsdLayerInfo> ConvertLayer(
           const std::shared_ptr<NAMESPACE_PSAPI::Layer<NAMESPACE_PSAPI::bpp8_t>>& PsLayer
       );
       
       // Extract pixel data from an ImageLayer into RGBA TArray
       static void ExtractPixelData(
           const NAMESPACE_PSAPI::ImageLayer<NAMESPACE_PSAPI::bpp8_t>& ImageLayer,
           TArray<uint8>& OutPixels,
           int32& OutWidth,
           int32& OutHeight
       );
   };
   ```

3. **Write FPsdDocument and FPsdLayerInfo structs**
   - Implement as described in "Key data structures" section above
   - Include serialization helpers for debugging (dump tree to log)

4. **Create UPsdImportFactory**
   ```cpp
   // Hook into UE's asset import pipeline
   UCLASS()
   class UPsdImportFactory : public UFactory
   {
       GENERATED_BODY()
   public:
       UPsdImportFactory();
       
       // UFactory interface
       virtual UObject* FactoryCreateFile(
           UClass* InClass,
           UObject* InParent,
           FName InName,
           EObjectFlags Flags,
           const FString& Filename,
           const TCHAR* Parms,
           FFeedbackContext* Warn,
           bool& bOutOperationCanceled
       ) override;
       
       virtual bool FactoryCanImport(const FString& Filename) override;
   };
   ```
   - For now, `FactoryCreateFile` calls `FPsdParser::ParseFile()` and logs the layer tree
   - Actual widget creation comes in Phase 2

5. **Remove Python scripts**
   - Delete `Content/Python/` directory
   - Remove all `IPythonScriptPlugin` calls from C++ code
   - Keep Python as optional for user scripting (but plugin core is pure C++)

### Acceptance criteria
- [ ] `FPsdParser::ParseFile("test.psd")` correctly returns layer tree
- [ ] Layer names, types, bounds, visibility, opacity are correct
- [ ] Image pixel data extraction works (verified by saving as PNG)
- [ ] Text layer content is extracted (font info may be incomplete — ok for now)
- [ ] Groups and nesting are preserved
- [ ] No Python dependency at runtime

---

## Phase 2 — Layer mapping to UMG widgets (~3-4 weeks)

**Goal:** Convert the parsed layer tree into actual UMG Widget Blueprints.

### Naming conventions (how designers mark up PSD layers)

```
Layer name                    →  UMG Widget
─────────────────────────────────────────────
Any image layer               →  UImage
Any text layer                →  UTextBlock
Any group (default)           →  UCanvasPanel
Button_MyButton               →  UButton
Progress_HealthBar            →  UProgressBar
ScrollBox_Content             →  UScrollBox
Slider_Volume                 →  USlider
CheckBox_Accept               →  UCheckBox
Input_Username                →  UEditableTextBox
HBox_NavBar                   →  UHorizontalBox
VBox_Sidebar                  →  UVerticalBox
Overlay_Stack                 →  UOverlay
Switcher_Pages                →  UWidgetSwitcher
List_Items                    →  UListView
Tile_Grid                     →  UTileView
Spacer_Gap                    →  USpacer
```

**Button states** (children of a `Button_` group):
```
_normal    →  Normal style brush
_hovered   →  Hovered style brush
_pressed   →  Pressed style brush
_disabled  →  Disabled style brush
```

**ProgressBar parts** (children of a `Progress_` group):
```
_background  →  Background image brush
_fill        →  Fill image brush
```

### Tasks

1. **Design IPsdLayerMapper interface**
   ```cpp
   class IPsdLayerMapper
   {
   public:
       virtual ~IPsdLayerMapper() = default;
       
       // Return true if this mapper handles the given layer
       virtual bool CanMap(const FPsdLayerInfo& Layer) const = 0;
       
       // Convert layer into a widget descriptor
       virtual TSharedPtr<FWidgetNodeDescriptor> Map(
           const FPsdLayerInfo& Layer,
           const FPsdDocument& Document
       ) const = 0;
       
       // Priority (higher = checked first)
       virtual int32 GetPriority() const { return 0; }
   };
   ```

2. **Implement built-in mappers**
   - `FButtonLayerMapper` — handles `Button_` prefix, collects child state images
   - `FProgressBarLayerMapper` — handles `Progress_` prefix
   - `FScrollBoxLayerMapper`, `FSliderLayerMapper`, etc.
   - `FTextLayerMapper` — handles any text layer
   - `FImageLayerMapper` — handles any image/shape layer (fallback)
   - `FGroupLayerMapper` — handles groups → CanvasPanel/Overlay/HBox/VBox based on prefix

3. **Build FLayerMappingRegistry**
   ```cpp
   class FLayerMappingRegistry
   {
   public:
       void RegisterMapper(TSharedPtr<IPsdLayerMapper> Mapper);
       TSharedPtr<FWidgetNodeDescriptor> MapLayer(
           const FPsdLayerInfo& Layer,
           const FPsdDocument& Document
       ) const;
       
   private:
       TArray<TSharedPtr<IPsdLayerMapper>> Mappers; // Sorted by priority
   };
   ```

4. **Write FWidgetBlueprintGenerator**
   ```cpp
   class FWidgetBlueprintGenerator
   {
   public:
       // Create a Widget Blueprint asset from a widget tree
       static UWidgetBlueprint* Generate(
           const FWidgetTreeModel& Tree,
           const FString& PackagePath,    // e.g. "/Game/UI/MyScreen"
           const FPsdImportSettings& Settings
       );
       
   private:
       // Recursively add widgets to the widget tree
       static UWidget* CreateWidget(
           UWidgetBlueprint* Blueprint,
           const FWidgetNodeDescriptor& Node,
           UPanelWidget* ParentPanel
       );
       
       // Import image data as UTexture2D asset
       static UTexture2D* ImportTexture(
           const TArray<uint8>& PixelData,
           int32 Width, int32 Height,
           const FString& AssetName,
           const FString& PackagePath
       );
       
       // Calculate and assign anchors based on position heuristics
       static FAnchorData CalculateAnchors(
           const FWidgetNodeDescriptor& Node,
           const FVector2D& ParentSize
       );
   };
   ```

5. **Texture export pipeline**
   - For each Image layer: extract RGBA pixels → save as temp PNG → import via `UTextureFactory` → create `FSlateBrush` → assign to `UImage::SetBrush()`
   - Organize textures: `{TargetDir}/Textures/{PsdName}/{LayerName}.png`
   - Handle duplicate names by appending index

6. **Anchor heuristics**
   ```
   Layer in top-left quadrant      → Anchor: Top-Left (0, 0, 0, 0)
   Layer in top-right quadrant     → Anchor: Top-Right (1, 0, 1, 0)
   Layer in bottom-left quadrant   → Anchor: Bottom-Left (0, 1, 0, 1)
   Layer in bottom-right quadrant  → Anchor: Bottom-Right (1, 1, 1, 1)
   Layer centered horizontally     → Anchor: Top-Center (0.5, 0, 0.5, 0)
   Layer spanning full width       → Anchor: Stretch-Horizontal (0, 0, 1, 0)
   Layer spanning full height      → Anchor: Stretch-Vertical (0, 0, 0, 1)
   Layer spanning full canvas      → Anchor: Stretch-All (0, 0, 1, 1)
   
   Override via suffix: _anchor-center, _stretch-h, _stretch-v, _stretch-all
   ```

### Acceptance criteria
- [ ] Simple PSD with 3 image layers → WBP with 3 UImage widgets at correct positions
- [ ] Group layers → nested CanvasPanel
- [ ] `Button_` groups → UButton with state images
- [ ] `Progress_` groups → UProgressBar with background/fill
- [ ] Anchors are assigned based on position heuristics
- [ ] Textures are imported and organized in target directory
- [ ] Widget Blueprint opens correctly in UMG Designer

---

## Phase 3 — Text, fonts and typography (~2 weeks)

**Goal:** Full text layer support with correct font, size, color, and effects.

### Critical: DPI conversion

Photoshop default is 72 DPI. UMG works at 96 DPI.  
**Conversion factor:** `UMG_size = PS_size × (72.0 / 96.0) = PS_size × 0.75`

Example: 33pt in Photoshop @ 72 DPI = 24.75pt in UMG @ 96 DPI.

### Tasks

1. **Extract text properties from PhotoshopAPI**
   - PhotoshopAPI has an open issue for text layers (#126) — check current status
   - If text extraction is incomplete, fallback: parse the raw PSD `lrFX`/`TySh` descriptors manually
   - Extract: string content, font name, size, color, alignment, bold/italic, letter spacing, line height

2. **Font mapping system**
   ```cpp
   USTRUCT()
   struct FPsdFontMapping
   {
       GENERATED_BODY()
       
       // Photoshop font name → UE font asset
       UPROPERTY(EditAnywhere)
       TMap<FString, TSoftObjectPtr<UFont>> FontMap;
       
       UPROPERTY(EditAnywhere)
       TSoftObjectPtr<UFont> DefaultFont;  // Fallback
   };
   ```
   - Stored in plugin settings (UDeveloperSettings subclass)
   - Editor UI to configure mappings (Phase 6)

3. **Create UTextBlock with full properties**
   ```cpp
   UTextBlock* TextWidget = WidgetTree->ConstructWidget<UTextBlock>();
   TextWidget->SetText(FText::FromString(TextInfo.Content));
   
   FSlateFontInfo FontInfo;
   FontInfo.FontObject = ResolveFont(TextInfo.FontFamily);
   FontInfo.Size = TextInfo.FontSize * 0.75f;  // DPI conversion
   
   if (TextInfo.bBold) FontInfo.TypefaceFontName = TEXT("Bold");
   if (TextInfo.bItalic) FontInfo.TypefaceFontName = TEXT("Italic");
   
   TextWidget->SetFont(FontInfo);
   TextWidget->SetColorAndOpacity(TextInfo.Color);
   TextWidget->SetJustification(TextInfo.Alignment);
   
   // Outline
   if (TextInfo.Outline.IsSet())
   {
       FontInfo.OutlineSettings.OutlineSize = TextInfo.Outline->Width;
       FontInfo.OutlineSettings.OutlineColor = TextInfo.Outline->Color;
   }
   
   // Shadow
   if (TextInfo.Shadow.IsSet())
   {
       TextWidget->SetShadowOffset(TextInfo.Shadow->Offset);
       TextWidget->SetShadowColorAndOpacity(TextInfo.Shadow->Color);
   }
   ```

4. **Multi-line text handling**
   - Detect line breaks in PSD text content
   - Set `AutoWrapText = true` if text box width is defined
   - Calculate size box wrapper if needed

### Acceptance criteria
- [ ] Text layers become UTextBlock with correct content
- [ ] Font size matches visually (with DPI conversion)
- [ ] Color, bold, italic are correct
- [ ] Text alignment (left/center/right) is correct
- [ ] Outline and shadow effects transfer
- [ ] Font mapping system works (PS font name → UE font asset)
- [ ] Multi-line text preserves line breaks

---

## Phase 4 — Layer effects and blend modes (~2-3 weeks)

**Goal:** Support common Photoshop layer effects in UMG.

### Mappable effects

| PS Effect | UMG Equivalent | Strategy |
|-----------|---------------|----------|
| Opacity | `SetRenderOpacity()` | Direct |
| Color Overlay (Multiply) | `Brush.TintColor` | Direct |
| Drop Shadow | Shadow material or extra UImage offset | Approximate |
| Outer Glow | Post-process material | Approximate |
| Stroke | UBorder or material | Approximate |
| Inner Shadow | Material | Approximate |
| Gradient Overlay | Material with gradient | Material-based |
| Blend modes (Normal, Multiply, Screen, Overlay) | Material blend modes | Material-based |

### Tasks

1. **Parse layer effects from PSD**
   ```cpp
   struct FPsdLayerEffect
   {
       EPsdEffectType Type;  // DropShadow, OuterGlow, InnerShadow, ColorOverlay, GradientOverlay, Stroke
       FLinearColor Color;
       float Opacity;
       FVector2D Offset;     // For shadows
       float Size;           // Blur radius / stroke width
       float Spread;
       EBlendMode BlendMode;
   };
   ```

2. **Direct mappings** (simple, no material needed)
   - Layer opacity → `Widget->SetRenderOpacity(Layer.Opacity)`
   - Color Overlay → `ImageWidget->SetBrushTintColor(Effect.Color)`
   - Visibility → `Widget->SetVisibility(Layer.bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed)`

3. **Approximate mappings** (shadow as offset duplicate)
   - Drop Shadow: create a second UImage behind the main one, offset by shadow distance, tinted to shadow color, with slight blur (scale up slightly as approximation)
   - This is a rough approximation — note in docs that complex shadows should be flattened

4. **Material-based effects**
   - Create a base `M_PSD_BlendMode` material with parameters for blend mode simulation
   - For each layer needing a material effect: create `UMaterialInstanceDynamic` at import time
   - Support Multiply and Screen as the most common modes

5. **Flatten fallback**
   - For any effect that can't be reasonably mapped: flatten the layer with its effects into a single rasterized image
   - User setting: `bFlattenComplexEffects = true` (default)
   - When flattening: compose the layer with its effects in memory → export as PNG → import as UTexture2D

### Acceptance criteria
- [ ] Layer opacity correctly applied
- [ ] Color Overlay works on image layers
- [ ] Drop Shadow creates approximate visual match
- [ ] Complex effects fall back to flattened raster
- [ ] User can choose between approximate mapping and flatten per-layer

---

## Phase 5 — 9-Slice, adaptive anchors, Smart Objects (~2-3 weeks)

**Goal:** Production-ready layout features.

### Tasks

1. **9-Slice support**
   - Trigger: layer name suffix `_9s` or `_9slice`
   - Or: detect if layer contains Photoshop slice data
   - Export texture, then set brush:
     ```cpp
     FSlateBrush Brush;
     Brush.DrawAs = ESlateBrushDrawType::Box;  // 9-slice mode
     Brush.Margin = FMargin(Left, Top, Right, Bottom);  // Normalized 0-1
     ```
   - Allow specifying margins via layer name: `MyButton_9s[16,16,16,16]`

2. **Improved anchor heuristics**
   - Analyze element position relative to canvas AND parent group
   - Detect patterns:
     - Multiple elements in a horizontal row → suggest HorizontalBox
     - Multiple elements in a vertical stack → suggest VerticalBox
     - Element at exact center → center anchor
     - Element touching edges → stretch to that edge
   - Allow override via suffix: `_anchor-tl`, `_anchor-c`, `_stretch-h`, `_stretch-v`, `_fill`

3. **Smart Object handling**
   - If a layer is a Smart Object → it's a self-contained sub-document
   - Try to extract embedded PSD/PSB from the Smart Object data
   - Recursively import as a separate Widget Blueprint
   - Reference it via `UUserWidget` child in the parent WBP
   - Fallback: if extraction fails, rasterize the Smart Object as an image

4. **Variant groups**
   - Group suffix `_variants` → create UWidgetSwitcher
   - Each child layer becomes one variant (one slot in the switcher)
   - Useful for state-based UI (selected/unselected tabs, active/inactive panels)

### Acceptance criteria
- [ ] `_9s` layers produce correctly sliced UImage brushes
- [ ] Anchors are intelligent based on position analysis
- [ ] Anchor overrides via suffixes work
- [ ] Smart Objects import as child Widget Blueprints
- [ ] `_variants` groups create UWidgetSwitcher

---

## Phase 6 — Editor UI, preview, and settings (~2 weeks)

**Goal:** User-friendly import workflow with preview and configuration.

### Tasks

1. **Plugin settings** (UDeveloperSettings)
   ```cpp
   UCLASS(config=Editor, defaultconfig, meta=(DisplayName="PSD2UMG"))
   class UPSD2UMGSettings : public UDeveloperSettings
   {
       GENERATED_BODY()
   public:
       // Where to save generated Widget Blueprints
       UPROPERTY(config, EditAnywhere, Category="Output")
       FDirectoryPath WidgetBlueprintOutputDir = {TEXT("/Game/UI/Generated")};
       
       // Where to save imported textures
       UPROPERTY(config, EditAnywhere, Category="Output")
       FDirectoryPath TextureOutputDir = {TEXT("/Game/UI/Generated/Textures")};
       
       // Font mapping (PS font name → UE font asset)
       UPROPERTY(config, EditAnywhere, Category="Fonts")
       TMap<FString, TSoftObjectPtr<UFont>> FontMap;
       
       UPROPERTY(config, EditAnywhere, Category="Fonts")
       TSoftObjectPtr<UFont> DefaultFont;
       
       // DPI setting (default 72 for Photoshop)
       UPROPERTY(config, EditAnywhere, Category="Import", meta=(ClampMin=72, ClampMax=300))
       int32 SourceDPI = 72;
       
       // Effect handling
       UPROPERTY(config, EditAnywhere, Category="Effects")
       bool bFlattenComplexEffects = true;
       
       // Auto-assign anchors
       UPROPERTY(config, EditAnywhere, Category="Layout")
       bool bAutoAssignAnchors = true;
       
       // Use CommonUI widgets instead of standard UMG
       UPROPERTY(config, EditAnywhere, Category="Widgets")
       bool bUseCommonUI = false;
   };
   ```

2. **Import preview dialog**
   - When user imports a PSD, show a modal dialog before creating assets
   - Left panel: layer tree with checkboxes (which layers to import)
   - Right panel: preview of what each layer will become (widget type badge)
   - Bottom: settings override for this specific import
   - Implement as `SWindow` + custom Slate widgets

3. **Reimport support**
   - Register reimport handler via `FReimportManager`
   - On reimport: compare old layer tree with new one
   - Update only changed layers (position, image data, text content)
   - Preserve any manual changes to the Widget Blueprint (event bindings, custom logic)
   - Use layer name as the stable identifier for matching

4. **Content Browser integration**
   - Right-click `.psd` asset → "Import as Widget Blueprint" context menu
   - Custom thumbnail for PSD assets (show flattened preview)
   - Toolbar button in Widget Blueprint Editor: "Re-import from PSD"

### Acceptance criteria
- [ ] Plugin settings appear in Project Settings → Plugins → PSD2UMG
- [ ] Import preview dialog shows layer tree before importing
- [ ] User can toggle layers on/off before import
- [ ] Reimport updates changed layers without destroying manual edits
- [ ] Right-click context menu works in Content Browser

---

## Phase 7 — CommonUI and interactive widgets (~2 weeks)

**Goal:** Optional CommonUI support and animation generation.

### Tasks

1. **CommonUI widget mapping** (when `bUseCommonUI = true`)
   ```
   Button_ prefix     → UCommonButtonBase (instead of UButton)
   Group (root)       → UCommonActivatableWidget (instead of UUserWidget)
   Input_ prefix      → UCommonTextInput (if available)
   ```
   - This is opt-in because CommonUI requires additional project setup

2. **Input action binding via layer names**
   - Syntax: `Button_Confirm[IA_Confirm]` → bind to Input Action `IA_Confirm`
   - Parse the `[IA_xxx]` suffix, find the corresponding `UInputAction` asset
   - Set up binding in the generated Blueprint graph

3. **Animation generation from layer variants**
   - If a group contains layers with suffixes `_show`, `_hide`, `_hover`:
     ```
     Group_MyPanel/
       Panel_show          ← keyframe for "show" animation
       Panel_hide          ← keyframe for "hide" animation
       Panel_hover         ← keyframe for "hover" animation
     ```
   - Generate `UWidgetAnimation` for each state transition
   - Animate: opacity, position (RenderTransform), scale
   - Calculate interpolation from the difference between states

4. **Scroll content setup**
   - `ScrollBox_` groups: measure total content height from children
   - Set up scroll box with correct content size
   - Children placed sequentially in a VerticalBox inside the ScrollBox

### Acceptance criteria
- [ ] CommonUI mode generates UCommonButtonBase instead of UButton
- [ ] Input action bindings from layer name syntax work
- [ ] Basic show/hide animations are generated
- [ ] ScrollBox with content taller than viewport scrolls correctly

---

## Phase 8 — Testing, documentation, release (~2 weeks)

### Tasks

1. **Unit tests** (no UE dependency)
   - PSD parsing: verify layer tree structure from known test PSDs
   - Layer mapping: verify correct widget type selection from layer names
   - Anchor calculation: verify anchor values from position inputs
   - DPI conversion: verify font size calculations
   - Place in `Source/PSD2UMG/Tests/`

2. **Integration tests** (require UE editor)
   - Full pipeline: PSD → import → open WBP in designer → verify widget count and types
   - Reimport: import → modify PSD → reimport → verify changes applied
   - Use `FAutomationTestBase` framework

3. **Test PSD files**
   - `Test_SimpleHUD.psd` — 5 image layers, basic layout
   - `Test_ComplexMenu.psd` — nested groups, buttons with states, progress bars
   - `Test_MobileUI.psd` — 1080x1920, various anchoring scenarios
   - `Test_Typography.psd` — text layers with different fonts, sizes, colors, effects
   - `Test_Effects.psd` — layers with shadows, overlays, blend modes
   - Include these in `Plugins/PSD2UMG/TestData/`

4. **Documentation**
   - `README.md` — quick start, installation, basic usage
   - `CONVENTIONS.md` — full naming convention reference for designers
   - `ARCHITECTURE.md` — code structure, pipeline stages, how to extend
   - `CHANGELOG.md` — version history

5. **Example project**
   - Standalone UE 5.7 project with the plugin
   - 3-4 pre-imported PSD → WBP examples
   - Demonstrates: basic HUD, main menu, settings screen, in-game popup

6. **CI/CD**
   - GitHub Actions workflow: compile plugin on Win64
   - Run unit tests on push to `develop` and `main`
   - Package plugin as zip for release

### Acceptance criteria
- [ ] All unit tests pass
- [ ] All integration tests pass
- [ ] Documentation covers installation, usage, and naming conventions
- [ ] Example project works out of the box
- [ ] CI builds and tests pass

---

## Module structure

```
Plugins/PSD2UMG/
├── PSD2UMG.uplugin
├── README.md
├── CONVENTIONS.md
├── Source/
│   ├── PSD2UMG/
│   │   ├── PSD2UMG.Build.cs
│   │   ├── Public/
│   │   │   ├── PSD2UMGModule.h
│   │   │   ├── PsdParser.h              ← Stage 1: PSD parsing
│   │   │   ├── PsdDocument.h            ← Intermediate data model
│   │   │   ├── PsdLayerInfo.h
│   │   │   ├── PsdTextInfo.h
│   │   │   ├── PsdLayerEffect.h
│   │   │   ├── LayerMapper/
│   │   │   │   ├── IPsdLayerMapper.h    ← Stage 2: Mapper interface
│   │   │   │   ├── LayerMappingRegistry.h
│   │   │   │   ├── ButtonLayerMapper.h
│   │   │   │   ├── ProgressBarLayerMapper.h
│   │   │   │   ├── TextLayerMapper.h
│   │   │   │   ├── ImageLayerMapper.h
│   │   │   │   └── GroupLayerMapper.h
│   │   │   ├── WidgetBuilder/
│   │   │   │   ├── WidgetNodeDescriptor.h  ← Stage 3: Widget tree model
│   │   │   │   ├── WidgetBlueprintGenerator.h
│   │   │   │   ├── AnchorCalculator.h
│   │   │   │   └── TextureImporter.h
│   │   │   ├── Editor/
│   │   │   │   ├── PsdImportFactory.h
│   │   │   │   ├── PsdImportSettings.h
│   │   │   │   ├── PsdImportPreviewDialog.h
│   │   │   │   └── PSD2UMGSettings.h
│   │   │   └── CommonUI/
│   │   │       └── CommonUIMapper.h      ← Optional CommonUI support
│   │   ├── Private/
│   │   │   ├── PSD2UMGModule.cpp
│   │   │   ├── PsdParser.cpp
│   │   │   ├── PsdDocument.cpp
│   │   │   ├── LayerMapper/
│   │   │   │   ├── LayerMappingRegistry.cpp
│   │   │   │   ├── ButtonLayerMapper.cpp
│   │   │   │   ├── ProgressBarLayerMapper.cpp
│   │   │   │   ├── TextLayerMapper.cpp
│   │   │   │   ├── ImageLayerMapper.cpp
│   │   │   │   └── GroupLayerMapper.cpp
│   │   │   ├── WidgetBuilder/
│   │   │   │   ├── WidgetBlueprintGenerator.cpp
│   │   │   │   ├── AnchorCalculator.cpp
│   │   │   │   └── TextureImporter.cpp
│   │   │   ├── Editor/
│   │   │   │   ├── PsdImportFactory.cpp
│   │   │   │   ├── PsdImportPreviewDialog.cpp
│   │   │   │   └── PSD2UMGSettings.cpp
│   │   │   └── CommonUI/
│   │   │       └── CommonUIMapper.cpp
│   │   └── Tests/
│   │       ├── PsdParserTest.cpp
│   │       ├── LayerMapperTest.cpp
│   │       └── AnchorCalculatorTest.cpp
│   └── ThirdParty/
│       └── PhotoshopAPI/                 ← git submodule
├── Config/
│   └── DefaultPSD2UMG.ini
├── Resources/
│   └── Icon128.png
└── TestData/
    ├── Test_SimpleHUD.psd
    ├── Test_ComplexMenu.psd
    ├── Test_MobileUI.psd
    ├── Test_Typography.psd
    └── Test_Effects.psd
```

---

## Build.cs reference

```csharp
using UnrealBuildTool;
using System.IO;

public class PSD2UMG : ModuleRules
{
    public PSD2UMG(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "UMG",
            "UMGEditor",
            "UnrealEd",
            "AssetTools",
            "ContentBrowser",
            "EditorScriptingUtilities",
            "Blutility",
            "InputCore",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "PropertyEditor",
            "EditorStyle",
            "ToolMenus",
            "ImageWrapper",
            "RenderCore",
        });

        // PhotoshopAPI integration
        string ThirdPartyPath = Path.Combine(ModuleDirectory, "..", "ThirdParty", "PhotoshopAPI");
        PrivateIncludePaths.Add(Path.Combine(ThirdPartyPath, "include"));
        
        // Link pre-built static library (build PhotoshopAPI externally with CMake first)
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicAdditionalLibraries.Add(
                Path.Combine(ThirdPartyPath, "build", "Release", "PhotoshopAPI.lib")
            );
        }
        
        // Optional: CommonUI support
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("CommonUI");
        }
    }
}
```

---

## Working order for Claude Code

**Start with Phase 0 and Phase 1 — they are the foundation.**

Phase 0 is pure mechanical porting. Phase 1 is the architectural core. If these two are done cleanly, everything else layers on top naturally.

**Each phase = one PR with a working compilation at the end.**

Do not try to do everything at once. After each phase, verify the acceptance criteria before moving on. The pipeline architecture means each stage can be tested independently — use that.

**When stuck on PhotoshopAPI integration:**
- Check their docs: https://photoshopapi.readthedocs.io/
- Check their examples: `PhotoshopExamples/` directory in the repo
- Text layer support is tracked in issue #126 — may need workarounds
- If CMake integration with UE is painful, pre-build the static lib and link it

**When stuck on UE Widget Blueprint creation:**
- Key classes: `UWidgetBlueprint`, `UWidgetTree`, `UPanelWidget`, `UCanvasPanel`, `UCanvasPanelSlot`
- `WidgetTree->ConstructWidget<UImage>(UImage::StaticClass())` to create widgets
- `CanvasPanel->AddChild(Widget)` then `Cast<UCanvasPanelSlot>(Widget->Slot)` to set position
- Look at how `UWidgetBlueprintFactory::FactoryCreateNew()` works in engine source

---

## Key risks and mitigations

| Risk | Mitigation |
|------|-----------|
| PhotoshopAPI doesn't compile with UE's build system | Pre-build as static lib with standalone CMake, link via .Build.cs |
| PhotoshopAPI text layer support incomplete | Parse raw PSD TySh descriptors manually as fallback |
| Widget Blueprint creation API is underdocumented | Study engine source: `WidgetBlueprintFactory.cpp`, `WidgetBlueprintEditor.cpp` |
| Complex PSD structures crash parser | Add robust error handling, skip problematic layers with warnings |
| DPI/coordinate mismatches | Create a test PSD with known pixel positions, verify roundtrip |
| Reimport destroys manual changes | Use layer name as stable key, only update visual properties |

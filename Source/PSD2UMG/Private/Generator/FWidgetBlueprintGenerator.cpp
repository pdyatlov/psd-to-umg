// Copyright 2018-2021 - John snow wind
#include "Generator/FWidgetBlueprintGenerator.h"

#include "Generator/FAnchorCalculator.h"
#include "Mapper/FLayerMappingRegistry.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGLog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/UObjectGlobals.h"
#include "WidgetBlueprintFactory.h"
#include "Blueprint/UserWidget.h"
#include "EditorAssetLibrary.h"
#include "Engine/Blueprint.h"
#include "WidgetBlueprint.h"

// ---------------------------------------------------------------------------
// Internal helper: recursively populate a canvas panel from a layer array
// ---------------------------------------------------------------------------
static void PopulateCanvas(
    FLayerMappingRegistry& Registry,
    UWidgetTree* Tree,
    UCanvasPanel* Parent,
    const TArray<FPsdLayer>& Layers,
    const FPsdDocument& Doc,
    const FIntPoint& CanvasSize)
{
    const int32 TotalLayers = Layers.Num();
    for (int32 i = 0; i < TotalLayers; ++i)
    {
        const FPsdLayer& Layer = Layers[i];

        // Skip invisible layers (D-08, D-14)
        if (!Layer.bVisible)
        {
            UE_LOG(LogPSD2UMG, Log, TEXT("Skipping invisible layer: %s"), *Layer.Name);
            continue;
        }

        // Skip zero-size non-groups (D-14)
        if (Layer.Bounds.IsEmpty() && Layer.Type != EPsdLayerType::Group)
        {
            UE_LOG(LogPSD2UMG, Warning, TEXT("Skipping zero-size layer: %s"), *Layer.Name);
            continue;
        }

        UWidget* Widget = Registry.MapLayer(Layer, Doc, Tree);
        if (!Widget)
        {
            UE_LOG(LogPSD2UMG, Warning, TEXT("No mapper for layer: %s (type=%d)"), *Layer.Name, static_cast<int32>(Layer.Type));
            continue; // D-08: skip + warn
        }

        // Add to canvas and configure slot
        UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(Widget);
        if (Slot)
        {
            FAnchorData Data;
            Data.Alignment = FVector2D(0.f, 0.f);

            FAnchorResult AnchorResult;
            if (Layer.Type == EPsdLayerType::Group)
            {
                // Groups are transparent containers: fill the parent canvas so children
                // stay in the same coordinate system as the root. PhotoshopAPI does not
                // populate bounds on group layers, so we must not use them here.
                AnchorResult.Anchors = FAnchors(0.f, 0.f, 1.f, 1.f);
                AnchorResult.bStretchH = true;
                AnchorResult.bStretchV = true;
                AnchorResult.CleanName = Layer.Name;
            }
            else
            {
                AnchorResult = FAnchorCalculator::Calculate(Layer.Name, Layer.Bounds, CanvasSize);
            }

            Data.Anchors = AnchorResult.Anchors;

            const float AnchorPixelX = AnchorResult.Anchors.Minimum.X * static_cast<float>(CanvasSize.X);
            const float AnchorPixelY = AnchorResult.Anchors.Minimum.Y * static_cast<float>(CanvasSize.Y);

            if (Layer.Type == EPsdLayerType::Group)
            {
                // Group fills parent: zero margins on all sides
                Data.Offsets = FMargin(0.f, 0.f, 0.f, 0.f);
            }
            else if (AnchorResult.bStretchH && AnchorResult.bStretchV)
            {
                // Full stretch: offsets are margins from all 4 edges
                Data.Offsets = FMargin(
                    static_cast<float>(Layer.Bounds.Min.X),
                    static_cast<float>(Layer.Bounds.Min.Y),
                    static_cast<float>(CanvasSize.X - Layer.Bounds.Max.X),
                    static_cast<float>(CanvasSize.Y - Layer.Bounds.Max.Y));
            }
            else if (AnchorResult.bStretchH)
            {
                // Horizontal stretch: Left/Right margins, Top offset from anchor Y, Bottom = height
                Data.Offsets = FMargin(
                    static_cast<float>(Layer.Bounds.Min.X),
                    static_cast<float>(Layer.Bounds.Min.Y) - AnchorPixelY,
                    static_cast<float>(CanvasSize.X - Layer.Bounds.Max.X),
                    static_cast<float>(Layer.Bounds.Height()));
            }
            else if (AnchorResult.bStretchV)
            {
                // Vertical stretch: Top/Bottom margins, Left offset from anchor X, Right = width
                Data.Offsets = FMargin(
                    static_cast<float>(Layer.Bounds.Min.X) - AnchorPixelX,
                    static_cast<float>(Layer.Bounds.Min.Y),
                    static_cast<float>(Layer.Bounds.Width()),
                    static_cast<float>(CanvasSize.Y - Layer.Bounds.Max.Y));
            }
            else
            {
                // Point anchor: offset from anchor point, size = width/height
                Data.Offsets = FMargin(
                    static_cast<float>(Layer.Bounds.Min.X) - AnchorPixelX,
                    static_cast<float>(Layer.Bounds.Min.Y) - AnchorPixelY,
                    static_cast<float>(Layer.Bounds.Width()),
                    static_cast<float>(Layer.Bounds.Height()));
            }

            Slot->SetLayout(Data);
            // ZOrder: PSD index 0 = topmost. UMG higher = on top. Invert.
            Slot->SetZOrder(TotalLayers - 1 - i);

            UE_LOG(LogPSD2UMG, Log,
                TEXT("Layer '%s': bounds=(%d,%d)-(%d,%d) canvas=%dx%d anchors=(%.2f,%.2f,%.2f,%.2f) stretchH=%d stretchV=%d offsets=(L%.1f T%.1f R%.1f B%.1f)"),
                *Layer.Name,
                Layer.Bounds.Min.X, Layer.Bounds.Min.Y, Layer.Bounds.Max.X, Layer.Bounds.Max.Y,
                CanvasSize.X, CanvasSize.Y,
                AnchorResult.Anchors.Minimum.X, AnchorResult.Anchors.Minimum.Y,
                AnchorResult.Anchors.Maximum.X, AnchorResult.Anchors.Maximum.Y,
                AnchorResult.bStretchH ? 1 : 0, AnchorResult.bStretchV ? 1 : 0,
                Data.Offsets.Left, Data.Offsets.Top, Data.Offsets.Right, Data.Offsets.Bottom);
        }

        // Recurse into group children
        if (Layer.Type == EPsdLayerType::Group && !Layer.Children.IsEmpty())
        {
            UCanvasPanel* ChildCanvas = Cast<UCanvasPanel>(Widget);
            if (ChildCanvas)
            {
                PopulateCanvas(Registry, Tree, ChildCanvas, Layer.Children, Doc, CanvasSize);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// FWidgetBlueprintGenerator::Generate
// ---------------------------------------------------------------------------
UWidgetBlueprint* FWidgetBlueprintGenerator::Generate(
    const FPsdDocument& Doc,
    const FString& WbpPackagePath,
    const FString& WbpAssetName)
{
    // Step 1: Create WBP package
    const FString FullPath = FString::Printf(TEXT("%s/%s"), *WbpPackagePath, *WbpAssetName);
    UPackage* WbpPackage = CreatePackage(*FullPath);
    if (!WbpPackage)
    {
        UE_LOG(LogPSD2UMG, Error, TEXT("FWidgetBlueprintGenerator: failed to create package for %s"), *FullPath);
        return nullptr;
    }
    WbpPackage->FullyLoad();

    // Step 2: Create WBP via factory (canonical editor path — same as "New Widget Blueprint" in Content Browser)
    UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>(GetTransientPackage());
    Factory->AddToRoot();
    Factory->ParentClass = UUserWidget::StaticClass();

    UWidgetBlueprint* WBP = CastChecked<UWidgetBlueprint>(
        Factory->FactoryCreateNew(
            UWidgetBlueprint::StaticClass(),
            WbpPackage,
            FName(*WbpAssetName),
            RF_Public | RF_Standalone | RF_Transactional,
            nullptr,
            GWarn));

    Factory->RemoveFromRoot();

    if (!WBP)
    {
        UE_LOG(LogPSD2UMG, Error, TEXT("FWidgetBlueprintGenerator: FactoryCreateNew returned null for %s"), *WbpAssetName);
        return nullptr;
    }

    // Step 3: Set up root canvas
    UCanvasPanel* RootCanvas = WBP->WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("Root_Canvas"));
    WBP->WidgetTree->RootWidget = RootCanvas;

    // Step 4: Populate widget tree recursively
    FLayerMappingRegistry Registry;
    PopulateCanvas(Registry, WBP->WidgetTree, RootCanvas, Doc.RootLayers, Doc, Doc.CanvasSize);

    // Step 5: Compile AFTER full tree population (critical — compiling before population leaves empty BP)
    FKismetEditorUtilities::CompileBlueprint(WBP);

    // Step 6: Mark dirty and save
    WbpPackage->MarkPackageDirty();
    UEditorAssetLibrary::SaveLoadedAsset(WBP, false);

    UE_LOG(LogPSD2UMG, Log, TEXT("FWidgetBlueprintGenerator: created and saved WBP: %s"), *FullPath);
    return WBP;
}

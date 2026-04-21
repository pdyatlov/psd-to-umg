// Copyright 2018-2021 - John snow wind
// Phase 14 / SHAPE-01 -- drawn vector shape mapper (solid-color fill via vscg).
// Produces a zero-texture UImage whose tint will be overwritten by the
// generator's FX-03 block (FWidgetBlueprintGenerator.cpp lines 326-340) using
// Effects.ColorOverlayColor. ScanShapeFillColor (Plan 14-02) set
// bHasColorOverlay=true and populated ColorOverlayColor on the FPsdLayer
// after confirming the vscg descriptor's Type enum == "solidColorLayer".
//
// Semantically distinct from FSolidFillLayerMapper (D-03): SolidFill =
// AdjustmentLayer with SoCo tagged block (a fill layer). Shape = ShapeLayer
// (Rectangle/Ellipse/Pen tool) with vscg tagged block carrying
// Type==solidColorLayer (a drawn vector shape). Future stroke rendering
// (vstk -> UMG border/outline) will attach here, not in FSolidFillLayerMapper.

#include "Mapper/AllMappers.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGLog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"

int32 FShapeLayerMapper::GetPriority() const { return 100; }

bool FShapeLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::Shape;
}

UWidget* FShapeLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree)
{
    // Zero-texture UImage -- the FX-03 block in FWidgetBlueprintGenerator::PopulateChildren
    // (lines 326-340) will apply Effects.ColorOverlayColor as Brush.TintColor. No texture
    // asset is created; the UImage renders as a solid rectangle of the tint colour.
    UImage* Img = Tree->ConstructWidget<UImage>(
        UImage::StaticClass(),
        FName(*Layer.ParsedTags.CleanName));

    FSlateBrush Brush;
    Brush.DrawAs = ESlateBrushDrawType::Image;
    Brush.ImageSize = FVector2D(
        static_cast<float>(Layer.Bounds.Width()),
        static_cast<float>(Layer.Bounds.Height()));
    // TintColor is intentionally left at the default (white). FX-03 in the
    // generator overwrites it with Effects.ColorOverlayColor once bHasColorOverlay
    // is seen on this UImage.
    Img->SetBrush(Brush);

    if (!Layer.Effects.bHasColorOverlay)
    {
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("FShapeLayerMapper: layer '%s' has Type=Shape but bHasColorOverlay=false; "
                 "ScanShapeFillColor may have failed to parse the vscg descriptor. UImage will render as white."),
            *Layer.Name);
    }

    return Img;
}

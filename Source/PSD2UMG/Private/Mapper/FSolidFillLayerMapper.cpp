// Copyright 2018-2021 - John snow wind
// Phase 13 / GRAD-01 -- solid color fill layer mapper.
// Produces a zero-texture UImage whose tint will be overwritten by the
// generator's FX-03 block (FWidgetBlueprintGenerator.cpp lines 326-340) using
// Effects.ColorOverlayColor. ScanSolidFillColor (Plan 13-02) set
// bHasColorOverlay=true and populated ColorOverlayColor on the FPsdLayer.

#include "Mapper/AllMappers.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGLog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"

int32 FSolidFillLayerMapper::GetPriority() const { return 100; }

bool FSolidFillLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::SolidFill;
}

UWidget* FSolidFillLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree)
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
            TEXT("FSolidFillLayerMapper: layer '%s' has Type=SolidFill but bHasColorOverlay=false; "
                 "ScanSolidFillColor may have failed to parse the SoCo descriptor. UImage will render as white."),
            *Layer.Name);
    }

    return Img;
}

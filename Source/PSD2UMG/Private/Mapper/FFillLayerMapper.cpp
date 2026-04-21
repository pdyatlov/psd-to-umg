// Copyright 2018-2021 - John snow wind
// Phase 13 / GRAD-01, GRAD-02 -- gradient fill layer mapper.
// Mirrors FImageLayerMapper exactly; the only difference is CanMap dispatches
// on the parser-set EPsdLayerType::Gradient value (Plan 13-02 ConvertLayerRecursive
// ShapeLayer branch) rather than on ParsedTags.Type.

#include "Mapper/AllMappers.h"
#include "Generator/FTextureImporter.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGLog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Misc/Paths.h"

int32 FFillLayerMapper::GetPriority() const { return 100; }

bool FFillLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::Gradient;
}

UWidget* FFillLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree)
{
    const FString PsdName = FPaths::GetBaseFilename(Doc.SourcePath);
    UTexture2D* Tex = FTextureImporter::ImportLayer(Layer, FTextureImporter::BuildTexturePath(PsdName));
    if (!Tex)
    {
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("FFillLayerMapper: Texture import returned nullptr for gradient layer '%s' — skipping"),
            *Layer.Name);
        return nullptr;
    }

    UImage* Img = Tree->ConstructWidget<UImage>(UImage::StaticClass(), FName(*Layer.ParsedTags.CleanName));
    // bMatchSize=true so Brush.ImageSize is set to the texture dimensions
    // (otherwise the UImage renders as a zero-sized border).
    Img->SetBrushFromTexture(Tex, /*bMatchSize=*/true);
    FSlateBrush Brush = Img->GetBrush();
    Brush.DrawAs = ESlateBrushDrawType::Image;
    Img->SetBrush(Brush);
    return Img;
}

// Copyright 2018-2021 - John snow wind
// F9SliceImageLayerMapper — maps image layers carrying the @9s tag (with or
// without :L,T,R,B values) to UImage widgets using ESlateBrushDrawType::Box
// (9-slice / border draw mode). Margin values come from FParsedLayerTags.NineSlice.

#include "Mapper/AllMappers.h"
#include "Generator/FTextureImporter.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGLog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Misc/Paths.h"

int32 F9SliceImageLayerMapper::GetPriority() const { return 150; }

bool F9SliceImageLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    // @9s applies to an image layer carrying pixel data. Groups that carry the
    // tag (e.g. via explicit @image + @9s misuse, or a container wrapping a
    // 9-slice image) must fall through to FGroupLayerMapper instead of being
    // silently skipped when the group has no pixels of its own.
    return Layer.ParsedTags.NineSlice.IsSet()
        && Layer.ParsedTags.Type == EPsdTagType::Image;
}

UWidget* F9SliceImageLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree)
{
    const FString PsdName = FPaths::GetBaseFilename(Doc.SourcePath);
    UTexture2D* Tex = FTextureImporter::ImportLayer(Layer, FTextureImporter::BuildTexturePath(PsdName));
    if (!Tex)
    {
        UE_LOG(LogPSD2UMG, Warning, TEXT("F9SliceImageLayerMapper: Texture import returned nullptr for layer '%s' — skipping"), *Layer.ParsedTags.CleanName);
        return nullptr;
    }

    UImage* Img = Tree->ConstructWidget<UImage>(UImage::StaticClass(), FName(*Layer.ParsedTags.CleanName));
    Img->SetBrushFromTexture(Tex, /*bMatchSize=*/true);

    FSlateBrush Brush = Img->GetBrush();
    Brush.DrawAs = ESlateBrushDrawType::Box;

    // Margin values from @9s tag. @9s shorthand defaults to {16,16,16,16, bExplicit=false}
    // matching the historical default; @9s:L,T,R,B sets explicit per-edge pixel values.
    // Normalise pixel margins to 0..1 via texture dimensions.
    const FPsdNineSliceMargin& M = Layer.ParsedTags.NineSlice.GetValue();
    const int32 TexW = Tex->GetSizeX();
    const int32 TexH = Tex->GetSizeY();
    const float W = (TexW > 0) ? static_cast<float>(TexW) : 1.f;
    const float H = (TexH > 0) ? static_cast<float>(TexH) : 1.f;

    Brush.Margin = FMargin(M.L / W, M.T / H, M.R / W, M.B / H);

    Img->SetBrush(Brush);
    return Img;
}

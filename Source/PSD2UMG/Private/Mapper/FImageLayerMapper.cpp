// Copyright 2018-2021 - John snow wind

#include "Mapper/AllMappers.h"
#include "Generator/FTextureImporter.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGLog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Misc/Paths.h"

int32 FImageLayerMapper::GetPriority() const { return 100; }

bool FImageLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.ParsedTags.Type == EPsdTagType::Image;
}

UWidget* FImageLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree)
{
    const FString PsdName = FPaths::GetBaseFilename(Doc.SourcePath);
    UTexture2D* Tex = FTextureImporter::ImportLayer(Layer, FTextureImporter::BuildTexturePath(PsdName));
    if (!Tex)
    {
        UE_LOG(LogPSD2UMG, Warning, TEXT("FImageLayerMapper: Texture import returned nullptr for layer '%s' — skipping"), *Layer.Name);
        return nullptr;
    }

    UImage* Img = Tree->ConstructWidget<UImage>(UImage::StaticClass(), FName(*Layer.ParsedTags.CleanName));
    // bMatchSize=true so Brush.ImageSize is set to the texture dimensions.
    // Without this, ImageSize stays (0,0) and the UImage renders as a thin
    // border instead of filling the slot.
    Img->SetBrushFromTexture(Tex, /*bMatchSize=*/true);
    // Ensure DrawAs is Image (not Border/Box) regardless of archetype defaults.
    FSlateBrush Brush = Img->GetBrush();
    Brush.DrawAs = ESlateBrushDrawType::Image;
    Img->SetBrush(Brush);
    return Img;
}

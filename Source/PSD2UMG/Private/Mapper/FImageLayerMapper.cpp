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
    return Layer.Type == EPsdLayerType::Image;
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

    UImage* Img = Tree->ConstructWidget<UImage>(UImage::StaticClass(), FName(*Layer.Name));
    Img->SetBrushFromTexture(Tex);
    return Img;
}

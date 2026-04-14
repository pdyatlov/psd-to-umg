// Copyright 2018-2021 - John snow wind

#include "Mapper/AllMappers.h"
#include "Generator/FSmartObjectImporter.h"
#include "Generator/FTextureImporter.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGLog.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Misc/Paths.h"
#include "WidgetBlueprint.h"

int32 FSmartObjectLayerMapper::GetPriority() const { return 150; }

bool FSmartObjectLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    // Native PSD smart-object layers always dispatch here; @smartobject lets a
    // designer force the smart-object path on a non-smartobject group too.
    return Layer.Type == EPsdLayerType::SmartObject
        || Layer.ParsedTags.Type == EPsdTagType::SmartObject;
}

UWidget* FSmartObjectLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree)
{
    // --- Attempt recursive import as a child Widget Blueprint ---
    const FString& ParentPackagePath = FSmartObjectImporter::GetCurrentPackagePath();

    UWidgetBlueprint* ChildWBP = FSmartObjectImporter::ImportAsChildWBP(
        Layer, ParentPackagePath, 0, Layer.ParsedTags.SmartObjectTypeName);

    if (ChildWBP && ChildWBP->GeneratedClass)
    {
        // Create a UUserWidget referencing the child WBP's generated class
        TSubclassOf<UUserWidget> UserWidgetClass = *ChildWBP->GeneratedClass;
        UUserWidget* UserWidget = Tree->ConstructWidget<UUserWidget>(
            UserWidgetClass,
            FName(*Layer.ParsedTags.CleanName));

        if (UserWidget)
        {
            UE_LOG(LogPSD2UMG, Log,
                TEXT("FSmartObjectLayerMapper: Created UUserWidget for Smart Object '%s' referencing '%s'"),
                *Layer.Name, *ChildWBP->GetName());
            return UserWidget;
        }

        UE_LOG(LogPSD2UMG, Warning,
            TEXT("FSmartObjectLayerMapper: ConstructWidget returned null for Smart Object '%s'; falling back to rasterized image."),
            *Layer.Name);
    }

    // --- Fallback: rasterize using composited preview pixels (D-07 per plan) ---
    UE_LOG(LogPSD2UMG, Warning,
        TEXT("FSmartObjectLayerMapper: Falling back to rasterized UImage for Smart Object '%s'."),
        *Layer.Name);

    if (Layer.RGBAPixels.Num() == 0)
    {
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("FSmartObjectLayerMapper: No pixel data available for Smart Object '%s'; skipping."),
            *Layer.Name);
        return nullptr;
    }

    const FString PsdName = FPaths::GetBaseFilename(Doc.SourcePath);
    UTexture2D* Tex = FTextureImporter::ImportLayer(Layer, FTextureImporter::BuildTexturePath(PsdName));
    if (!Tex)
    {
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("FSmartObjectLayerMapper: Texture import returned nullptr for Smart Object '%s'; skipping."),
            *Layer.Name);
        return nullptr;
    }

    UImage* Img = Tree->ConstructWidget<UImage>(UImage::StaticClass(), FName(*Layer.ParsedTags.CleanName));
    Img->SetBrushFromTexture(Tex, /*bMatchSize=*/true);
    FSlateBrush Brush = Img->GetBrush();
    Brush.DrawAs = ESlateBrushDrawType::Image;
    Img->SetBrush(Brush);
    return Img;
}

// Copyright 2018-2021 - John snow wind

#include "Mapper/AllMappers.h"
#include "Generator/FTextureImporter.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGLog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Misc/Paths.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

int32 FProgressLayerMapper::GetPriority() const { return 200; }

bool FProgressLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::Group && Layer.Name.StartsWith(TEXT("Progress_"));
}

UWidget* FProgressLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree)
{
    UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), FName(*Layer.Name));

    FProgressBarStyle Style = FProgressBarStyle::GetDefault();

    const FString PsdName = FPaths::GetBaseFilename(Doc.SourcePath);
    const FString BaseTexturePath = FTextureImporter::BuildTexturePath(PsdName);

    for (const FPsdLayer& Child : Layer.Children)
    {
        if (Child.Type != EPsdLayerType::Image)
        {
            continue;
        }

        UTexture2D* Tex = FTextureImporter::ImportLayer(Child, BaseTexturePath);
        if (!Tex)
        {
            continue;
        }

        FSlateBrush Brush;
        Brush.SetResourceObject(Tex);
        Brush.ImageSize = FVector2D(static_cast<float>(Child.PixelWidth), static_cast<float>(Child.PixelHeight));

        const FString ChildNameLower = Child.Name.ToLower();
        if (ChildNameLower.EndsWith(TEXT("_fill")) || ChildNameLower.EndsWith(TEXT("_foreground")))
        {
            Style.FillImage = Brush;
        }
        else
        {
            // _BG, _Background, or first child fallback
            Style.BackgroundImage = Brush;
        }
    }

    Bar->SetWidgetStyle(Style);
    return Bar;
}

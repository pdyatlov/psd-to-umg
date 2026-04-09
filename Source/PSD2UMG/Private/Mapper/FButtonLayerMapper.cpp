// Copyright 2018-2021 - John snow wind

#include "Mapper/AllMappers.h"
#include "Generator/FTextureImporter.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGLog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Misc/Paths.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

int32 FButtonLayerMapper::GetPriority() const { return 200; }

bool FButtonLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::Group && Layer.Name.StartsWith(TEXT("Button_"));
}

UWidget* FButtonLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree)
{
    UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass(), FName(*Layer.Name));

    FButtonStyle Style = FButtonStyle::GetDefault();

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
        if (ChildNameLower.EndsWith(TEXT("_hovered")) || ChildNameLower.EndsWith(TEXT("_hover")))
        {
            Style.SetHovered(Brush);
        }
        else if (ChildNameLower.EndsWith(TEXT("_pressed")) || ChildNameLower.EndsWith(TEXT("_press")))
        {
            Style.SetPressed(Brush);
        }
        else if (ChildNameLower.EndsWith(TEXT("_disabled")))
        {
            Style.SetDisabled(Brush);
        }
        else
        {
            // _Normal, _Bg, or first image child fallback
            Style.SetNormal(Brush);
        }
    }

    Btn->SetStyle(Style);
    return Btn;
}

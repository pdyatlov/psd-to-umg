// Copyright 2018-2021 - John snow wind

#include "Mapper/IPsdLayerMapper.h"
#include "Generator/FTextureImporter.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGLog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Misc/Paths.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

class FButtonLayerMapper : public IPsdLayerMapper
{
public:
    // Higher priority than Group (50) so prefix wins
    int32 GetPriority() const override { return 200; }

    bool CanMap(const FPsdLayer& Layer) const override
    {
        return Layer.Type == EPsdLayerType::Group && Layer.Name.StartsWith(TEXT("Button_"));
    }

    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override
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
};

// Copyright 2018-2021 - John snow wind

#include "Mapper/AllMappers.h"
#include "Generator/FTextureImporter.h"
#include "Parser/FLayerTagParser.h"
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
    return Layer.ParsedTags.Type == EPsdTagType::Button;
}

namespace
{
    static bool ApplyChildBrush(
        const FPsdLayer* Child,
        const FString& BaseTexturePath,
        TFunctionRef<void(const FSlateBrush&)> Apply)
    {
        if (!Child)
        {
            return false;
        }
        UTexture2D* Tex = FTextureImporter::ImportLayer(*Child, BaseTexturePath);
        if (!Tex)
        {
            return false;
        }
        FSlateBrush Brush;
        Brush.SetResourceObject(Tex);
        Brush.ImageSize = FVector2D(static_cast<float>(Child->PixelWidth), static_cast<float>(Child->PixelHeight));
        Apply(Brush);
        return true;
    }
}

UWidget* FButtonLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree)
{
    UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass(), FName(*Layer.ParsedTags.CleanName));

    FButtonStyle Style = FButtonStyle::GetDefault();

    const FString PsdName = FPaths::GetBaseFilename(Doc.SourcePath);
    const FString BaseTexturePath = FTextureImporter::BuildTexturePath(PsdName);

    // D-12/D-13: explicit @state child match first; Normal falls back to first untagged Image.
    ApplyChildBrush(FLayerTagParser::FindChildByState(Layer.Children, EPsdStateTag::Normal),   BaseTexturePath, [&](const FSlateBrush& B){ Style.SetNormal(B); });
    ApplyChildBrush(FLayerTagParser::FindChildByState(Layer.Children, EPsdStateTag::Hover),    BaseTexturePath, [&](const FSlateBrush& B){ Style.SetHovered(B); });
    ApplyChildBrush(FLayerTagParser::FindChildByState(Layer.Children, EPsdStateTag::Pressed),  BaseTexturePath, [&](const FSlateBrush& B){ Style.SetPressed(B); });
    ApplyChildBrush(FLayerTagParser::FindChildByState(Layer.Children, EPsdStateTag::Disabled), BaseTexturePath, [&](const FSlateBrush& B){ Style.SetDisabled(B); });

    Btn->SetStyle(Style);
    return Btn;
}

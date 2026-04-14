// Copyright 2018-2021 - John snow wind

#include "Mapper/AllMappers.h"
#include "Generator/FTextureImporter.h"
#include "Parser/FLayerTagParser.h"
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
    return Layer.ParsedTags.Type == EPsdTagType::Progress;
}

namespace
{
    static bool BrushFromChild(
        const FPsdLayer* Child,
        const FString& BaseTexturePath,
        FSlateBrush& OutBrush)
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
        OutBrush.SetResourceObject(Tex);
        OutBrush.ImageSize = FVector2D(static_cast<float>(Child->PixelWidth), static_cast<float>(Child->PixelHeight));
        return true;
    }
}

UWidget* FProgressLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree)
{
    UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), FName(*Layer.ParsedTags.CleanName));

    FProgressBarStyle Style = FProgressBarStyle::GetDefault();

    const FString PsdName = FPaths::GetBaseFilename(Doc.SourcePath);
    const FString BaseTexturePath = FTextureImporter::BuildTexturePath(PsdName);

    // D-12: child with @state:fill -> FillImage; @state:bg (or first untagged Image) -> BackgroundImage.
    FSlateBrush FillBrush;
    if (BrushFromChild(FLayerTagParser::FindChildByState(Layer.Children, EPsdStateTag::Fill), BaseTexturePath, FillBrush))
    {
        Style.FillImage = FillBrush;
    }

    FSlateBrush BgBrush;
    const FPsdLayer* BgChild = FLayerTagParser::FindChildByState(Layer.Children, EPsdStateTag::Bg);
    if (!BgChild)
    {
        // Fall back to the first untagged Image child as background.
        BgChild = FLayerTagParser::FindChildByState(Layer.Children, EPsdStateTag::Normal);
    }
    if (BrushFromChild(BgChild, BaseTexturePath, BgBrush))
    {
        Style.BackgroundImage = BgBrush;
    }

    Bar->SetWidgetStyle(Style);
    return Bar;
}

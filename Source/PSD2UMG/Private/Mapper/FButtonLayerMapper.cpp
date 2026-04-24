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
        // State children may be Group wrappers (e.g. component sets where each variant
        // is a named group and the actual pixels live in its first Image child).
        const FPsdLayer* ImageLayer = Child;
        if (Child->Type == EPsdLayerType::Group)
        {
            ImageLayer = nullptr;
            // @background-tagged image takes priority; fall back to first Image.
            for (const FPsdLayer& GChild : Child->Children)
            {
                if (GChild.Type == EPsdLayerType::Image && GChild.ParsedTags.bIsBackground)
                {
                    ImageLayer = &GChild;
                    break;
                }
            }
            if (!ImageLayer)
            {
                for (const FPsdLayer& GChild : Child->Children)
                {
                    if (GChild.Type == EPsdLayerType::Image)
                    {
                        ImageLayer = &GChild;
                        break;
                    }
                }
            }
        }
        if (!ImageLayer)
        {
            return false;
        }
        UTexture2D* Tex = FTextureImporter::ImportLayer(*ImageLayer, BaseTexturePath);
        if (!Tex)
        {
            return false;
        }
        FSlateBrush Brush;
        Brush.SetResourceObject(Tex);
        Brush.ImageSize = FVector2D(static_cast<float>(ImageLayer->PixelWidth), static_cast<float>(ImageLayer->PixelHeight));
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
    // D-03 (Phase 17.1): track filled slots so we can emit ONE aggregate warning when a
    // @button group is missing state children — designers get actionable feedback without
    // the import being aborted; unfilled slots retain FButtonStyle::GetDefault().
    TArray<FString> MissingSlots;

    const bool bNormal = ApplyChildBrush(FLayerTagParser::FindChildByState(Layer.Children, EPsdStateTag::Normal),   BaseTexturePath, [&](const FSlateBrush& B){ Style.SetNormal(B); });
    if (!bNormal) { MissingSlots.Add(TEXT("Normal")); }

    const bool bHover = ApplyChildBrush(FLayerTagParser::FindChildByState(Layer.Children, EPsdStateTag::Hover),    BaseTexturePath, [&](const FSlateBrush& B){ Style.SetHovered(B); });
    if (!bHover) { MissingSlots.Add(TEXT("Hovered")); }

    const bool bPressed = ApplyChildBrush(FLayerTagParser::FindChildByState(Layer.Children, EPsdStateTag::Pressed),  BaseTexturePath, [&](const FSlateBrush& B){ Style.SetPressed(B); });
    if (!bPressed) { MissingSlots.Add(TEXT("Pressed")); }

    const bool bDisabled = ApplyChildBrush(FLayerTagParser::FindChildByState(Layer.Children, EPsdStateTag::Disabled), BaseTexturePath, [&](const FSlateBrush& B){ Style.SetDisabled(B); });
    if (!bDisabled) { MissingSlots.Add(TEXT("Disabled")); }

    if (MissingSlots.Num() > 0)
    {
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("FButtonLayerMapper: Layer '%s' — %d/4 button state slots populated; missing: %s"),
            *Layer.ParsedTags.CleanName,
            4 - MissingSlots.Num(),
            *FString::Join(MissingSlots, TEXT(", ")));
    }

    Btn->SetStyle(Style);
    return Btn;
}

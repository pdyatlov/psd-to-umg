// Copyright 2018-2021 - John snow wind

#include "Mapper/IPsdLayerMapper.h"
#include "Parser/PsdTypes.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Math/UnrealMathUtility.h"
#include "Styling/SlateColor.h"

class FTextLayerMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override { return 100; }

    bool CanMap(const FPsdLayer& Layer) const override
    {
        return Layer.Type == EPsdLayerType::Text;
    }

    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override
    {
        UTextBlock* TextWidget = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*Layer.Name));

        // Set text content
        TextWidget->SetText(FText::FromString(Layer.Text.Content));

        // DPI conversion: Photoshop 72 DPI -> UMG 96 DPI (multiply by 0.75)
        const float UmgSize = Layer.Text.SizePx * 0.75f;
        FSlateFontInfo FontInfo = TextWidget->GetFont();
        FontInfo.Size = FMath::RoundToInt(UmgSize);
        TextWidget->SetFont(FontInfo);

        // Set color
        TextWidget->SetColorAndOpacity(FSlateColor(Layer.Text.Color));

        // Set justification
        TextWidget->SetJustification(Layer.Text.Alignment);

        return TextWidget;
    }
};

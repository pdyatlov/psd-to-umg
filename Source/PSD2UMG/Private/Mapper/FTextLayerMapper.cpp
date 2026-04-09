// Copyright 2018-2021 - John snow wind

#include "Mapper/AllMappers.h"
#include "Parser/PsdTypes.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Math/UnrealMathUtility.h"
#include "Styling/SlateColor.h"

int32 FTextLayerMapper::GetPriority() const { return 100; }

bool FTextLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::Text;
}

UWidget* FTextLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
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

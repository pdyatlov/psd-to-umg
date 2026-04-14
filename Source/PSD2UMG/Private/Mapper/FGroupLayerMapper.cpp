// Copyright 2018-2021 - John snow wind

#include "Mapper/AllMappers.h"
#include "Parser/PsdTypes.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"

int32 FGroupLayerMapper::GetPriority() const { return 50; }

bool FGroupLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.ParsedTags.Type == EPsdTagType::Canvas;
}

UWidget* FGroupLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    UCanvasPanel* Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), FName(*Layer.ParsedTags.CleanName));
    // Children are recursed by the generator, not the mapper
    return Canvas;
}

// Copyright 2018-2021 - John snow wind

#include "Mapper/IPsdLayerMapper.h"
#include "Parser/PsdTypes.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"

class FGroupLayerMapper : public IPsdLayerMapper
{
public:
    // Lower priority than prefix mappers (200) so named prefixes win
    int32 GetPriority() const override { return 50; }

    bool CanMap(const FPsdLayer& Layer) const override
    {
        return Layer.Type == EPsdLayerType::Group;
    }

    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override
    {
        UCanvasPanel* Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), FName(*Layer.Name));
        // Children are recursed by the generator, not the mapper
        return Canvas;
    }
};

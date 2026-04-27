// Copyright 2018-2021 - John snow wind

#include "Mapper/FLayerMappingRegistry.h"
#include "Mapper/AllMappers.h"
#include "Mapper/IPsdLayerMapper.h"
#include "PSD2UMGLog.h"
#include "Parser/PsdTypes.h"

FLayerMappingRegistry::FLayerMappingRegistry()
{
    RegisterDefaults();
}

void FLayerMappingRegistry::RegisterDefaults()
{
    // CommonUI button mapper (priority 210) — checked before FButtonLayerMapper when bUseCommonUI is true
    Mappers.Add(MakeUnique<FCommonUIButtonLayerMapper>());

    // Prefix mappers (priority 200) — must be checked before Group (priority 50)
    Mappers.Add(MakeUnique<FButtonLayerMapper>());
    Mappers.Add(MakeUnique<FProgressLayerMapper>());
    Mappers.Add(MakeUnique<FHBoxLayerMapper>());
    Mappers.Add(MakeUnique<FVBoxLayerMapper>());
    Mappers.Add(MakeUnique<FOverlayLayerMapper>());
    Mappers.Add(MakeUnique<FScrollBoxLayerMapper>());
    Mappers.Add(MakeUnique<FSliderLayerMapper>());
    Mappers.Add(MakeUnique<FCheckBoxLayerMapper>());
    Mappers.Add(MakeUnique<FInputLayerMapper>());
    Mappers.Add(MakeUnique<FListLayerMapper>());
    Mappers.Add(MakeUnique<FTileLayerMapper>());
    Mappers.Add(MakeUnique<FSwitcherLayerMapper>());

    // Suffix mappers (priority 150 — above type, below prefix)
    Mappers.Add(MakeUnique<F9SliceImageLayerMapper>());
    Mappers.Add(MakeUnique<FSmartObjectLayerMapper>());

    // Suffix mappers (priority 200 — same tier as prefix mappers)
    Mappers.Add(MakeUnique<FVariantsSuffixMapper>());

    // Type-based mappers — image (priority 100) + rich text (priority 110) + fill/shape (priority 101, Phase 20)
    Mappers.Add(MakeUnique<FImageLayerMapper>());                                        // priority 100 (default image)
    Mappers.Add(MakeUnique<FRichTextLayerMapper>()); // Phase 16 / RICH-01, RICH-02      // priority 110 (multi-run text)
    Mappers.Add(MakeUnique<FTextLayerMapper>());     // priority 100 (single-run text; CanMap narrowed to Spans.Num() <= 1)
    // Phase 20 / D-01: fill / shape mappers run at priority 101 (above FImageLayerMapper)
    // because Phase 16.1 D-02 maps EPsdLayerType::Gradient/SolidFill/Shape to EPsdTagType::Image,
    // making FImageLayerMapper::CanMap also return true for these layers. Priority delta
    // makes the specialized mapper win deterministically regardless of sort stability.
    Mappers.Add(MakeUnique<FFillLayerMapper>());        // Phase 13 / GRAD-01, GRAD-02 -- gradient fill (priority 101)
    Mappers.Add(MakeUnique<FSolidFillLayerMapper>());   // Phase 13 / GRAD-01 -- solid color fill (priority 101)
    Mappers.Add(MakeUnique<FShapeLayerMapper>());       // Phase 14 / SHAPE-01 -- drawn vector shape, solid-color fill (vscg) (priority 101)

    // Default group mapper (priority 50)
    Mappers.Add(MakeUnique<FGroupLayerMapper>());

    // Sort descending by priority so higher-priority mappers are checked first
    Mappers.Sort([](const TUniquePtr<IPsdLayerMapper>& A, const TUniquePtr<IPsdLayerMapper>& B)
    {
        return A->GetPriority() > B->GetPriority();
    });
}

UWidget* FLayerMappingRegistry::MapLayer(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree)
{
    for (const TUniquePtr<IPsdLayerMapper>& Mapper : Mappers)
    {
        if (Mapper->CanMap(Layer))
        {
            return Mapper->Map(Layer, Doc, Tree);
        }
    }
    UE_LOG(LogPSD2UMG, Warning, TEXT("FLayerMappingRegistry: No mapper found for layer '%s'"), *Layer.Name);
    return nullptr;
}

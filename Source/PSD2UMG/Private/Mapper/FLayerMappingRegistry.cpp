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

    // Type-based mappers (priority 100)
    Mappers.Add(MakeUnique<FImageLayerMapper>());
    Mappers.Add(MakeUnique<FTextLayerMapper>());
    Mappers.Add(MakeUnique<FFillLayerMapper>());        // Phase 13 / GRAD-01, GRAD-02 -- gradient fill
    Mappers.Add(MakeUnique<FSolidFillLayerMapper>());   // Phase 13 / GRAD-01 -- solid color fill
    Mappers.Add(MakeUnique<FShapeLayerMapper>());       // Phase 14 / SHAPE-01 -- drawn vector shape, solid-color fill (vscg)

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

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

    // Type-based mappers (priority 100)
    Mappers.Add(MakeUnique<FImageLayerMapper>());
    Mappers.Add(MakeUnique<FTextLayerMapper>());

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

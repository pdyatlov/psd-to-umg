// Copyright 2018-2021 - John snow wind

#include "Mapper/FLayerMappingRegistry.h"
#include "Mapper/IPsdLayerMapper.h"
#include "PSD2UMGLog.h"
#include "Parser/PsdTypes.h"

FLayerMappingRegistry::FLayerMappingRegistry()
{
    RegisterDefaults();
}

void FLayerMappingRegistry::RegisterDefaults()
{
    // Mappers are added in Plan 02. After all are added, sort by priority descending.
    Algo::SortBy(Mappers, [](const TUniquePtr<IPsdLayerMapper>& M) { return -M->GetPriority(); });
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

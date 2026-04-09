// Copyright 2018-2021 - John snow wind
#pragma once
#include "CoreMinimal.h"
#include "Mapper/IPsdLayerMapper.h"

struct FPsdLayer;
struct FPsdDocument;
class UWidgetTree;
class UWidget;

class PSD2UMG_API FLayerMappingRegistry
{
public:
    FLayerMappingRegistry();
    ~FLayerMappingRegistry() = default;

    FLayerMappingRegistry(const FLayerMappingRegistry&) = delete;
    FLayerMappingRegistry& operator=(const FLayerMappingRegistry&) = delete;
    FLayerMappingRegistry(FLayerMappingRegistry&&) = default;
    FLayerMappingRegistry& operator=(FLayerMappingRegistry&&) = default;

    UWidget* MapLayer(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree);
private:
    TArray<TUniquePtr<IPsdLayerMapper>> Mappers;
    void RegisterDefaults();
};

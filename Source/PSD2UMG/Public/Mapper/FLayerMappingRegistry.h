// Copyright 2018-2021 - John snow wind
#pragma once
#include "CoreMinimal.h"

class IPsdLayerMapper;
struct FPsdLayer;
struct FPsdDocument;
class UWidgetTree;
class UWidget;

class PSD2UMG_API FLayerMappingRegistry
{
public:
    FLayerMappingRegistry();
    UWidget* MapLayer(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree);
private:
    TArray<TUniquePtr<IPsdLayerMapper>> Mappers;
    void RegisterDefaults();
};

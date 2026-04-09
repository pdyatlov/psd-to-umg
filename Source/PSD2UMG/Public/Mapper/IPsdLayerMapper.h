// Copyright 2018-2021 - John snow wind
#pragma once
#include "CoreMinimal.h"

struct FPsdLayer;
struct FPsdDocument;
class UWidgetTree;
class UWidget;
class UPanelWidget;

class IPsdLayerMapper
{
public:
    virtual ~IPsdLayerMapper() = default;
    virtual int32 GetPriority() const = 0;
    virtual bool CanMap(const FPsdLayer& Layer) const = 0;
    virtual UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) = 0;
};

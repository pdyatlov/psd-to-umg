// Copyright 2018-2021 - John snow wind
#pragma once

#include "Mapper/IPsdLayerMapper.h"

// Defined in FCommonUIButtonLayerMapper.cpp
// Active only when UPSD2UMGSettings::Get()->bUseCommonUI == true.
// Priority 210 — checked before FButtonLayerMapper (200) when CommonUI is enabled.
class FCommonUIButtonLayerMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override;
    bool CanMap(const FPsdLayer& Layer) const override;
    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override;
    FString GetWidgetTypeName() const { return TEXT("CommonButtonBase"); }
};

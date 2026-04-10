// Copyright 2018-2021 - John snow wind
// Internal header: declares all mapper classes so FLayerMappingRegistry.cpp
// can instantiate them without depending on individual mapper headers.
// Each mapper class is defined in its corresponding .cpp file.
#pragma once

#include "Mapper/IPsdLayerMapper.h"

// --- Type-based mappers (priority 100) ---
// Defined in FImageLayerMapper.cpp
class FImageLayerMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override;
    bool CanMap(const FPsdLayer& Layer) const override;
    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override;
};

// Defined in FTextLayerMapper.cpp
class FTextLayerMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override;
    bool CanMap(const FPsdLayer& Layer) const override;
    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override;
};

// --- Fallback group mapper (priority 50) ---
// Defined in FGroupLayerMapper.cpp
class FGroupLayerMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override;
    bool CanMap(const FPsdLayer& Layer) const override;
    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override;
};

// --- Prefix mappers (priority 200) ---
// Defined in FButtonLayerMapper.cpp
class FButtonLayerMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override;
    bool CanMap(const FPsdLayer& Layer) const override;
    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override;
};

// Defined in FProgressLayerMapper.cpp
class FProgressLayerMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override;
    bool CanMap(const FPsdLayer& Layer) const override;
    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override;
};

// Defined in FSimplePrefixMappers.cpp
class FHBoxLayerMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override;
    bool CanMap(const FPsdLayer& Layer) const override;
    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override;
};

class FVBoxLayerMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override;
    bool CanMap(const FPsdLayer& Layer) const override;
    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override;
};

class FOverlayLayerMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override;
    bool CanMap(const FPsdLayer& Layer) const override;
    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override;
};

class FScrollBoxLayerMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override;
    bool CanMap(const FPsdLayer& Layer) const override;
    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override;
};

class FSliderLayerMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override;
    bool CanMap(const FPsdLayer& Layer) const override;
    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override;
};

class FCheckBoxLayerMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override;
    bool CanMap(const FPsdLayer& Layer) const override;
    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override;
};

class FInputLayerMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override;
    bool CanMap(const FPsdLayer& Layer) const override;
    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override;
};

class FListLayerMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override;
    bool CanMap(const FPsdLayer& Layer) const override;
    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override;
};

class FTileLayerMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override;
    bool CanMap(const FPsdLayer& Layer) const override;
    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override;
};

class FSwitcherLayerMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override;
    bool CanMap(const FPsdLayer& Layer) const override;
    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override;
};

// --- Suffix mappers (priority 150 / 200) ---
// Defined in F9SliceImageLayerMapper.cpp
class F9SliceImageLayerMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override;
    bool CanMap(const FPsdLayer& Layer) const override;
    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override;
};

// Defined in FVariantsSuffixMapper.cpp
class FVariantsSuffixMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override;
    bool CanMap(const FPsdLayer& Layer) const override;
    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override;
};

// Copyright 2018-2021 - John snow wind
// FVariantsSuffixMapper — maps Group layers with _variants suffix to
// UWidgetSwitcher. Children become slots in PSD layer order (index 0 = first child).
// Coexists with FSwitcherLayerMapper (Switcher_ prefix); both produce UWidgetSwitcher.

#include "Mapper/AllMappers.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGLog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/WidgetSwitcher.h"

int32 FVariantsSuffixMapper::GetPriority() const { return 200; }

bool FVariantsSuffixMapper::CanMap(const FPsdLayer& Layer) const
{
    // D-01 (Phase 17.1 / d85 fix): Allow Canvas because EPsdLayerType::Group is
    // silently mapped to EPsdTagType::Canvas by the D-02 default-type pass in
    // FLayerTagParser — it is the implicit group default, not an explicit @canvas
    // tag. Only a *different* explicit type tag (e.g. @button, @progress, @hbox)
    // should prevent FVariantsSuffixMapper from claiming the layer.
    return Layer.ParsedTags.bIsVariants
        && (Layer.ParsedTags.Type == EPsdTagType::None
            || Layer.ParsedTags.Type == EPsdTagType::Canvas);
}

UWidget* FVariantsSuffixMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    UE_LOG(LogPSD2UMG, Log, TEXT("FVariantsSuffixMapper: Creating UWidgetSwitcher for '%s' (%d children)"), *Layer.ParsedTags.CleanName, Layer.Children.Num());
    return Tree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), FName(*Layer.ParsedTags.CleanName));
}

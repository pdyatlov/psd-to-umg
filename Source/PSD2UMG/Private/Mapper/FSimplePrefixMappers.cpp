// Copyright 2018-2021 - John snow wind
// Implementations for all simple tag-to-widget mappers (Phase 9 retag).
// CanMap dispatches on Layer.ParsedTags.Type; widget FName from CleanName.
// Children are traversed recursively by the generator, not these mappers.

#include "Mapper/AllMappers.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGLog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/Overlay.h"
#include "Components/ScrollBox.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"
#include "Components/TileView.h"
#include "Components/WidgetSwitcher.h"
#include "UObject/UnrealType.h"
#include "Widgets/PsdListEntryPlaceholder.h"

// ---------------------------------------------------------------------------
// FHBoxLayerMapper  (@hbox)
// ---------------------------------------------------------------------------
int32 FHBoxLayerMapper::GetPriority() const { return 200; }
bool FHBoxLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.ParsedTags.Type == EPsdTagType::HBox;
}
UWidget* FHBoxLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    return Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName(*Layer.ParsedTags.CleanName));
}

// ---------------------------------------------------------------------------
// FVBoxLayerMapper  (@vbox)
// ---------------------------------------------------------------------------
int32 FVBoxLayerMapper::GetPriority() const { return 200; }
bool FVBoxLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.ParsedTags.Type == EPsdTagType::VBox;
}
UWidget* FVBoxLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    return Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName(*Layer.ParsedTags.CleanName));
}

// ---------------------------------------------------------------------------
// FOverlayLayerMapper  (@overlay)
// ---------------------------------------------------------------------------
int32 FOverlayLayerMapper::GetPriority() const { return 200; }
bool FOverlayLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.ParsedTags.Type == EPsdTagType::Overlay;
}
UWidget* FOverlayLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    return Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), FName(*Layer.ParsedTags.CleanName));
}

// ---------------------------------------------------------------------------
// FScrollBoxLayerMapper  (@scrollbox)
// ---------------------------------------------------------------------------
int32 FScrollBoxLayerMapper::GetPriority() const { return 200; }
bool FScrollBoxLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.ParsedTags.Type == EPsdTagType::ScrollBox;
}
UWidget* FScrollBoxLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    return Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), FName(*Layer.ParsedTags.CleanName));
}

// ---------------------------------------------------------------------------
// FSliderLayerMapper  (@slider)
// ---------------------------------------------------------------------------
int32 FSliderLayerMapper::GetPriority() const { return 200; }
bool FSliderLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.ParsedTags.Type == EPsdTagType::Slider;
}
UWidget* FSliderLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    return Tree->ConstructWidget<USlider>(USlider::StaticClass(), FName(*Layer.ParsedTags.CleanName));
}

// ---------------------------------------------------------------------------
// FCheckBoxLayerMapper  (@checkbox)
// ---------------------------------------------------------------------------
int32 FCheckBoxLayerMapper::GetPriority() const { return 200; }
bool FCheckBoxLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.ParsedTags.Type == EPsdTagType::CheckBox;
}
UWidget* FCheckBoxLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    return Tree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), FName(*Layer.ParsedTags.CleanName));
}

// ---------------------------------------------------------------------------
// FInputLayerMapper  (@input)
// ---------------------------------------------------------------------------
int32 FInputLayerMapper::GetPriority() const { return 200; }
bool FInputLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.ParsedTags.Type == EPsdTagType::Input;
}
UWidget* FInputLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    return Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), FName(*Layer.ParsedTags.CleanName));
}

namespace
{
    // EntryWidgetClass is protected on UListViewBase with no public setter.
    // Use reflection to set it so blueprint compilation does not fail on null entry class.
    static void SetListEntryClass(UListViewBase* View, UClass* EntryClass)
    {
        if (FClassProperty* Prop = FindFProperty<FClassProperty>(UListViewBase::StaticClass(), TEXT("EntryWidgetClass")))
        {
            Prop->SetObjectPropertyValue_InContainer(View, EntryClass);
        }
    }
}

// ---------------------------------------------------------------------------
// FListLayerMapper  (@list)
// ---------------------------------------------------------------------------
int32 FListLayerMapper::GetPriority() const { return 200; }
bool FListLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.ParsedTags.Type == EPsdTagType::List;
}
UWidget* FListLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    UListView* ListView = Tree->ConstructWidget<UListView>(UListView::StaticClass(), FName(*Layer.ParsedTags.CleanName));
    SetListEntryClass(ListView, UPsdListEntryPlaceholder::StaticClass());
    UE_LOG(LogPSD2UMG, Warning, TEXT("FListLayerMapper: '%s' — EntryWidgetClass set to UUserWidget placeholder; replace with your entry widget class."), *Layer.ParsedTags.CleanName);
    return ListView;
}

// ---------------------------------------------------------------------------
// FTileLayerMapper  (@tile)
// ---------------------------------------------------------------------------
int32 FTileLayerMapper::GetPriority() const { return 200; }
bool FTileLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.ParsedTags.Type == EPsdTagType::Tile;
}
UWidget* FTileLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    UTileView* TileView = Tree->ConstructWidget<UTileView>(UTileView::StaticClass(), FName(*Layer.ParsedTags.CleanName));
    SetListEntryClass(TileView, UPsdListEntryPlaceholder::StaticClass());
    UE_LOG(LogPSD2UMG, Warning, TEXT("FTileLayerMapper: '%s' — EntryWidgetClass set to UUserWidget placeholder; replace with your entry widget class."), *Layer.ParsedTags.CleanName);
    return TileView;
}

// ---------------------------------------------------------------------------
// FSwitcherLayerMapper  — historically `Switcher_` prefix; with the tag grammar,
// switcher behaviour is expressed via `@variants` (handled by FVariantsSuffixMapper).
// There is no dedicated `@switcher` tag in the grammar (D-22). This mapper now
// dispatches on `ParsedTags.bIsVariants` as a redundant safety net so the legacy
// class doesn't dead-code-eliminate to nothing — both this and
// FVariantsSuffixMapper produce a UWidgetSwitcher; the higher-priority one wins.
// ---------------------------------------------------------------------------
int32 FSwitcherLayerMapper::GetPriority() const { return 199; } // below FVariantsSuffixMapper(200)
bool FSwitcherLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.ParsedTags.bIsVariants;
}
UWidget* FSwitcherLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    return Tree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), FName(*Layer.ParsedTags.CleanName));
}

// Copyright 2018-2021 - John snow wind
// Implementations for all simple prefix-to-widget mappers.
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

// ---------------------------------------------------------------------------
// FHBoxLayerMapper  (HBox_)
// ---------------------------------------------------------------------------
int32 FHBoxLayerMapper::GetPriority() const { return 200; }
bool FHBoxLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::Group && Layer.Name.StartsWith(TEXT("HBox_"));
}
UWidget* FHBoxLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    return Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName(*Layer.Name));
}

// ---------------------------------------------------------------------------
// FVBoxLayerMapper  (VBox_)
// ---------------------------------------------------------------------------
int32 FVBoxLayerMapper::GetPriority() const { return 200; }
bool FVBoxLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::Group && Layer.Name.StartsWith(TEXT("VBox_"));
}
UWidget* FVBoxLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    return Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName(*Layer.Name));
}

// ---------------------------------------------------------------------------
// FOverlayLayerMapper  (Overlay_)
// ---------------------------------------------------------------------------
int32 FOverlayLayerMapper::GetPriority() const { return 200; }
bool FOverlayLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::Group && Layer.Name.StartsWith(TEXT("Overlay_"));
}
UWidget* FOverlayLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    return Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), FName(*Layer.Name));
}

// ---------------------------------------------------------------------------
// FScrollBoxLayerMapper  (ScrollBox_)
// ---------------------------------------------------------------------------
int32 FScrollBoxLayerMapper::GetPriority() const { return 200; }
bool FScrollBoxLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::Group && Layer.Name.StartsWith(TEXT("ScrollBox_"));
}
UWidget* FScrollBoxLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    return Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), FName(*Layer.Name));
}

// ---------------------------------------------------------------------------
// FSliderLayerMapper  (Slider_)
// ---------------------------------------------------------------------------
int32 FSliderLayerMapper::GetPriority() const { return 200; }
bool FSliderLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::Group && Layer.Name.StartsWith(TEXT("Slider_"));
}
UWidget* FSliderLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    return Tree->ConstructWidget<USlider>(USlider::StaticClass(), FName(*Layer.Name));
}

// ---------------------------------------------------------------------------
// FCheckBoxLayerMapper  (CheckBox_)
// ---------------------------------------------------------------------------
int32 FCheckBoxLayerMapper::GetPriority() const { return 200; }
bool FCheckBoxLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::Group && Layer.Name.StartsWith(TEXT("CheckBox_"));
}
UWidget* FCheckBoxLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    return Tree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), FName(*Layer.Name));
}

// ---------------------------------------------------------------------------
// FInputLayerMapper  (Input_)
// ---------------------------------------------------------------------------
int32 FInputLayerMapper::GetPriority() const { return 200; }
bool FInputLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::Group && Layer.Name.StartsWith(TEXT("Input_"));
}
UWidget* FInputLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    return Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), FName(*Layer.Name));
}

// ---------------------------------------------------------------------------
// FListLayerMapper  (List_)
// ---------------------------------------------------------------------------
int32 FListLayerMapper::GetPriority() const { return 200; }
bool FListLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::Group && Layer.Name.StartsWith(TEXT("List_"));
}
UWidget* FListLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    UListView* ListView = Tree->ConstructWidget<UListView>(UListView::StaticClass(), FName(*Layer.Name));
    // EntryWidgetClass cannot be determined from PSD layer data — must be set manually in the Widget Blueprint.
    UE_LOG(LogPSD2UMG, Log, TEXT("FListLayerMapper: Created UListView for '%s' — EntryWidgetClass must be set manually."), *Layer.Name);
    return ListView;
}

// ---------------------------------------------------------------------------
// FTileLayerMapper  (Tile_)
// ---------------------------------------------------------------------------
int32 FTileLayerMapper::GetPriority() const { return 200; }
bool FTileLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::Group && Layer.Name.StartsWith(TEXT("Tile_"));
}
UWidget* FTileLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    UTileView* TileView = Tree->ConstructWidget<UTileView>(UTileView::StaticClass(), FName(*Layer.Name));
    // EntryWidgetClass cannot be determined from PSD layer data — must be set manually in the Widget Blueprint.
    UE_LOG(LogPSD2UMG, Log, TEXT("FTileLayerMapper: Created UTileView for '%s' — EntryWidgetClass must be set manually."), *Layer.Name);
    return TileView;
}

// ---------------------------------------------------------------------------
// FSwitcherLayerMapper  (Switcher_)
// ---------------------------------------------------------------------------
int32 FSwitcherLayerMapper::GetPriority() const { return 200; }
bool FSwitcherLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::Group && Layer.Name.StartsWith(TEXT("Switcher_"));
}
UWidget* FSwitcherLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    return Tree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), FName(*Layer.Name));
}

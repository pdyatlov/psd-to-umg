// Copyright 2018-2021 - John snow wind
#pragma once
#include "CoreMinimal.h"
#include "Math/Color.h"
#include "Parser/PsdTypes.h"

/** Change annotation for reimport mode — indicates how this layer compares to the existing WBP. */
enum class EPsdChangeAnnotation : uint8
{
    None,       // First import — no annotation shown
    New,        // Layer exists in PSD but not in existing WBP
    Changed,    // Layer exists in both but properties differ
    Unchanged   // Layer exists in both, no changes
};

/** Tree item for the SPsdImportPreviewDialog STreeView. One item per PSD layer. */
struct FPsdLayerTreeItem
{
    FString LayerName;
    FString WidgetTypeName;         // "Button", "Image", "TextBlock", "CanvasPanel", etc.
    FLinearColor BadgeColor;
    bool bChecked = true;
    EPsdChangeAnnotation ChangeAnnotation = EPsdChangeAnnotation::None;
    int32 Depth = 0;
    TWeakPtr<FPsdLayerTreeItem> Parent;
    TArray<TSharedPtr<FPsdLayerTreeItem>> Children;

    // Phase 9 (D-26/D-27): parsed tag state + unknown-tag diagnostics for chip rendering.
    FParsedLayerTags ParsedTags;
};

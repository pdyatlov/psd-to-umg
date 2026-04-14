// Copyright 2018-2021 - John snow wind
#pragma once
#include "CoreMinimal.h"
#include "Components/CanvasPanelSlot.h"

struct FPsdLayer;

struct FAnchorResult
{
    FAnchors Anchors;
    bool bStretchH = false;
    bool bStretchV = false;
    FString CleanName; // Layer name (now sourced from ParsedTags.CleanName)
};

class FAnchorCalculator
{
public:
    // Returns anchor + whether stretch is active on each axis + cleaned name.
    // Reads explicit anchor selection from Layer.ParsedTags.Anchor; falls back
    // to the quadrant heuristic when no @anchor tag is present.
    static FAnchorResult Calculate(const FPsdLayer& Layer, const FIntRect& Bounds, const FIntPoint& CanvasSize);

private:
    static FAnchors QuadrantAnchor(const FIntRect& Bounds, const FIntPoint& CanvasSize, bool& bOutStretchH, bool& bOutStretchV);
};

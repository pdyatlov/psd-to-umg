// Copyright 2018-2021 - John snow wind
#pragma once
#include "CoreMinimal.h"
#include "Components/CanvasPanelSlot.h"

struct FAnchorResult
{
    FAnchors Anchors;
    bool bStretchH = false;
    bool bStretchV = false;
    FString CleanName; // Layer name with suffix stripped
};

class FAnchorCalculator
{
public:
    // Returns anchor + whether stretch is active on each axis + cleaned name
    static FAnchorResult Calculate(const FString& LayerName, const FIntRect& Bounds, const FIntPoint& CanvasSize);

private:
    static bool TryParseSuffix(const FString& Name, FAnchors& OutAnchors, bool& bOutStretchH, bool& bOutStretchV, FString& OutCleanName, const FIntRect& Bounds, const FIntPoint& CanvasSize);
    static FAnchors QuadrantAnchor(const FIntRect& Bounds, const FIntPoint& CanvasSize, bool& bOutStretchH, bool& bOutStretchV);
};

// Copyright 2018-2021 - John snow wind
#include "FAnchorCalculator.h"

#include "Parser/FLayerTagParser.h"
#include "Parser/PsdTypes.h"

// ---------------------------------------------------------------------------
// QuadrantAnchor — auto-anchor heuristic based on layer bounds within canvas.
// Auto-stretch is disabled by default; designers opt in via @anchor:stretch-h,
// @anchor:stretch-v, or @anchor:fill.
// ---------------------------------------------------------------------------
FAnchors FAnchorCalculator::QuadrantAnchor(const FIntRect& Bounds, const FIntPoint& CanvasSize, bool& bOutStretchH, bool& bOutStretchV)
{
    bOutStretchH = false;
    bOutStretchV = false;

    const float CX = (Bounds.Min.X + Bounds.Max.X) * 0.5f;
    const float CY = (Bounds.Min.Y + Bounds.Max.Y) * 0.5f;
    const float W  = static_cast<float>(CanvasSize.X);
    const float H  = static_cast<float>(CanvasSize.Y);

    const float VZone = (H > 0.f && CY < H / 3.f) ? 0.f : (H > 0.f && CY < H * 2.f / 3.f) ? 0.5f : 1.f;
    const float HZone = (W > 0.f && CX < W / 3.f) ? 0.f : (W > 0.f && CX < W * 2.f / 3.f) ? 0.5f : 1.f;

    return FAnchors(HZone, VZone);
}

// ---------------------------------------------------------------------------
// ResolveExplicitAnchor — enum-switch replacement for the legacy GSuffixes table.
// Returns true when Anchor != None (an explicit @anchor tag was present).
// ---------------------------------------------------------------------------
static bool ResolveExplicitAnchor(
    EPsdAnchorTag Anchor,
    const FIntRect& Bounds,
    const FIntPoint& CanvasSize,
    FAnchors& OutAnchors,
    bool& bOutStretchH,
    bool& bOutStretchV)
{
    bOutStretchH = false;
    bOutStretchV = false;

    switch (Anchor)
    {
    case EPsdAnchorTag::TL: OutAnchors = FAnchors(0.f,  0.f,  0.f,  0.f);  return true;
    case EPsdAnchorTag::TC: OutAnchors = FAnchors(0.5f, 0.f,  0.5f, 0.f);  return true;
    case EPsdAnchorTag::TR: OutAnchors = FAnchors(1.f,  0.f,  1.f,  0.f);  return true;
    case EPsdAnchorTag::CL: OutAnchors = FAnchors(0.f,  0.5f, 0.f,  0.5f); return true;
    case EPsdAnchorTag::C:  OutAnchors = FAnchors(0.5f, 0.5f, 0.5f, 0.5f); return true;
    case EPsdAnchorTag::CR: OutAnchors = FAnchors(1.f,  0.5f, 1.f,  0.5f); return true;
    case EPsdAnchorTag::BL: OutAnchors = FAnchors(0.f,  1.f,  0.f,  1.f);  return true;
    case EPsdAnchorTag::BC: OutAnchors = FAnchors(0.5f, 1.f,  0.5f, 1.f);  return true;
    case EPsdAnchorTag::BR: OutAnchors = FAnchors(1.f,  1.f,  1.f,  1.f);  return true;

    case EPsdAnchorTag::Fill:
        OutAnchors  = FAnchors(0.f, 0.f, 1.f, 1.f);
        bOutStretchH = true;
        bOutStretchV = true;
        return true;

    case EPsdAnchorTag::StretchH:
    {
        const float H = static_cast<float>(CanvasSize.Y);
        const float CY = (Bounds.Min.Y + Bounds.Max.Y) * 0.5f;
        const float VZone = (H > 0.f && CY < H / 3.f) ? 0.f : (H > 0.f && CY < H * 2.f / 3.f) ? 0.5f : 1.f;
        OutAnchors = FAnchors(0.f, VZone, 1.f, VZone);
        bOutStretchH = true;
        return true;
    }

    case EPsdAnchorTag::StretchV:
    {
        const float W = static_cast<float>(CanvasSize.X);
        const float CX = (Bounds.Min.X + Bounds.Max.X) * 0.5f;
        const float HZone = (W > 0.f && CX < W / 3.f) ? 0.f : (W > 0.f && CX < W * 2.f / 3.f) ? 0.5f : 1.f;
        OutAnchors = FAnchors(HZone, 0.f, HZone, 1.f);
        bOutStretchV = true;
        return true;
    }

    case EPsdAnchorTag::None:
    default:
        return false;
    }
}

// ---------------------------------------------------------------------------
// Calculate (public entry point)
// ---------------------------------------------------------------------------
FAnchorResult FAnchorCalculator::Calculate(const FPsdLayer& Layer, const FIntRect& Bounds, const FIntPoint& CanvasSize)
{
    FAnchorResult Result;
    Result.CleanName = Layer.ParsedTags.CleanName;

    FAnchors ParsedAnchors;
    bool bStretchH = false;
    bool bStretchV = false;

    if (ResolveExplicitAnchor(Layer.ParsedTags.Anchor, Bounds, CanvasSize, ParsedAnchors, bStretchH, bStretchV))
    {
        Result.Anchors   = ParsedAnchors;
        Result.bStretchH = bStretchH;
        Result.bStretchV = bStretchV;
    }
    else
    {
        Result.Anchors = QuadrantAnchor(Bounds, CanvasSize, Result.bStretchH, Result.bStretchV);
    }

    return Result;
}

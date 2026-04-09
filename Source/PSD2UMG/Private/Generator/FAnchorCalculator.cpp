// Copyright 2018-2021 - John snow wind
#include "FAnchorCalculator.h"

// ---------------------------------------------------------------------------
// Suffix table — longest first to avoid partial matches
// ---------------------------------------------------------------------------
struct FSuffixEntry
{
    const TCHAR* Suffix;
    float MinX, MinY, MaxX, MaxY;
    bool bStretchH;
    bool bStretchV;
    bool bComputed; // true = anchors depend on quadrant, not fixed values
};

static const FSuffixEntry GSuffixes[] = {
    // Fixed-anchor suffixes
    { TEXT("_anchor-tl"),  0.f,   0.f,   0.f,   0.f,   false, false, false },
    { TEXT("_anchor-tc"),  0.5f,  0.f,   0.5f,  0.f,   false, false, false },
    { TEXT("_anchor-tr"),  1.f,   0.f,   1.f,   0.f,   false, false, false },
    { TEXT("_anchor-cl"),  0.f,   0.5f,  0.f,   0.5f,  false, false, false },
    { TEXT("_anchor-cr"),  1.f,   0.5f,  1.f,   0.5f,  false, false, false },
    { TEXT("_anchor-bl"),  0.f,   1.f,   0.f,   1.f,   false, false, false },
    { TEXT("_anchor-bc"),  0.5f,  1.f,   0.5f,  1.f,   false, false, false },
    { TEXT("_anchor-br"),  1.f,   1.f,   1.f,   1.f,   false, false, false },
    // _anchor-c must come after longer suffixes that share the prefix
    { TEXT("_anchor-c"),   0.5f,  0.5f,  0.5f,  0.5f,  false, false, false },
    // Stretch suffixes (anchors computed from quadrant)
    { TEXT("_stretch-h"),  0.f,   0.f,   1.f,   0.f,   true,  false, true  },
    { TEXT("_stretch-v"),  0.f,   0.f,   0.f,   1.f,   false, true,  true  },
    { TEXT("_fill"),       0.f,   0.f,   1.f,   1.f,   true,  true,  false },
};

// ---------------------------------------------------------------------------
// QuadrantAnchor
// ---------------------------------------------------------------------------
FAnchors FAnchorCalculator::QuadrantAnchor(const FIntRect& Bounds, const FIntPoint& CanvasSize, bool& bOutStretchH, bool& bOutStretchV)
{
    const float CX = (Bounds.Min.X + Bounds.Max.X) * 0.5f;
    const float CY = (Bounds.Min.Y + Bounds.Max.Y) * 0.5f;
    const float W  = static_cast<float>(CanvasSize.X);
    const float H  = static_cast<float>(CanvasSize.Y);

    bOutStretchH = (W > 0.f) && (Bounds.Width()  >= W * 0.8f);
    bOutStretchV = (H > 0.f) && (Bounds.Height() >= H * 0.8f);

    if (bOutStretchH && bOutStretchV)
    {
        return FAnchors(0.f, 0.f, 1.f, 1.f);
    }

    const float VZone = (H > 0.f && CY < H / 3.f) ? 0.f : (H > 0.f && CY < H * 2.f / 3.f) ? 0.5f : 1.f;
    const float HZone = (W > 0.f && CX < W / 3.f) ? 0.f : (W > 0.f && CX < W * 2.f / 3.f) ? 0.5f : 1.f;

    if (bOutStretchH)
    {
        return FAnchors(0.f, VZone, 1.f, VZone);
    }
    if (bOutStretchV)
    {
        return FAnchors(HZone, 0.f, HZone, 1.f);
    }
    return FAnchors(HZone, VZone);
}

// ---------------------------------------------------------------------------
// TryParseSuffix
// ---------------------------------------------------------------------------
bool FAnchorCalculator::TryParseSuffix(const FString& Name, FAnchors& OutAnchors, bool& bOutStretchH, bool& bOutStretchV, FString& OutCleanName, const FIntRect& Bounds, const FIntPoint& CanvasSize)
{
    for (const FSuffixEntry& Entry : GSuffixes)
    {
        if (Name.EndsWith(Entry.Suffix))
        {
            const int32 SuffixLen = FCString::Strlen(Entry.Suffix);
            OutCleanName = Name.LeftChop(SuffixLen);

            if (Entry.bComputed)
            {
                // _stretch-h or _stretch-v: compute zone from quadrant but override bStretch flags
                bool bDummyH = false, bDummyV = false;
                FAnchors Quadrant = QuadrantAnchor(Bounds, CanvasSize, bDummyH, bDummyV);

                const float W = static_cast<float>(CanvasSize.X);
                const float H = static_cast<float>(CanvasSize.Y);
                const float CX = (Bounds.Min.X + Bounds.Max.X) * 0.5f;
                const float CY = (Bounds.Min.Y + Bounds.Max.Y) * 0.5f;
                const float VZone = (H > 0.f && CY < H / 3.f) ? 0.f : (H > 0.f && CY < H * 2.f / 3.f) ? 0.5f : 1.f;
                const float HZone = (W > 0.f && CX < W / 3.f) ? 0.f : (W > 0.f && CX < W * 2.f / 3.f) ? 0.5f : 1.f;

                bOutStretchH = Entry.bStretchH;
                bOutStretchV = Entry.bStretchV;

                if (Entry.bStretchH && Entry.bStretchV)
                {
                    // _fill
                    OutAnchors = FAnchors(0.f, 0.f, 1.f, 1.f);
                }
                else if (Entry.bStretchH)
                {
                    // _stretch-h: horizontal stretch, Y from vertical quadrant
                    OutAnchors = FAnchors(0.f, VZone, 1.f, VZone);
                }
                else
                {
                    // _stretch-v: vertical stretch, X from horizontal quadrant
                    OutAnchors = FAnchors(HZone, 0.f, HZone, 1.f);
                }
            }
            else
            {
                OutAnchors    = FAnchors(Entry.MinX, Entry.MinY, Entry.MaxX, Entry.MaxY);
                bOutStretchH  = false;
                bOutStretchV  = false;
            }
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Calculate (public entry point)
// ---------------------------------------------------------------------------
FAnchorResult FAnchorCalculator::Calculate(const FString& LayerName, const FIntRect& Bounds, const FIntPoint& CanvasSize)
{
    FAnchorResult Result;

    FAnchors     ParsedAnchors;
    bool         bStretchH = false;
    bool         bStretchV = false;
    FString      CleanName;

    if (TryParseSuffix(LayerName, ParsedAnchors, bStretchH, bStretchV, CleanName, Bounds, CanvasSize))
    {
        Result.Anchors   = ParsedAnchors;
        Result.bStretchH = bStretchH;
        Result.bStretchV = bStretchV;
        Result.CleanName = CleanName;
    }
    else
    {
        Result.Anchors   = QuadrantAnchor(Bounds, CanvasSize, Result.bStretchH, Result.bStretchV);
        Result.CleanName = LayerName;
    }

    return Result;
}

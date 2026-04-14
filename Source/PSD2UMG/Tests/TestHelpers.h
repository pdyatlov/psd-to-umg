// Copyright 2018-2021 - John snow wind
// Test-only helpers shared across PSD2UMG specs. Phase 9 Pitfall 5 mitigation:
// every synthetic FPsdLayer constructed in tests must have ParsedTags populated
// or downstream tag-dispatched mappers will reject it. Use MakeTaggedTestLayer
// to build a layer whose ParsedTags is parsed from its Name.

#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Parser/FLayerTagParser.h"
#include "Parser/PsdTypes.h"

namespace PSD2UMG::Tests
{
    /** Construct a synthetic FPsdLayer with ParsedTags populated from Name. */
    inline FPsdLayer MakeTaggedTestLayer(
        FStringView Name,
        EPsdLayerType Type,
        int32 Index = 0)
    {
        FPsdLayer L;
        L.Name = FString(Name);
        L.Type = Type;
        FString Diag;
        L.ParsedTags = FLayerTagParser::Parse(L.Name, L.Type, Index, Diag);
        return L;
    }

    /** Populate ParsedTags on a layer that was constructed field-by-field. */
    inline void PopulateParsedTags(FPsdLayer& L, int32 Index = 0)
    {
        FString Diag;
        L.ParsedTags = FLayerTagParser::Parse(L.Name, L.Type, Index, Diag);
    }

    /** Recursively populate ParsedTags on every layer in a tree. Mirrors the
     *  PsdParser::ParseFile post-pass so synthetic test documents (which never
     *  go through ParseFile) still satisfy tag-dispatched mappers. */
    inline void PopulateParsedTagsRecursive(TArray<FPsdLayer>& Layers)
    {
        int32 Index = 0;
        for (FPsdLayer& L : Layers)
        {
            FString Diag;
            L.ParsedTags = FLayerTagParser::Parse(L.Name, L.Type, Index, Diag);
            if (L.Children.Num() > 0)
            {
                PopulateParsedTagsRecursive(L.Children);
            }
            ++Index;
        }
    }

    inline void PopulateParsedTagsForDocument(FPsdDocument& Doc)
    {
        PopulateParsedTagsRecursive(Doc.RootLayers);
    }
}

#endif // WITH_DEV_AUTOMATION_TESTS

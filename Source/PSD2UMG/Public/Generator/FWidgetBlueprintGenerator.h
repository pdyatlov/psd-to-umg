// Copyright 2018-2021 - John snow wind
#pragma once
#include "CoreMinimal.h"

struct FPsdDocument;
class UWidgetBlueprint;

class PSD2UMG_API FWidgetBlueprintGenerator
{
public:
    // Generate a Widget Blueprint from a parsed PSD document.
    // WbpPackagePath: e.g. "/Game/UI/Widgets"
    // WbpAssetName:   e.g. "WBP_MyHUD"
    // Returns nullptr on failure.
    static UWidgetBlueprint* Generate(const FPsdDocument& Doc, const FString& WbpPackagePath, const FString& WbpAssetName);
};

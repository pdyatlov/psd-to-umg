// Copyright 2018-2021 - John snow wind
#pragma once
#include "CoreMinimal.h"
#include "UI/PsdLayerTreeItem.h"

struct FPsdDocument;
struct FPsdLayer;
class UWidget;
class UWidgetBlueprint;

class PSD2UMG_API FWidgetBlueprintGenerator
{
public:
    // Generate a Widget Blueprint from a parsed PSD document.
    // WbpPackagePath: e.g. "/Game/UI/Widgets"
    // WbpAssetName:   e.g. "WBP_MyHUD"
    // Returns nullptr on failure.
    static UWidgetBlueprint* Generate(const FPsdDocument& Doc, const FString& WbpPackagePath, const FString& WbpAssetName, const TSet<FString>& SkippedLayerNames = TSet<FString>());

    // Update an existing Widget Blueprint from a re-parsed PSD document.
    // - Matches existing widgets by name (D-05)
    // - Updates PSD-sourced properties: position, size, image data, text (D-06)
    // - Preserves manual edits: Blueprint logic, bindings, animations, manually-added widgets (D-06)
    // - Widgets from deleted PSD layers are kept as orphans (D-07)
    // - SkippedLayerNames: layers the user unchecked in the preview dialog
    static bool Update(
        UWidgetBlueprint* ExistingWBP,
        const FPsdDocument& NewDoc,
        const TSet<FString>& SkippedLayerNames);

    // Detect change annotation for a PSD layer vs an existing widget.
    // Returns New if ExistingWidget is null, Changed if properties differ, Unchanged otherwise.
    static EPsdChangeAnnotation DetectChange(
        const FPsdLayer& NewLayer,
        UWidget* ExistingWidget);
};

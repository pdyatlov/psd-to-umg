// Copyright 2018-2021 - John snow wind
#pragma once
#include "CoreMinimal.h"

class UWidgetBlueprint;
class UWidgetAnimation;

class FPsdWidgetAnimationBuilder
{
public:
    // Create opacity fade animation (for _show and _hide variants).
    // FromOpacity/ToOpacity: 0.0-1.0, DurationSec in seconds.
    static UWidgetAnimation* CreateOpacityFade(
        UWidgetBlueprint* WBP,
        const FString& AnimName,
        const FName& TargetWidgetName,
        float FromOpacity, float ToOpacity, float DurationSec);

    // Create scale animation (for _hover variant).
    // FromScale/ToScale as FVector2D.
    static UWidgetAnimation* CreateScaleAnim(
        UWidgetBlueprint* WBP,
        const FString& AnimName,
        const FName& TargetWidgetName,
        FVector2D FromScale, FVector2D ToScale, float DurationSec);

    // Detect animation variants in layer name suffixes and create animations.
    // Checks for _show, _hide, _hover suffixes; creates appropriate animations.
    // Call after widget tree is built.
    static void ProcessAnimationVariants(
        UWidgetBlueprint* WBP,
        const FName& WidgetName,
        const FString& LayerName);
};

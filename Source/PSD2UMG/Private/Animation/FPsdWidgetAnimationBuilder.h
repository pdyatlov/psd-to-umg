// Copyright 2018-2021 - John snow wind
#pragma once
#include "CoreMinimal.h"
#include "Parser/FLayerTagParser.h"

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

    // Phase 17.2 (BTN-ANIM-01) -- animate a UTextBlock::ColorAndOpacity (FSlateColor/FLinearColor)
    // over DurationSec seconds. Uses UMovieSceneColorTrack with 4 linear-key channels.
    // Used by FWidgetBlueprintGenerator to build {CleanName}_Hover / {CleanName}_Pressed
    // animations targeting the button's inner text widget.
    static UWidgetAnimation* CreateColorAnim(
        UWidgetBlueprint* WBP,
        const FString& AnimName,
        const FName& TargetWidgetName,
        FLinearColor FromColor, FLinearColor ToColor, float DurationSec);

    // Create the animation matching a parsed @anim tag (Show / Hide / Hover).
    // AnimNamePrefix is used to build a unique animation asset name.
    // Pass EPsdAnimTag::None for a no-op.
    static void ProcessAnimationVariants(
        UWidgetBlueprint* WBP,
        const FName& WidgetName,
        const FString& AnimNamePrefix,
        EPsdAnimTag AnimTag);
};

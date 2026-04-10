// Copyright 2018-2021 - John snow wind

#include "Animation/FPsdWidgetAnimationBuilder.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprint.h"

#include "MovieScene.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "Sections/MovieSceneFloatSection.h"
#include "Channels/MovieSceneFloatChannel.h"
#include "MovieSceneBinding.h"

// Helper: build a UMovieScene with the standard UMG tick/display settings
static UMovieScene* CreateMovieScene(UWidgetBlueprint* WBP, float DurationSec)
{
    UMovieScene* Scene = NewObject<UMovieScene>(WBP, MakeUniqueObjectName(WBP, UMovieScene::StaticClass()), RF_Transactional);

    // Standard UMG MovieScene settings: 24000 ticks per second, 30 fps display rate
    Scene->SetTickResolutionDirectly(FFrameRate(24000, 1));
    Scene->SetDisplayRate(FFrameRate(30, 1));

    const FFrameNumber StartFrame(0);
    const FFrameNumber EndFrame(FMath::RoundToInt(DurationSec * 24000.0f));
    Scene->SetPlaybackRange(TRange<FFrameNumber>(StartFrame, EndFrame));

    return Scene;
}

// Helper: add a float track with two linear keys (start and end)
static void AddFloatTrack(
    UMovieScene* Scene,
    const FGuid& ObjectGuid,
    const FString& PropertyName,
    float FromValue, float ToValue,
    FFrameNumber StartFrame, FFrameNumber EndFrame)
{
    UMovieSceneFloatTrack* Track = Scene->AddTrack<UMovieSceneFloatTrack>(ObjectGuid);
    if (!Track)
    {
        return;
    }
    Track->SetPropertyNameAndPath(FName(*PropertyName), PropertyName);

    UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(Track->CreateNewSection());
    if (!Section)
    {
        return;
    }
    Section->SetRange(TRange<FFrameNumber>(StartFrame, EndFrame));
    Track->AddSection(*Section);

    FMovieSceneFloatChannel& Channel = Section->GetChannel();
    Channel.AddLinearKey(StartFrame, FromValue);
    Channel.AddLinearKey(EndFrame, ToValue);
}

UWidgetAnimation* FPsdWidgetAnimationBuilder::CreateOpacityFade(
    UWidgetBlueprint* WBP,
    const FString& AnimName,
    const FName& TargetWidgetName,
    float FromOpacity, float ToOpacity, float DurationSec)
{
    if (!WBP)
    {
        return nullptr;
    }

    UWidgetAnimation* Anim = NewObject<UWidgetAnimation>(
        WBP,
        FName(*AnimName),
        RF_Transactional | RF_Public);

    UMovieScene* Scene = CreateMovieScene(WBP, DurationSec);
    Anim->SetMovieScene(Scene);

    const FGuid ObjectGuid = Scene->AddPossessable(TargetWidgetName.ToString(), UObject::StaticClass());

    const FFrameNumber StartFrame(0);
    const FFrameNumber EndFrame(FMath::RoundToInt(DurationSec * 24000.0f));
    AddFloatTrack(Scene, ObjectGuid, TEXT("RenderOpacity"), FromOpacity, ToOpacity, StartFrame, EndFrame);

    // Register binding so runtime can resolve the possessable to the named widget
    FWidgetAnimationBinding Binding;
    Binding.WidgetName = TargetWidgetName;
    Binding.AnimationGuid = ObjectGuid;
    Binding.SlotWidgetName = NAME_None;
    Anim->AnimationBindings.Add(Binding);

    WBP->Animations.Add(Anim);

    return Anim;
}

UWidgetAnimation* FPsdWidgetAnimationBuilder::CreateScaleAnim(
    UWidgetBlueprint* WBP,
    const FString& AnimName,
    const FName& TargetWidgetName,
    FVector2D FromScale, FVector2D ToScale, float DurationSec)
{
    if (!WBP)
    {
        return nullptr;
    }

    UWidgetAnimation* Anim = NewObject<UWidgetAnimation>(
        WBP,
        FName(*AnimName),
        RF_Transactional | RF_Public);

    UMovieScene* Scene = CreateMovieScene(WBP, DurationSec);
    Anim->SetMovieScene(Scene);

    const FGuid ObjectGuid = Scene->AddPossessable(TargetWidgetName.ToString(), UObject::StaticClass());

    const FFrameNumber StartFrame(0);
    const FFrameNumber EndFrame(FMath::RoundToInt(DurationSec * 24000.0f));

    AddFloatTrack(Scene, ObjectGuid, TEXT("RenderTransform.Scale.X"),
        static_cast<float>(FromScale.X), static_cast<float>(ToScale.X), StartFrame, EndFrame);
    AddFloatTrack(Scene, ObjectGuid, TEXT("RenderTransform.Scale.Y"),
        static_cast<float>(FromScale.Y), static_cast<float>(ToScale.Y), StartFrame, EndFrame);

    FWidgetAnimationBinding Binding;
    Binding.WidgetName = TargetWidgetName;
    Binding.AnimationGuid = ObjectGuid;
    Binding.SlotWidgetName = NAME_None;
    Anim->AnimationBindings.Add(Binding);

    WBP->Animations.Add(Anim);

    return Anim;
}

void FPsdWidgetAnimationBuilder::ProcessAnimationVariants(
    UWidgetBlueprint* WBP,
    const FName& WidgetName,
    const FString& LayerName)
{
    if (!WBP)
    {
        return;
    }

    const FString LayerLower = LayerName.ToLower();

    if (LayerLower.EndsWith(TEXT("_show")))
    {
        CreateOpacityFade(WBP, LayerName + TEXT("_Show"), WidgetName, 0.0f, 1.0f, 0.3f);
    }
    if (LayerLower.EndsWith(TEXT("_hide")))
    {
        CreateOpacityFade(WBP, LayerName + TEXT("_Hide"), WidgetName, 1.0f, 0.0f, 0.3f);
    }
    if (LayerLower.EndsWith(TEXT("_hover")))
    {
        CreateScaleAnim(WBP, LayerName + TEXT("_Hover"), WidgetName,
            FVector2D(1.0f, 1.0f), FVector2D(1.05f, 1.05f), 0.15f);
    }
}

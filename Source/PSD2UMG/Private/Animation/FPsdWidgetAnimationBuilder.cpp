// Copyright 2018-2021 - John snow wind

#include "Animation/FPsdWidgetAnimationBuilder.h"
#include "PSD2UMGLog.h"

#include "Animation/WidgetAnimation.h"
#include "WidgetBlueprint.h"
#include "Components/TextBlock.h"

#include "MovieScene.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "Sections/MovieSceneFloatSection.h"
#include "Channels/MovieSceneFloatChannel.h"
#include "Tracks/MovieSceneColorTrack.h"
#include "Sections/MovieSceneColorSection.h"
#include "MovieSceneBinding.h"

// Helper: build a UMovieScene with the standard UMG tick/display settings.
// Outer must be the owning UWidgetAnimation and name must match the animation FName so
// BindAnimationsStatic can locate the UPROPERTY via Animation->GetMovieScene()->GetFName().
static UMovieScene* CreateMovieScene(UWidgetAnimation* Anim, float DurationSec)
{
    UMovieScene* Scene = NewObject<UMovieScene>(Anim, Anim->GetFName(), RF_Transactional);

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

    UMovieScene* Scene = CreateMovieScene(Anim, DurationSec);
    Anim->MovieScene = Scene;

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

    UMovieScene* Scene = CreateMovieScene(Anim, DurationSec);
    Anim->MovieScene = Scene;

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

UWidgetAnimation* FPsdWidgetAnimationBuilder::CreateColorAnim(
    UWidgetBlueprint* WBP,
    const FString& AnimName,
    const FName& TargetWidgetName,
    FLinearColor FromColor, FLinearColor ToColor, float DurationSec)
{
    if (!WBP)
    {
        return nullptr;
    }

    // Pitfall 4: use the exact requested name when available; only generate a unique
    // suffix when StaticFindObjectFast confirms a real naming conflict (e.g. two
    // @button groups share a CleanName in the same PSD). MakeUniqueObjectName in
    // UE5 always appends _0 even for fresh names, so we must guard it with an
    // explicit conflict check.
    FName ActualName(*AnimName);
    if (UObject* Existing = StaticFindObjectFast(nullptr, WBP, ActualName))
    {
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("CreateColorAnim: '%s' name conflict (existing class=%s) — generating unique suffix"),
            *AnimName, *Existing->GetClass()->GetName());
        ActualName = MakeUniqueObjectName(WBP, UWidgetAnimation::StaticClass(), FName(*AnimName));
    }
    UWidgetAnimation* Anim = NewObject<UWidgetAnimation>(
        WBP,
        ActualName,
        RF_Transactional | RF_Public);

    UMovieScene* Scene = CreateMovieScene(Anim, DurationSec);
    Anim->MovieScene = Scene;

    const FGuid ObjectGuid = Scene->AddPossessable(
        TargetWidgetName.ToString(), UTextBlock::StaticClass());

    const FFrameNumber StartFrame(0);
    const FFrameNumber EndFrame(FMath::RoundToInt(DurationSec * 24000.0f));

    UMovieSceneColorTrack* Track = Scene->AddTrack<UMovieSceneColorTrack>(ObjectGuid);
    if (!Track)
    {
        return Anim; // defensive; AddTrack should always succeed on a fresh scene
    }
    Track->SetPropertyNameAndPath(FName(TEXT("ColorAndOpacity")), TEXT("ColorAndOpacity"));

    UMovieSceneColorSection* Section = Cast<UMovieSceneColorSection>(Track->CreateNewSection());
    if (!Section)
    {
        return Anim;
    }
    Section->SetRange(TRange<FFrameNumber>(StartFrame, EndFrame));
    Track->AddSection(*Section);

    Section->GetRedChannel().AddLinearKey(StartFrame,   FromColor.R);
    Section->GetRedChannel().AddLinearKey(EndFrame,     ToColor.R);
    Section->GetGreenChannel().AddLinearKey(StartFrame, FromColor.G);
    Section->GetGreenChannel().AddLinearKey(EndFrame,   ToColor.G);
    Section->GetBlueChannel().AddLinearKey(StartFrame,  FromColor.B);
    Section->GetBlueChannel().AddLinearKey(EndFrame,    ToColor.B);
    Section->GetAlphaChannel().AddLinearKey(StartFrame, FromColor.A);
    Section->GetAlphaChannel().AddLinearKey(EndFrame,   ToColor.A);

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
    const FString& AnimNamePrefix,
    EPsdAnimTag AnimTag)
{
    if (!WBP)
    {
        return;
    }

    switch (AnimTag)
    {
    case EPsdAnimTag::Show:
        CreateOpacityFade(WBP, AnimNamePrefix + TEXT("_Show"), WidgetName, 0.0f, 1.0f, 0.3f);
        break;
    case EPsdAnimTag::Hide:
        CreateOpacityFade(WBP, AnimNamePrefix + TEXT("_Hide"), WidgetName, 1.0f, 0.0f, 0.3f);
        break;
    case EPsdAnimTag::Hover:
        CreateScaleAnim(WBP, AnimNamePrefix + TEXT("_Hover"), WidgetName,
            FVector2D(1.0f, 1.0f), FVector2D(1.05f, 1.05f), 0.15f);
        break;
    case EPsdAnimTag::None:
    default:
        break;
    }
}

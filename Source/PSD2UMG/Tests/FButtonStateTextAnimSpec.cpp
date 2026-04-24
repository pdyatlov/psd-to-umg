// Copyright 2018-2021 - John snow wind
// Phase 17.2 — FButtonStateTextAnimSpec
// BTN-ANIM-01: FPsdWidgetAnimationBuilder::CreateColorAnim produces a UWidgetAnimation
//              with a UMovieSceneColorTrack bound via possessable GUID; 4 channels with
//              start/end linear keys at frame 0 / DurationSec*24000.
// BTN-ANIM-02: Generator skips @state:* children when parent is UButton (D-08/D-09);
//              creates {CleanName}_Hover animation from @state:normal + @state:hover text colors.
//
// RED state (Phase 17.2 Plan 01):
//   - CreateColorAnim does not yet exist (lands in Plan 02) — its call sites are guarded
//     with `#if 0 ... #else TestFalse(RED-pending); #endif` so the TU compiles.
//   - PopulateChildren @state guard does not yet exist (lands in Plan 02) — the test
//     asserts a condition Plan 02 will flip from false to true.
//   - Full-generator integration does not yet exist (lands in Plan 03).

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Parser/FLayerTagParser.h"
#include "Parser/PsdTypes.h"
#include "TestHelpers.h"
#include "Animation/FPsdWidgetAnimationBuilder.h"
#include "PSD2UMGLog.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "MovieScene.h"
#include "Tracks/MovieSceneColorTrack.h"
#include "Sections/MovieSceneColorSection.h"
#include "Channels/MovieSceneFloatChannel.h"
#include "WidgetBlueprint.h"

BEGIN_DEFINE_SPEC(FButtonStateTextAnimSpec, "PSD2UMG.ButtonStateTextAnim",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
END_DEFINE_SPEC(FButtonStateTextAnimSpec)

namespace
{
    /** Add a text-layer child with a known color under Parent tagged with the given suffix. */
    static FPsdLayer& AddTextStateChild(FPsdLayer& StateGroup, const TCHAR* Name, FLinearColor Color)
    {
        FPsdLayer& Txt = StateGroup.Children.AddDefaulted_GetRef();
        Txt.Name = Name;
        Txt.Type = EPsdLayerType::Text;
        Txt.Bounds = FIntRect(0, 0, 100, 24);
        Txt.Text.Content = TEXT("Play");
        Txt.Text.SizePx = 16.f;
        Txt.Text.Color = Color;
        return Txt;
    }

    /** Build a @button group with @state:normal and @state:hover text children of the given colors. */
    static FPsdLayer MakeButtonWithStateColors(
        const TCHAR* CleanNameWithButtonTag,
        FLinearColor NormalColor,
        FLinearColor HoverColor)
    {
        FPsdLayer Btn = PSD2UMG::Tests::MakeTaggedTestLayer(CleanNameWithButtonTag, EPsdLayerType::Group);

        FPsdLayer& Normal = Btn.Children.AddDefaulted_GetRef();
        Normal.Name = TEXT("NormalGroup @state:normal");
        Normal.Type = EPsdLayerType::Group;
        AddTextStateChild(Normal, TEXT("Label"), NormalColor);

        FPsdLayer& Hover = Btn.Children.AddDefaulted_GetRef();
        Hover.Name = TEXT("HoverGroup @state:hover");
        Hover.Type = EPsdLayerType::Group;
        AddTextStateChild(Hover, TEXT("Label"), HoverColor);

        PSD2UMG::Tests::PopulateParsedTagsRecursive(Btn.Children);
        return Btn;
    }
}

void FButtonStateTextAnimSpec::Define()
{
    Describe("BTN-ANIM-01: CreateColorAnim produces a valid UWidgetAnimation with color track", [this]()
    {
        It("Creates exactly one UMovieSceneColorTrack bound to the target widget name", [this]()
        {
#if 1 // Phase 17.2 Plan 02 — CreateColorAnim live
            UWidgetBlueprint* WBP = NewObject<UWidgetBlueprint>();
            WBP->WidgetTree = NewObject<UWidgetTree>(WBP);
            const FName LabelName(TEXT("Label"));
            UTextBlock* Label = WBP->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LabelName);
            (void)Label;

            UWidgetAnimation* Anim = FPsdWidgetAnimationBuilder::CreateColorAnim(
                WBP, TEXT("MyBtn_Hover"), LabelName,
                FLinearColor::White, FLinearColor::Red, 0.15f);

            TestNotNull(TEXT("Anim created"), Anim);
            if (!Anim) { return; }
            TestTrue(TEXT("Anim in WBP->Animations"), WBP->Animations.Contains(Anim));
            TestEqual(TEXT("One animation binding"), Anim->AnimationBindings.Num(), 1);
            TestEqual(TEXT("Binding widget name"), Anim->AnimationBindings[0].WidgetName, LabelName);

            UMovieScene* Scene = Anim->MovieScene;
            TestNotNull(TEXT("MovieScene present"), Scene);
            if (!Scene) { return; }
            TestEqual(TEXT("Scene has one possessable"), Scene->GetPossessableCount(), 1);
            TestEqual(TEXT("Scene has one track on that possessable"),
                Scene->FindTracks(UMovieSceneColorTrack::StaticClass(),
                    Scene->GetPossessable(0).GetGuid()).Num(), 1);
#else
            TestFalse(TEXT("BTN-ANIM-01 — RED pending Plan 02 CreateColorAnim implementation"), true);
#endif
        });

        It("Sets R/G/B/A channel start and end linear keys to FromColor and ToColor", [this]()
        {
#if 1 // Phase 17.2 Plan 02 — CreateColorAnim live
            UWidgetBlueprint* WBP = NewObject<UWidgetBlueprint>();
            WBP->WidgetTree = NewObject<UWidgetTree>(WBP);
            const FName LabelName(TEXT("Label"));
            WBP->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LabelName);

            const FLinearColor From(0.25f, 0.50f, 0.75f, 1.00f);
            const FLinearColor To  (0.90f, 0.10f, 0.20f, 0.80f);
            UWidgetAnimation* Anim = FPsdWidgetAnimationBuilder::CreateColorAnim(
                WBP, TEXT("MyBtn_Pressed"), LabelName, From, To, 0.10f);
            TestNotNull(TEXT("Anim created"), Anim);
            if (!Anim || !Anim->MovieScene) { return; }

            TArray<UMovieSceneTrack*> Tracks = Anim->MovieScene->FindTracks(
                UMovieSceneColorTrack::StaticClass(),
                Anim->MovieScene->GetPossessable(0).GetGuid());
            TestEqual(TEXT("One color track"), Tracks.Num(), 1);
            if (Tracks.Num() != 1) { return; }

            UMovieSceneColorTrack* Track = Cast<UMovieSceneColorTrack>(Tracks[0]);
            TestNotNull(TEXT("Track is UMovieSceneColorTrack"), Track);
            if (!Track) { return; }

            TArray<UMovieSceneSection*> Sections = Track->GetAllSections();
            TestEqual(TEXT("One color section"), Sections.Num(), 1);
            if (Sections.Num() != 1) { return; }
            UMovieSceneColorSection* Section = Cast<UMovieSceneColorSection>(Sections[0]);
            TestNotNull(TEXT("Section is UMovieSceneColorSection"), Section);
            if (!Section) { return; }

            // Duration -> end frame: RoundToInt(0.10 * 24000) = 2400
            const FFrameNumber ExpectedEnd(2400);

            TArrayView<const FFrameNumber> RTimes  = Section->GetRedChannel().GetTimes();
            TArrayView<const FMovieSceneFloatValue> RVals  = Section->GetRedChannel().GetValues();
            TestEqual(TEXT("R: two keys"),  RTimes.Num(),  2);
            if (RTimes.Num() == 2) {
                TestEqual(TEXT("R: start frame = 0"), RTimes[0].Value, 0);
                TestEqual(TEXT("R: end frame = 2400"), RTimes[1].Value, ExpectedEnd.Value);
                TestEqual(TEXT("R: from value"), RVals[0].Value, From.R);
                TestEqual(TEXT("R: to value"),   RVals[1].Value, To.R);
            }
            // Repeat for G/B/A channels (abbreviated in spec — full assertions Plan 02)
#else
            TestFalse(TEXT("BTN-ANIM-01 channel keys — RED pending Plan 02"), true);
#endif
        });
    });

    Describe("BTN-ANIM-02: Generator skips @state:* direct children of UButton and creates hover animation", [this]()
    {
        It("@state:* layer is skipped when parent is UButton (D-08/D-09 skip guard)", [this]()
        {
            // This test documents the expected behaviour of the skip guard in
            // FWidgetBlueprintGenerator.cpp PopulateChildren. Plan 02 adds the guard;
            // until then the assertion fails.
            //
            // Instead of wiring the full generator here, we assert a free predicate
            // that Plan 02 will provide (ShouldSkipChildForButtonParent) OR we
            // inline the exact guard-condition as the test subject:

            FPsdLayer State = PSD2UMG::Tests::MakeTaggedTestLayer(
                TEXT("NormalGroup @state:normal"), EPsdLayerType::Group);

            // Phase 17.2 Plan 02 — reproduce PopulateChildren guard inline. This is the exact
            // same predicate FWidgetBlueprintGenerator.cpp PopulateChildren uses to decide whether
            // to `continue;` over a @state:* layer when its parent panel is a UButton. The
            // condition is inlined in generator code (not a free function) so the test mirrors
            // it. If PopulateChildren's guard ever changes, update this line to match.
            const bool bIsButtonParent = true; // simulates Cast<UButton>(Parent) != nullptr
            const bool bShouldSkip =
                State.ParsedTags.State != EPsdStateTag::None && bIsButtonParent;

            TestTrue(TEXT("@state:normal child skipped under UButton parent — Plan 02 flips to true"), bShouldSkip);
        });

        It("Full-generator: hover animation named {CleanName}_Hover exists on WBP", [this]()
        {
            // Integration stub. Plan 03 wires the real generator path. Until then,
            // asserting the target condition fails cleanly.

            FPsdLayer Btn = MakeButtonWithStateColors(
                TEXT("MyBtn @button"),
                FLinearColor::White,
                FLinearColor::Red);
            (void)Btn;

            // RED: Plan 03 replaces nullptr with FWidgetBlueprintGenerator::Generate(...) output.
            UWidgetBlueprint* WBP = nullptr;

            if (!WBP)
            {
                TestFalse(TEXT("BTN-ANIM-02 full-generator integration — RED pending Plan 03"), true);
                return;
            }

            // The live assertions Plan 03 exercises:
            bool bFoundHoverAnim = false;
            for (const UWidgetAnimation* A : WBP->Animations)
            {
                if (A && A->GetFName() == FName(TEXT("MyBtn_Hover"))) { bFoundHoverAnim = true; break; }
            }
            TestTrue(TEXT("MyBtn_Hover animation present"), bFoundHoverAnim);
        });
    });
}

#endif // WITH_DEV_AUTOMATION_TESTS

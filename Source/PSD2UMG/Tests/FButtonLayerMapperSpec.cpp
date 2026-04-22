// Copyright 2018-2021 - John snow wind
// Phase 17.1 — FButtonLayerMapperSpec
// BTN-STATE-01: @button @variants conflict resolves deterministically to FButtonLayerMapper.
// BTN-STATE-02: @state:* child layers wire to FButtonStyle slots; partial state children do not abort.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Mapper/AllMappers.h"
#include "Parser/FLayerTagParser.h"
#include "Parser/PsdTypes.h"
#include "TestHelpers.h"
#include "PSD2UMGLog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "WidgetBlueprint.h"

BEGIN_DEFINE_SPEC(FButtonLayerMapperSpec, "PSD2UMG.Mapper.ButtonLayerMapper",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

    FButtonLayerMapper ButtonMapper;
    FVariantsSuffixMapper VariantsMapper;

END_DEFINE_SPEC(FButtonLayerMapperSpec)

namespace
{
    /** Add a 64x64 zero-filled image child to Parent with the given @state:* tag. */
    static FPsdLayer& AddStateChild(FPsdLayer& Parent, const TCHAR* NameWithTag)
    {
        FPsdLayer& Child = Parent.Children.AddDefaulted_GetRef();
        Child.Name = FString(NameWithTag);
        Child.Type = EPsdLayerType::Image;
        Child.Bounds = FIntRect(0, 0, 64, 64);
        Child.PixelWidth = 64;
        Child.PixelHeight = 64;
        Child.RGBAPixels.SetNumZeroed(64 * 64 * 4);
        return Child;
    }
}

void FButtonLayerMapperSpec::Define()
{
    Describe("BTN-STATE-01: @button @variants conflict resolution", [this]()
    {
        It("FVariantsSuffixMapper rejects @button @variants layer (HasType guard)", [this]()
        {
            FPsdLayer L = PSD2UMG::Tests::MakeTaggedTestLayer(TEXT("MyBtn @button @variants"), EPsdLayerType::Group);
            TestFalse(TEXT("VariantsMapper rejects typed layer"), VariantsMapper.CanMap(L));
        });

        It("FButtonLayerMapper accepts @button @variants layer", [this]()
        {
            FPsdLayer L = PSD2UMG::Tests::MakeTaggedTestLayer(TEXT("MyBtn @button @variants"), EPsdLayerType::Group);
            TestTrue(TEXT("ButtonMapper accepts typed layer"), ButtonMapper.CanMap(L));
        });

        It("Parsed tags: Type==Button and bIsVariants==true", [this]()
        {
            FPsdLayer L = PSD2UMG::Tests::MakeTaggedTestLayer(TEXT("MyBtn @button @variants"), EPsdLayerType::Group);
            TestEqual(TEXT("Type=Button"), static_cast<int32>(L.ParsedTags.Type), static_cast<int32>(EPsdTagType::Button));
            TestTrue(TEXT("bIsVariants"), L.ParsedTags.bIsVariants);
        });

        It("FVariantsSuffixMapper still accepts bare @variants group (no over-reject)", [this]()
        {
            FPsdLayer L = PSD2UMG::Tests::MakeTaggedTestLayer(TEXT("MyGroup @variants"), EPsdLayerType::Group);
            TestTrue(TEXT("VariantsMapper accepts bare @variants"), VariantsMapper.CanMap(L));
            TestFalse(TEXT("ButtonMapper rejects bare @variants"), ButtonMapper.CanMap(L));
        });
    });

    Describe("BTN-STATE-02: state child slot wiring", [this]()
    {
        It("All four @state:* children wire to FButtonStyle Normal/Hovered/Pressed/Disabled slots", [this]()
        {
            FPsdLayer Layer = PSD2UMG::Tests::MakeTaggedTestLayer(TEXT("MyBtn @button"), EPsdLayerType::Group);
            AddStateChild(Layer, TEXT("N @state:normal"));
            AddStateChild(Layer, TEXT("H @state:hover"));
            AddStateChild(Layer, TEXT("P @state:pressed"));
            AddStateChild(Layer, TEXT("D @state:disabled"));
            // Children were added after MakeTaggedTestLayer — populate their ParsedTags.
            PSD2UMG::Tests::PopulateParsedTagsRecursive(Layer.Children);

            UWidgetBlueprint* WBP = NewObject<UWidgetBlueprint>();
            UWidgetTree* Tree = NewObject<UWidgetTree>(WBP);

            FPsdDocument Doc;
            Doc.SourcePath = TEXT("C:/test/ButtonSpec.psd");

            UWidget* Result = ButtonMapper.Map(Layer, Doc, Tree);
            UButton* Btn = Cast<UButton>(Result);
            TestNotNull(TEXT("Result is UButton"), Btn);
            if (!Btn) { return; }

            const FButtonStyle& Style = Btn->GetStyle();
            TestNotNull(TEXT("Normal slot populated"),   Style.Normal.GetResourceObject());
            TestNotNull(TEXT("Hovered slot populated"),  Style.Hovered.GetResourceObject());
            TestNotNull(TEXT("Pressed slot populated"),  Style.Pressed.GetResourceObject());
            TestNotNull(TEXT("Disabled slot populated"), Style.Disabled.GetResourceObject());
        });

        It("Partial state children: import succeeds, missing slots remain default, no abort", [this]()
        {
            FPsdLayer Layer = PSD2UMG::Tests::MakeTaggedTestLayer(TEXT("MyBtn @button"), EPsdLayerType::Group);
            AddStateChild(Layer, TEXT("N @state:normal"));
            PSD2UMG::Tests::PopulateParsedTagsRecursive(Layer.Children);

            UWidgetBlueprint* WBP = NewObject<UWidgetBlueprint>();
            UWidgetTree* Tree = NewObject<UWidgetTree>(WBP);

            FPsdDocument Doc;
            Doc.SourcePath = TEXT("C:/test/ButtonSpecPartial.psd");

            UWidget* Result = ButtonMapper.Map(Layer, Doc, Tree);
            UButton* Btn = Cast<UButton>(Result);
            TestNotNull(TEXT("Result is UButton even with partial state"), Btn);
            if (!Btn) { return; }

            const FButtonStyle& Style = Btn->GetStyle();
            TestNotNull(TEXT("Normal slot populated"),          Style.Normal.GetResourceObject());
            TestNull(TEXT("Hovered slot remains default/null"),  Style.Hovered.GetResourceObject());
            TestNull(TEXT("Pressed slot remains default/null"),  Style.Pressed.GetResourceObject());
            TestNull(TEXT("Disabled slot remains default/null"), Style.Disabled.GetResourceObject());
        });
    });
}

#endif // WITH_DEV_AUTOMATION_TESTS

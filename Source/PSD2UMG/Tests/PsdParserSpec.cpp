// Copyright 2018-2021 - John snow wind

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"
#include "Framework/Text/TextLayout.h"

#include "Parser/PsdParser.h"
#include "Parser/PsdTypes.h"
#include "Parser/PsdDiagnostics.h"

/**
 * FPsdParserSpec — Automation Spec that drives PSD2UMG::Parser::ParseFile
 * against a hand-crafted fixture PSD at
 * Source/PSD2UMG/Tests/Fixtures/MultiLayer.psd.
 *
 * See .planning/phases/02-c-psd-parser/02-05-PLAN.md for the fixture
 * contract this spec asserts against.
 */
BEGIN_DEFINE_SPEC(FPsdParserSpec, "PSD2UMG.Parser.MultiLayer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

    FString FixturePath;
    FPsdDocument Doc;
    FPsdParseDiagnostics Diag;
    bool bParsed = false;

    static const FPsdLayer* FindLayerByName(const TArray<FPsdLayer>& Layers, const FString& Name)
    {
        for (const FPsdLayer& L : Layers)
        {
            if (L.Name.Equals(Name, ESearchCase::CaseSensitive))
            {
                return &L;
            }
        }
        return nullptr;
    }

    static bool ColorsNearlyEqual(const FLinearColor& A, const FLinearColor& B, float Tol = 0.05f)
    {
        return FMath::IsNearlyEqual(A.R, B.R, Tol)
            && FMath::IsNearlyEqual(A.G, B.G, Tol)
            && FMath::IsNearlyEqual(A.B, B.B, Tol);
    }

END_DEFINE_SPEC(FPsdParserSpec)

void FPsdParserSpec::Define()
{
    BeforeEach([this]()
    {
        Doc = FPsdDocument();
        Diag = FPsdParseDiagnostics();
        bParsed = false;

        TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("PSD2UMG"));
        if (!Plugin.IsValid())
        {
            AddError(TEXT("PSD2UMG plugin not found via IPluginManager"));
            return;
        }

        FixturePath = FPaths::Combine(
            Plugin->GetBaseDir(),
            TEXT("Source/PSD2UMG/Tests/Fixtures/MultiLayer.psd"));

        if (!FPaths::FileExists(FixturePath))
        {
            AddError(FString::Printf(TEXT("Fixture PSD missing: %s"), *FixturePath));
            return;
        }

        bParsed = PSD2UMG::Parser::ParseFile(FixturePath, Doc, Diag);
    });

    Describe("ParseFile on MultiLayer.psd", [this]()
    {
        It("returns true with no error diagnostics", [this]()
        {
            TestTrue(TEXT("bParsed"), bParsed);
            TestFalse(TEXT("Diag.HasErrors()"), Diag.HasErrors());
        });

        It("produces the expected canvas size (256x128)", [this]()
        {
            TestEqual(TEXT("CanvasSize.X"), Doc.CanvasSize.X, 256);
            TestEqual(TEXT("CanvasSize.Y"), Doc.CanvasSize.Y, 128);
        });

        It("has 3 root layers", [this]()
        {
            TestEqual(TEXT("RootLayers.Num"), Doc.RootLayers.Num(), 3);
        });

        It("contains a Title text layer with the expected content and style", [this]()
        {
            const FPsdLayer* Title = FindLayerByName(Doc.RootLayers, TEXT("Title"));
            if (!TestNotNull(TEXT("Title layer"), Title)) return;
            TestEqual(TEXT("Title.Type"), (int32)Title->Type, (int32)EPsdLayerType::Text);
            TestTrue(TEXT("Title.bVisible"), Title->bVisible);
            TestTrue(TEXT("Title.Opacity ~= 1.0"), FMath::IsNearlyEqual(Title->Opacity, 1.f, 0.02f));
            TestEqual(TEXT("Title.Text.Content"), Title->Text.Content, FString(TEXT("Hello PSD2UMG")));
            TestFalse(TEXT("Title.Text.FontName non-empty"), Title->Text.FontName.IsEmpty());
            TestTrue(TEXT("Title.Text.SizePx > 0"), Title->Text.SizePx > 0.f);
            TestTrue(TEXT("Title.Text.Color ~= Red"),
                ColorsNearlyEqual(Title->Text.Color, FLinearColor::Red, 0.05f));
            TestEqual(TEXT("Title.Text.Alignment == Left"),
                (int32)Title->Text.Alignment.GetValue(), (int32)ETextJustify::Left);
        });

        It("contains a Buttons group with 2 children", [this]()
        {
            const FPsdLayer* Buttons = FindLayerByName(Doc.RootLayers, TEXT("Buttons"));
            if (!TestNotNull(TEXT("Buttons layer"), Buttons)) return;
            TestEqual(TEXT("Buttons.Type"), (int32)Buttons->Type, (int32)EPsdLayerType::Group);
            TestEqual(TEXT("Buttons.Children.Num"), Buttons->Children.Num(), 2);
        });

        It("contains BtnNormal image child (visible, 64x32, RGBA populated)", [this]()
        {
            const FPsdLayer* Buttons = FindLayerByName(Doc.RootLayers, TEXT("Buttons"));
            if (!TestNotNull(TEXT("Buttons layer"), Buttons)) return;
            const FPsdLayer* Btn = FindLayerByName(Buttons->Children, TEXT("BtnNormal"));
            if (!TestNotNull(TEXT("BtnNormal layer"), Btn)) return;
            TestEqual(TEXT("BtnNormal.Type"), (int32)Btn->Type, (int32)EPsdLayerType::Image);
            TestTrue(TEXT("BtnNormal.bVisible"), Btn->bVisible);
            TestTrue(TEXT("BtnNormal.Opacity ~= 1.0"),
                FMath::IsNearlyEqual(Btn->Opacity, 1.f, 0.02f));
            TestEqual(TEXT("BtnNormal.PixelWidth"), Btn->PixelWidth, 64);
            TestEqual(TEXT("BtnNormal.PixelHeight"), Btn->PixelHeight, 32);
            TestEqual(TEXT("BtnNormal.RGBAPixels.Num"),
                Btn->RGBAPixels.Num(), 64 * 32 * 4);
        });

        It("contains BtnHover image child (hidden, opacity ~0.5)", [this]()
        {
            const FPsdLayer* Buttons = FindLayerByName(Doc.RootLayers, TEXT("Buttons"));
            if (!TestNotNull(TEXT("Buttons layer"), Buttons)) return;
            const FPsdLayer* Btn = FindLayerByName(Buttons->Children, TEXT("BtnHover"));
            if (!TestNotNull(TEXT("BtnHover layer"), Btn)) return;
            TestEqual(TEXT("BtnHover.Type"), (int32)Btn->Type, (int32)EPsdLayerType::Image);
            TestFalse(TEXT("BtnHover.bVisible"), Btn->bVisible);
            TestTrue(TEXT("BtnHover.Opacity ~= 0.5"),
                FMath::IsNearlyEqual(Btn->Opacity, 0.5f, 0.05f));
        });

        It("contains a Background image layer sized to the canvas", [this]()
        {
            const FPsdLayer* Bg = FindLayerByName(Doc.RootLayers, TEXT("Background"));
            if (!TestNotNull(TEXT("Background layer"), Bg)) return;
            TestEqual(TEXT("Background.Type"), (int32)Bg->Type, (int32)EPsdLayerType::Image);
            TestTrue(TEXT("Background.bVisible"), Bg->bVisible);
            TestEqual(TEXT("Background.PixelWidth"), Bg->PixelWidth, 256);
            TestEqual(TEXT("Background.PixelHeight"), Bg->PixelHeight, 128);
            TestEqual(TEXT("Background.RGBAPixels.Num"),
                Bg->RGBAPixels.Num(), 256 * 128 * 4);
        });
    });
}

#endif // WITH_DEV_AUTOMATION_TESTS

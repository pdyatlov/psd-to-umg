// Copyright 2018-2021 - John snow wind

#include "Mapper/AllMappers.h"
#include "Mapper/FontResolver.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGSetting.h"
#include "PSD2UMGLog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Math/UnrealMathUtility.h"
#include "Styling/SlateColor.h"

int32 FTextLayerMapper::GetPriority() const { return 100; }

bool FTextLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    // Phase 16: single-run text only. Multi-run layers (Spans.Num() > 1) route to
    // FRichTextLayerMapper at priority 110. Spans.Num() <= 1 covers both 0 (parser
    // left the array empty, legacy path) and 1 (edge case -- one run after sentinel
    // strip; dominant-run scalars already capture it so UTextBlock suffices).
    return Layer.ParsedTags.Type == EPsdTagType::Text
        && Layer.Text.Spans.Num() <= 1;
}

UWidget* FTextLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    UTextBlock* TextWidget = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*Layer.ParsedTags.CleanName));

    // Content.
    TextWidget->SetText(FText::FromString(Layer.Text.Content));

    // DPI conversion: PhotoshopAPI returns designer_pt * (4/3) for font sizes.
    // Multiplying by 0.75 (= 3/4) recovers the designer's intended pt value.
    // E.g. 24pt designer → raw 32.0 → 32 * 0.75 = 24 UMG.
    const float UmgSize = Layer.Text.SizePx * 0.75f;
    UE_LOG(LogPSD2UMG, Log,
        TEXT("FTextLayerMapper: layer '%s' SizePx=%.2f → UmgSize=%.2f → rounded=%d"),
        *Layer.Name, Layer.Text.SizePx, UmgSize, FMath::RoundToInt(UmgSize));

    // Resolve font via settings (TEXT-05) + apply bold/italic typeface (TEXT-02).
    const UPSD2UMGSettings* Settings = UPSD2UMGSettings::Get();
    const PSD2UMG::FFontResolution Resolved = PSD2UMG::FFontResolver::Resolve(
        Layer.Text.FontName,
        Layer.Text.bBold,
        Layer.Text.bItalic,
        Settings);

    FSlateFontInfo FontInfo = TextWidget->GetFont();
    if (Resolved.Font != nullptr)
    {
        FontInfo.FontObject = Resolved.Font;
        // Reset typeface before applying new one (per 04-CONTEXT.md specifics).
        FontInfo.TypefaceFontName = NAME_None;

        // Verify the resolved font actually exposes the requested typeface.
        if (Resolved.TypefaceName != NAME_None)
        {
            bool bTypefaceFound = false;
            if (const FCompositeFont* Composite = Resolved.Font->GetCompositeFont())
            {
                for (const FTypefaceEntry& Entry : Composite->DefaultTypeface.Fonts)
                {
                    if (Entry.Name == Resolved.TypefaceName)
                    {
                        bTypefaceFound = true;
                        break;
                    }
                }
            }

            if (bTypefaceFound)
            {
                FontInfo.TypefaceFontName = Resolved.TypefaceName;
            }
            else
            {
                UE_LOG(LogPSD2UMG, Warning,
                    TEXT("Font '%s' has no typeface '%s'; using default"),
                    *Resolved.Font->GetName(), *Resolved.TypefaceName.ToString());
            }
        }
    }
    else if (Resolved.TypefaceName != NAME_None)
    {
        // No custom font but style flags present — apply typeface to engine default.
        FontInfo.TypefaceFontName = Resolved.TypefaceName;
    }
    else
    {
        // No custom font, no style flags — explicitly reset to "Regular" to avoid
        // inheriting a non-Regular typeface from the CDO or blueprint compilation defaults.
        FontInfo.TypefaceFontName = FName(TEXT("Regular"));
    }

    FontInfo.Size = FMath::RoundToInt(UmgSize);
    TextWidget->SetFont(FontInfo);

    // TEXT-03 — outline via FSlateFontInfo::OutlineSettings.
    // OutlineSize is in pixels in PhotoshopAPI; apply the same 0.75 DPI conversion
    // we use for font size.
    if (Layer.Text.OutlineSize > 0.f)
    {
        FSlateFontInfo OutlineInfo = TextWidget->GetFont();
        OutlineInfo.OutlineSettings.OutlineSize = FMath::RoundToInt(Layer.Text.OutlineSize * 0.75f);
        OutlineInfo.OutlineSettings.OutlineColor = Layer.Text.OutlineColor;
        TextWidget->SetFont(OutlineInfo);
    }

    // TEXT-06 — AutoWrapText on paragraph (box) text only. Point text layers keep
    // wrap disabled so short button/title labels never wrap unexpectedly.
    TextWidget->SetAutoWrapText(Layer.Text.bHasExplicitWidth);

    // TEXT-04 — drop shadow via UTextBlock native API (D-08).
    // DPI conversion (x0.75) applied here to match the outline pattern above (D-09 Option B).
    // FPsdTextRun.ShadowOffset/ShadowColor are raw PSD pixels / linear color populated by
    // PsdParser::RouteTextEffects when the text layer had a PS Drop Shadow effect.
    if (!Layer.Text.ShadowOffset.IsZero() || Layer.Text.ShadowColor.A > 0.f)
    {
        TextWidget->SetShadowOffset(Layer.Text.ShadowOffset * 0.75);
        TextWidget->SetShadowColorAndOpacity(Layer.Text.ShadowColor);
    }

    // TXT-FX-01 — Color Overlay priority for text.
    // PsdParser::RouteTextEffects (Phase 12) already copies Effects.ColorOverlayColor
    // into Text.Color and clears bHasColorOverlay (D-13 double-render guard) for every
    // text layer. By the time we get here, Text.Color ALWAYS carries the final
    // designer-intended display color (overlay if present, fill otherwise) and
    // Effects.bHasColorOverlay is ALWAYS false for text.
    //
    // The ternary below is therefore dead in the true branch at runtime — it is kept
    // intentionally as a defensive belt-and-braces guard so that if a future change
    // ever removes or skips RouteTextEffects, overlay priority is preserved here
    // instead of silently regressing to the underlying fill color. Do not "simplify"
    // this to `Layer.Text.Color` without first re-validating PsdParserSpec
    // "text_overlay_gray" and FTextPipelineSpec "text_overlay_gray uses OVERLAY color".
    const FLinearColor& TextColor = Layer.Effects.bHasColorOverlay
        ? Layer.Effects.ColorOverlayColor
        : Layer.Text.Color;
    TextWidget->SetColorAndOpacity(FSlateColor(TextColor));

    // Justification (Phase 3 baseline).
    TextWidget->SetJustification(Layer.Text.Alignment);

    return TextWidget;
}

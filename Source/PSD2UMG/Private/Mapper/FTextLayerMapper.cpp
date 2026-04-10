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
    return Layer.Type == EPsdLayerType::Text;
}

UWidget* FTextLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    UTextBlock* TextWidget = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*Layer.Name));

    // Content.
    TextWidget->SetText(FText::FromString(Layer.Text.Content));

    // DPI conversion: Photoshop 72 DPI -> UMG 96 DPI (multiply by 0.75).
    // TEXT-01 — already shipped Phase 3, preserved verbatim.
    const float UmgSize = Layer.Text.SizePx * 0.75f;

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

    // TEXT-04 — drop shadow DEFERRED.
    // PhotoshopAPI v0.9 does not expose text drop-shadow effect data (verified
    // against vendored headers in .planning/phases/04-text-fonts-typography/
    // 04-RESEARCH.md). The requirement is tracked as a partial delivery; a
    // follow-up decimal phase (4.1) will close the gap when either PhotoshopAPI
    // adds support or manual TySh descriptor parsing is implemented.
    // Intentionally no shadow offset or shadow color calls here.

    // Color (Phase 3 baseline).
    TextWidget->SetColorAndOpacity(FSlateColor(Layer.Text.Color));

    // Justification (Phase 3 baseline).
    TextWidget->SetJustification(Layer.Text.Alignment);

    return TextWidget;
}

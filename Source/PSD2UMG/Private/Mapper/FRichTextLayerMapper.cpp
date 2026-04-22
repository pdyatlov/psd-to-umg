// Copyright 2018-2021 - John snow wind
//
// Phase 16 / RICH-01, RICH-02: Multi-run rich text mapper.
//
// Emits URichTextBlock when Layer.Text.Spans.Num() > 1 (multi-style text layer).
// Builds inline markup (<s0>...</><s1>...</>) and a persistent UDataTable companion
// asset holding per-span FTextBlockStyle rows. DataTable asset path is sibling to
// the WBP: {WbpPackagePath}/{LayerCleanName}_RichStyles/DT_{CleanName}_RichStyles.
//
// Priority 110 -- above FTextLayerMapper (100). FTextLayerMapper::CanMap narrows to
// Spans.Num() <= 1 so the two mappers are mutually exclusive.

#include "Mapper/AllMappers.h"
#include "Mapper/FontResolver.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGSetting.h"
#include "PSD2UMGLog.h"
#include "Generator/FSmartObjectImporter.h"  // thread-local current WBP package path

#include "Blueprint/WidgetTree.h"
#include "Components/RichTextBlock.h"
#include "Engine/DataTable.h"
#include "Engine/Font.h"
#include "EditorAssetLibrary.h"
#include "Math/UnrealMathUtility.h"
#include "Styling/SlateColor.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

namespace
{
    // HTML-escape markup metacharacters in span text (research Pattern 4).
    // & -> &amp; must run FIRST so subsequent < -> &lt; does not double-escape &amp; to &amp;amp;.
    static FString EscapeMarkup(const FString& In)
    {
        return In
            .Replace(TEXT("&"), TEXT("&amp;"))
            .Replace(TEXT("<"), TEXT("&lt;"))
            .Replace(TEXT(">"), TEXT("&gt;"));
    }

    // Build FTextBlockStyle for a span (or the Default row when Span == nullptr).
    // When Span is null, this builds the Default row from FPsdTextRun dominant-run scalars.
    // When Span is non-null, per-span fields override the dominant scalars.
    //
    // DPI conversion (* 0.75f) matches FTextLayerMapper.cpp line 32 -- both the Default
    // row and per-span rows use identical SizePx -> UMG pt conversion.
    static FTextBlockStyle BuildTextBlockStyle(
        const FPsdTextRun& Run,
        const FPsdTextRunSpan* Span,  // nullptr = Default row
        const UPSD2UMGSettings* Settings)
    {
        const FString& FontName = Span ? Span->FontName    : Run.FontName;
        const bool     bBold    = Span ? Span->bBold       : Run.bBold;
        const bool     bItalic  = Span ? Span->bItalic     : Run.bItalic;
        const float    SizePx   = Span ? Span->SizePx      : Run.SizePx;
        const FLinearColor Col  = Span ? Span->Color       : Run.Color;

        FTextBlockStyle Style;

        // Font via FFontResolver (same helper FTextLayerMapper uses)
        const PSD2UMG::FFontResolution Resolved = PSD2UMG::FFontResolver::Resolve(
            FontName, bBold, bItalic, Settings);

        FSlateFontInfo FontInfo;
        if (Resolved.Font != nullptr)
        {
            FontInfo.FontObject       = Resolved.Font;
            FontInfo.TypefaceFontName = NAME_None;

            if (Resolved.TypefaceName != NAME_None)
            {
                bool bTypefaceFound = false;
                if (const FCompositeFont* Composite = Resolved.Font->GetCompositeFont())
                {
                    for (const FTypefaceEntry& Entry : Composite->DefaultTypeface.Fonts)
                    {
                        if (Entry.Name == Resolved.TypefaceName) { bTypefaceFound = true; break; }
                    }
                }
                FontInfo.TypefaceFontName = bTypefaceFound ? Resolved.TypefaceName : FName(TEXT("Regular"));
            }
            else
            {
                FontInfo.TypefaceFontName = FName(TEXT("Regular"));
            }
        }
        else if (Resolved.TypefaceName != NAME_None)
        {
            FontInfo.TypefaceFontName = Resolved.TypefaceName;
        }
        else
        {
            FontInfo.TypefaceFontName = FName(TEXT("Regular"));
        }

        // DPI conversion (same factor as FTextLayerMapper.cpp line 32).
        FontInfo.Size = FMath::RoundToInt(SizePx * 0.75f);

        Style.SetFont(FontInfo);
        Style.SetColorAndOpacity(FSlateColor(Col));

        return Style;
    }

    // Build the DataTable package path. Uses the thread-local WBP path that the generator
    // sets via FSmartObjectImporter::SetCurrentPackagePath before PopulateChildren runs.
    // Fallback to /Game/UI if the thread-local is empty (should not happen during a real import).
    static FString BuildStyleTablePackagePath(const FPsdLayer& Layer)
    {
        const FString WbpPath = FSmartObjectImporter::GetCurrentPackagePath();
        const FString BasePath = WbpPath.IsEmpty() ? FString(TEXT("/Game/UI")) : WbpPath;
        // Sibling sub-folder: {WbpPackagePath}/{LayerCleanName}_RichStyles/DT_{CleanName}_RichStyles
        FString Clean = Layer.ParsedTags.CleanName.IsEmpty() ? Layer.Name : Layer.ParsedTags.CleanName;
        for (TCHAR& Ch : Clean)
        {
            if (!FChar::IsAlnum(Ch) && Ch != TEXT('_')) { Ch = TEXT('_'); }
        }
        const FString Folder    = FString::Printf(TEXT("%s_RichStyles"), *Clean);
        const FString AssetName = FString::Printf(TEXT("DT_%s_RichStyles"), *Clean);
        return BasePath / Folder / AssetName;
    }

    // Create (and persist) a UDataTable sibling to the WBP containing Default + s0..sN rows.
    static UDataTable* CreateStyleTableAsset(
        const FPsdLayer& Layer,
        const UPSD2UMGSettings* Settings)
    {
        const FString PackagePath = BuildStyleTablePackagePath(Layer);
        int32 LastSlash = INDEX_NONE;
        PackagePath.FindLastChar(TEXT('/'), LastSlash);
        const FString AssetName = (LastSlash != INDEX_NONE) ? PackagePath.Mid(LastSlash + 1) : PackagePath;

        UPackage* Pkg = CreatePackage(*PackagePath);
        if (!Pkg)
        {
            UE_LOG(LogPSD2UMG, Warning,
                TEXT("FRichTextLayerMapper: CreatePackage failed for %s"), *PackagePath);
            return nullptr;
        }
        Pkg->FullyLoad();

        // Dedup on collision (two layers with same CleanName in different groups).
        FString UniqueAssetName = AssetName;
        if (FindObject<UDataTable>(Pkg, *UniqueAssetName) != nullptr)
        {
            for (int32 Suffix = 1; Suffix < 100; ++Suffix)
            {
                const FString CandidatePath = FString::Printf(TEXT("%s_%d"), *PackagePath, Suffix);
                Pkg = CreatePackage(*CandidatePath);
                if (!Pkg) continue;
                Pkg->FullyLoad();
                UniqueAssetName = FString::Printf(TEXT("%s_%d"), *AssetName, Suffix);
                if (FindObject<UDataTable>(Pkg, *UniqueAssetName) == nullptr) break;
            }
        }

        UDataTable* Table = NewObject<UDataTable>(Pkg, FName(*UniqueAssetName), RF_Public | RF_Standalone);
        if (!Table)
        {
            UE_LOG(LogPSD2UMG, Warning,
                TEXT("FRichTextLayerMapper: NewObject<UDataTable> failed for %s"), *UniqueAssetName);
            return nullptr;
        }
        Table->RowStruct = FRichTextStyleRow::StaticStruct();

        // "Default" row from dominant-run scalars (used for text NOT wrapped in a tag).
        {
            FRichTextStyleRow DefaultRow;
            DefaultRow.TextStyle = BuildTextBlockStyle(Layer.Text, /*Span=*/nullptr, Settings);
            Table->AddRow(FName(TEXT("Default")), DefaultRow);
        }

        // One row per span (s0, s1, ...).
        for (int32 i = 0; i < Layer.Text.Spans.Num(); ++i)
        {
            FRichTextStyleRow SpanRow;
            SpanRow.TextStyle = BuildTextBlockStyle(Layer.Text, &Layer.Text.Spans[i], Settings);
            const FString RowName = FString::Printf(TEXT("s%d"), i);
            Table->AddRow(FName(*RowName), SpanRow);
        }

        Pkg->MarkPackageDirty();
        UEditorAssetLibrary::SaveLoadedAsset(Table, /*bShowConfirmation=*/false);

        UE_LOG(LogPSD2UMG, Log,
            TEXT("FRichTextLayerMapper: Persisted style DataTable '%s' with %d rows (Default + %d spans)"),
            *UniqueAssetName, Layer.Text.Spans.Num() + 1, Layer.Text.Spans.Num());

        return Table;
    }

    // Build the inline markup: <s0>esc(Spans[0].Text)</><s1>esc(Spans[1].Text)</>...
    static FString BuildMarkup(const TArray<FPsdTextRunSpan>& Spans)
    {
        FString Out;
        for (int32 i = 0; i < Spans.Num(); ++i)
        {
            const FString Escaped = EscapeMarkup(Spans[i].Text);
            Out += FString::Printf(TEXT("<s%d>%s</>"), i, *Escaped);
        }
        return Out;
    }
} // anonymous namespace

int32 FRichTextLayerMapper::GetPriority() const { return 110; }

bool FRichTextLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.ParsedTags.Type == EPsdTagType::Text
        && Layer.Text.Spans.Num() > 1;
}

UWidget* FRichTextLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    URichTextBlock* RTB = Tree->ConstructWidget<URichTextBlock>(
        URichTextBlock::StaticClass(),
        FName(*Layer.ParsedTags.CleanName));

    const UPSD2UMGSettings* Settings = UPSD2UMGSettings::Get();

    // 1. Persist the style DataTable as a UAsset sibling to the WBP.
    if (UDataTable* Table = CreateStyleTableAsset(Layer, Settings))
    {
        RTB->SetTextStyleSet(Table);
    }
    else
    {
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("FRichTextLayerMapper: style DataTable creation failed for '%s' -- URichTextBlock will use engine defaults"),
            *Layer.Name);
    }

    // 2. Build + set the markup string.
    const FString Markup = BuildMarkup(Layer.Text.Spans);
    RTB->SetText(FText::FromString(Markup));

    // 3. Layer-level properties (same as FTextLayerMapper for consistency).
    RTB->SetAutoWrapText(Layer.Text.bHasExplicitWidth);
    // URichTextBlock::SetJustification takes ETextJustify::Type. Layer.Text.Alignment is
    // TEnumAsByte<ETextJustify::Type>, so extract the underlying enum value explicitly.
    RTB->SetJustification(static_cast<ETextJustify::Type>(Layer.Text.Alignment.GetValue()));

    UE_LOG(LogPSD2UMG, Log,
        TEXT("FRichTextLayerMapper: '%s' -> URichTextBlock (%d spans, markup len=%d)"),
        *Layer.Name, Layer.Text.Spans.Num(), Markup.Len());

    return RTB;
}

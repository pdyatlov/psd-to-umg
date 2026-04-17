// Copyright 2018-2021 - John snow wind

#include "UI/SPsdImportPreviewDialog.h"

#include "Parser/FLayerTagParser.h"
#include "Parser/PsdTypes.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STreeView.h"
#include "Styling/AppStyle.h"

#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Framework/Application/SlateApplication.h"
#include "Internationalization/Text.h"

#define LOCTEXT_NAMESPACE "PSD2UMG"

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

namespace
{
    const TCHAR* TagTypeToken(EPsdTagType Type)
    {
        switch (Type)
        {
        case EPsdTagType::Button:      return TEXT("button");
        case EPsdTagType::Image:       return TEXT("image");
        case EPsdTagType::Text:        return TEXT("text");
        case EPsdTagType::Progress:    return TEXT("progress");
        case EPsdTagType::HBox:        return TEXT("hbox");
        case EPsdTagType::VBox:        return TEXT("vbox");
        case EPsdTagType::Overlay:     return TEXT("overlay");
        case EPsdTagType::ScrollBox:   return TEXT("scrollbox");
        case EPsdTagType::Slider:      return TEXT("slider");
        case EPsdTagType::CheckBox:    return TEXT("checkbox");
        case EPsdTagType::Input:       return TEXT("input");
        case EPsdTagType::List:        return TEXT("list");
        case EPsdTagType::Tile:        return TEXT("tile");
        case EPsdTagType::SmartObject: return TEXT("smartobject");
        case EPsdTagType::Canvas:      return TEXT("canvas");
        case EPsdTagType::None:
        default:                       return nullptr;
        }
    }

    const TCHAR* AnchorTagToken(EPsdAnchorTag Anchor)
    {
        switch (Anchor)
        {
        case EPsdAnchorTag::TL:       return TEXT("tl");
        case EPsdAnchorTag::TC:       return TEXT("tc");
        case EPsdAnchorTag::TR:       return TEXT("tr");
        case EPsdAnchorTag::CL:       return TEXT("cl");
        case EPsdAnchorTag::C:        return TEXT("c");
        case EPsdAnchorTag::CR:       return TEXT("cr");
        case EPsdAnchorTag::BL:       return TEXT("bl");
        case EPsdAnchorTag::BC:       return TEXT("bc");
        case EPsdAnchorTag::BR:       return TEXT("br");
        case EPsdAnchorTag::StretchH: return TEXT("stretch-h");
        case EPsdAnchorTag::StretchV: return TEXT("stretch-v");
        case EPsdAnchorTag::Fill:     return TEXT("fill");
        case EPsdAnchorTag::None:
        default:                      return nullptr;
        }
    }

    const TCHAR* AnimTagToken(EPsdAnimTag Anim)
    {
        switch (Anim)
        {
        case EPsdAnimTag::Show:  return TEXT("show");
        case EPsdAnimTag::Hide:  return TEXT("hide");
        case EPsdAnimTag::Hover: return TEXT("hover");
        case EPsdAnimTag::None:
        default:                 return nullptr;
        }
    }

    const TCHAR* StateTagToken(EPsdStateTag State)
    {
        switch (State)
        {
        case EPsdStateTag::Normal:   return TEXT("normal");
        case EPsdStateTag::Hover:    return TEXT("hover");
        case EPsdStateTag::Pressed:  return TEXT("pressed");
        case EPsdStateTag::Disabled: return TEXT("disabled");
        case EPsdStateTag::Fill:     return TEXT("fill");
        case EPsdStateTag::Bg:       return TEXT("bg");
        case EPsdStateTag::None:
        default:                     return nullptr;
        }
    }

    FString FormatFloat(float V)
    {
        // Emit as integer when whole, otherwise trimmed decimal — matches "@9s:16,16,16,16".
        if (FMath::IsNearlyEqual(V, FMath::RoundToFloat(V)))
        {
            return FString::Printf(TEXT("%d"), static_cast<int32>(FMath::RoundToFloat(V)));
        }
        FString S = FString::SanitizeFloat(V);
        return S;
    }
}

TArray<FString> SPsdImportPreviewDialog::ReconstructTagChips(const FParsedLayerTags& Tags)
{
    TArray<FString> Chips;

    if (const TCHAR* TypeTok = TagTypeToken(Tags.Type))
    {
        Chips.Add(FString::Printf(TEXT("@%s"), TypeTok));
    }

    if (const TCHAR* AnchorTok = AnchorTagToken(Tags.Anchor))
    {
        Chips.Add(FString::Printf(TEXT("@anchor:%s"), AnchorTok));
    }

    if (const TCHAR* AnimTok = AnimTagToken(Tags.Anim))
    {
        Chips.Add(FString::Printf(TEXT("@anim:%s"), AnimTok));
    }

    if (const TCHAR* StateTok = StateTagToken(Tags.State))
    {
        Chips.Add(FString::Printf(TEXT("@state:%s"), StateTok));
    }

    if (Tags.InputAction.IsSet())
    {
        Chips.Add(FString::Printf(TEXT("@ia:%s"), *Tags.InputAction.GetValue()));
    }

    if (Tags.NineSlice.IsSet())
    {
        const FPsdNineSliceMargin& M = Tags.NineSlice.GetValue();
        if (M.bExplicit)
        {
            Chips.Add(FString::Printf(TEXT("@9s:%s,%s,%s,%s"),
                *FormatFloat(M.L), *FormatFloat(M.T),
                *FormatFloat(M.R), *FormatFloat(M.B)));
        }
        else
        {
            Chips.Add(TEXT("@9s"));
        }
    }

    if (Tags.bIsVariants)
    {
        Chips.Add(TEXT("@variants"));
    }

    if (Tags.SmartObjectTypeName.IsSet())
    {
        Chips.Add(FString::Printf(TEXT("@smartobject:%s"), *Tags.SmartObjectTypeName.GetValue()));
    }

    return Chips;
}

FString SPsdImportPreviewDialog::InferWidgetTypeName(const FPsdLayer& Layer)
{
    // Tag-based dispatch (Phase 9 D-15 hard cutover — no legacy prefix fallback).
    if (Layer.ParsedTags.bIsVariants)
    {
        return TEXT("WidgetSwitcher");
    }

    switch (Layer.ParsedTags.Type)
    {
    case EPsdTagType::Button:      return TEXT("Button");
    case EPsdTagType::Progress:    return TEXT("ProgressBar");
    case EPsdTagType::List:        return TEXT("ListView");
    case EPsdTagType::Tile:        return TEXT("TileView");
    case EPsdTagType::HBox:        return TEXT("HorizontalBox");
    case EPsdTagType::VBox:        return TEXT("VerticalBox");
    case EPsdTagType::Overlay:     return TEXT("Overlay");
    case EPsdTagType::ScrollBox:   return TEXT("ScrollBox");
    case EPsdTagType::Slider:      return TEXT("Slider");
    case EPsdTagType::CheckBox:    return TEXT("CheckBox");
    case EPsdTagType::Input:       return TEXT("EditableTextBox");
    case EPsdTagType::SmartObject: return TEXT("UserWidget");
    case EPsdTagType::Canvas:      return TEXT("CanvasPanel");
    case EPsdTagType::Image:       return TEXT("Image");
    case EPsdTagType::Text:        return TEXT("TextBlock");
    case EPsdTagType::None:
    default:
        // Fallback when no tag yielded a Type (e.g. Unknown PSD layer kind).
        switch (Layer.Type)
        {
        case EPsdLayerType::Text:        return TEXT("TextBlock");
        case EPsdLayerType::Group:       return TEXT("CanvasPanel");
        case EPsdLayerType::Image:       return TEXT("Image");
        case EPsdLayerType::SmartObject: return TEXT("Image");
        default:                         return TEXT("Unknown");
        }
    }
}

FLinearColor SPsdImportPreviewDialog::BadgeColorForType(const FString& WidgetTypeName)
{
    if (WidgetTypeName.Contains(TEXT("Button")))
    {
        return FLinearColor(0.2f, 0.5f, 0.9f, 1.f);   // blue
    }
    if (WidgetTypeName.Contains(TEXT("Image")))
    {
        return FLinearColor(0.3f, 0.65f, 0.3f, 1.f);  // green
    }
    if (WidgetTypeName.Contains(TEXT("TextBlock")))
    {
        return FLinearColor(0.8f, 0.6f, 0.1f, 1.f);   // amber
    }
    if (WidgetTypeName.Contains(TEXT("CanvasPanel")))
    {
        return FLinearColor(0.5f, 0.5f, 0.5f, 1.f);   // grey
    }
    return FLinearColor(0.4f, 0.4f, 0.6f, 1.f);        // slate-blue (fallback)
}

void SPsdImportPreviewDialog::BuildTreeRecursive(
    const TArray<FPsdLayer>& Layers,
    TArray<TSharedPtr<FPsdLayerTreeItem>>& OutItems,
    int32 Depth,
    TWeakPtr<FPsdLayerTreeItem> Parent,
    bool bParentChecked)
{
    for (const FPsdLayer& Layer : Layers)
    {
        TSharedPtr<FPsdLayerTreeItem> Item = MakeShared<FPsdLayerTreeItem>();
        Item->LayerName    = Layer.Name;                                                         // D-07: raw name as skip key
        Item->DisplayName  = Layer.ParsedTags.CleanName.IsEmpty()                                // D-07: tag-free name for display
                           ? Layer.Name : Layer.ParsedTags.CleanName;
        Item->bLayerVisible = Layer.bVisible;                                                    // Phase 11: PSD source truth
        Item->bChecked     = bParentChecked && Layer.bVisible;                                   // D-01 + D-02: inherit parent state
        Item->WidgetTypeName = InferWidgetTypeName(Layer);
        Item->BadgeColor   = BadgeColorForType(Item->WidgetTypeName);
        Item->ChangeAnnotation = EPsdChangeAnnotation::None;
        Item->Depth        = Depth;
        Item->Parent       = Parent;
        Item->ParsedTags   = Layer.ParsedTags;

        if (Layer.Children.Num() > 0)
        {
            BuildTreeRecursive(Layer.Children, Item->Children, Depth + 1, Item, Item->bChecked); // D-02: propagate
        }

        OutItems.Add(Item);
    }
}

// ---------------------------------------------------------------------------
// BuildTreeFromDocument
// ---------------------------------------------------------------------------

TArray<TSharedPtr<FPsdLayerTreeItem>> SPsdImportPreviewDialog::BuildTreeFromDocument(
    const FPsdDocument& Doc)
{
    TArray<TSharedPtr<FPsdLayerTreeItem>> Result;
    BuildTreeRecursive(Doc.RootLayers, Result, 0, TWeakPtr<FPsdLayerTreeItem>());
    return Result;
}

// ---------------------------------------------------------------------------
// Construct
// ---------------------------------------------------------------------------

void SPsdImportPreviewDialog::Construct(const FArguments& InArgs)
{
    RootItems    = InArgs._RootItems;
    Diagnostics  = InArgs._Diagnostics;
    bIsReimport  = InArgs._bIsReimport;
    OnCancelled  = InArgs._OnCancelled;
    OnConfirmed  = InArgs._OnConfirmed;

    // Build diagnostics list (warnings + errors) for the expandable section
    int32 WarningCount = 0;
    int32 ErrorCount = 0;
    for (const FPsdDiagnostic& D : Diagnostics)
    {
        if (D.Severity == EPsdDiagnosticSeverity::Warning) { ++WarningCount; }
        else if (D.Severity == EPsdDiagnosticSeverity::Error) { ++ErrorCount; }
    }
    const bool bHasDiagnostics = (WarningCount + ErrorCount) > 0;

    TSharedRef<SVerticalBox> DiagList = SNew(SVerticalBox);
    if (bHasDiagnostics)
    {
        for (const FPsdDiagnostic& D : Diagnostics)
        {
            if (D.Severity == EPsdDiagnosticSeverity::Info) { continue; }

            const bool bIsError = (D.Severity == EPsdDiagnosticSeverity::Error);
            const FLinearColor Color = bIsError
                ? FLinearColor(0.95f, 0.35f, 0.25f, 1.f)
                : FLinearColor(0.95f, 0.80f, 0.25f, 1.f);
            const FString Prefix = bIsError ? TEXT("[ERROR] ") : TEXT("[WARN]  ");
            const FString Line = D.LayerName.IsEmpty()
                ? FString::Printf(TEXT("%s%s"), *Prefix, *D.Message)
                : FString::Printf(TEXT("%s%s: %s"), *Prefix, *D.LayerName, *D.Message);

            DiagList->AddSlot()
                .AutoHeight()
                .Padding(FMargin(4.f, 1.f))
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(Line))
                    .Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
                    .ColorAndOpacity(Color)
                    .AutoWrapText(true)
                ];
        }
    }

    const FText DiagHeader = FText::FromString(FString::Printf(
        TEXT("Diagnostics — %d warnings, %d errors"), WarningCount, ErrorCount));

    // Build tree view
    SAssignNew(TreeView, STreeView<TSharedPtr<FPsdLayerTreeItem>>)
        .TreeItemsSource(&RootItems)
        .OnGenerateRow(this, &SPsdImportPreviewDialog::OnGenerateRow)
        .OnGetChildren(this, &SPsdImportPreviewDialog::OnGetChildren)
        .SelectionMode(ESelectionMode::None);

    // Expand all items by default
    ExpandAll(RootItems);

    // Build output path box
    SAssignNew(OutputPathBox, SEditableTextBox)
        .Text(FText::FromString(InArgs._InitialOutputPath))
        .Font(FAppStyle::GetFontStyle(TEXT("NormalFont")));

    const FString ImportLabel = bIsReimport
        ? TEXT("Apply Reimport")
        : TEXT("Import");

    ChildSlot
    [
        SNew(SVerticalBox)

        // ---------------------------------------------------------------
        // Slot A — Layer Tree
        // ---------------------------------------------------------------
        + SVerticalBox::Slot()
        .FillHeight(1.f)
        .Padding(8.f)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
            .Padding(4.f)
            [
                SNew(SScrollBox)
                + SScrollBox::Slot()
                [
                    TreeView.ToSharedRef()
                ]
            ]
        ]

        // ---------------------------------------------------------------
        // Slot A2 — Diagnostics (collapsible, only if any warnings/errors)
        // ---------------------------------------------------------------
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FMargin(8.f, 0.f, 8.f, 4.f))
        [
            SNew(SExpandableArea)
            .Visibility(bHasDiagnostics ? EVisibility::Visible : EVisibility::Collapsed)
            .InitiallyCollapsed(ErrorCount == 0)  // auto-open when there are errors
            .AreaTitle(DiagHeader)
            .BodyContent()
            [
                SNew(SBox)
                .MaxDesiredHeight(160.f)
                [
                    SNew(SScrollBox)
                    + SScrollBox::Slot()
                    [
                        DiagList
                    ]
                ]
            ]
        ]

        // ---------------------------------------------------------------
        // Slot B — Output Path Row
        // ---------------------------------------------------------------
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FMargin(8.f, 4.f))
        [
            SNew(SVerticalBox)

            // Path row
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SHorizontalBox)

                // "Output Path:" label — fixed 88px
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SBox)
                    .WidthOverride(88.f)
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("OutputPathLabel", "Output Path:"))
                        .Font(FAppStyle::GetFontStyle(TEXT("NormalFont")))
                    ]
                ]

                // Editable path field — fills remaining space
                + SHorizontalBox::Slot()
                .FillWidth(1.f)
                .Padding(FMargin(4.f, 0.f))
                [
                    OutputPathBox.ToSharedRef()
                ]

                // Browse button — 28x28
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SBox)
                    .WidthOverride(28.f)
                    .HeightOverride(28.f)
                    [
                        SNew(SButton)
                        .ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
                        .OnClicked(this, &SPsdImportPreviewDialog::OnBrowseClicked)
                        .ToolTipText(LOCTEXT("BrowseTip", "Browse for output folder"))
                        [
                            SNew(SImage)
                            .Image(FAppStyle::GetBrush(TEXT("Icons.FolderOpen")))
                        ]
                    ]
                ]
            ]

            // Validation error text (conditionally visible)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(88.f + 4.f, 2.f, 0.f, 0.f))
            [
                SNew(STextBlock)
                .Text(LOCTEXT("PathError", "Path must start with /Game/ -- enter a valid content path."))
                .Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
                .ColorAndOpacity(FLinearColor(0.9f, 0.2f, 0.2f, 1.f))
                .Visibility(this, &SPsdImportPreviewDialog::GetPathErrorVisibility)
            ]
        ]

        // ---------------------------------------------------------------
        // Slot C — Button Bar
        // ---------------------------------------------------------------
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
            .Padding(FMargin(8.f, 4.f))
            [
                SNew(SHorizontalBox)

                // Fill spacer (pushes buttons to right)
                + SHorizontalBox::Slot()
                .FillWidth(1.f)
                [
                    SNew(SSpacer)
                ]

                // Cancel button
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(FMargin(4.f, 0.f))
                [
                    SNew(SButton)
                    .Text(LOCTEXT("CancelBtn", "Cancel"))
                    .TextStyle(FAppStyle::Get(), TEXT("NormalText"))
                    .OnClicked(this, &SPsdImportPreviewDialog::OnCancelClicked)
                ]

                // Import / Apply Reimport button
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(FMargin(4.f, 0.f))
                [
                    SNew(SButton)
                    .Text(FText::FromString(ImportLabel))
                    .TextStyle(FAppStyle::Get(), TEXT("NormalText"))
                    .ButtonStyle(FAppStyle::Get(), TEXT("PrimaryButton"))
                    .OnClicked(this, &SPsdImportPreviewDialog::OnImportClicked)
                ]
            ]
        ]
    ];
}

// ---------------------------------------------------------------------------
// STreeView delegates
// ---------------------------------------------------------------------------

TSharedRef<ITableRow> SPsdImportPreviewDialog::OnGenerateRow(
    TSharedPtr<FPsdLayerTreeItem> Item,
    const TSharedRef<STableViewBase>& OwnerTable)
{
    check(Item.IsValid());

    const float IndentPx = static_cast<float>(Item->Depth) * 8.f;

    // Change annotation text and color
    FText AnnotationText = FText::GetEmpty();
    FSlateColor AnnotationColor = FSlateColor::UseForeground();

    if (bIsReimport)
    {
        switch (Item->ChangeAnnotation)
        {
        case EPsdChangeAnnotation::New:
            AnnotationText = LOCTEXT("AnnotNew", "[new]");
            AnnotationColor = FLinearColor(0.3f, 0.75f, 0.3f, 1.f);
            break;
        case EPsdChangeAnnotation::Changed:
            AnnotationText = LOCTEXT("AnnotChanged", "[changed]");
            AnnotationColor = FLinearColor(0.9f, 0.65f, 0.1f, 1.f);
            break;
        case EPsdChangeAnnotation::Unchanged:
            AnnotationText = LOCTEXT("AnnotUnchanged", "[unchanged]");
            AnnotationColor = FSlateColor(FLinearColor(
                FAppStyle::GetSlateColor(TEXT("DefaultForeground")).GetSpecifiedColor().R,
                FAppStyle::GetSlateColor(TEXT("DefaultForeground")).GetSpecifiedColor().G,
                FAppStyle::GetSlateColor(TEXT("DefaultForeground")).GetSpecifiedColor().B,
                0.5f));
            break;
        default:
            break;
        }
    }

    return SNew(STableRow<TSharedPtr<FPsdLayerTreeItem>>, OwnerTable)
    [
        SNew(SHorizontalBox)

        // Col 0: Eye-closed icon for hidden PSD layers (Phase 11, D-03/D-04)
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(FMargin(2.f, 0.f))
        [
            SNew(SBox)
            .WidthOverride(20.f)
            .HeightOverride(16.f)
            [
                SNew(SImage)
                .Image(FAppStyle::GetBrush(TEXT("Layer.NotVisibleIcon16x")))
                .Visibility(Item->bLayerVisible ? EVisibility::Hidden : EVisibility::Visible)
                .ToolTipText(LOCTEXT("HiddenLayerTip", "This layer is hidden in Photoshop — unchecked by default."))
            ]
        ]

        // Col 1: Checkbox
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(FMargin(4.f, 0.f))
        .VAlign(VAlign_Center)
        [
            SNew(SCheckBox)
            .IsChecked(this, &SPsdImportPreviewDialog::GetCheckState, Item)
            .OnCheckStateChanged(this, &SPsdImportPreviewDialog::OnCheckStateChanged, Item)
        ]

        // Col 2: Indent spacer
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(SSpacer)
            .Size(FVector2D(IndentPx, 1.f))
        ]

        // Col 3: Layer name
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(FText::FromString(Item->DisplayName))
            .Font(FAppStyle::GetFontStyle(TEXT("NormalFont")))
        ]

        // Col 3b: Tag chips — recognized (neutral) + unknown (warning) — D-26/D-27
        + SHorizontalBox::Slot()
        .FillWidth(1.f)
        .VAlign(VAlign_Center)
        .Padding(FMargin(6.f, 0.f))
        [
            BuildTagChipsForItem(Item)
        ]

        // Col 4: Widget type badge — fixed 120px
        + SHorizontalBox::Slot()
        .AutoWidth()
        .HAlign(HAlign_Right)
        .VAlign(VAlign_Center)
        .Padding(FMargin(4.f, 0.f))
        [
            SNew(SBox)
            .WidthOverride(120.f)
            .HAlign(HAlign_Fill)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush(TEXT("RoundedWarning")))
                .BorderBackgroundColor(Item->BadgeColor)
                .Padding(FMargin(4.f, 2.f))
                .HAlign(HAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(Item->WidgetTypeName))
                    .Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
                    .ColorAndOpacity(FLinearColor::White)
                ]
            ]
        ]

        // Col 5: Change annotation — visible only in reimport mode
        + SHorizontalBox::Slot()
        .AutoWidth()
        .HAlign(HAlign_Right)
        .VAlign(VAlign_Center)
        .Padding(FMargin(4.f, 0.f))
        [
            SNew(SBox)
            .WidthOverride(80.f)
            .Visibility(bIsReimport ? EVisibility::Visible : EVisibility::Collapsed)
            [
                SNew(STextBlock)
                .Text(AnnotationText)
                .Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
                .ColorAndOpacity(AnnotationColor)
            ]
        ]
    ];
}

TSharedRef<SWidget> SPsdImportPreviewDialog::BuildTagChipsForItem(const TSharedPtr<FPsdLayerTreeItem>& Item) const
{
    if (!Item.IsValid())
    {
        return SNew(SSpacer);
    }

    const TArray<FString> RecognizedChips = ReconstructTagChips(Item->ParsedTags);
    const TArray<FString>& UnknownChips = Item->ParsedTags.UnknownTags;

    if (RecognizedChips.Num() == 0 && UnknownChips.Num() == 0)
    {
        return SNew(SSpacer);
    }

    TSharedRef<SWrapBox> Wrap = SNew(SWrapBox)
        .UseAllottedSize(true)
        .InnerSlotPadding(FVector2D(3.f, 2.f));

    const FLinearColor RecognizedColor(0.30f, 0.65f, 1.00f, 1.0f);
    const FLinearColor UnknownColor(1.00f, 0.30f, 0.30f, 1.0f);

    // Neutral chips — parser-recognized tags (D-26).
    for (const FString& Chip : RecognizedChips)
    {
        Wrap->AddSlot()
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush(TEXT("Border")))
            .BorderBackgroundColor(RecognizedColor)
            .Padding(FMargin(5.f, 1.f))
            [
                SNew(STextBlock)
                .Text(FText::FromString(Chip))
                .Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
                .ColorAndOpacity(RecognizedColor)
            ]
        ];
    }

    // Warning chips — unknown / invalid tags (D-27). Tooltip carries the raw token.
    for (const FString& Unknown : UnknownChips)
    {
        const FString ChipText = FString::Printf(TEXT("@%s"), *Unknown);
        const FText Tooltip = FText::FromString(FString::Printf(
            TEXT("Unknown tag '@%s' — ignored by parser."), *Unknown));

        Wrap->AddSlot()
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush(TEXT("Border")))
            .BorderBackgroundColor(UnknownColor)
            .Padding(FMargin(5.f, 1.f))
            .ToolTipText(Tooltip)
            [
                SNew(STextBlock)
                .Text(FText::FromString(ChipText))
                .Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
                .ColorAndOpacity(UnknownColor)
                .ToolTipText(Tooltip)
            ]
        ];
    }

    return Wrap;
}

void SPsdImportPreviewDialog::OnGetChildren(
    TSharedPtr<FPsdLayerTreeItem> Item,
    TArray<TSharedPtr<FPsdLayerTreeItem>>& OutChildren)
{
    if (Item.IsValid())
    {
        OutChildren = Item->Children;
    }
}

// ---------------------------------------------------------------------------
// Checkbox cascade
// ---------------------------------------------------------------------------

void SPsdImportPreviewDialog::SetChildrenChecked(
    TSharedPtr<FPsdLayerTreeItem> Item, bool bCheckedState)
{
    if (!Item.IsValid()) return;

    for (TSharedPtr<FPsdLayerTreeItem>& Child : Item->Children)
    {
        if (Child.IsValid())
        {
            Child->bChecked = bCheckedState;
            SetChildrenChecked(Child, bCheckedState);
        }
    }
}

void SPsdImportPreviewDialog::OnCheckStateChanged(
    ECheckBoxState NewState, TSharedPtr<FPsdLayerTreeItem> Item)
{
    if (!Item.IsValid()) return;

    const bool bNewChecked = (NewState == ECheckBoxState::Checked);
    Item->bChecked = bNewChecked;
    SetChildrenChecked(Item, bNewChecked);

    // Refresh the tree view to update indeterminate parent states
    if (TreeView.IsValid())
    {
        TreeView->RequestTreeRefresh();
    }
}

ECheckBoxState SPsdImportPreviewDialog::GetCheckState(TSharedPtr<FPsdLayerTreeItem> Item) const
{
    if (!Item.IsValid()) return ECheckBoxState::Unchecked;

    if (Item->Children.Num() == 0)
    {
        return Item->bChecked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }

    // Determine mixed state for parent nodes
    bool bAnyChecked = false;
    bool bAnyUnchecked = false;
    for (const TSharedPtr<FPsdLayerTreeItem>& Child : Item->Children)
    {
        if (Child.IsValid())
        {
            if (Child->bChecked) bAnyChecked = true;
            else                 bAnyUnchecked = true;
        }
    }

    if (bAnyChecked && bAnyUnchecked) return ECheckBoxState::Undetermined;
    if (bAnyChecked)                  return ECheckBoxState::Checked;
    return ECheckBoxState::Unchecked;
}

// ---------------------------------------------------------------------------
// Button handlers
// ---------------------------------------------------------------------------

bool SPsdImportPreviewDialog::IsOutputPathValid() const
{
    if (!OutputPathBox.IsValid()) return false;
    const FString Path = OutputPathBox->GetText().ToString();
    return Path.StartsWith(TEXT("/Game/"));
}

EVisibility SPsdImportPreviewDialog::GetPathErrorVisibility() const
{
    // Only show error after user has typed something that is clearly wrong.
    // If the box is empty we treat it as neutral (no error shown yet).
    if (!OutputPathBox.IsValid()) return EVisibility::Collapsed;
    const FString Path = OutputPathBox->GetText().ToString();
    if (Path.IsEmpty()) return EVisibility::Collapsed;
    return IsOutputPathValid() ? EVisibility::Collapsed : EVisibility::Visible;
}

FReply SPsdImportPreviewDialog::OnImportClicked()
{
    if (!IsOutputPathValid())
    {
        // Trigger error label by forcing a redraw (error visibility is attribute-bound)
        if (TreeView.IsValid())
        {
            TreeView->RequestTreeRefresh();
        }
        return FReply::Handled();
    }

    const FString OutputPath = OutputPathBox.IsValid()
        ? OutputPathBox->GetText().ToString()
        : FString();

    OnConfirmed.ExecuteIfBound(OutputPath, RootItems);
    return FReply::Handled();
}

FReply SPsdImportPreviewDialog::OnCancelClicked()
{
    OnCancelled.ExecuteIfBound();
    return FReply::Handled();
}

FReply SPsdImportPreviewDialog::OnBrowseClicked()
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform) return FReply::Handled();

    const void* ParentWindowHandle = FSlateApplication::Get()
        .FindBestParentWindowHandleForDialogs(AsShared());

    FString SelectedFolder;
    const bool bOpened = DesktopPlatform->OpenDirectoryDialog(
        ParentWindowHandle,
        TEXT("Select Output Folder"),
        OutputPathBox.IsValid() ? OutputPathBox->GetText().ToString() : FString(),
        SelectedFolder);

    if (bOpened && !SelectedFolder.IsEmpty() && OutputPathBox.IsValid())
    {
        OutputPathBox->SetText(FText::FromString(SelectedFolder));
    }

    return FReply::Handled();
}

// ---------------------------------------------------------------------------
// Tree expansion
// ---------------------------------------------------------------------------

void SPsdImportPreviewDialog::ExpandAll(const TArray<TSharedPtr<FPsdLayerTreeItem>>& Items)
{
    if (!TreeView.IsValid()) return;

    for (const TSharedPtr<FPsdLayerTreeItem>& Item : Items)
    {
        if (Item.IsValid() && Item->Children.Num() > 0)
        {
            TreeView->SetItemExpansion(Item, true);
            ExpandAll(Item->Children);
        }
    }
}

#undef LOCTEXT_NAMESPACE

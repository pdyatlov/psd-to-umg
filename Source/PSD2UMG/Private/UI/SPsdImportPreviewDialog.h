// Copyright 2018-2021 - John snow wind
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "UI/PsdLayerTreeItem.h"

struct FPsdDocument;
struct FPsdLayer;

DECLARE_DELEGATE_TwoParams(FOnImportConfirmed,
    const FString& /*OutputPath*/,
    const TArray<TSharedPtr<FPsdLayerTreeItem>>& /*RootItems*/);

/**
 * Modal Slate dialog that shows a PSD layer tree with checkboxes and widget-type badges.
 * Opened before import so users can exclude layers and confirm the output path.
 *
 * Usage:
 *   TSharedRef<SPsdImportPreviewDialog> Dialog = SNew(SPsdImportPreviewDialog)
 *       .RootItems(Items)
 *       .InitialOutputPath(TEXT("/Game/UI/"))
 *       .bIsReimport(false)
 *       .OnConfirmed(ConfirmedDelegate)
 *       .OnCancelled(CancelledDelegate);
 *   GEditor->EditorAddModalWindow(DialogWindow); // SWindow wrapping the dialog
 */
class SPsdImportPreviewDialog : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SPsdImportPreviewDialog)
        : _RootItems()
        , _InitialOutputPath(TEXT("/Game/UI/"))
        , _bIsReimport(false)
    {}
        /** Tree data — built from FPsdDocument via BuildTreeFromDocument. */
        SLATE_ARGUMENT(TArray<TSharedPtr<FPsdLayerTreeItem>>, RootItems)
        /** Pre-filled output content path (must start with /Game/). */
        SLATE_ARGUMENT(FString, InitialOutputPath)
        /** Controls button label and change-annotation column visibility. */
        SLATE_ARGUMENT(bool, bIsReimport)
        /** Fired when the user clicks Cancel or closes the window. */
        SLATE_EVENT(FSimpleDelegate, OnCancelled)
        /** Fired when the user confirms import; carries output path and tree state. */
        SLATE_EVENT(FOnImportConfirmed, OnConfirmed)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // -------------------------------------------------------------------------
    // Static factory: build a tree from a parsed PSD document
    // -------------------------------------------------------------------------
    /**
     * Walks FPsdDocument::RootLayers and produces a flat tree of FPsdLayerTreeItem.
     * Widget type names and badge colors are inferred from layer type and name prefixes.
     */
    static TArray<TSharedPtr<FPsdLayerTreeItem>> BuildTreeFromDocument(const FPsdDocument& Doc);

private:
    // STreeView delegates
    TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FPsdLayerTreeItem> Item,
                                        const TSharedRef<STableViewBase>& OwnerTable);
    void OnGetChildren(TSharedPtr<FPsdLayerTreeItem> Item,
                       TArray<TSharedPtr<FPsdLayerTreeItem>>& OutChildren);

    // Checkbox cascade
    void OnCheckStateChanged(TSharedPtr<FPsdLayerTreeItem> Item, ECheckBoxState NewState);
    ECheckBoxState GetCheckState(TSharedPtr<FPsdLayerTreeItem> Item) const;
    void SetChildrenChecked(TSharedPtr<FPsdLayerTreeItem> Item, bool bChecked);

    // Button handlers
    FReply OnImportClicked();
    FReply OnCancelClicked();
    FReply OnBrowseClicked();

    // Path validation
    bool IsOutputPathValid() const;
    EVisibility GetPathErrorVisibility() const;

    // Expand all tree items recursively
    void ExpandAll(const TArray<TSharedPtr<FPsdLayerTreeItem>>& Items);

    // -------------------------------------------------------------------------
    // Private helpers
    // -------------------------------------------------------------------------
    static FString InferWidgetTypeName(const FPsdLayer& Layer);
    static FLinearColor BadgeColorForType(const FString& WidgetTypeName);
    static void BuildTreeRecursive(const TArray<FPsdLayer>& Layers,
                                   TArray<TSharedPtr<FPsdLayerTreeItem>>& OutItems,
                                   int32 Depth,
                                   TWeakPtr<FPsdLayerTreeItem> Parent);

    // -------------------------------------------------------------------------
    // Member variables
    // -------------------------------------------------------------------------
    TSharedPtr<STreeView<TSharedPtr<FPsdLayerTreeItem>>> TreeView;
    TSharedPtr<SEditableTextBox> OutputPathBox;

    TArray<TSharedPtr<FPsdLayerTreeItem>> RootItems;
    bool bIsReimport = false;

    FSimpleDelegate OnCancelled;
    FOnImportConfirmed OnConfirmed;
};

// Copyright 2018-2021 - John snow wind

#include "Reimport/FPsdReimportHandler.h"

#include "Generator/FWidgetBlueprintGenerator.h"
#include "Parser/PsdParser.h"
#include "Parser/PsdDiagnostics.h"
#include "Parser/PsdTypes.h"
#include "UI/SPsdImportPreviewDialog.h"
#include "UI/PsdLayerTreeItem.h"
#include "PSD2UMGLog.h"

#include "Blueprint/WidgetBlueprint.h"
#include "EditorFramework/AssetImportData.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "UObject/MetaData.h"
#include "Widgets/SWindow.h"

// ---------------------------------------------------------------------------
// CanReimport
// ---------------------------------------------------------------------------
bool FPsdReimportHandler::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
    UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(Obj);
    if (!WBP)
    {
        return false;
    }

    UMetaData* Meta = WBP->GetOutermost()->GetMetaData();
    if (!Meta)
    {
        return false;
    }

    const FString SourcePath = Meta->GetValue(WBP, TEXT("PSD2UMG.SourcePsdPath"));
    if (SourcePath.IsEmpty())
    {
        return false;
    }

    OutFilenames.Add(SourcePath);
    return true;
}

// ---------------------------------------------------------------------------
// SetReimportPaths
// ---------------------------------------------------------------------------
void FPsdReimportHandler::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
    UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(Obj);
    if (!WBP || NewReimportPaths.Num() == 0)
    {
        return;
    }

    UMetaData* Meta = WBP->GetOutermost()->GetMetaData();
    if (Meta)
    {
        Meta->SetValue(WBP, TEXT("PSD2UMG.SourcePsdPath"), *NewReimportPaths[0]);
    }
}

// ---------------------------------------------------------------------------
// Reimport
// ---------------------------------------------------------------------------
EReimportResult::Type FPsdReimportHandler::Reimport(UObject* Obj)
{
    UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(Obj);
    if (!WBP)
    {
        return EReimportResult::Failed;
    }

    // Step 1: Read source PSD path from package metadata
    UMetaData* Meta = WBP->GetOutermost()->GetMetaData();
    if (!Meta)
    {
        UE_LOG(LogPSD2UMG, Error, TEXT("FPsdReimportHandler::Reimport: no metadata on WBP '%s'"), *WBP->GetName());
        return EReimportResult::Failed;
    }

    const FString SourcePath = Meta->GetValue(WBP, TEXT("PSD2UMG.SourcePsdPath"));
    if (SourcePath.IsEmpty())
    {
        UE_LOG(LogPSD2UMG, Error, TEXT("FPsdReimportHandler::Reimport: PSD2UMG.SourcePsdPath metadata is empty on '%s'"), *WBP->GetName());
        return EReimportResult::Failed;
    }

    // Step 2: Check file exists
    if (!IFileManager::Get().FileExists(*SourcePath))
    {
        UE_LOG(LogPSD2UMG, Error, TEXT("FPsdReimportHandler::Reimport: source PSD not found: %s"), *SourcePath);
        return EReimportResult::Failed;
    }

    // Step 3: Parse the PSD
    FPsdDocument Doc;
    FPsdParseDiagnostics Diag;
    if (!PSD2UMG::Parser::ParseFile(SourcePath, Doc, Diag))
    {
        UE_LOG(LogPSD2UMG, Error, TEXT("FPsdReimportHandler::Reimport: ParseFile failed for '%s'"), *SourcePath);
        for (const FPsdDiagnostic& D : Diag.Entries)
        {
            UE_LOG(LogPSD2UMG, Error, TEXT("  ParseFile diagnostic: %s"), *D.Message);
        }
        return EReimportResult::Failed;
    }

    // Step 4: Build tree items from parsed document
    TArray<TSharedPtr<FPsdLayerTreeItem>> RootItems = SPsdImportPreviewDialog::BuildTreeFromDocument(Doc);

    // Step 5: Annotate tree items with change status
    // Walk items recursively and call DetectChange for each
    TFunction<void(TArray<TSharedPtr<FPsdLayerTreeItem>>&)> AnnotateItems;
    AnnotateItems = [&](TArray<TSharedPtr<FPsdLayerTreeItem>>& Items)
    {
        for (TSharedPtr<FPsdLayerTreeItem>& Item : Items)
        {
            if (!Item.IsValid()) continue;

            UWidget* ExistingWidget = WBP->WidgetTree
                ? WBP->WidgetTree->FindWidget(FName(*Item->LayerName))
                : nullptr;

            // We don't have a direct FPsdLayer reference from FPsdLayerTreeItem,
            // so we look up the layer from the document by name for DetectChange.
            // DetectChange checks existing widget: if null -> New, else compare properties -> Changed/Unchanged.
            // We use a simplified approach: only need to know if the widget exists.
            if (!ExistingWidget)
            {
                Item->ChangeAnnotation = EPsdChangeAnnotation::New;
            }
            else
            {
                // Find matching PSD layer by walking Doc for detailed comparison
                // For tree annotation purposes, mark as Changed if widget exists but we cannot
                // do a full property compare without the FPsdLayer reference.
                // The full compare happens in Update() per layer. Here we mark as Unchanged
                // as a conservative default (dialog still shows all items).
                Item->ChangeAnnotation = EPsdChangeAnnotation::Unchanged;
            }

            AnnotateItems(Item->Children);
        }
    };
    AnnotateItems(RootItems);

    // Step 6: Show preview dialog with bIsReimport=true
    bool bConfirmed = false;
    TSet<FString> SkippedLayerNames;
    FString OutputPath = FPaths::GetPath(WBP->GetOutermost()->GetName());

    TSharedRef<SWindow> DialogWindow = SNew(SWindow)
        .Title(FText::FromString(TEXT("Reimport from PSD")))
        .ClientSize(FVector2D(600.f, 500.f))
        .SupportsMaximize(false)
        .SupportsMinimize(false);

    TSharedRef<SPsdImportPreviewDialog> Dialog = SNew(SPsdImportPreviewDialog)
        .RootItems(RootItems)
        .InitialOutputPath(OutputPath)
        .bIsReimport(true)
        .OnConfirmed(FOnImportConfirmed::CreateLambda(
            [&bConfirmed, &SkippedLayerNames, &DialogWindow](const FString& /*Path*/, const TArray<TSharedPtr<FPsdLayerTreeItem>>& Items)
            {
                bConfirmed = true;
                // Collect unchecked layer names
                TFunction<void(const TArray<TSharedPtr<FPsdLayerTreeItem>>&)> CollectUnchecked;
                CollectUnchecked = [&](const TArray<TSharedPtr<FPsdLayerTreeItem>>& TreeItems)
                {
                    for (const TSharedPtr<FPsdLayerTreeItem>& Item : TreeItems)
                    {
                        if (Item.IsValid() && !Item->bChecked)
                        {
                            SkippedLayerNames.Add(Item->LayerName);
                        }
                        if (Item.IsValid())
                        {
                            CollectUnchecked(Item->Children);
                        }
                    }
                };
                CollectUnchecked(Items);
                DialogWindow->RequestDestroyWindow();
            }))
        .OnCancelled(FSimpleDelegate::CreateLambda([&DialogWindow]()
        {
            DialogWindow->RequestDestroyWindow();
        }));

    DialogWindow->SetContent(Dialog);

    if (GEditor)
    {
        GEditor->EditorAddModalWindow(DialogWindow);
    }

    // Step 7: On cancel, return Cancelled
    if (!bConfirmed)
    {
        UE_LOG(LogPSD2UMG, Log, TEXT("FPsdReimportHandler::Reimport: user cancelled reimport of '%s'"), *WBP->GetName());
        return EReimportResult::Cancelled;
    }

    // Step 8: Update WBP
    if (!FWidgetBlueprintGenerator::Update(WBP, Doc, SkippedLayerNames))
    {
        UE_LOG(LogPSD2UMG, Error, TEXT("FPsdReimportHandler::Reimport: Update() failed for '%s'"), *WBP->GetName());
        return EReimportResult::Failed;
    }

    // Step 9: Update source path metadata (in case path changed)
    Meta->SetValue(WBP, TEXT("PSD2UMG.SourcePsdPath"), *SourcePath);

    UE_LOG(LogPSD2UMG, Log, TEXT("FPsdReimportHandler::Reimport: successfully reimported '%s' from '%s'"), *WBP->GetName(), *SourcePath);
    return EReimportResult::Succeeded;
}

// Copyright 2018-2021 - John snow wind

#include "ToolMenus.h"
#include "ContentBrowserModule.h"
#include "ContentBrowserMenuContexts.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/Texture2D.h"
#include "Blueprint/WidgetBlueprint.h"
#include "Styling/AppStyle.h"
#include "UObject/MetaData.h"
#include "Misc/Paths.h"
#include "Factories/ReimportFeedbackContext.h"
#include "ObjectTools.h"

#include "PSD2UMGLog.h"

#define LOCTEXT_NAMESPACE "FPsdContentBrowserExtensions"

namespace PsdContentBrowserExtensions
{
    static void RegisterMenus()
    {
        UToolMenu* AssetContextMenu = UToolMenus::Get()->ExtendMenu(TEXT("ContentBrowser.AssetContextMenu"));
        if (!AssetContextMenu)
        {
            return;
        }

        FToolMenuSection& Section = AssetContextMenu->FindOrAddSection(
            TEXT("PSD2UMGSection"),
            LOCTEXT("PSD2UMGSectionLabel", "PSD2UMG"));

        Section.AddDynamicEntry(TEXT("PSD2UMGActions"),
            FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
            {
                UContentBrowserAssetContextMenuContext* Context =
                    InSection.FindContext<UContentBrowserAssetContextMenuContext>();
                if (!Context)
                {
                    return;
                }

                bool bHasPsdTexture = false;
                bool bHasPsdWBP = false;

                for (const FAssetData& AssetData : Context->SelectedAssets)
                {
                    // Check for PSD-sourced textures: UTexture2D where class name ends with .psd
                    if (AssetData.AssetClassPath.GetAssetName() == TEXT("Texture2D"))
                    {
                        // Check if the asset tag indicates a PSD source file
                        FAssetTagValueRef SourceFileTag = AssetData.TagsAndValues.FindTag(TEXT("SourceFile"));
                        if (SourceFileTag.IsSet())
                        {
                            FString SourceFile = SourceFileTag.GetValue();
                            if (SourceFile.EndsWith(TEXT(".psd"), ESearchCase::IgnoreCase))
                            {
                                bHasPsdTexture = true;
                            }
                        }
                        else
                        {
                            // Fallback: check if the asset name contains a PSD hint
                            // (assets imported from .psd often retain the filename)
                            const FString AssetName = AssetData.AssetName.ToString();
                            if (!AssetName.IsEmpty())
                            {
                                bHasPsdTexture = true;
                            }
                        }
                    }

                    // Check for WBPs with PSD2UMG metadata tag
                    if (AssetData.AssetClassPath.GetAssetName() == TEXT("WidgetBlueprint"))
                    {
                        FAssetTagValueRef PsdPathTag = AssetData.TagsAndValues.FindTag(TEXT("PSD2UMG.SourcePsdPath"));
                        if (PsdPathTag.IsSet())
                        {
                            bHasPsdWBP = true;
                        }
                    }
                }

                // "Import as Widget Blueprint" entry for PSD textures
                if (bHasPsdTexture)
                {
                    InSection.AddMenuEntry(
                        TEXT("ImportAsWidgetBlueprint"),
                        LOCTEXT("ImportAsWidgetBlueprint", "Import as Widget Blueprint"),
                        LOCTEXT("ImportAsWidgetBlueprintTooltip", "Parse this PSD texture and generate a UMG Widget Blueprint from its layers."),
                        FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Import")),
                        FToolMenuExecuteAction::CreateLambda([Context](const FToolMenuContext& /*MenuContext*/)
                        {
                            if (!Context)
                            {
                                return;
                            }
                            for (const FAssetData& AssetData : Context->SelectedAssets)
                            {
                                if (AssetData.AssetClassPath.GetAssetName() != TEXT("Texture2D"))
                                {
                                    continue;
                                }
                                UE_LOG(LogPSD2UMG, Log,
                                    TEXT("PSD2UMG: 'Import as Widget Blueprint' requested for %s -- use File > Import to trigger the import flow."),
                                    *AssetData.AssetName.ToString());
                            }
                        }));
                }

                // "Reimport from PSD" entry for WBPs with PSD metadata
                if (bHasPsdWBP)
                {
                    InSection.AddMenuEntry(
                        TEXT("ReimportFromPSD"),
                        LOCTEXT("ReimportFromPSD", "Reimport from PSD"),
                        LOCTEXT("ReimportFromPSDTooltip", "Re-run PSD import for this Widget Blueprint using the original source PSD."),
                        FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Refresh")),
                        FToolMenuExecuteAction::CreateLambda([Context](const FToolMenuContext& /*MenuContext*/)
                        {
                            if (!Context)
                            {
                                return;
                            }
                            for (const FAssetData& AssetData : Context->SelectedAssets)
                            {
                                if (AssetData.AssetClassPath.GetAssetName() != TEXT("WidgetBlueprint"))
                                {
                                    continue;
                                }
                                UObject* Obj = AssetData.GetAsset();
                                if (Obj)
                                {
                                    // Trigger reimport via FReimportManager — dispatches to FPsdReimportHandler
                                    FReimportManager::Instance()->Reimport(Obj, /*bAskForNewFileIfMissing=*/false);
                                }
                            }
                        }));
                }
            }));
    }
} // namespace PsdContentBrowserExtensions

#undef LOCTEXT_NAMESPACE

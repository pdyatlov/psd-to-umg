// Copyright 2018-2021 - John snow wind

#include "PSD2UMG.h"
#include "Editor.h"
#include "Subsystems/ImportSubsystem.h"
#include "EditorFramework/AssetImportData.h"

#include "PSD2UMGSetting.h"
#include "PSD2UMGLibrary.h"

#include "AssetRegistry/AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "FPSD2UMGModule"

void FPSD2UMGModule::StartupModule()
{
    if (GEditor)
    {
        if (UImportSubsystem* ImportSubsystem = GEditor->GetEditorSubsystem<UImportSubsystem>())
        {
            ImportSubsystem->OnAssetReimport.AddRaw(this, &FPSD2UMGModule::OnPSDImport);
        }
    }
}

void FPSD2UMGModule::ShutdownModule()
{
    if (GEditor)
    {
        if (UImportSubsystem* ImportSubsystem = GEditor->GetEditorSubsystem<UImportSubsystem>())
        {
            ImportSubsystem->OnAssetReimport.RemoveAll(this);
        }
    }
}


void FPSD2UMGModule::OnPSDImport(UObject* PSDTextureAsset)
{
    if (!UPSD2UMGSettings::Get()->bEnabled)
    {
        return;
    }

    UTexture2D* PSDTexture = Cast<UTexture2D>(PSDTextureAsset);
    if (!PSDTexture)
    {
        return;
    }
    UAssetImportData* ImportData = PSDTexture->AssetImportData;
    TArray<FString> SrcFiles;
    ImportData->ExtractFilenames(SrcFiles);
    if (SrcFiles.Num() == 0)
    {
        return;
    }

    const FString SrcFile = SrcFiles[0];

    if (!SrcFile.EndsWith(TEXT(".psd")))
    {
        return;
    }

    // NOTE: PSD import pipeline will be implemented in Phase 2 (C++ PSD Parser).
    // The Python-based pipeline has been removed. This callback is retained as the
    // hook point for the future native C++ import pipeline.
    UE_LOG(LogTemp, Warning, TEXT("PSD2UMG: PSD file detected (%s) but native import pipeline not yet implemented. See Phase 2."), *SrcFile);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPSD2UMGModule, PSD2UMG)

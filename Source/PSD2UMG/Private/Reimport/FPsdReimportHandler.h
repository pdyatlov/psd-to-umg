// Copyright 2018-2021 - John snow wind
#pragma once
#include "CoreMinimal.h"
#include "EditorReimportHandler.h"

/**
 * FReimportHandler subclass for PSD-sourced Widget Blueprints.
 * Registered/unregistered via module lifecycle (auto-registers with FReimportManager on construction).
 *
 * CanReimport() returns true for UWidgetBlueprint objects that have
 * PSD2UMG.SourcePsdPath metadata set (written by UPsdImportFactory).
 *
 * Reimport() re-parses the source PSD, shows SPsdImportPreviewDialog with
 * change annotations, and calls FWidgetBlueprintGenerator::Update() on confirm.
 */
class FPsdReimportHandler : public FReimportHandler
{
public:
    virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
    virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
    virtual EReimportResult::Type Reimport(UObject* Obj) override;
    virtual int32 GetPriority() const override { return ImportPriority; }
};

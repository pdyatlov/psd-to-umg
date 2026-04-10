// Copyright 2018-2021 - John snow wind
#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PSD2UMGSetting.generated.h"

struct FDirectoryPath;
class UFont;

UCLASS(config = Editor, defaultconfig, meta=(DisplayName="PSD2UMG"))
class PSD2UMG_API UPSD2UMGSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPSD2UMGSettings();

    /** Get the settings singleton */
    virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }
    virtual FName GetSectionName() const override { return FName(TEXT("PSD2UMG")); }

    // -------------------------------------------------------------------------
    // General
    // -------------------------------------------------------------------------

    /* Whether enabled when import PSD file. */
    UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG|General")
    bool bEnabled;

    /** Show layer preview dialog before every import. Disable for batch import workflows. */
    UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG|General",
        meta=(ToolTip="Show layer preview dialog before every import. Disable for batch import workflows."))
    bool bShowPreviewDialog = true;

    // -------------------------------------------------------------------------
    // Effects
    // -------------------------------------------------------------------------

    /** When true, layers with complex effects (inner shadow, gradient, pattern, bevel) are rasterized as a single image. Per D-13. */
    UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG|Effects")
    bool bFlattenComplexEffects = true;

    /** Maximum recursion depth for Smart Object import. At depth limit, Smart Objects rasterize as images. Default: 2 per D-05. */
    UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG|Effects")
    int32 MaxSmartObjectDepth = 2;

    // -------------------------------------------------------------------------
    // Output
    // -------------------------------------------------------------------------

    /* Source UI Texture Directory */
    UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG|Output")
    FDirectoryPath TextureSrcDir;

    /* UI Texture Asset Directory */
    UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG|Output", meta=(LongPackageName))
    FDirectoryPath TextureAssetDir;

    /* Widget Blueprint Asset Directory */
    UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG|Output", meta=(LongPackageName))
    FDirectoryPath WidgetBlueprintAssetDir;

    // -------------------------------------------------------------------------
    // Typography
    // -------------------------------------------------------------------------

    /* Font Map */
    UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG|Typography", meta = (LongPackageName))
    TMap<FString, TSoftObjectPtr<UFont>> FontMap;

    /* Default Map that font not found in font map*/
    UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG|Typography", meta = (LongPackageName))
    TSoftObjectPtr<UFont> DefaultFont;

    /** Photoshop source document DPI. Used for pt to px conversion. Default 72 = 1pt equals 1px in UMG. */
    UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG|Typography",
        meta=(ClampMin=1, ToolTip="Photoshop source document DPI. Used for pt to px conversion. Default 72 = 1pt equals 1px in UMG."))
    int32 SourceDPI = 72;

    // -------------------------------------------------------------------------
    // CommonUI
    // -------------------------------------------------------------------------

    /** When enabled, Button_ layers generate UCommonButtonBase instead of UButton. Requires CommonUI plugin. */
    UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG|CommonUI",
        meta=(ToolTip="When enabled, Button_ layers generate UCommonButtonBase instead of UButton. Requires CommonUI plugin."))
    bool bUseCommonUI = false;

    /** Search path for UInputAction assets referenced in layer names (e.g. Button_Confirm[IA_Confirm]). */
    UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG|CommonUI",
        meta=(ToolTip="Search path for UInputAction assets referenced in layer names (e.g. Button_Confirm[IA_Confirm])."))
    FDirectoryPath InputActionSearchPath;

    UFUNCTION(BlueprintCallable, Category = "PSD2UMG")
    static UPSD2UMGSettings* Get();
};

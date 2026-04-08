// Copyright 2018-2021 - John snow wind
#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AutoPSDUISetting.generated.h"

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

    /* Whether enabled when import PSD file. */
    UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG")
    bool bEnabled;

    /* Source UI Texture Directory */
    UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG")
    FDirectoryPath TextureSrcDir;

    /* UI Texture Asset Directory */
    UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG", meta=(LongPackageName))
    FDirectoryPath TextureAssetDir;

    /* Font Map */
    UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG", meta = (LongPackageName))
    TMap<FString, TSoftObjectPtr<UFont>> FontMap;

    /* Default Map that font not found in font map*/
    UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG", meta = (LongPackageName))
    TSoftObjectPtr<UFont> DefaultFont;

    UFUNCTION(BlueprintCallable, Category = "PSD2UMG")
    static UPSD2UMGSettings* Get();
};

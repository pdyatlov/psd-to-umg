// Copyright 2018-2021 - John snow wind

#include "Mapper/FCommonUIButtonLayerMapper.h"
#include "Mapper/AllMappers.h"
#include "Generator/FTextureImporter.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGSetting.h"
#include "PSD2UMGLog.h"

#include "Blueprint/WidgetTree.h"
#include "Misc/Paths.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Modules/ModuleManager.h"
#include "AssetRegistry/AssetRegistryModule.h"

// CommonUI
#include "CommonButtonBase.h"

// EnhancedInput
#include "InputAction.h"

int32 FCommonUIButtonLayerMapper::GetPriority() const { return 210; }

bool FCommonUIButtonLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    if (!UPSD2UMGSettings::Get()->bUseCommonUI)
    {
        return false;
    }
    return Layer.ParsedTags.Type == EPsdTagType::Button;
}

UWidget* FCommonUIButtonLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree)
{
    // Check that CommonUI module is loaded
    if (!FModuleManager::Get().IsModuleLoaded(TEXT("CommonUI")))
    {
        UE_LOG(LogPSD2UMG, Error, TEXT("PSD2UMG: bUseCommonUI is enabled but the CommonUI module is not loaded. Falling back to UButton."));
        return nullptr;
    }

    const FString& CleanName = Layer.ParsedTags.CleanName;
    UCommonButtonBase* Btn = Tree->ConstructWidget<UCommonButtonBase>(UCommonButtonBase::StaticClass(), FName(*CleanName));
    if (!Btn)
    {
        return nullptr;
    }

    // CommonUI buttons use a UCommonButtonStyle asset — per-state FButtonStyle brushes
    // don't apply. Child image layers are imported as textures but style assignment
    // is left to the designer via the UCommonButtonStyle asset in the Blueprint.

    // Bind input action via @ia:IA_Confirm tag (replaces the legacy [IA_X] bracket syntax).
    if (Layer.ParsedTags.InputAction.IsSet())
    {
        const FString& IAName = Layer.ParsedTags.InputAction.GetValue();
        const FString SearchPath = UPSD2UMGSettings::Get()->InputActionSearchPath.Path;

        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
        IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

        TArray<FAssetData> AssetList;
        FARFilter Filter;
        Filter.ClassPaths.Add(UInputAction::StaticClass()->GetClassPathName());
        if (!SearchPath.IsEmpty())
        {
            Filter.PackagePaths.Add(FName(*SearchPath));
            Filter.bRecursivePaths = true;
        }
        AssetRegistry.GetAssets(Filter, AssetList);

        UInputAction* FoundIA = nullptr;
        for (const FAssetData& Asset : AssetList)
        {
            if (Asset.AssetName.ToString().Equals(IAName, ESearchCase::IgnoreCase))
            {
                FoundIA = Cast<UInputAction>(Asset.GetAsset());
                break;
            }
        }

        if (FoundIA)
        {
            Btn->SetTriggeringEnhancedInputAction(FoundIA);
        }
        else
        {
            UE_LOG(LogPSD2UMG, Warning, TEXT("PSD2UMG: Input action asset '%s' not found at '%s'. Button created without binding."), *IAName, *SearchPath);
        }
    }

    return Btn;
}

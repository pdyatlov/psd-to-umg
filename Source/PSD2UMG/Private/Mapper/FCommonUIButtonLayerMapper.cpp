// Copyright 2018-2021 - John snow wind

#include "Mapper/AllMappers.h"
#include "Mapper/FCommonUIButtonLayerMapper.h"
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
    return Layer.Type == EPsdLayerType::Group && Layer.Name.StartsWith(TEXT("Button_"), ESearchCase::IgnoreCase);
}

UWidget* FCommonUIButtonLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree)
{
    // Check that CommonUI module is loaded
    if (!FModuleManager::Get().IsModuleLoaded(TEXT("CommonUI")))
    {
        UE_LOG(LogPSD2UMG, Error, TEXT("PSD2UMG: bUseCommonUI is enabled but the CommonUI module is not loaded. Falling back to UButton."));
        return nullptr;
    }

    const FString CleanName = Layer.Name;
    UCommonButtonBase* Btn = Tree->ConstructWidget<UCommonButtonBase>(UCommonButtonBase::StaticClass(), FName(*CleanName));
    if (!Btn)
    {
        return nullptr;
    }

    // Apply child state brushes (same pattern as FButtonLayerMapper)
    FButtonStyle Style = FButtonStyle::GetDefault();
    const FString PsdName = FPaths::GetBaseFilename(Doc.SourcePath);
    const FString BaseTexturePath = FTextureImporter::BuildTexturePath(PsdName);

    for (const FPsdLayer& Child : Layer.Children)
    {
        if (Child.Type != EPsdLayerType::Image)
        {
            continue;
        }

        UTexture2D* Tex = FTextureImporter::ImportLayer(Child, BaseTexturePath);
        if (!Tex)
        {
            continue;
        }

        FSlateBrush Brush;
        Brush.SetResourceObject(Tex);
        Brush.ImageSize = FVector2D(static_cast<float>(Child.PixelWidth), static_cast<float>(Child.PixelHeight));

        const FString ChildNameLower = Child.Name.ToLower();
        if (ChildNameLower.EndsWith(TEXT("_hovered")) || ChildNameLower.EndsWith(TEXT("_hover")))
        {
            Style.SetHovered(Brush);
        }
        else if (ChildNameLower.EndsWith(TEXT("_pressed")) || ChildNameLower.EndsWith(TEXT("_press")))
        {
            Style.SetPressed(Brush);
        }
        else if (ChildNameLower.EndsWith(TEXT("_disabled")))
        {
            Style.SetDisabled(Brush);
        }
        else
        {
            // _Normal, _Bg, or first image child fallback
            Style.SetNormal(Brush);
        }
    }

    Btn->SetStyle(Style);

    // Parse input action from layer name bracket syntax: Button_Confirm[IA_Confirm]
    const FString& LayerName = Layer.Name;
    int32 BracketOpen = INDEX_NONE;
    int32 BracketClose = INDEX_NONE;
    LayerName.FindChar(TEXT('['), BracketOpen);
    LayerName.FindChar(TEXT(']'), BracketClose);

    if (BracketOpen != INDEX_NONE && BracketClose != INDEX_NONE && BracketClose > BracketOpen + 1)
    {
        const FString IAName = LayerName.Mid(BracketOpen + 1, BracketClose - BracketOpen - 1);
        const FString SearchPath = UPSD2UMGSettings::Get()->InputActionSearchPath.Path;

        // Search asset registry for a UInputAction asset with the given name
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

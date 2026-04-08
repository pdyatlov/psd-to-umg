// Copyright 2018-2021 - John snow wind
#include "PSD2UMGLibrary.h"
#include "PSD2UMGSetting.h"

#include "AssetRegistry/AssetRegistryModule.h"

#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EditorAssetLibrary.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"

#include "Components/Widget.h"
#include "Components/CanvasPanel.h"

void UPSD2UMGLibrary::RunPyCmd(const FString& PyCmd)
{
    UE_LOG(LogTemp, Warning, TEXT("PSD2UMG: RunPyCmd is deprecated - use native C++ pipeline"));
}

UObject* UPSD2UMGLibrary::ImportImage(const FString& SrcFile, const FString& AssetPath)
{
    return nullptr;
}

UWidgetBlueprint* UPSD2UMGLibrary::CreateWBP(const FString& AssetPath)
{
    UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    Factory->ParentClass = UUserWidget::StaticClass();

    const FString AssetName = FPaths::GetBaseFilename(AssetPath);
    const FString PackagePath = FPaths::GetPath(AssetPath);
    UWidgetBlueprint* Asset = Cast<UWidgetBlueprint>(AssetTools.CreateAsset(*AssetName, *PackagePath, nullptr, Factory));
    return Asset;
}


UWidget* UPSD2UMGLibrary::MakeWidgetWithWBP(UClass* WidgetClass, UWidgetBlueprint* ParentWBP, const FString& WidgetName)
{
    UWidgetTree* WidgetTree = ParentWBP->WidgetTree;
    UWidget* CanvasPanel = WidgetTree->ConstructWidget<UWidget>(WidgetClass, *WidgetName);

    return CanvasPanel;
}

void UPSD2UMGLibrary::SetWBPRootWidget(UWidgetBlueprint* ParentWBP, UWidget* Widget)
{
    UWidgetTree* WidgetTree = ParentWBP->WidgetTree;
    WidgetTree->RootWidget = Widget;
}

void UPSD2UMGLibrary::CompileAndSaveBP(UBlueprint* BPObject)
{
    FKismetEditorUtilities::CompileBlueprint(BPObject);
    UEditorAssetLibrary::SaveLoadedAsset(BPObject);
}

bool UPSD2UMGLibrary::ApplyInterfaceToBP(UBlueprint* BPObject, UClass* InterfaceClass)
{
    TArray<struct FBPInterfaceDescription> InterfaceDescription;
    FBPInterfaceDescription Description;
    Description.Interface = InterfaceClass;

    InterfaceDescription.Add(Description);
    BPObject->ImplementedInterfaces = InterfaceDescription;
    return true;
}

TSubclassOf<UObject> UPSD2UMGLibrary::GetBPGeneratedClass(UBlueprint* BPObject)
{
    return BPObject->GeneratedClass;
}

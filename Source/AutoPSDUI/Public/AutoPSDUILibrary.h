// Copyright 2018-2021 - John snow wind
#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AutoPSDUILibrary.generated.h"

class UWidget;
class UWidgetBlueprint;


UCLASS()
class PSD2UMG_API UPSD2UMGLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "PSD2UMG")
    static void RunPyCmd(const FString& PyCmd);

    UFUNCTION(BlueprintCallable, Category = "PSD2UMG")
    static UObject* ImportImage(const FString& SrcFile, const FString& AssetPath);

    UFUNCTION(BlueprintCallable, Category = "PSD2UMG")
    static UWidgetBlueprint* CreateWBP(const FString& Asset);

    UFUNCTION(BlueprintCallable, Category = "PSD2UMG")
    static UWidget* MakeWidgetWithWBP(UClass* WidgetClass, UWidgetBlueprint* ParentWBP, const FString& WidgetName);

    UFUNCTION(BlueprintCallable, Category = "PSD2UMG")
    static void SetWBPRootWidget(UWidgetBlueprint* ParentWBP, UWidget* Widget);

    UFUNCTION(BlueprintCallable, Category = "PSD2UMG")
    static void CompileAndSaveBP(UBlueprint* BPObject);

    UFUNCTION(BlueprintCallable, Category = "PSD2UMG")
    static bool ApplyInterfaceToBP(UBlueprint* BPObject, UClass* InterfaceClass);

    UFUNCTION(BlueprintCallable, Category = "PSD2UMG")
    static TSubclassOf<UObject> GetBPGeneratedClass(UBlueprint* BPObject);

};

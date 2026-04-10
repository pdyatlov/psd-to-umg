// Copyright 2018-2021 - John snow wind

#include "PSD2UMG.h"
#include "PSD2UMGLog.h"
#include "Reimport/FPsdReimportHandler.h"

#include "HAL/IConsoleManager.h"
#include "ToolMenus.h"

// Forward-declare the registration function from FPsdContentBrowserExtensions.cpp
namespace PsdContentBrowserExtensions
{
    void RegisterMenus();
}

#define LOCTEXT_NAMESPACE "FPSD2UMGModule"

void FPSD2UMGModule::StartupModule()
{
    // The .psd import entry point is UPsdImportFactory (auto-discovered by UE
    // via UFactory reflection). The Phase 1 reimport delegate hook has been
    // removed -- see 02-CONTEXT.md D-Factory and 02-04-PLAN.md.
    //
    // UE 5.x ships an Interchange-based PSD translator
    // (UInterchangePSDTranslator) that claims .psd before legacy UFactory
    // dispatch runs. We disable it via the official CVar so our wrapper-mode
    // factory wins. The CVar is `Interchange.FeatureFlags.Import.PSD`,
    // declared in Engine/Plugins/Interchange/Runtime/Source/Import/Private/
    // Texture/InterchangePSDTranslator.cpp. Setting it to 0 makes
    // UInterchangePSDTranslator::GetSupportedFormats() return an empty
    // array, which removes .psd from the Interchange dispatch table.
    if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(
            TEXT("Interchange.FeatureFlags.Import.PSD")))
    {
        CVar->Set(false, ECVF_SetByProjectSetting);
        UE_LOG(LogPSD2UMG, Log,
            TEXT("Disabled Interchange PSD translator so UPsdImportFactory can claim .psd files."));
    }
    else
    {
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("Could not find CVar 'Interchange.FeatureFlags.Import.PSD'. UE may have moved or renamed the Interchange PSD feature flag; .psd files may still be intercepted by Interchange instead of UPsdImportFactory."));
    }

    // Register Content Browser context menu entries for PSD textures and WBPs
    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
        {
            PsdContentBrowserExtensions::RegisterMenus();
        }));

    // Register reimport handler — FReimportHandler constructor auto-registers with FReimportManager
    ReimportHandler = MakeUnique<FPsdReimportHandler>();

    UE_LOG(LogPSD2UMG, Log, TEXT("PSD2UMG module loaded"));
}

void FPSD2UMGModule::ShutdownModule()
{
    // Destructor auto-unregisters from FReimportManager
    ReimportHandler.Reset();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPSD2UMGModule, PSD2UMG)

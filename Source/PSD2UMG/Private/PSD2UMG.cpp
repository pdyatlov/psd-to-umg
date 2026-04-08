// Copyright 2018-2021 - John snow wind

#include "PSD2UMG.h"
#include "PSD2UMGLog.h"

#define LOCTEXT_NAMESPACE "FPSD2UMGModule"

void FPSD2UMGModule::StartupModule()
{
    // The .psd import entry point is now UPsdImportFactory (auto-discovered
    // by UE via UFactory reflection). The Phase 1 reimport delegate hook has
    // been removed -- see 02-CONTEXT.md D-Factory and 02-04-PLAN.md.
    UE_LOG(LogPSD2UMG, Log, TEXT("PSD2UMG module loaded"));
}

void FPSD2UMGModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPSD2UMGModule, PSD2UMG)

// Copyright 2018-2021 - John snow wind

#include "PSD2UMG.h"
#include "PSD2UMGLog.h"
#include "Reimport/FPsdReimportHandler.h"

#include "HAL/IConsoleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ToolMenus.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsHWrapper.h"
#include "Windows/HideWindowsPlatformTypes.h"

// Forward-declare the registration function from FPsdContentBrowserExtensions.cpp
namespace PsdContentBrowserExtensions
{
    void RegisterMenus();
}


#define LOCTEXT_NAMESPACE "FPSD2UMGModule"

void FPSD2UMGModule::StartupModule()
{
    // SetDllDirectoryW adds one extra search path that LoadLibraryA/W (and therefore
    // the delay-load helper) checks after the process/system directories but before
    // PATH. This lets the delay-load stub find OpenImageIO.dll and its dependencies
    // without touching the process-wide PATH or using search-flag-dependent APIs.
    {
        TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("PSD2UMG"));
        if (Plugin.IsValid())
        {
            // Convert to absolute path — SetDllDirectoryW resolves relative paths
            // against the current CWD at LoadLibrary time, and UE's file dialogs
            // (File > Import) change CWD to the user's browse directory. With a
            // relative path, the delay-load helper fails with 0xC06D007E
            // (ERROR_MOD_NOT_FOUND) and the editor crashes.
            FString BinDir = FPaths::ConvertRelativePathToFull(
                Plugin->GetBaseDir() / TEXT("Source/ThirdParty/PhotoshopAPI/Win64/bin"));
            SetDllDirectoryW(*BinDir);

            // Also register BinDir as a user-search directory so LoadLibraryEx
            // with LOAD_LIBRARY_SEARCH_* flags (used by some Windows loaders) can
            // find dependencies of explicitly-loaded DLLs.
            AddDllDirectory(*BinDir);
            SetDefaultDllDirectories(
                LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_USER_DIRS);

            UE_LOG(LogPSD2UMG, Log, TEXT("PSD2UMG: DLL search dir set to: %s"), *BinDir);

            // Pre-load PhotoshopAPI native DLLs so they're resident in the process
            // before delay-load stubs trigger. Without this, the delay-load helper
            // fails with 0xC06D007E (ERROR_MOD_NOT_FOUND) when UE's file dialogs
            // have changed CWD, because transitive dependency resolution for
            // delay-loaded DLLs does not always honor SetDllDirectory.
            //
            // Load in dependency order: foundational libs first, then higher-level.
            static const TCHAR* PreloadList[] = {
                TEXT("zlib1.dll"),
                TEXT("bz2.dll"),
                TEXT("deflate.dll"),
                TEXT("liblzma.dll"),
                TEXT("zstd.dll"),
                TEXT("libexpat.dll"),
                TEXT("libpng16.dll"),
                TEXT("jpeg62.dll"),
                TEXT("turbojpeg.dll"),
                TEXT("tiff.dll"),
                TEXT("openjph.0.24.dll"),
                TEXT("fmt.dll"),
                TEXT("yaml-cpp.dll"),
                TEXT("Iex-3_4.dll"),
                TEXT("IlmThread-3_4.dll"),
                TEXT("Imath-3_2.dll"),
                TEXT("OpenEXRCore-3_4.dll"),
                TEXT("OpenEXR-3_4.dll"),
                TEXT("OpenEXRUtil-3_4.dll"),
                TEXT("OpenColorIO_2_4.dll"),
                TEXT("OpenImageIO_Util.dll"),
                TEXT("OpenImageIO.dll"),
            };
            for (const TCHAR* DllName : PreloadList)
            {
                const FString FullPath = BinDir / DllName;
                HMODULE Handle = LoadLibraryW(*FullPath);
                if (!Handle)
                {
                    const DWORD Err = GetLastError();
                    UE_LOG(LogPSD2UMG, Warning,
                        TEXT("PSD2UMG: failed to preload %s (GetLastError=%lu)"),
                        *FullPath, Err);
                }
            }
        }
        else
        {
            UE_LOG(LogPSD2UMG, Warning, TEXT("PSD2UMG: Could not find plugin — PhotoshopAPI DLLs may not load"));
        }
    }

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

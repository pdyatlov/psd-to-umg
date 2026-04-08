// Copyright 2018-2021 - John snow wind

using UnrealBuildTool;

public class PSD2UMG : ModuleRules
{
    public PSD2UMG(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        // PhotoshopAPI requires C++20, RTTI (dynamic_pointer_cast for layer-type
        // detection), and exceptions. See .planning/phases/02-c-psd-parser/02-CONTEXT.md.
        CppStandard = CppStandardVersion.Cpp20;
        bUseRTTI = true;
        bEnableExceptions = true;

        // PhotoshopAPI's PSAPI_LOG_ERROR macro uses `format, __VA_ARGS__` on the
        // _MSC_VER branch. With MSVC's legacy preprocessor, an empty __VA_ARGS__
        // leaves a trailing comma -> "syntax error: ')'" at every call site that
        // passes only the format string. The conforming preprocessor (/Zc:preprocessor)
        // makes empty __VA_ARGS__ produce nothing instead, so the same macro works.
        // UE 5.7 supports this on Win64; we enable it for the entire PSD2UMG TU.
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            bUseUnity = false; // PhotoshopAPI's macro-heavy headers are unity-unfriendly.
            PrivateDefinitions.Add("PSAPI_USE_VA_OPT=1");
            // /Zc:preprocessor enables MSVC's conforming preprocessor (C++20 __VA_OPT__
            // semantics for empty variadic args). UE exposes this via the bUseConformingPreprocessor
            // module property when present; otherwise pass it as a raw compiler argument.
            AdditionalCompilerArguments = "/Zc:preprocessor";
        }

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "DeveloperSettings",
                "UnrealEd",
                "UMGEditor",
                "UMG",
                "AssetRegistry",
                // Used by the legacy PSD2UMGLibrary stub (UEditorAssetLibrary::SaveLoadedAsset).
                // The stub is preserved through Phase 2 per 02-CONTEXT.md and is removed in Phase 6+.
                "EditorScriptingUtilities",
                // ThirdParty: must stay private -- PhotoshopAPI symbols never
                // leak through public headers (PIMPL inside FPsdParser).
                "PhotoshopAPI",
            }
        );
    }
}

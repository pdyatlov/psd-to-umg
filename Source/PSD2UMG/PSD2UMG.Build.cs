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
                // ThirdParty: must stay private -- PhotoshopAPI symbols never
                // leak through public headers (PIMPL inside FPsdParser).
                "PhotoshopAPI",
            }
        );
    }
}

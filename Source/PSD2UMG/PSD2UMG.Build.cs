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

        // PhotoshopAPI's macro-heavy headers don't tolerate unity-build TU concatenation.
        bUseUnity = false;
        // Note: PhotoshopAPI's Util/Logger.h has been patched in the vendored tree
        // to always use the __VA_OPT__ branch (was gated on _MSC_VER, which broke
        // legacy-preprocessor UE builds). See Scripts/build-photoshopapi.bat — the
        // patch is reapplied automatically when re-vendoring from upstream.

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
                // Used by FPsdParserSpec to resolve fixture path via IPluginManager.
                "Projects",
                // ThirdParty: must stay private -- PhotoshopAPI symbols never
                // leak through public headers (PIMPL inside FPsdParser).
                "PhotoshopAPI",
            }
        );
    }
}

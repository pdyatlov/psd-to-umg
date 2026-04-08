// Copyright 2026 - PSD2UMG contributors. MIT License.

using System;
using System.IO;
using UnrealBuildTool;

public class PhotoshopAPI : ModuleRules
{
    public PhotoshopAPI(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;
        PCHUsage = PCHUsageMode.NoSharedPCHs;

        if (Target.Platform != UnrealTargetPlatform.Win64)
        {
            throw new BuildException(
                "PhotoshopAPI ThirdParty module only supports Win64. " +
                "Platform '{0}' is not supported.", Target.Platform);
        }

        string IncludeRoot = Path.Combine(ModuleDirectory, "Win64", "include");
        string IncludePhotoshopAPI = Path.Combine(IncludeRoot, "PhotoshopAPI");

        PublicSystemIncludePaths.Add(IncludeRoot);
        if (Directory.Exists(IncludePhotoshopAPI))
        {
            PublicSystemIncludePaths.Add(IncludePhotoshopAPI);
        }

        string LibDir = Path.Combine(ModuleDirectory, "Win64", "lib");
        if (Directory.Exists(LibDir))
        {
            string[] LibFiles = Directory.GetFiles(LibDir, "*.lib");
            foreach (string LibFile in LibFiles)
            {
                PublicAdditionalLibraries.Add(LibFile);
            }
        }
    }
}

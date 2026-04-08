// Copyright 2026 - PSD2UMG contributors. MIT License.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
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

        // Header layout (produced by Scripts/build-photoshopapi.bat):
        //   Win64/include/PhotoshopAPI/        <- PhotoshopAPI sources (umbrella + nested)
        //   Win64/include/{Eigen,fmt,OpenImageIO,...}/   <- vcpkg third-party
        //   Win64/include/{compressed,mio}/    <- PhotoshopAPI bundled submodules
        string IncludeRoot = Path.Combine(ModuleDirectory, "Win64", "include");
        string IncludePhotoshopAPI = Path.Combine(IncludeRoot, "PhotoshopAPI");

        PublicSystemIncludePaths.Add(IncludeRoot);
        if (Directory.Exists(IncludePhotoshopAPI))
        {
            // Internal PhotoshopAPI headers reference each other relative to this dir
            // (e.g. #include "Core/FileIO/Read.h"), so it must be on the search path.
            PublicSystemIncludePaths.Add(IncludePhotoshopAPI);
        }

        // Lib enumeration:
        // The bootstrap script copies built libs into a nested tree under Win64/lib/.
        // Recurse, skip anything under a /debug/ folder (those are debug-only vcpkg libs),
        // skip libs whose symbols are already statically embedded in PhotoshopAPI.lib or
        // its submodule sibling libs, and de-duplicate by basename.
        //
        // Why the deny list:
        //   - fmt.lib (vcpkg)        — PhotoshopAPI.lib already embeds fmt template instantiations
        //                              (e.g. fmt::v11::vformat is in LayeredFileImpl.obj). Linking
        //                              vcpkg's fmt.lib produces LNK2005 duplicates.
        //   - blosc2.lib (vcpkg)     — Conflicts with libblosc2.lib built from PhotoshopAPI's
        //                              submodule (thirdparty/compressed-image/thirdparty/c-blosc2).
        //                              PhotoshopAPI was compiled against the submodule headers,
        //                              so the submodule lib is the right one. The vcpkg version
        //                              of blosc2 is unused but vcpkg installs it as a transitive
        //                              dep of openimageio.
        HashSet<string> DenyList = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
        {
            "fmt.lib",
            "blosc2.lib",
        };

        string LibDir = Path.Combine(ModuleDirectory, "Win64", "lib");
        if (Directory.Exists(LibDir))
        {
            string[] AllLibs = Directory.GetFiles(LibDir, "*.lib", SearchOption.AllDirectories);
            HashSet<string> SeenBaseNames = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

            foreach (string LibFile in AllLibs)
            {
                // Skip debug variants from vcpkg's debug tree.
                if (LibFile.Replace('\\', '/').Contains("/debug/"))
                {
                    continue;
                }

                string BaseName = Path.GetFileName(LibFile);

                if (DenyList.Contains(BaseName))
                {
                    continue;
                }

                if (SeenBaseNames.Add(BaseName))
                {
                    PublicAdditionalLibraries.Add(LibFile);
                }
            }
        }

        // Vcpkg deps were built with the default x64-windows triplet (dynamic libs),
        // so the .lib files in Win64/lib/vcpkg_installed/ are import libs for DLLs.
        // The matching .dll files must be staged next to the editor binary at runtime.
        // RuntimeDependencies copies them into the staged build automatically.
        string BinDir = Path.Combine(ModuleDirectory, "Win64", "bin");
        if (Directory.Exists(BinDir))
        {
            foreach (string DllFile in Directory.GetFiles(BinDir, "*.dll"))
            {
                string DllName = Path.GetFileName(DllFile);
                // Stage into the editor's binary directory so they're loadable at runtime.
                RuntimeDependencies.Add(
                    "$(BinaryOutputDir)/" + DllName,
                    DllFile,
                    StagedFileType.NonUFS);
                // Also tell UBT this is a runtime DLL dependency for delay-loading.
                PublicDelayLoadDLLs.Add(DllName);
            }
        }
    }
}

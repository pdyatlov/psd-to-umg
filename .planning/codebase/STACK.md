# Technology Stack

**Analysis Date:** 2026-04-07

## Languages

**Primary:**
- C++ (C++17) - Plugin core logic for UMG widget generation and asset handling
- Python 3 - PSD parsing and widget blueprint creation scripts
- JSON - Plugin descriptor and configuration

**Secondary:**
- Blueprints - Generated widget blueprint assets (output only)

## Runtime

**Environment:**
- Unreal Engine 4.26+ (currently UE4, being ported to UE5)
- Windows 64-bit (primary development target)
- macOS (secondary support)

**Package Manager:**
- Python Package Index (PyPI) - For dynamically downloading psd_tools library

## Frameworks

**Core:**
- Unreal Engine 4.26+ - Game engine framework
- UMG (Unreal Motion Graphics) - UI framework for widget creation
- Slate - UI rendering system (used via UMG)

**Editor:**
- Unreal Editor - For plugin development and UI asset generation
- Python Script Plugin - Embedded Python interpreter in Unreal Editor

**Asset Management:**
- Asset Registry Module - For asset tracking and loading
- Asset Tools Module - For programmatic asset creation and import

## Key Dependencies

**C++ Libraries (Engine-Provided):**
- Core - Unreal core utilities and types
- CoreUObject - Object system and reflection
- Engine - Core engine functionality
- Slate - UI framework layer
- SlateCore - Slate primitive rendering
- UMG - Unreal Motion Graphics UI toolkit
- UMGEditor - Editor-specific UMG tools
- EditorScriptingUtilities - Editor automation APIs
- DeveloperSettings - Configuration system
- AssetRegistry - Asset discovery and metadata

**Python Libraries (Bundled in ThirdParty):**

*Primary PSD Processing:*
- psd_tools 1.9.17 - PSD file parsing and layer manipulation
- Pillow (PIL) 8.2.0 - Image processing and export

*Image Processing & Scientific Computing:*
- numpy 1.20.3 - Numerical computing
- scikit-image 0.18.1 - Image processing algorithms
- scipy 1.6.3 - Scientific computing
- PyWavelets 1.1.1 - Image filtering and wavelets

*Visualization & Graphics:*
- matplotlib 3.4.2 - Data visualization
- imageio 2.9.0 - Image I/O
- aggdraw 1.3.12 - Image drawing

*Utilities:*
- attrs 21.2.0 - Class definitions
- cycler 0.10.0 - Cycling through data
- decorator 4.4.2 - Function decorators
- networkx 2.5.1 - Graph algorithms
- pyparsing 2.4.7 - Parsing library
- python-dateutil 2.8.1 - Date utilities
- six 1.12.0 - Python 2/3 compatibility
- tifffile 2021.4.8 - TIFF file I/O
- docopt 0.6.2 - Command-line interface

**Unreal Python APIs:**
- unreal (Unreal Python API) - Editor scripting and asset manipulation

## Configuration

**Environment:**
- No explicit .env configuration used
- Settings stored in Unreal Developer Settings system
- Configuration accessed via `UAutoPSDUISetting` class

**Build:**
- `AutoPSDUI.uplugin` - Plugin descriptor with module configuration
- `AutoPSDUI.Build.cs` - C++ module build rules
- Plugin targets Windows 64-bit and macOS platforms

**Plugin Descriptor (`AutoPSDUI.uplugin`):**
```json
{
  "FileVersion": 3,
  "Version": 1,
  "VersionName": "1.0",
  "EngineVersion": "4.26.0",
  "Modules": [
    {
      "Name": "AutoPSDUI",
      "Type": "Editor",
      "LoadingPhase": "PostEngineInit"
    }
  ],
  "Plugins": [
    {"Name": "PythonScriptPlugin", "Enabled": true},
    {"Name": "EditorScriptingUtilities", "Enabled": true}
  ]
}
```

## Platform Requirements

**Development:**
- Windows 10/11 with Visual Studio 2019+ (C++ compilation)
- Unreal Engine 4.26+ installed
- Python 3.7+ (from Unreal Engine's bundled Python)
- PSD file assets for testing

**Production:**
- Windows 64-bit Unreal Engine 4.26+
- macOS Unreal Engine 4.26+
- PSD source files (external asset)

## Python Interpreter

**Version:**
- Python 3.7+ (from Unreal Engine binary)
- Located: `Engine/Binaries/ThirdParty/Python3/[Win64|Mac]`

**Execution:**
- Invoked via `IPythonScriptPlugin::Get()->ExecPythonCommand()` from C++
- Command line: `auto_psd_ui.py -i <psd_file> -o <wbp_asset_path>`

---

*Stack analysis: 2026-04-07*

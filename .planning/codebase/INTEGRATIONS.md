# External Integrations

**Analysis Date:** 2026-04-07

## APIs & External Services

**PSD File Processing:**
- psd_tools - Parse and extract layer data from Adobe Photoshop files
  - SDK/Client: psd_tools 1.9.17
  - Location: `Source/ThirdParty/[Win64|Mac]/`
  - Dynamic Installation: Downloads from GitHub on first use if not present

**Image Processing:**
- Pillow/PIL - Export layer images as PNG files
  - SDK/Client: Pillow 8.2.0
  - Purpose: Compositing and exporting PSD layer pixels to standalone images

**Unreal Engine APIs:**
- Python Script Plugin - Embedded interpreter for Python automation
  - Integration: `IPythonScriptPlugin::Get()->ExecPythonCommand()` in `AutoPSDUILibrary.cpp`
  - Command execution: Passes Python code as strings

- Asset Tools - Programmatic asset creation
  - Client: `IAssetTools` module
  - Usage: Create Widget Blueprints in `AutoPSDUILibrary::CreateWBP()`

- Asset Registry - Asset discovery and loading
  - Client: `FAssetRegistryModule`
  - Usage: Load texture assets, check for existing widget blueprints

- Editor Scripting Utilities - Editor automation
  - Client: `EditorAssetLibrary`, `EditorScriptingUtilities`
  - Usage: Save assets, load assets, compile blueprints

## Data Storage

**Databases:**
- None - This is a stateless editor plugin

**File Storage:**
- Local filesystem only
  - PSD source files: User-provided external files
  - Texture exports: `Source/TextureSrcDir` (configurable, default: `Content/Textures`)
  - Widget blueprints: `/Game/` directory (configurable in settings)
  - Generated assets: Unreal Engine asset database

**Configuration Storage:**
- Unreal Developer Settings System
  - Stored in: `DefaultEditor.ini` and editor project settings
  - Accessed via: `UAutoPSDUISetting::Get()`
  - Persisted properties:
    - `bEnabled` - Plugin enable/disable flag
    - `TextureSrcDir` - Temporary texture export directory
    - `TextureAssetDir` - Final texture asset location in Unreal
    - `FontMap` - Mapping of font names to Unreal Font assets
    - `DefaultFont` - Fallback font for unmapped fonts

**Caching:**
- None explicit - Unreal Engine handles asset caching
- Third-party dependencies cached in: `Source/ThirdParty/`

## Authentication & Identity

**Auth Provider:**
- None - Plugin is editor-only, no user authentication

## Monitoring & Observability

**Error Tracking:**
- None - Relies on Unreal Editor logs

**Logs:**
- Unreal Editor Log System
  - Implementation: `unreal.log()`, `unreal.log_warning()`, `unreal.log_error()` in Python
  - Implementation: `UE_LOG()` macros in C++
  - Python errors logged to Unreal Editor output log
  - File location: `Saved/Logs/`

**Editor Dialogs:**
- Modal user prompts for dependency installation failures
  - `unreal.EditorDialog.show_message()` - Warns user if psd_tools library missing
  - Option to cancel workflow if dependencies unavailable

## CI/CD & Deployment

**Hosting:**
- GitHub repository (documentation and source)
- GitHub Releases - For plugin distribution
- GitHub (JohnSnowWind/AutoPSDUI_Dependencies) - Third-party library hosting

**CI Pipeline:**
- Not detected in current codebase
- Manual build and release process

**Marketplace:**
- Epic Games Launcher Marketplace
  - Product ID: `d5362c4d44784473ad84d946d95659e3`
  - Listed in: `AutoPSDUI.uplugin` MarketplaceURL

**Documentation:**
- GitHub documentation repository: `https://github.com/JohnSnowWind/AutoPSDUIDoc`

## Environment Configuration

**Required environment variables:**
- None explicitly required

**System Environment:**
- Unreal Engine Python binary path automatically resolved
  - Windows: `Engine/Binaries/ThirdParty/Python3/Win64/python.exe`
  - macOS: `Engine/Binaries/ThirdParty/Python3/Mac/python`

**Optional Configuration (Editor Settings):**
- `bEnabled` - Enable/disable plugin (default: enabled)
- `TextureSrcDir` - Temporary texture output directory
- `TextureAssetDir` - Final texture asset path in Unreal project
- `FontMap` - Font name to asset mapping
- `DefaultFont` - Default font fallback

## Webhooks & Callbacks

**Incoming:**
- Asset reimport callback: Triggered when PSD texture imported
  - Hook: `UImportSubsystem::OnAssetReimport`
  - Handler: `FAutoPSDUIModule::OnPSDImport()` in `AutoPSDUI.cpp`
  - Trigger condition: User imports/re-imports a `.psd` file as texture

**Outgoing:**
- Asset creation callbacks (implicit)
  - Generated widget blueprints trigger standard Unreal asset callbacks
  - No explicit external webhooks

## Third-Party Dependency Management

**Initial Install:**
- Dependencies pre-bundled in `Source/ThirdParty/Win64/` and `Source/ThirdParty/Mac/`
- Located in plugin directory (committed to repository)

**Runtime Install Fallback:**
- If `psd_tools` import fails, user prompted to download
- Download source: `https://github.com/JohnSnowWind/AutoPSDUI_Dependencies/archive/refs/tags/v1.0.zip`
- Installation logic in `Content/Python/AutoPSDUI/common.py::download_dependencies()`
- Downloaded to: `Source/ThirdParty/` directory
- Process: Download → Unzip → Add to `sys.path`

**Python Path Configuration:**
- Plugin setup in `Content/Python/AutoPSDUI/common.py`
- Platform-specific paths:
  - Win64: `Source/ThirdParty/Win64/`
  - macOS: `Source/ThirdParty/Mac/`
- Appended to `sys.path` before imports

## Key Integration Points

**C++ to Python Bridge:**
1. C++ detects PSD file import via Asset Registry
2. `FAutoPSDUIModule::OnPSDImport()` extracts PSD file path
3. Constructs Python command: `auto_psd_ui.py -i <psd_path> -o <wbp_path>`
4. Executes via `IPythonScriptPlugin::Get()->ExecPythonCommand()`

**Python to Unreal Bridge:**
1. Python `unreal` module accesses Editor APIs
2. `unreal.AssetToolsHelpers.get_asset_tools()` - Import images
3. `unreal.EditorAssetLibrary` - Load/save assets
4. `unreal.AutoPSDUILibrary` - Custom C++ functions exposed to Python
5. Creates widget hierarchy using UMG widgets

**Asset Flow:**
1. User imports PSD texture → Unreal Editor
2. Plugin detects import → Triggers Python script
3. Python parses PSD → Exports layer images → Creates widget blueprint
4. Blueprint saved to game project

---

*Integration audit: 2026-04-07*

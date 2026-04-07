# Architecture

**Analysis Date:** 2026-04-07

## Pattern Overview

**Overall:** Layered Editor Plugin with Hybrid C++/Python Pipeline

**Key Characteristics:**
- Editor-only plugin (loads at PostEngineInit)
- Event-driven architecture: PSD import triggers automatic Widget Blueprint generation
- Two-layer processing: C++ for UE integration, Python for PSD parsing and widget construction
- Asynchronous Python execution via Unreal's PythonScriptPlugin
- Configuration-driven behavior via DeveloperSettings

## Layers

**C++ Editor Plugin Layer:**
- Purpose: Hooks into Unreal's asset import pipeline, manages module lifecycle, exposes C++ utilities to Python
- Location: `Source/AutoPSDUI/`
- Contains: Module initialization, asset import listeners, C++ function library, developer settings
- Depends on: Core UE Editor modules (UnrealEd, UMGEditor, EditorScriptingUtilities, PythonScriptPlugin)
- Used by: Python runtime environment

**Python Processing Layer:**
- Purpose: Parses PSD files, extracts layer hierarchy and visual properties, generates widget hierarchies, manages asset creation
- Location: `Content/Python/`
- Contains: Main orchestration script, PSD parsing utilities, widget creation functions, dependency management
- Depends on: psd_tools (third-party Python library), UE Python API (unreal module)
- Used by: C++ layer via IPythonScriptPlugin

**Data Models:**
- **PSD Content Structure:** Dictionary-based nested representation of PSD layers with type, position, dimensions, and visual properties
- **Widget Blueprint Schema:** Maps PSD types (Canvas, Button, Image, Text, ProgressBar, ListView, TileView) to UMG widget classes
- **Asset Paths:** Content paths in `/Game/` format for widget blueprints and textures

## Data Flow

**Import-Triggered Widget Generation:**

1. User imports PSD file into Unreal Editor
2. `FAutoPSDUIModule::OnPSDImport()` fires (registered with UImportSubsystem in StartupModule)
3. Extract source PSD file path from UTexture2D import data
4. Construct Python command: `-i <psd_path> -o <wbp_path>`
5. Pass to `UAutoPSDUILibrary::RunPyCmd()` → invokes `IPythonScriptPlugin::ExecPythonCommand()`
6. Python script executes asynchronously:
   - Load PSD file with psd_tools
   - Parse layer hierarchy recursively (Canvas, Button, Image, Text, ProgressBar, ListView, TileView)
   - Export layer images to texture directory
   - Create or load target Widget Blueprint
   - Construct widget hierarchy in blueprint's WidgetTree
   - Apply interfaces (ListEntryInterface for ListView/TileView items)
   - Compile and save blueprint
7. Result: Fully populated UMG Widget Blueprint ready for use

**State Management:**
- **Plugin State:** Module-level settings stored in `UAutoPSDUISetting` (DeveloperSettings)
- **Blueprint State:** Widget hierarchy persisted in `.uasset` (blueprint asset)
- **Image Assets:** Texture2D assets created during import, linked to widgets via SlateBrush
- **Temporary State:** During Python execution, PSD structure held in memory as nested dictionaries

## Key Abstractions

**Module Interface (C++):**
- Purpose: Plugin lifecycle management and event subscription
- Location: `Source/AutoPSDUI/Public/AutoPSDUI.h`
- Pattern: Standard Unreal IModuleInterface implementation
- Responsibilities: Register import callback on startup, clean up on shutdown

**C++ Function Library:**
- Purpose: Expose critical UE operations to Python runtime
- Location: `Source/AutoPSDUI/Public/AutoPSDUILibrary.h`, `Source/AutoPSDUI/Private/AutoPSDUILibrary.cpp`
- Pattern: UBlueprintFunctionLibrary (static functions, no instances)
- Operations:
  - `RunPyCmd()`: Execute Python commands via IPythonScriptPlugin
  - `CreateWBP()`: Factory pattern for widget blueprint creation
  - `MakeWidgetWithWBP()`: Construct widgets in blueprint's widget tree
  - `SetWBPRootWidget()`: Set root canvas panel
  - `CompileAndSaveBP()`: Compile and persist blueprint
  - `ApplyInterfaceToBP()`: Add interfaces to blueprint (ListEntryInterface)
  - `GetBPGeneratedClass()`: Retrieve generated class from blueprint

**Settings Manager:**
- Purpose: Centralized configuration for texture paths, font mapping, enabled state
- Location: `Source/AutoPSDUI/Public/AutoPSDUISetting.h`
- Pattern: DeveloperSettings singleton
- Configuration: `bool bEnabled`, font map (string → UFont*), texture source/asset directories

**PSD Parser:**
- Purpose: Transform Photoshop layer tree into widget-compatible data structure
- Location: `Content/Python/AutoPSDUI/psd_utils.py`
- Pattern: Recursive descent with type-based dispatch
- Key Functions:
  - `load_psd()`: Open PSD file with psd_tools
  - `parse_psd()`: Convert top-level PSD to Canvas + recursively parse children
  - `parse_layer()`: Type-dispatch based on layer name prefix (Button_, Progress_, List_, Tile_)
  - `parse_button()`, `parse_image()`, `parse_text()`, `parse_progress_bar()`: Extract type-specific properties
  - Layer images exported to disk as PNG files during parsing

**Widget Factory:**
- Purpose: Create UMG widget instances in blueprint context
- Location: `Content/Python/auto_psd_ui.py` (functions like `create_canvas()`, `create_button()`, `create_image()`, etc.)
- Pattern: Type-based factory dispatch with recursive composition
- Responsibilities:
  - Instantiate UWidget subclasses in WidgetTree
  - Set visual properties (position, size, colors, fonts, brushes)
  - Handle parent-child relationships via CanvasPanel slots
  - Manage nested widget hierarchies (e.g., canvas inside button)

## Entry Points

**C++ Module Entry:**
- Location: `Source/AutoPSDUI/Private/AutoPSDUI.cpp`
- Triggers: Engine initialization (PostEngineInit loading phase)
- Responsibilities: Register OnPSDImport callback with UImportSubsystem

**Import Trigger:**
- Method: `FAutoPSDUIModule::OnPSDImport(UObject* PSDTextureAsset)`
- Condition: Fires when texture asset is imported, checks bEnabled and validates .psd source
- Responsibility: Initiate Python pipeline

**Python Entry Point:**
- Location: `Content/Python/auto_psd_ui.py`
- Invocation: Via `IPythonScriptPlugin::ExecPythonCommand()` with args: `-i <src.psd> -o /Game/Path/WBP_Name`
- Main Logic: `main()` function orchestrates: parse → fix names → import images → create blueprint hierarchy

## Error Handling

**Strategy:** Graceful degradation with logging

**Patterns:**

- **Plugin Disabled Check:** `if (!UAutoPSDUISetting::Get()->bEnabled) return;` - Silent exit if user disabled feature
- **Asset Validation:** Check UTexture2D cast, verify import data exists, confirm .psd extension
- **PSD File Existence:** `if (!os.path.exists(p_psd_file))` - Return None if file not found
- **Library Dependency:** `check_psd_tools()` - Prompt user to download psd_tools if missing, return False if user cancels
- **Asset Loading:** `unreal.EditorAssetLibrary.load_asset()` returns None if asset not found, widgets check before using
- **Layer Type Unknown:** Log warning via `unreal.log_warning()`, skip unknown layer types
- **Missing Images:** Collect in `invalid_image_list`, skip invalid image paths without crashing
- **Unreal Logging:** Use `unreal.log()` and `unreal.log_error()` for Python-side messages
- **Compilation Failures:** `CompileAndSaveBP()` via FKismetEditorUtilities handles compilation errors

## Cross-Cutting Concerns

**Logging:**
- C++: Standard `UE_LOG` macros (none currently used in module)
- Python: `unreal.log()`, `unreal.log_warning()`, `unreal.log_error()` for debug output
- User Dialogs: `unreal.EditorDialog.show_message()` for critical prompts (dependency download)

**Validation:**
- Type checking: C++ static casting (UTexture2D)
- Path validation: File existence checks, PSD extension verification
- Asset validation: Check blueprint exists before loading, validate widget tree operations
- Layer type validation: Prefix-based (Button_, Progress_, List_, Tile_) for custom types

**Configuration:**
- DeveloperSettings (UAutoPSDUISetting) for plugin-wide options
- Environment: Python platform detection (win32 vs darwin) for third-party directory paths
- Feature Flags: `bEnabled` controls entire import pipeline

---

*Architecture analysis: 2026-04-07*

<!-- GSD:project-start source:PROJECT.md -->
## Project

**PSD2UMG**

A production-grade Unreal Engine 5.7 editor plugin that imports `.psd` files and converts them into fully functional UMG Widget Blueprints in one click. It preserves layer hierarchy, positions, text properties, images, and effects — so a designer's Photoshop mockup becomes working UI without manual reconstruction. Targets both internal team use and public open-source release.

**Core Value:** A designer drops a PSD into Unreal Editor and gets a correctly structured, immediately usable Widget Blueprint — with no Python dependency, no manual tweaking, and no loss of layer intent.

### Constraints

- **Engine**: UE 5.7.4 — must use UE5 APIs (FAppStyle not FEditorStyle, PlatformAllowList not WhitelistPlatforms, etc.)
- **Language**: C++20 — CppStandard = CppStandardVersion.Cpp20 in Build.cs
- **No Python at runtime**: PythonScriptPlugin dependency removed from plugin core
- **PhotoshopAPI linkage**: Pre-built static lib via CMake (not compiled inside UE build system) — avoids CMake/UBT conflicts
- **Editor-only**: Module type "Editor", LoadingPhase "PostEngineInit" — no shipping module
- **License**: MIT (same as original AutoPSDUI fork)
<!-- GSD:project-end -->

<!-- GSD:stack-start source:codebase/STACK.md -->
## Technology Stack

## Languages
- C++ (C++17) - Plugin core logic for UMG widget generation and asset handling
- Python 3 - PSD parsing and widget blueprint creation scripts
- JSON - Plugin descriptor and configuration
- Blueprints - Generated widget blueprint assets (output only)
## Runtime
- Unreal Engine 4.26+ (currently UE4, being ported to UE5)
- Windows 64-bit (primary development target)
- macOS (secondary support)
- Python Package Index (PyPI) - For dynamically downloading psd_tools library
## Frameworks
- Unreal Engine 4.26+ - Game engine framework
- UMG (Unreal Motion Graphics) - UI framework for widget creation
- Slate - UI rendering system (used via UMG)
- Unreal Editor - For plugin development and UI asset generation
- Python Script Plugin - Embedded Python interpreter in Unreal Editor
- Asset Registry Module - For asset tracking and loading
- Asset Tools Module - For programmatic asset creation and import
## Key Dependencies
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
- psd_tools 1.9.17 - PSD file parsing and layer manipulation
- Pillow (PIL) 8.2.0 - Image processing and export
- numpy 1.20.3 - Numerical computing
- scikit-image 0.18.1 - Image processing algorithms
- scipy 1.6.3 - Scientific computing
- PyWavelets 1.1.1 - Image filtering and wavelets
- matplotlib 3.4.2 - Data visualization
- imageio 2.9.0 - Image I/O
- aggdraw 1.3.12 - Image drawing
- attrs 21.2.0 - Class definitions
- cycler 0.10.0 - Cycling through data
- decorator 4.4.2 - Function decorators
- networkx 2.5.1 - Graph algorithms
- pyparsing 2.4.7 - Parsing library
- python-dateutil 2.8.1 - Date utilities
- six 1.12.0 - Python 2/3 compatibility
- tifffile 2021.4.8 - TIFF file I/O
- docopt 0.6.2 - Command-line interface
- unreal (Unreal Python API) - Editor scripting and asset manipulation
## Configuration
- No explicit .env configuration used
- Settings stored in Unreal Developer Settings system
- Configuration accessed via `UAutoPSDUISetting` class
- `AutoPSDUI.uplugin` - Plugin descriptor with module configuration
- `AutoPSDUI.Build.cs` - C++ module build rules
- Plugin targets Windows 64-bit and macOS platforms
## Platform Requirements
- Windows 10/11 with Visual Studio 2019+ (C++ compilation)
- Unreal Engine 4.26+ installed
- Python 3.7+ (from Unreal Engine's bundled Python)
- PSD file assets for testing
- Windows 64-bit Unreal Engine 4.26+
- macOS Unreal Engine 4.26+
- PSD source files (external asset)
## Python Interpreter
- Python 3.7+ (from Unreal Engine binary)
- Located: `Engine/Binaries/ThirdParty/Python3/[Win64|Mac]`
- Invoked via `IPythonScriptPlugin::Get()->ExecPythonCommand()` from C++
- Command line: `auto_psd_ui.py -i <psd_file> -o <wbp_asset_path>`
<!-- GSD:stack-end -->

<!-- GSD:conventions-start source:CONVENTIONS.md -->
## Conventions

## Language Overview
- **C++**: Unreal Engine 5 plugin code (UE5, porting from UE4)
- **Python**: Unreal Python scripting API for PSD processing
## C++ Conventions (Unreal Engine)
### Naming Patterns
- Prefix with `U` for UObject-derived classes: `UAutoPSDUILibrary`, `UAutoPSDUISetting`
- Prefix with `F` for non-UObject value types (not used in this plugin)
- Module class prefix with `F`: `FAutoPSDUIModule`
- Always in PascalCase
- Public functions in UObject classes follow Unreal conventions
- Methods: `StartupModule()`, `ShutdownModule()`, `CreateWBP()`
- Always PascalCase
- Boolean members prefixed with `b`: `bEnabled`
- Private/protected members have no special prefix
- Function parameters often prefixed with `p_` (legacy style): Parameter naming varies
- Use `TEXT()` macro for string literals: `TEXT(".psd")`, `TEXT("AutoPSDUI")`
- Use Unreal string types: `FString`, `FDirectoryPath`
- Unreal macro system: `UCLASS()`, `UFUNCTION()`, `UPROPERTY()`, `GENERATED_BODY()`
### File Organization
- Location: `Source/AutoPSDUI/Public/*.h`
- Pattern: One public class per header file
- Include guards: `#pragma once`
- Example files:
- Location: `Source/AutoPSDUI/Private/*.cpp`
- Include pattern: Include own header first, then dependencies
- Example: `AutoPSDUILibrary.cpp` includes `AutoPSDUILibrary.h` then other headers
### Class Declarations
### Metadata and Reflection
- `EditAnywhere`: Editable in editor details panel
- `config`: Saved to configuration file
- `BlueprintReadWrite`: Accessible from Blueprint
- `Category`: Used for UI grouping
- `meta=(LongPackageName)`: Path selection hint
- `BlueprintCallable`: Callable from Blueprint
- `Category`: Functional grouping in Blueprint menu
- `config = Editor`: Configuration saved to Editor ini
- `defaultconfig`: Use default configuration
### Comments
- Single-line comments: `// Comment`
- Multi-line comments: Less common in this codebase
- Minimal documentation; copyright header present on each file
- Example: `// Copyright 2018-2021 - John snow wind`
- Very minimal inline documentation
- No doxygen/comment blocks for functions in the main code
- Comments mainly for copyright and occasional TODO/logic clarification
### Include Organization
- Uses explicit module includes for AssetRegistry, Python, widget tools
- No barrel imports; imports are specific
### Error Handling (C++)
- Use `Cast<TargetType>()` to safely downcast
- Check result against nullptr before use
- No exceptions; all errors handled via return/null checks
- Modern Unreal uses `nullptr` instead of NULL or 0
- No explicit null pointer exception throwing
### Code Style
- Braces: Allman style (opening brace on same line)
- Indentation: Tabs (standard Unreal)
- Maximum line length: No strict limit observed
- Spacing: Standard around operators
- Functions are small and focused
- `OnPSDImport()` in module: Single responsibility (14 lines)
- Library functions: Thin wrappers around Unreal API
### Module Dependencies
- Public dependencies: `Core`
- Private dependencies: `CoreUObject`, `Engine`, `Slate`, `SlateCore`, `DeveloperSettings`, `PythonScriptPlugin`, `UnrealEd`, `UMGEditor`, `UMG`, `AssetRegistry`, `EditorScriptingUtilities`
- No external third-party dependencies in C++; uses Unreal-provided libraries
## Python Conventions
### Naming Patterns
- Lowercase with underscores: `auto_psd_ui.py`, `psd_utils.py`
- Package structure: `AutoPSDUI/` (follows import naming)
- Imports: `from AutoPSDUI import common`, `from AutoPSDUI.psd_utils import load_psd`
- Lowercase with underscores: `parse_args()`, `check_psd_tools()`, `fix_names()`, `create_canvas()`
- Prefix `p_` for parameters: `p_psd_content`, `p_layer`, `p_button_info`
- Intent-based names: `gather_psd_images()`, `import_images()`, `process_child_layer()`
- Lowercase with underscores: `input_file`, `output_asset`, `image_list`, `name_set`
- Dictionary keys: PascalCase: `"Type"`, `"Name"`, `"X"`, `"Y"`, `"Children"`
- Global module vars: `psd_gui_setting`, `dst_path`, `default_font`, `font_map`
- Private globals: `_dir` suffix: `module_dir`, `third_party_dir`, `target_dir`
- Modern type hints used: `p_layer: Layers.PixelLayer`, `dst_path: str`, `p_psd_content: PSDImage`
- Return types not consistently annotated
### File Organization
- `Content/Python/auto_psd_ui.py` (571 lines)
- Single file containing orchestration and widget creation logic
- Entry via `if __name__ == "__main__"`
- `Content/Python/AutoPSDUI/common.py`: Dependency management, settings access
- `Content/Python/AutoPSDUI/psd_utils.py`: PSD parsing and layer processing
- `Content/Python/AutoPSDUI/__init__.py`: Empty module marker
### Import Organization
- Used for development: `reload_module()` in `auto_psd_ui.py` reloads all AutoPSDUI modules
- Enables iteration without restarting engine
### Comments
- Sparse inline comments
- Docstring pattern used occasionally:
- Comments used for clarification on non-obvious logic
- Chinese comments in some functions (legacy): `# 处理颜色叠加` (handle color overlay), `# 其他情况均按照Canvas处理` (others are treated as Canvas)
- Limited documentation on complex parsing functions
### Error Handling
- `unreal.log()`: Info level
- `unreal.log_warning()`: Warning level
- `unreal.log_error()`: Error level
### Data Structures
- Layer information passed as dictionaries throughout codebase
- Unreal objects (UClass instances) used only at widget creation boundary
- Set-based collections for uniqueness: `name_set`, `image_list`, `invalid_image_list`
### Function Design
- Heavy use of dictionary parameters: `p_psd_content`, `p_button_info`
- Multiple return types: `bool`, `dict`, `None`, objects
### Global State
- Globals initialized at module load: `psd_gui_setting`, `third_party_dir`, `default_font`, `font_map`
- Mutable globals modified during execution: `dst_path` in `auto_psd_ui.py`
### Unreal API Conventions
- Unreal C++ `SetWBPRootWidget()` exposed as `set_wbp_root_widget()`
- Unreal C++ `CreateWBP()` exposed as `create_wbp()`
- Unreal class access: `unreal.CanvasPanel.static_class()`, `unreal.TextBlock.static_class()`
## Cross-Language Patterns
### Separation of Concerns
### Data Flow
### Configuration
- C++ class `UAutoPSDUISetting` exposes configuration in editor
- Accessed from Python via `unreal.AutoPSDUISetting.get()`
- Includes font mappings, texture directories, enable flag
## Key Deviations from Standards
<!-- GSD:conventions-end -->

<!-- GSD:architecture-start source:ARCHITECTURE.md -->
## Architecture

## Pattern Overview
- Editor-only plugin (loads at PostEngineInit)
- Event-driven architecture: PSD import triggers automatic Widget Blueprint generation
- Two-layer processing: C++ for UE integration, Python for PSD parsing and widget construction
- Asynchronous Python execution via Unreal's PythonScriptPlugin
- Configuration-driven behavior via DeveloperSettings
## Layers
- Purpose: Hooks into Unreal's asset import pipeline, manages module lifecycle, exposes C++ utilities to Python
- Location: `Source/AutoPSDUI/`
- Contains: Module initialization, asset import listeners, C++ function library, developer settings
- Depends on: Core UE Editor modules (UnrealEd, UMGEditor, EditorScriptingUtilities, PythonScriptPlugin)
- Used by: Python runtime environment
- Purpose: Parses PSD files, extracts layer hierarchy and visual properties, generates widget hierarchies, manages asset creation
- Location: `Content/Python/`
- Contains: Main orchestration script, PSD parsing utilities, widget creation functions, dependency management
- Depends on: psd_tools (third-party Python library), UE Python API (unreal module)
- Used by: C++ layer via IPythonScriptPlugin
- **PSD Content Structure:** Dictionary-based nested representation of PSD layers with type, position, dimensions, and visual properties
- **Widget Blueprint Schema:** Maps PSD types (Canvas, Button, Image, Text, ProgressBar, ListView, TileView) to UMG widget classes
- **Asset Paths:** Content paths in `/Game/` format for widget blueprints and textures
## Data Flow
- **Plugin State:** Module-level settings stored in `UAutoPSDUISetting` (DeveloperSettings)
- **Blueprint State:** Widget hierarchy persisted in `.uasset` (blueprint asset)
- **Image Assets:** Texture2D assets created during import, linked to widgets via SlateBrush
- **Temporary State:** During Python execution, PSD structure held in memory as nested dictionaries
## Key Abstractions
- Purpose: Plugin lifecycle management and event subscription
- Location: `Source/AutoPSDUI/Public/AutoPSDUI.h`
- Pattern: Standard Unreal IModuleInterface implementation
- Responsibilities: Register import callback on startup, clean up on shutdown
- Purpose: Expose critical UE operations to Python runtime
- Location: `Source/AutoPSDUI/Public/AutoPSDUILibrary.h`, `Source/AutoPSDUI/Private/AutoPSDUILibrary.cpp`
- Pattern: UBlueprintFunctionLibrary (static functions, no instances)
- Operations:
- Purpose: Centralized configuration for texture paths, font mapping, enabled state
- Location: `Source/AutoPSDUI/Public/AutoPSDUISetting.h`
- Pattern: DeveloperSettings singleton
- Configuration: `bool bEnabled`, font map (string → UFont*), texture source/asset directories
- Purpose: Transform Photoshop layer tree into widget-compatible data structure
- Location: `Content/Python/AutoPSDUI/psd_utils.py`
- Pattern: Recursive descent with type-based dispatch
- Key Functions:
- Purpose: Create UMG widget instances in blueprint context
- Location: `Content/Python/auto_psd_ui.py` (functions like `create_canvas()`, `create_button()`, `create_image()`, etc.)
- Pattern: Type-based factory dispatch with recursive composition
- Responsibilities:
## Entry Points
- Location: `Source/AutoPSDUI/Private/AutoPSDUI.cpp`
- Triggers: Engine initialization (PostEngineInit loading phase)
- Responsibilities: Register OnPSDImport callback with UImportSubsystem
- Method: `FAutoPSDUIModule::OnPSDImport(UObject* PSDTextureAsset)`
- Condition: Fires when texture asset is imported, checks bEnabled and validates .psd source
- Responsibility: Initiate Python pipeline
- Location: `Content/Python/auto_psd_ui.py`
- Invocation: Via `IPythonScriptPlugin::ExecPythonCommand()` with args: `-i <src.psd> -o /Game/Path/WBP_Name`
- Main Logic: `main()` function orchestrates: parse → fix names → import images → create blueprint hierarchy
## Error Handling
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
- C++: Standard `UE_LOG` macros (none currently used in module)
- Python: `unreal.log()`, `unreal.log_warning()`, `unreal.log_error()` for debug output
- User Dialogs: `unreal.EditorDialog.show_message()` for critical prompts (dependency download)
- Type checking: C++ static casting (UTexture2D)
- Path validation: File existence checks, PSD extension verification
- Asset validation: Check blueprint exists before loading, validate widget tree operations
- Layer type validation: Prefix-based (Button_, Progress_, List_, Tile_) for custom types
- DeveloperSettings (UAutoPSDUISetting) for plugin-wide options
- Environment: Python platform detection (win32 vs darwin) for third-party directory paths
- Feature Flags: `bEnabled` controls entire import pipeline
<!-- GSD:architecture-end -->

<!-- GSD:workflow-start source:GSD defaults -->
## GSD Workflow Enforcement

Before using Edit, Write, or other file-changing tools, start work through a GSD command so planning artifacts and execution context stay in sync.

Use these entry points:
- `/gsd:quick` for small fixes, doc updates, and ad-hoc tasks
- `/gsd:debug` for investigation and bug fixing
- `/gsd:execute-phase` for planned phase work

Do not make direct repo edits outside a GSD workflow unless the user explicitly asks to bypass it.
<!-- GSD:workflow-end -->



<!-- GSD:profile-start -->
## Developer Profile

> Profile not yet configured. Run `/gsd:profile-user` to generate your developer profile.
> This section is managed by `generate-claude-profile` -- do not edit manually.
<!-- GSD:profile-end -->

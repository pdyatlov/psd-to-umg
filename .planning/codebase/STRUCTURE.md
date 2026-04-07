# Codebase Structure

**Analysis Date:** 2026-04-07

## Directory Layout

```
psd-to-umg/
├── Source/                          # C++ plugin source
│   └── AutoPSDUI/
│       ├── Public/                  # Header files (exported API)
│       │   ├── AutoPSDUI.h
│       │   ├── AutoPSDUILibrary.h
│       │   └── AutoPSDUISetting.h
│       ├── Private/                 # Implementation files
│       │   ├── AutoPSDUI.cpp
│       │   ├── AutoPSDUILibrary.cpp
│       │   └── AutoPSDUISetting.cpp
│       ├── AutoPSDUI.Build.cs       # Build configuration
│       └── Resources/               # Plugin resources (icons, etc.)
├── Content/                         # Plugin content (Python, assets)
│   └── Python/
│       ├── auto_psd_ui.py          # Main orchestration script
│       └── AutoPSDUI/              # Python package
│           ├── __init__.py
│           ├── common.py           # Utilities (path normalization, dependency download)
│           └── psd_utils.py        # PSD parsing and image extraction
├── Config/                         # Plugin configuration
│   └── FilterPlugin.ini            # Plugin filter settings
├── AutoPSDUI.uplugin              # Plugin manifest
└── README.md                       # Documentation
```

## Directory Purposes

**Source/AutoPSDUI/Public/:**
- Purpose: Exported C++ API headers
- Contains: Header files for module interface, function library, and settings
- Key files: `AutoPSDUI.h` (module), `AutoPSDUILibrary.h` (blueprint-callable utilities), `AutoPSDUISetting.h` (configuration)

**Source/AutoPSDUI/Private/:**
- Purpose: Implementation files for C++ layer
- Contains: Module startup/shutdown, library implementations, settings initialization
- Key files: `AutoPSDUI.cpp` (event subscription), `AutoPSDUILibrary.cpp` (UE operations), `AutoPSDUISetting.cpp` (defaults)

**Source/AutoPSDUI/:**
- Purpose: Plugin module root
- Contains: Build configuration, module interface
- Key files: `AutoPSDUI.Build.cs` (dependencies: CoreUObject, Engine, Slate, SlateCore, UnrealEd, UMGEditor, UMG, PythonScriptPlugin, EditorScriptingUtilities)

**Content/Python/:**
- Purpose: Python runtime scripts and modules
- Contains: Main entry point and utility modules
- Key files: `auto_psd_ui.py` (orchestrator), `AutoPSDUI/psd_utils.py` (parser), `AutoPSDUI/common.py` (helpers)

**Content/Python/AutoPSDUI/:**
- Purpose: Python package with parsing and utility functions
- Contains: Modules for PSD parsing, common utilities, dependency management
- Key files: `psd_utils.py` (PSD → data structure), `common.py` (platform detection, dependency download)

**Config/:**
- Purpose: Plugin configuration files
- Contains: FilterPlugin.ini (defines which categories the plugin appears in)
- Key files: `FilterPlugin.ini`

## Key File Locations

**Entry Points:**

- `Source/AutoPSDUI/Private/AutoPSDUI.cpp`: C++ module entry point
  - Implements `FAutoPSDUIModule::StartupModule()` - registers PSD import callback
  - Implements `FAutoPSDUIModule::OnPSDImport()` - triggered on texture import

- `Content/Python/auto_psd_ui.py`: Python entry point
  - Invoked via `RunPyCmd()` with args: `-i <psd_path> -o <wbp_path>`
  - `main()` function: orchestrates PSD parsing → image import → blueprint creation

**Configuration:**

- `AutoPSDUI.uplugin`: Plugin manifest
  - Declares module "AutoPSDUI" as Editor plugin
  - Specifies dependencies: PythonScriptPlugin, EditorScriptingUtilities
  - Engine version: 4.26.0 (UE4, being ported to UE5)

- `Source/AutoPSDUI/AutoPSDUI.Build.cs`: Build configuration
  - Public dependencies: Core
  - Private dependencies: CoreUObject, Engine, Slate, SlateCore, DeveloperSettings, PythonScriptPlugin, UnrealEd, UMGEditor, UMG, AssetRegistry, EditorScriptingUtilities

- `Source/AutoPSDUI/Public/AutoPSDUISetting.h`: Developer settings class
  - `bEnabled`: Toggle entire pipeline (default: true)
  - `TextureSrcDir`: Disk path for exported layer images (default: `Project/Art/UI/Texture`)
  - `TextureAssetDir`: UE asset path for textures (default: `/Game/Widgets/Texture`)
  - `FontMap`: String → Font* mapping for PSD text styles
  - `DefaultFont`: Fallback font if text font not in map

**Core Logic:**

- `Source/AutoPSDUI/Public/AutoPSDUILibrary.h`: C++ function library (8 static functions)
  - `RunPyCmd()`: Execute Python command
  - `CreateWBP()`: Create new Widget Blueprint asset
  - `MakeWidgetWithWBP()`: Instantiate widget in blueprint's WidgetTree
  - `SetWBPRootWidget()`: Set canvas as root
  - `CompileAndSaveBP()`: Compile and save blueprint
  - `ApplyInterfaceToBP()`: Add interface to blueprint
  - `GetBPGeneratedClass()`: Get generated class from blueprint
  - `ImportImage()`: Stub (not implemented)

- `Content/Python/AutoPSDUI/psd_utils.py`: PSD parsing (400+ lines)
  - `load_psd()`: Open PSD with psd_tools
  - `parse_psd()`: Root parser → Canvas
  - `parse_layer()`: Dispatch based on layer name prefix
  - `parse_button()`, `parse_image()`, `parse_text()`, `parse_progress_bar()`, `parse_list()`, `parse_tile()`: Type-specific parsers
  - `export_image()`: Save layer as PNG to texture_src_dir
  - `gather_psd_images()`: Collect all image references
  - `fix_image_link()`: Update paths to imported assets

- `Content/Python/auto_psd_ui.py`: Orchestration (570+ lines)
  - `parse_args()`: Extract -i (input) and -o (output) flags
  - `check_psd_tools()`: Verify psd_tools installed, prompt download if missing
  - `fix_names()`: Ensure widget names are unique in hierarchy
  - `create_canvas()`, `create_button()`, `create_image()`, `create_text()`, `create_progress_bar()`, `create_list_view()`, `create_tile_view()`: Widget creation (type-specific implementations)
  - `process_parent_widget()`: Set position/size on canvas slot
  - `gather_list_tile_children()`: Find ListView/TileView items for blueprint creation

**Testing:**

- No test files found (no automated testing framework detected)

## Naming Conventions

**Files:**

- C++ Headers: `CamelCaseWithoutSpaces.h` (e.g., `AutoPSDUILibrary.h`)
- C++ Implementation: `CamelCaseWithoutSpaces.cpp` (e.g., `AutoPSDUILibrary.cpp`)
- Python Scripts: `snake_case.py` (e.g., `auto_psd_ui.py`)
- Python Packages: `snake_case/` (e.g., `AutoPSDUI/`)
- Build Config: `ModuleName.Build.cs` (e.g., `AutoPSDUI.Build.cs`)
- Plugin Manifest: `PluginName.uplugin` (e.g., `AutoPSDUI.uplugin`)

**C++ Classes and Functions:**

- Module: `FAutoPSDUIModule` (F prefix for module)
- Function Library: `UAutoPSDUILibrary` (U prefix for Unreal-exposed class)
- Settings: `UAutoPSDUISetting` (U prefix, singular)
- Functions: `PascalCase()` with verb prefix (e.g., `RunPyCmd()`, `CreateWBP()`)

**Python Functions:**

- Functions: `snake_case()` (e.g., `parse_psd()`, `create_canvas()`)
- Constants: `UPPER_SNAKE_CASE()` where applicable (none observed)

**Naming Patterns:**

- PSD Layer Prefixes (custom types): `Button_`, `Progress_`, `List_`, `Tile_`, `Image_` (implicit)
- Image Layer Suffixes (state variants): `_normal`, `_hovered`, `_pressed`, `_disabled` (buttons); `_background`, `_fill` (progress bars)
- Widget Blueprint Naming: `WBP_<DescritiveName>` (e.g., `WBP_MainMenu`)
- Asset Paths: `/Game/Widgets/Texture/<ImageName>` or `/Game/<WidgetName>`
- Widget Instance Names: Derived from PSD layer names (may be deduplicated with `_1`, `_2` suffixes)

**Directories:**

- Source module: `Source/<ModuleName>/` (e.g., `Source/AutoPSDUI/`)
- Public headers: `Public/` (exported API)
- Private implementation: `Private/` (internal)
- Python modules: `Content/Python/<PackageName>/` (e.g., `Content/Python/AutoPSDUI/`)
- Configuration: `Config/` (project-specific settings)

## Where to Add New Code

**New Widget Type Support:**
- Add type parser in `Content/Python/AutoPSDUI/psd_utils.py`: New function `parse_<type>()` with layer name prefix detection
- Add widget creator in `Content/Python/auto_psd_ui.py`: New function `create_<type>()` with type-specific properties
- Update dispatcher in `process_child_layer()` in `auto_psd_ui.py` to handle new type
- Extend `gather_psd_images()` if type includes image references
- Add type check to `fix_image_link()` if type uses image paths

**New C++ Utilities:**
- Add static function to `Source/AutoPSDUI/Public/AutoPSDUILibrary.h` (header)
- Implement in `Source/AutoPSDUI/Private/AutoPSDUILibrary.cpp`
- Use UFUNCTION(BlueprintCallable, Category = "AutoPSDUILibrary") macro for Python access

**New Configuration Options:**
- Add UPROPERTY to `Source/AutoPSDUI/Public/AutoPSDUISetting.h`
- Initialize default in `Source/AutoPSDUI/Private/AutoPSDUISetting.cpp` constructor
- Access in Python via `unreal.AutoPSDUISetting.get().<property_name>`

**Module-Level Hooks:**
- Add new event handlers in `FAutoPSDUIModule::StartupModule()` (register callbacks)
- Implement handler methods in `FAutoPSDUIModule` class
- Clean up in `ShutdownModule()` if needed

**Python Utilities:**
- General helpers: `Content/Python/AutoPSDUI/common.py` (platform detection, dependency management)
- PSD-specific parsing: `Content/Python/AutoPSDUI/psd_utils.py`
- Orchestration: `Content/Python/auto_psd_ui.py` (main flow, type dispatch)

## Special Directories

**Source/ThirdParty/:**
- Purpose: Third-party dependencies (NumPy headers, psd_tools package)
- Generated: Yes (populated by `download_dependencies()`)
- Committed: Partially (Mac/numpy headers committed; Win64/psd_tools downloaded at runtime)
- Structure: Platform-specific subdirectories (Win64, Mac)

**Content/:**
- Purpose: Plugin content (Python scripts and future UMG assets)
- Generated: No (manually created)
- Committed: Yes (Python scripts are source)

**Config/:**
- Purpose: Plugin configuration
- Generated: No
- Committed: Yes

---

*Structure analysis: 2026-04-07*

# Coding Conventions

**Analysis Date:** 2026-04-07

## Language Overview

This codebase combines two distinct code languages:
- **C++**: Unreal Engine 5 plugin code (UE5, porting from UE4)
- **Python**: Unreal Python scripting API for PSD processing

## C++ Conventions (Unreal Engine)

### Naming Patterns

**Classes:**
- Prefix with `U` for UObject-derived classes: `UAutoPSDUILibrary`, `UAutoPSDUISetting`
- Prefix with `F` for non-UObject value types (not used in this plugin)
- Module class prefix with `F`: `FAutoPSDUIModule`
- Always in PascalCase

**Functions:**
- Public functions in UObject classes follow Unreal conventions
- Methods: `StartupModule()`, `ShutdownModule()`, `CreateWBP()`
- Always PascalCase

**Variables:**
- Boolean members prefixed with `b`: `bEnabled`
- Private/protected members have no special prefix
- Function parameters often prefixed with `p_` (legacy style): Parameter naming varies

**Macros and Types:**
- Use `TEXT()` macro for string literals: `TEXT(".psd")`, `TEXT("AutoPSDUI")`
- Use Unreal string types: `FString`, `FDirectoryPath`
- Unreal macro system: `UCLASS()`, `UFUNCTION()`, `UPROPERTY()`, `GENERATED_BODY()`

### File Organization

**Headers:**
- Location: `Source/AutoPSDUI/Public/*.h`
- Pattern: One public class per header file
- Include guards: `#pragma once`
- Example files:
  - `Source/AutoPSDUI/Public/AutoPSDUI.h`: Module interface
  - `Source/AutoPSDUI/Public/AutoPSDUILibrary.h`: Blueprint function library
  - `Source/AutoPSDUI/Public/AutoPSDUISetting.h`: Developer settings

**Implementation:**
- Location: `Source/AutoPSDUI/Private/*.cpp`
- Include pattern: Include own header first, then dependencies
- Example: `AutoPSDUILibrary.cpp` includes `AutoPSDUILibrary.h` then other headers

### Class Declarations

**Standard Pattern:**
```cpp
// Source/AutoPSDUI/Public/AutoPSDUILibrary.h
UCLASS()
class AUTOPSDUI_API UAutoPSDUILibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "AutoPSDUILibrary")
    static void FunctionName(const FString& Parameter);
};
```

**Module Pattern:**
```cpp
// Source/AutoPSDUI/Public/AutoPSDUI.h
class FAutoPSDUIModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

protected:
    void OnPSDImport(UObject* PSDTextureAsset);
};
```

### Metadata and Reflection

**UPROPERTY Specifiers:**
- `EditAnywhere`: Editable in editor details panel
- `config`: Saved to configuration file
- `BlueprintReadWrite`: Accessible from Blueprint
- `Category`: Used for UI grouping
- `meta=(LongPackageName)`: Path selection hint

**UFUNCTION Specifiers:**
- `BlueprintCallable`: Callable from Blueprint
- `Category`: Functional grouping in Blueprint menu

**Class Specifiers:**
- `config = Editor`: Configuration saved to Editor ini
- `defaultconfig`: Use default configuration

### Comments

**Style:**
- Single-line comments: `// Comment`
- Multi-line comments: Less common in this codebase
- Minimal documentation; copyright header present on each file
- Example: `// Copyright 2018-2021 - John snow wind`

**Observation:**
- Very minimal inline documentation
- No doxygen/comment blocks for functions in the main code
- Comments mainly for copyright and occasional TODO/logic clarification

### Include Organization

**Order (Unreal Standard):**
1. Own header first: `#include "AutoPSDUILibrary.h"`
2. Unreal Core: `#include "CoreMinimal.h"`
3. Engine modules: `#include "Engine.h"`
4. Specialized modules: `#include "UMG/Public/UMG.h"`
5. Editor modules: `#include "UnrealEd/Public/UnrealEd.h"`
6. Plugin modules: `#include "IPythonScriptPlugin.h"`

**Observed Pattern in `AutoPSDUILibrary.cpp`:**
- Uses explicit module includes for AssetRegistry, Python, widget tools
- No barrel imports; imports are specific

### Error Handling (C++)

**Pattern: Early Return with Validation**
```cpp
// From AutoPSDUI.cpp
if (!UAutoPSDUISetting::Get()->bEnabled)
{
    return;
}

UTexture2D* PSDTexture = Cast<UTexture2D>(PSDTextureAsset);
if (!PSDTexture)
{
    return;
}
```

**Casting:**
- Use `Cast<TargetType>()` to safely downcast
- Check result against nullptr before use
- No exceptions; all errors handled via return/null checks

**nullptr vs Null:**
- Modern Unreal uses `nullptr` instead of NULL or 0
- No explicit null pointer exception throwing

### Code Style

**Formatting:**
- Braces: Allman style (opening brace on same line)
- Indentation: Tabs (standard Unreal)
- Maximum line length: No strict limit observed
- Spacing: Standard around operators

**Function Design:**
- Functions are small and focused
- `OnPSDImport()` in module: Single responsibility (14 lines)
- Library functions: Thin wrappers around Unreal API

### Module Dependencies

**Build Configuration (`AutoPSDUI.Build.cs`):**
- Public dependencies: `Core`
- Private dependencies: `CoreUObject`, `Engine`, `Slate`, `SlateCore`, `DeveloperSettings`, `PythonScriptPlugin`, `UnrealEd`, `UMGEditor`, `UMG`, `AssetRegistry`, `EditorScriptingUtilities`
- No external third-party dependencies in C++; uses Unreal-provided libraries

---

## Python Conventions

### Naming Patterns

**Modules:**
- Lowercase with underscores: `auto_psd_ui.py`, `psd_utils.py`
- Package structure: `AutoPSDUI/` (follows import naming)
- Imports: `from AutoPSDUI import common`, `from AutoPSDUI.psd_utils import load_psd`

**Functions:**
- Lowercase with underscores: `parse_args()`, `check_psd_tools()`, `fix_names()`, `create_canvas()`
- Prefix `p_` for parameters: `p_psd_content`, `p_layer`, `p_button_info`
- Intent-based names: `gather_psd_images()`, `import_images()`, `process_child_layer()`

**Variables:**
- Lowercase with underscores: `input_file`, `output_asset`, `image_list`, `name_set`
- Dictionary keys: PascalCase: `"Type"`, `"Name"`, `"X"`, `"Y"`, `"Children"`
- Global module vars: `psd_gui_setting`, `dst_path`, `default_font`, `font_map`
- Private globals: `_dir` suffix: `module_dir`, `third_party_dir`, `target_dir`

**Type Hints:**
- Modern type hints used: `p_layer: Layers.PixelLayer`, `dst_path: str`, `p_psd_content: PSDImage`
- Return types not consistently annotated

### File Organization

**Main Entry Point:**
- `Content/Python/auto_psd_ui.py` (571 lines)
- Single file containing orchestration and widget creation logic
- Entry via `if __name__ == "__main__"`

**Utility Modules:**
- `Content/Python/AutoPSDUI/common.py`: Dependency management, settings access
- `Content/Python/AutoPSDUI/psd_utils.py`: PSD parsing and layer processing
- `Content/Python/AutoPSDUI/__init__.py`: Empty module marker

**Pattern: Modular Decomposition**
```
auto_psd_ui.py:
  - Check dependencies
  - Parse command-line arguments
  - Load PSD file
  - Process names
  - Import images
  - Create Widget Blueprints

psd_utils.py:
  - PSD loading and parsing
  - Layer type detection and parsing
  - Image export

common.py:
  - Cross-platform path setup
  - Dependency downloading
  - Settings access
```

### Import Organization

**Order:**
1. Standard library: `import os`, `import sys`, `import getopt`, `import math`, `import zipfile`
2. External packages: `from psd_tools import PSDImage`, `from urllib.request import urlretrieve`
3. Unreal API: `import unreal`

**Lazy Loading Pattern:**
```python
# From auto_psd_ui.py
def check_psd_tools():
    try:
        import psd_tools
        return True
    except ModuleNotFoundError:
        # ... prompt user
        return download_dependencies()
```

**Module Reloading:**
- Used for development: `reload_module()` in `auto_psd_ui.py` reloads all AutoPSDUI modules
- Enables iteration without restarting engine

### Comments

**Style:**
- Sparse inline comments
- Docstring pattern used occasionally:
  ```python
  def get_image_dst_path(image_path):
      """
      Obtain the imported asset path through the absolute path of the image
      """
  ```

**Observed Patterns:**
- Comments used for clarification on non-obvious logic
- Chinese comments in some functions (legacy): `# 处理颜色叠加` (handle color overlay), `# 其他情况均按照Canvas处理` (others are treated as Canvas)
- Limited documentation on complex parsing functions

### Error Handling

**Pattern 1: Try-Except for Dependencies**
```python
try:
    import psd_tools
    return True
except ModuleNotFoundError:
    # ... handle gracefully with user dialog
    return download_dependencies()
```

**Pattern 2: File Existence Checks**
```python
if not os.path.exists(image_link):
    invalid_image_list.add(image_link)
```

**Pattern 3: Unreal Logging**
- `unreal.log()`: Info level
- `unreal.log_warning()`: Warning level
- `unreal.log_error()`: Error level

```python
unreal.log_warning("Unknown PSD Layer Type: %s" % p_child_layer.kind)
unreal.log_error(message)
```

**Pattern 4: Asset Validation**
```python
if unreal.EditorAssetLibrary.does_asset_exist(child_wbp_asset):
    child_wbp = unreal.EditorAssetLibrary.load_asset(child_wbp_asset)
    if child_wbp:
        # use it
```

**Pattern 5: Bare Except (Code Smell)**
```python
# From common.py - poor error handling practice
try:
    urlretrieve(third_party_url, target_file, progress)
except:  # Catches everything - should specify exception type
    message = "Download Failed: "
    unreal.log_error(message)
    return False
```

### Data Structures

**Dictionary-Based Layer Info:**
```python
layer_info = {
    "Type": "Canvas",
    "Name": layer_name,
    "X": x,
    "Y": y,
    "Width": width,
    "Height": height,
    "Children": [],
    "AbsX": p_psd_layer.left,
    "AbsY": p_psd_layer.top
}
```

**Rationale:** Avoids creating custom classes; leverages Python's dynamic typing

**Pattern Usage:**
- Layer information passed as dictionaries throughout codebase
- Unreal objects (UClass instances) used only at widget creation boundary
- Set-based collections for uniqueness: `name_set`, `image_list`, `invalid_image_list`

### Function Design

**Size:** Functions are medium-sized (10-50 lines typical)

**Parameters:**
- Heavy use of dictionary parameters: `p_psd_content`, `p_button_info`
- Multiple return types: `bool`, `dict`, `None`, objects

**Polymorphism via Type Checking:**
```python
def process_child_layer(p_child_layer, p_parent_layer):
    if p_child_layer.kind == "group":
        child = parse_layer(p_child_layer, p_parent_layer)
    elif p_child_layer.kind in ("pixel", "smartobject", "shape"):
        child = parse_image(p_child_layer, p_parent_layer)
    elif p_child_layer.kind == "type":
        child = parse_text(p_child_layer, p_parent_layer)
    else:
        unreal.log_warning("Unknown PSD Layer Type: %s" % p_child_layer.kind)
        child = None
    return child
```

### Global State

**Usage:**
- Globals initialized at module load: `psd_gui_setting`, `third_party_dir`, `default_font`, `font_map`
- Mutable globals modified during execution: `dst_path` in `auto_psd_ui.py`

**Rationale:** Simplifies parameter passing through deep call chains; acceptable for scripting context

### Unreal API Conventions

**Method Naming (snake_case adaptation):**
- Unreal C++ `SetWBPRootWidget()` exposed as `set_wbp_root_widget()`
- Unreal C++ `CreateWBP()` exposed as `create_wbp()`
- Unreal class access: `unreal.CanvasPanel.static_class()`, `unreal.TextBlock.static_class()`

**Common Patterns:**
```python
widget = unreal.AutoPSDUILibrary.make_widget_with_wbp(unreal.Button.static_class(), wbp_obj, name)
slot = parent_widget.add_child(widget)
slot.set_position(unreal.Vector2D(x, y))
unreal.EditorAssetLibrary.load_asset(asset_path)
unreal.EditorDialog.show_message("Title", "Message", message_type, return_type)
```

---

## Cross-Language Patterns

### Separation of Concerns

1. **C++ (Plugin):** Blueprint interface, asset management, settings
2. **Python (Script):** PSD parsing, widget creation orchestration, image import

### Data Flow

1. **Trigger:** PSD texture imported in editor → `OnPSDImport()` in C++
2. **C++ → Python:** Execute Python script via `RunPyCmd()` with command-line arguments
3. **Python:** Parse PSD, create widget blueprints via C++ bridge functions
4. **Python → C++:** Call `unreal.AutoPSDUILibrary` functions to create/compile blueprints

### Configuration

**Settings Model:**
- C++ class `UAutoPSDUISetting` exposes configuration in editor
- Accessed from Python via `unreal.AutoPSDUISetting.get()`
- Includes font mappings, texture directories, enable flag

---

## Key Deviations from Standards

**Noted Issues:**

1. **Bare Exception Handling:** `common.py` line 58 uses bare `except:` instead of specific exceptions
2. **Print Statement:** `psd_utils.py` line 389 has debug `print(layer.kind)` that should use Unreal logging
3. **Minimal Documentation:** Limited docstrings and function documentation
4. **Global Mutable State:** `dst_path` is modified globally in `auto_psd_ui.py` main()
5. **Mixed Parameter Naming:** C++ uses legacy `p_` prefix sporadically; Python uses it consistently
6. **Chinese Comments:** Legacy codebase has Chinese comments mixed with English

---

*Convention analysis complete: 2026-04-07*

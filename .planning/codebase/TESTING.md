# Testing Patterns

**Analysis Date:** 2026-04-07

## Test Status Summary

**Current State:** No automated tests found in codebase.

This is a plugin project without dedicated test suites. Testing is manual/integration-based through Unreal Editor.

---

## C++ Testing (Unreal Plugin)

### Framework & Infrastructure

**Status:** No test framework configured

**Unreal Testing Options Available (Not Used):**
- Unreal Automation Framework (`FAutomationTestBase`)
- Built into engine; can write automation tests in `.Tests.cpp` files
- Would require module `.Build.cs` modification to include testing dependencies

**Current Approach:** Manual testing in editor

### How the Module is Currently Tested

**Integration Testing Via Editor:**
1. Enable the AutoPSDUI plugin
2. Configure settings in Project Settings → AutoPSDUI Settings
3. Import a PSD texture file
4. Plugin hooks into asset import → triggers `OnPSDImport()`
5. Visual verification of generated Widget Blueprint

**Entry Points for Testing:**
- `FAutoPSDUIModule::OnPSDImport()` in `Source/AutoPSDUI/Private/AutoPSDUI.cpp`
  - Triggered on texture reimport
  - Validates PSD file extension
  - Calls Python script via `RunPyCmd()`
  
- `UAutoPSDUILibrary::CreateWBP()` in `Source/AutoPSDUI/Private/AutoPSDUILibrary.cpp`
  - Creates widget blueprint asset
  - No validation of output; relies on Unreal API correctness

- `UAutoPSDUISetting` in `Source/AutoPSDUI/Public/AutoPSDUISetting.h`
  - Developer settings UI configuration
  - Default paths hardcoded: `"Art/UI/Texture"`, `"/Game/Widgets/Texture"`

### Test Gaps (C++)

**Missing Unit Tests:**
- Blueprint function library functions have no test coverage:
  - `CreateWBP()`: No validation of asset creation
  - `CompileAndSaveBP()`: No validation of compilation success
  - `ApplyInterfaceToBP()`: No validation of interface application
  - `MakeWidgetWithWBP()`: No validation of widget hierarchy

- Module lifecycle: No tests for startup/shutdown behavior

- Settings validation: No tests for configuration loading

- Error cases: No tests for missing settings, invalid parameters, null inputs

---

## Python Testing (PSD Script)

### Framework & Infrastructure

**Status:** No test framework configured

**Available Frameworks (Not Used):**
- `pytest`: Would be suitable for testing PSD parsing and image processing
- `unittest`: Built-in Python testing; would work but more verbose

**Current Approach:** Manual testing through Unreal Editor with PSD files

### Script Entry Point

**Main Script:** `Content/Python/auto_psd_ui.py`

**Flow:**
```
if __name__ == "__main__":
    reload_module()                    # Development aid
    check_psd_tools()                  # Dependency check
    parse_args()                       # CLI argument parsing
    load_psd()                         # Load from file
    parse_psd()                        # Parse structure
    fix_names()                        # Ensure uniqueness
    gather_psd_images()                # Collect image resources
    import_images()                    # Import to Unreal
    fix_image_link()                   # Update asset paths
    create_widgets_for_wbp()           # Generate widget hierarchy
    compile_and_save_bp()              # Save blueprint
```

### Test Gaps (Python)

**Core Logic Not Tested:**

1. **Argument Parsing** (`parse_args()` - lines 46-60)
   - No validation of argument combinations
   - No tests for missing required arguments
   - No tests for malformed paths

2. **PSD Loading** (`load_psd()` in `psd_utils.py`)
   - No validation for non-existent files (returns None silently)
   - No tests for corrupted PSD files
   - No exception handling for file read errors

3. **Layer Parsing** (`parse_psd()`, `parse_layer()`)
   - Multiple parse functions with conditional logic (lines 81-487 in `psd_utils.py`)
   - Complex type detection: `parse_button()`, `parse_image()`, `parse_text()`, `parse_progress_bar()`, `parse_list()`, `parse_tile()`
   - No tests for edge cases:
     - Empty PSD documents
     - Missing child layers in lists/tiles
     - Nested button structures
     - Invalid layer names

4. **Name Fixing** (`fix_names()` - lines 63-79)
   - Recursive name deduplication logic
   - Not tested for collision resolution correctness
   - No tests for unicode/special character handling

5. **Image Processing**
   - `gather_psd_images()`: Collects all image references (lines 93-146)
   - `import_images()`: Imports to Unreal (lines 154-166)
   - `fix_image_link()`: Updates paths (lines 181-213)
   - No validation of image export success
   - No tests for missing image files
   - No tests for invalid image paths

6. **Widget Creation** (lines 216-503)
   - `create_canvas()`, `create_text()`, `create_image()`, `create_button()`, `create_progress_bar()`, `create_list_view()`, `create_tile_view()`
   - Depends on Unreal API availability
   - No tests for widget hierarchy validation
   - No tests for position/size calculations
   - No tests for style application (colors, fonts, effects)

7. **Error Handling Issues:**
   - Line 389: Debug `print(layer.kind)` left in production code
   - Lines 58-64 in `common.py`: Bare `except:` clause catches all exceptions
   - Missing null checks before accessing dictionary keys in many places
   - No recovery from partial failures (e.g., some images fail to import)

### Dependencies

**External Libraries:**
- `psd_tools`: Dynamically installed; version not pinned
- `unreal`: Engine Python API; environment-provided
- Standard library: `os`, `sys`, `math`, `zipfile`, `getopt`, `urllib`

**Dependency Management:**
- `Content/Python/AutoPSDUI/common.py` provides `download_dependencies()`
- Downloads from hardcoded URL: `https://github.com/JohnSnowWind/AutoPSDUI_Dependencies/archive/refs/tags/v1.0.zip`
- No version control; no validation of downloaded content
- Third-party files added to `Source/ThirdParty/` (included with repo)

---

## Testing Strategies for Future Implementation

### C++ Testing Recommendations

**Unit Test Structure:**
```cpp
// Would create: Source/AutoPSDUI/Tests/AutoPSDUILibrary.Tests.cpp
#if WITH_AUTOMATION_TESTS
class FAutoPSDUILibraryTest : public FAutomationTestBase
{
public:
    virtual void GetTests(TArray<FString>& OutBeautifiedNames, TArray<FTestFunctionPointer>& OutTestFunctions) const override;
};

// Test cases would cover:
// - CreateWBP with valid asset path
// - CreateWBP with invalid path
// - CompileAndSaveBP success/failure
// - ApplyInterfaceToBP with valid/invalid interfaces
#endif
```

**Integration Test (Manual/Editor):**
1. Place test PSD files in `Art/UI/Texture/`
2. Import via editor
3. Verify generated Widget Blueprint structure
4. Validate widget count, hierarchy, properties

### Python Testing Recommendations

**Unit Test Structure:**
```python
# Would create: Content/Python/tests/test_psd_utils.py
import pytest
from AutoPSDUI.psd_utils import (
    fix_names, get_layer_pos_size, parse_image, parse_button, parse_text
)

def test_fix_names_deduplication():
    """Test that duplicate names are suffixed with index."""
    content = {
        "Name": "Button",
        "Children": [
            {"Name": "Button", "Children": []},
            {"Name": "Button", "Children": []}
        ]
    }
    name_set = set()
    fix_names(content, name_set)
    # Assert all names are unique
    assert len(name_set) == 3
    assert all(name in name_set for name in ["Button", "Button_1", "Button_2"])

def test_parse_image_with_valid_layer():
    """Test image parsing from valid PSD layer."""
    # Mock psd_tools layer
    pass

def test_parse_button_naming():
    """Test button image layer naming conventions."""
    # Test _normal, _hovered, _pressed, _disabled suffix detection
    pass
```

**Integration Test (With Engine):**
```python
# Would create: Content/Python/tests/test_auto_psd_ui.py
import unreal

def test_full_psd_conversion():
    """Test full PSD to WBP conversion with real Unreal editor."""
    # Load a test PSD file
    # Run parse_psd() and widget creation
    # Verify generated asset in editor
    pass
```

### Test Data

**Needed Test Fixtures:**
- `Tests/Data/simple.psd`: Single canvas with one button
- `Tests/Data/complex.psd`: Multiple layers, nested groups, images
- `Tests/Data/malformed.psd`: Corrupted/invalid PSD file
- `Tests/Data/empty.psd`: Empty document
- `Tests/Data/large.psd`: Many layers (stress test)

---

## Manual Testing Procedures (Current Approach)

### Smoke Test (Basic Functionality)

**Steps:**
1. Create a simple PSD file with basic shapes in design software (Photoshop, GIMP, etc.)
2. Name layers following conventions (e.g., `Button_MyButton`, `Image_Bg`, `Text_Label`)
3. Export/save as `.psd` to project's `Art/UI/Texture/` directory
4. In Unreal Editor, import the PSD as texture
5. Check that:
   - Python script runs without errors (see Editor Output Log)
   - Widget Blueprint is created at expected path
   - Blueprint contains correct widget hierarchy
   - Buttons have correct styles applied
   - Images are imported and linked correctly

### Regression Testing (Current - Ad Hoc)

**Typical Verification Workflow:**
1. Open generated Widget Blueprint
2. Check widget names match PSD layer names
3. Verify widget positions and sizes
4. Check image assets are properly imported
5. Visual inspection of rendered widget

**Known Issues to Watch For:**
- Image path resolution failures (fixed by `fix_image_link()`)
- Name collisions causing widget overwrite (mitigated by `fix_names()`)
- Missing child layer detection (logged as warning)
- Font mapping failures (falls back to `DefaultFont`)

---

## Configuration Files

**No test configuration files found:**
- No `pytest.ini`, `setup.cfg`, or `pyproject.toml`
- No `.editorconfig` or `.clang-format` for code style enforcement
- No CI/CD pipeline (GitHub Actions, etc.)

---

## Code Coverage

**Status:** Unknown / Not measured

**Gaps Identified:**
- C++ library functions: 0% coverage (no automated tests)
- Python script logic: 0% coverage (no automated tests)
- Edge cases: 0% coverage
- Error paths: Partially tested via manual edge cases

**Recommended Target:** Minimum 70% for core parsing logic in `psd_utils.py`

---

## Testing Anti-Patterns Observed

1. **Debug Code Left in Production**
   - `psd_utils.py` line 389: `print(layer.kind)` should be removed or use Unreal logging

2. **Broad Exception Handling**
   - `common.py` line 58: `except:` without specifying exception type catches all, including system exits

3. **Silent Failures**
   - `load_psd()` returns `None` for missing file without logging
   - No explicit error messages for failed asset imports
   - Asset loading failures silently skipped (lines 343-346 in `auto_psd_ui.py`)

4. **No Timeout Handling**
   - `download_dependencies()` has no timeout on URL retrieval
   - Large file downloads could hang indefinitely

5. **Hard-Coded Test URLs**
   - Dependency download URL not configurable (`common.py` line 36)
   - No fallback or retry logic

---

## How to Add Tests Going Forward

### For C++ (UE5 Automation Framework)

1. Create `Source/AutoPSDUI/Tests/AutoPSDUILibrary.Tests.cpp`
2. Include `CoreMinimal.h`, `Misc/AutomationTest.h`
3. Register test functions with macro system
4. Run via Editor: Window → Developer Tools → Automation, or `Automate [TestName]` in console

### For Python (pytest approach recommended)

1. Create `Content/Python/tests/` directory
2. Add `__init__.py` for package recognition
3. Write test files with `test_*.py` naming
4. Run: `python -m pytest Content/Python/tests/` (requires pytest in Python path)
5. Consider integrating with Unreal's Python console via custom UFunction

---

## Testing Recommendations Summary

**High Priority:**
- Test `parse_psd()` and layer detection logic
- Test `fix_names()` collision handling
- Test image import success/failure paths
- Add error logging instead of silent failures

**Medium Priority:**
- Test C++ Blueprint creation functions
- Test settings loading and defaults
- Test cross-platform path handling (Windows/Mac)

**Low Priority:**
- Test UI in editor (inherent to Unreal)
- Stress test with very large PSD files
- Performance profiling of parsing logic

---

*Testing analysis complete: 2026-04-07*

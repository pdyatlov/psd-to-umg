# Codebase Concerns

**Analysis Date:** 2026-04-07

## Tech Debt

**UE4 to UE5 Migration Incomplete:**
- Issue: Plugin still targets UE4.26 with UE4 API patterns. Core migration work underway but not finished.
- Files: `AutoPSDUI.uplugin`, `Source/AutoPSDUI/AutoPSDUI.Build.cs`, `Source/AutoPSDUI/Private/AutoPSDUI.cpp`
- Impact: Plugin will not compile/load in UE5 until all API changes are made. UE5 has breaking changes in several areas.
- Fix approach: 
  1. Update `EngineVersion` in `AutoPSDUI.uplugin` from "4.26.0" to appropriate UE5 version
  2. Replace deprecated include `AssetRegistryModule.h` with `AssetRegistry/AssetRegistryModule.h` (already done in `AutoPSDUILibrary.cpp` but not in `AutoPSDUI.cpp`)
  3. Update all deprecated UE4 subsystem/module APIs to UE5 equivalents
  4. Replace deprecated `FEditorStyle` if used with Slate style system updates

**Python Dependency Hard Coupling:**
- Issue: Plugin requires `PythonScriptPlugin` to be enabled and available, creating hard dependency chain.
- Files: `AutoPSDUI.uplugin` line 29-31, `Source/AutoPSDUI/AutoPSDUI.Build.cs` line 42, `Source/AutoPSDUI/Private/AutoPSDUILibrary.cpp` line 5, 23
- Impact: Python script execution will fail silently if plugin unavailable. No graceful degradation.
- Fix approach: Make Python execution optional with error callbacks. Add runtime checks for `IPythonScriptPlugin::Get()` returning valid pointer. Add user-facing error messages.

**Hardcoded Plugin Directory Path:**
- Issue: Python command construction uses hardcoded plugin path structure.
- Files: `Source/AutoPSDUI/Private/AutoPSDUI.cpp` line 65
- Code: `const FString PyFile = FPaths::ProjectPluginsDir() / TEXT("AutoPSDUI/Content/Python/auto_psd_ui.py");`
- Impact: Path breaks if plugin is relocated or packaging changes directory structure. Will fail silently.
- Fix approach: Use plugin module directory API instead. `FPaths::GetPath(FString(ANSI_TO_TCHAR(__FILE__)))` or module descriptor APIs.

**Third-party Dependencies Missing Cleanup:**
- Issue: Plugin includes vendored Python dependencies (attr, dateutil, PIL, etc.) in `Source/ThirdParty/` for Win64 and Mac platforms.
- Files: `Source/ThirdParty/Win64/`, `Source/ThirdParty/Mac/`
- Impact: Binary/plugin size bloat. Third-party licenses unclear. Maintenance burden.
- Fix approach: Either remove and rely on engine Python, or document all licenses clearly in LICENSE file. Consider moving to proper dependency management.

## Known Bugs

**Button Style Application Bug:**
- Symptoms: Button hovered/pressed/disabled states always show the same image as normal state.
- Files: `Content/Python/auto_psd_ui.py` lines 386, 395, 404
- Trigger: Creating button widget from PSD with different state images.
- Cause: Lines 386, 395, 404 incorrectly assign `normal_brush` to hovered/pressed/disabled styles instead of their respective brushes:
  ```python
  button_style.hovered = normal_brush  # Should be hovered_brush
  button_style.pressed = normal_brush  # Should be pressed_brush
  button_style.disabled = normal_brush # Should be disabled_brush
  ```
- Workaround: Manually fix button styles in Blueprint editor after import.
- Fix: Replace assignment targets with correct brush variables.

**Progress Bar Fill Calculation Bug:**
- Symptoms: Progress bar fill link always shows background color instead of fill color overlay.
- Files: `Content/Python/auto_psd_ui.py` line 397
- Cause: Layer name check endswith "_fill" but path construction uses hardcoded slice `[:-11]` (length of "_background"). Should be `[:-5]` for "_fill".
- Trigger: Importing progress bar with fill layer in PSD.
- Workaround: None - manually rebuild progress bar.

**Progress Bar Fill Overlay Flag Bug:**
- Symptoms: Progress bar fill color overlay state incorrectly set to False when it should track True.
- Files: `Content/Python/auto_psd_ui.py` line 419
- Cause: Line 419 sets `bFColorOverlay = False` unconditionally; should be `True`.
- Impact: Fill color overlay effects ignored during import.

**Uninitialized Button Style Assignment:**
- Symptoms: Python error or undefined behavior when creating buttons without all state images defined.
- Files: `Content/Python/auto_psd_ui.py` lines 370-405
- Cause: Code creates `ButtonStyle()` with default empty brushes, then iterates layers. If a state image is missing, that brush stays empty/default.
- Impact: Partial button definitions may render incorrectly or crash Unreal.
- Workaround: Always provide all four button state images in PSD.

## Security Considerations

**Arbitrary Code Execution via Python:**
- Risk: Any Python command passed to `RunPyCmd()` executes in engine's Python context with full editor access.
- Files: `Source/AutoPSDUI/Private/AutoPSDUILibrary.cpp` line 23, `Content/Python/auto_psd_ui.py` line 1-10
- Current mitigation: Plugin only calls own hardcoded Python script. No external input accepted.
- Recommendations: Keep this restriction. Do NOT expose `RunPyCmd()` to user blueprint calls without validation. Do NOT pass user-supplied file paths to Python script without sanitization.

**File Path Injection:**
- Risk: PSD file paths could contain special characters or escape sequences.
- Files: `Source/AutoPSDUI/Private/AutoPSDUI.cpp` lines 60-68 (path construction), `Content/Python/auto_psd_ui.py` lines 505-510 (path parsing)
- Current mitigation: Paths come from asset import system, not user input. Still, no validation.
- Recommendations: Add path validation. Ensure destination paths are within expected project directories.

**Unvalidated Asset References:**
- Risk: Python code loads assets by path string without verifying asset type.
- Files: `Content/Python/auto_psd_ui.py` lines 343, 374, 382, 391, 442, 451
- Current mitigation: Uses `EditorAssetLibrary.load_asset()` which may return None silently.
- Recommendations: Add explicit type checking. Log warnings when assets fail to load instead of silent failures.

## Performance Bottlenecks

**Inefficient Asset Import Loop:**
- Problem: Python imports all images sequentially (one at a time) instead of batch.
- Files: `Content/Python/auto_psd_ui.py` lines 154-166
- Cause: Loop creates individual `AssetImportTask` objects in sequence. Could batch into single call.
- Improvement path: Collect all tasks into single list first, then call `import_asset_tasks()` once. Current code already structures this correctly, but comment and structure could be clearer.

**Image Export Redundancy:**
- Problem: Same image exported multiple times if referenced in multiple layers (button states, overlays, etc.).
- Files: `Content/Python/auto_psd_ui.py` lines 152, 157, 162, 167 (in `parse_button()`)
- Cause: Each button state layer exports individually. No deduplication check.
- Improvement path: Collect all image paths first, deduplicate, then export once. Avoids disk I/O duplication.

**Global Module Reload on Every Import:**
- Problem: `reload_module()` iterates entire `sys.modules` on every PSD import.
- Files: `Content/Python/auto_psd_ui.py` lines 37-43
- Cause: Called at startup to clear old module state. Inefficient.
- Improvement path: Only reload if modules changed, or use proper module lifecycle management.

**No Progress Reporting During Long Operations:**
- Problem: Large PSD files with many layers show no feedback during 5-30 second imports.
- Files: `Content/Python/auto_psd_ui.py` lines 505-558 (main loop)
- Cause: No `ScopedSlowTask` or progress updates during widget creation loop.
- Improvement path: Wrap widget creation in slow task with frame updates per widget/layer.

## Fragile Areas

**Import Subsystem Hook Without Cleanup:**
- Files: `Source/AutoPSDUI/Private/AutoPSDUI.cpp` lines 15-19
- Why fragile: Module registers raw pointer callback with `ImportSubsystem->OnAssetReimport.AddRaw()`. No corresponding removal in `ShutdownModule()` (line 21-25 is empty).
- Risk: If module is reloaded/unloaded during runtime, callback remains registered and fires for deleted module instance. Causes crash.
- Safe modification: Add `RemoveRaw()` call in `ShutdownModule()` using same parameters as `AddRaw()`.
- Test coverage: No tests for module lifecycle.

**Null Pointer Dereference in Button Child Processing:**
- Files: `Content/Python/auto_psd_ui.py` lines 407-420
- Why fragile: Code checks `if len(children) != 0` but doesn't check if `children` list itself is valid. Also assumes `children[0]` exists without bounds check.
- Safe modification: Check both list existence and length: `if children and len(children) > 0:`.
- Test coverage: Likely untested with edge case of empty children array.

**Global Settings State:**
- Files: `Content/Python/auto_psd_ui.py` lines 10-11, 34, `Source/AutoPSDUI/Private/AutoPSDUISetting.cpp`
- Why fragile: Global `psd_gui_setting` loaded once at module import. If settings change during runtime, changes won't be reflected. Multiple concurrent imports could race.
- Safe modification: Load settings fresh for each import operation rather than caching.
- Test coverage: No tests for concurrent imports or runtime setting changes.

**String Key Access Without Validation:**
- Files: `Content/Python/auto_psd_ui.py` - extensively uses dict key access like `p_psd_content["Type"]`, `color[b'Rd  ']`, etc.
- Why fragile: KeyError exceptions if keys missing. No defensive coding.
- Safe modification: Use `.get()` method with defaults: `p_psd_content.get("Type", "Unknown")`.
- Test coverage: No validation tests for malformed PSD structures.

## Scaling Limits

**Single-Threaded Python Execution:**
- Current capacity: One PSD import at a time (synchronous).
- Limit: 50+ layer PSD files can block editor for 30+ seconds. Multiple imports queued = long wait.
- Scaling path: Make Python execution async or use process-based execution. Requires `IPythonScriptPlugin` async support (may not exist in UE4).

**Memory Usage with Large PSDs:**
- Current capacity: Full PSD loaded into memory via `psd_tools.PSDImage.open()`.
- Limit: Multi-layer PSDs with many large smart objects can consume 500MB+ RAM.
- Scaling path: Stream layer-by-layer instead of full document load. Requires `psd_tools` API refactor.

**Hardcoded Asset Directory Assumptions:**
- Current capacity: Flat directory structure in `/Game/Widgets/Texture/`.
- Limit: 1000+ textures in single directory becomes slow to browse. No organization by widget.
- Scaling path: Generate subdirectories per widget. Requires path templating in settings.

## Dependencies at Risk

**psd_tools Library:**
- Risk: External Python package with uncertain maintenance. Last release 2023. May not support newer PSD formats.
- Impact: Large modern PSDs may fail to parse. No fallback.
- Migration plan: Monitor project releases. Consider fork if abandoned. Or integrate native PSD parser (e.g., psd-parse Rust library).

**Deprecated UE4 APIs (Migration Burden):**
- Risk: Every UE5 version update may deprecate more APIs.
- Impact: Plugin must be updated for each engine version.
- Migration plan: Use abstraction layers. Create platform/version-agnostic wrapper functions for subsystem/module access.

## Missing Critical Features

**Error Logging and Diagnostics:**
- Problem: Plugin silently fails if Python command fails, asset import fails, or settings invalid. No output to user.
- Blocks: Users can't diagnose why import failed. Creates support burden.
- Recommendation: Add comprehensive error callbacks, output to message log, and user notifications for failures.

**Validation of PSD Structure:**
- Problem: Plugin assumes specific layer naming convention (Button_, Progress_, List_, Tile_) without validation.
- Blocks: Non-conforming PSDs silently produce incorrect widgets.
- Recommendation: Add pre-flight validation that checks layer structure and reports issues to user.

**Support for Linked Smart Objects:**
- Problem: Plugin only exports first frame of smart objects. Linked smart objects not fully handled.
- Blocks: Complex PSDs with linked assets fail.
- Recommendation: Document limitations clearly or extend to handle linked objects.

**Batch Processing:**
- Problem: Only one PSD can be imported per `OnPSDImport` event.
- Blocks: Converting large design systems requires manual re-import per file.
- Recommendation: Add batch/folder import capability.

## Test Coverage Gaps

**Zero Unit Test Coverage:**
- Untested area: All C++ code (`AutoPSDUI.cpp`, `AutoPSDUILibrary.cpp`, `AutoPSDUISetting.cpp`).
- Files: `Source/AutoPSDUI/Private/*.cpp`
- Risk: Crashes and null pointer dereferences undetected until user reports.
- Priority: High

**Minimal Python Test Coverage:**
- Untested area: All Python parsing and widget creation (lines 1-570 of `auto_psd_ui.py`).
- Files: `Content/Python/auto_psd_ui.py`
- Risk: The PSD parsing logic is complex and fragile. Malformed PSDs crash. No regression detection.
- Priority: High

**No Integration Tests:**
- Untested area: Full PSD-to-UMG conversion workflow end-to-end.
- Risk: Changes to individual functions may break the full pipeline.
- Priority: Medium

**No Platform Testing:**
- Untested area: Windows vs Mac (code has platform-specific paths in `common.py`).
- Files: `Content/Python/AutoPSDUI/common.py` lines 17-30
- Risk: Mac build may be broken. Path construction differs between platforms.
- Priority: Medium

**No Settings Validation Testing:**
- Untested area: Edge cases in `UAutoPSDUISetting` (invalid paths, missing fonts, etc.).
- Files: `Source/AutoPSDUI/Public/AutoPSDUISetting.h`, `Source/AutoPSDUI/Private/AutoPSDUISetting.cpp`
- Risk: Invalid settings silently cause import failures.
- Priority: Low

---

*Concerns audit: 2026-04-07*

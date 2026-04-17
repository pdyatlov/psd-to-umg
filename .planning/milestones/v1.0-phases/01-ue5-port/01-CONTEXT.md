# Phase 1: UE5 Port - Context

**Gathered:** 2026-04-08
**Status:** Ready for planning

<domain>
## Phase Boundary

Rename the existing AutoPSDUI UE4 plugin to PSD2UMG and migrate it to compile cleanly in UE 5.7.4 with no deprecated API warnings. No new functionality is added in this phase. The output is a plugin that loads in UE 5.7.4, appears as "PSD2UMG" in Edit → Plugins, and compiles with zero warnings.

</domain>

<decisions>
## Implementation Decisions

### Rename Scope
- **D-01:** Rename at the file level only — do NOT restructure directories (Source/AutoPSDUI/ stays as-is). Full directory restructure is deferred to when the plugin is placed in a real UE project's Plugins/ folder.
- **D-02:** All C++ class names rename fully: `FAutoPSDUIModule` → `FPSD2UMGModule`, `UAutoPSDUILibrary` → `UPSD2UMGLibrary`, `UAutoPSDUISetting` → `UPSD2UMGSettings`. No typedef aliases — clean break.
- **D-03:** Macro prefix: `AUTOPSDUI_API` → `PSD2UMG_API`.
- **D-04:** Module name in Build.cs, .uplugin, and IMPLEMENT_MODULE macro: `AutoPSDUI` → `PSD2UMG`.

### .uplugin Changes
- **D-05:** `EngineVersion` → `"5.7"`
- **D-06:** `WhitelistPlatforms` → `PlatformAllowList`
- **D-07:** Remove `PythonScriptPlugin` from the `Plugins` array
- **D-08:** `FriendlyName` → `"PSD2UMG"`, `Description` → `"Import PSD files as UMG Widget Blueprints"`, remove marketplace/docs URLs
- **D-09:** `Installed` → remove or set to `false` (plugin is not a marketplace install)

### Deprecated API Fixes
- **D-10:** `FEditorStyle::` → `FAppStyle::` (all occurrences)
- **D-11:** `AssetRegistryModule.h` include → `AssetRegistry/AssetRegistryModule.h`
- **D-12:** Any `GEditor->GetEditorSubsystem<>()` patterns — verify still valid in UE 5.7 (currently used in StartupModule for UImportSubsystem)
- **D-13:** `PythonScriptPlugin` dependency removed from Build.cs (`PrivateDependencyModuleNames`)
- **D-14:** `IPythonScriptPlugin.h` include removed

### Library Handling
- **D-15:** Rename `AutoPSDUILibrary` → `PSD2UMGLibrary`. Keep all WBP helper functions intact (`CreateWBP`, `MakeWidgetWithWBP`, `SetWBPRootWidget`, `CompileAndSaveBP`, `ApplyInterfaceToBP`, `GetBPGeneratedClass`) — these will be used in Phase 3.
- **D-16:** `RunPyCmd` is stubbed: signature kept but body replaced with a `UE_LOG` warning: `"PSD2UMG: RunPyCmd is deprecated — use native C++ pipeline"`. The `IPythonScriptPlugin` include and dependency are removed.

### Settings Rename
- **D-17:** `UAutoPSDUISetting` → `UPSD2UMGSettings`, config section renamed from `AutoPSDUISetting` to `PSD2UMG` in the UCLASS macro.

### Python Cleanup
- **D-18:** Move `Content/Python/` to `docs/python-reference/` — preserves the scripts as reference material but removes them from the active plugin Content folder.
- **D-19:** Delete `Source/ThirdParty/Mac/` entirely — these Python packages (aggdraw, psd_tools, numpy, etc.) are fully replaced by PhotoshopAPI in Phase 2.

### Verification
- **D-20:** UE 5.7.4 is installed locally. Verification method: place the plugin in a test UE project's `Plugins/PSD2UMG/` folder, run `GenerateProjectFiles`, compile with `Build.bat` (or via IDE), open editor and confirm plugin appears in Edit → Plugins.
- **D-21:** Success bar: zero compile errors, zero deprecated-API warnings in Output Log on editor load.

### Claude's Discretion
- Exact handling of `Config/FilterPlugin.ini` — update or leave as-is at Claude's discretion
- Whether to add a minimal `Resources/Icon128.png` if missing — Claude decides based on what's there
- Exact `.gitignore` entries for intermediate build artifacts

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project Spec
- `PSD2UMG_DEVELOPMENT_PLAN.md` — Phase 0 and Phase 1 sections define the target .uplugin structure, module naming, and deprecated API list. Use as the authoritative spec for this phase.

### Current Plugin Files (to modify)
- `AutoPSDUI.uplugin` — Current descriptor; target post-rename: `AutoPSDUI.uplugin` (filename change deferred, but content updated)
- `Source/AutoPSDUI/AutoPSDUI.Build.cs` — Remove PythonScriptPlugin, update module name
- `Source/AutoPSDUI/Private/AutoPSDUI.cpp` — Rename module class, fix any deprecated calls
- `Source/AutoPSDUI/Private/AutoPSDUILibrary.cpp` — Rename + stub RunPyCmd
- `Source/AutoPSDUI/Private/AutoPSDUISetting.cpp` — Rename
- `Source/AutoPSDUI/Public/AutoPSDUI.h` — Rename module class
- `Source/AutoPSDUI/Public/AutoPSDUILibrary.h` — Rename class and macro
- `Source/AutoPSDUI/Public/AutoPSDUISetting.h` — Rename class and macro

### Research Context
- `.planning/research/STACK.md` — UE 5.7 API migration specifics (FAppStyle, PlatformAllowList, include path changes)
- `.planning/research/PITFALLS.md` — Phase 0/1 migration pitfalls (WhitelistPlatforms silent failure, CRT issues for Phase 2)
- `.planning/codebase/CONCERNS.md` — Full list of deprecated patterns identified in codebase analysis

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `UAutoPSDUISetting` (→ `UPSD2UMGSettings`): Already a `UDeveloperSettings` subclass with FontMap and texture dirs — keep and rename, it's the right base for Phase 6 settings expansion
- `UAutoPSDUILibrary` (→ `UPSD2UMGLibrary`): WBP helper functions are proven working — keep them as the foundation for Phase 3's widget creation

### Established Patterns
- Module startup uses `GEditor->GetEditorSubsystem<UImportSubsystem>()` to hook `OnAssetReimport` — this pattern is valid in UE 5.7, verify during port
- `UFactory` pattern for `CreateWBP` via `UWidgetBlueprintFactory` — confirmed valid in UE 5.7 (from architecture research)

### Integration Points
- `OnPSDImport` callback currently invokes Python via `RunPyCmd` — after stub, it becomes a no-op warning. Phase 2 replaces the body with the C++ parser call.

</code_context>

<specifics>
## Specific Ideas

- The uplugin filename itself (`AutoPSDUI.uplugin`) doesn't need to be renamed in Phase 1 since the plugin isn't yet in a UE project folder — just update the content (FriendlyName, module name, etc.). Physical rename happens when the plugin is placed in a project.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 01-ue5-port*
*Context gathered: 2026-04-08*

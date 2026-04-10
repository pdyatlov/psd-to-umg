# Phase 7: Editor UI, Preview & Settings - Context

**Gathered:** 2026-04-10
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver a user-friendly import workflow: a preview dialog before import, plugin settings panel, reimport support that preserves manual edits, Content Browser context menus, optional CommonUI mode with input action binding, and basic animation generation from layer variants.

</domain>

<decisions>
## Implementation Decisions

### Import Preview Dialog (EDITOR-02, EDITOR-03)
- **D-01:** Preview dialog appears on **every PSD import**. It shows the parsed layer tree before creating any assets.
- **D-02:** Each layer row shows: checkbox (toggle on/off), layer name, and a **colored badge** showing resolved widget type (UButton, UImage, UTextBlock, etc.). No thumbnails, no dimensions.
- **D-03:** Widget type badges are **read-only** — no dropdown to override type. Widget type is determined by naming convention only.
- **D-04:** Dialog includes an **editable output path** field, pre-filled from `UPSD2UMGSettings::WidgetBlueprintAssetDir`, with a browse button for per-import override.

### Reimport Strategy (EDITOR-04)
- **D-05:** Layer identity is matched by **PSD layer name** (not path, not internal ID). Renaming a layer in Photoshop = new widget.
- **D-06:** Reimport **updates PSD-sourced properties** (position, size, image data, text content, brushes) and **preserves manual edits** (Blueprint logic, bindings, animations, manually-added widgets).
- **D-07:** Widgets from **deleted PSD layers are kept** as orphans in the Blueprint. User deletes manually if needed.
- **D-08:** Reimport **shows the preview dialog** with change annotations: [new], [changed], [unchanged] indicators per layer. User confirms before applying.

### CommonUI & Animations (CUI-01 through CUI-04)
- **D-09:** CommonUI mode is a **global toggle** in `UPSD2UMGSettings` (`bUseCommonUI`, default: false). When on, all Button_ layers produce `UCommonButtonBase` instead of `UButton`.
- **D-10:** Input action binding `Button_Confirm[IA_Confirm]` **resolves the UInputAction asset at import time** by searching the configured `InputActionSearchPath`. If asset not found, log warning and create button without binding.
- **D-11:** Animation generation uses **simple opacity fades**: `_show` = 0→1 opacity over 0.3s, `_hide` = 1→0 opacity, `_hover` = widget scale 1.0→1.05. Designer replaces with custom animation if needed.
- **D-12:** ScrollBox content height: **let UMG handle it at runtime** — no manual height calculation at import time. Just place children in ScrollBox.

### Settings & Context Menu (EDITOR-01, EDITOR-05)
- **D-13:** New settings to add to `UPSD2UMGSettings`:
  - `bShowPreviewDialog` (bool, default: true) — toggle preview dialog on import
  - `bUseCommonUI` (bool, default: false) — CommonUI mode
  - `InputActionSearchPath` (FDirectoryPath, default: `/Game/Input/`) — base path for UInputAction asset lookup
  - `SourceDPI` (int32, default: 72) — Photoshop source DPI for conversion calculations
- **D-14:** Right-click context menu appears on **both PSD texture assets and existing Widget Blueprints**:
  - On .psd textures: "Import as Widget Blueprint"
  - On existing WBPs (with PSD source metadata): "Reimport from PSD"

### Claude's Discretion
- Dialog layout and Slate widget arrangement
- Change detection algorithm for reimport (hash-based, property comparison, etc.)
- How PSD source path is stored as metadata on Widget Blueprints for reimport
- UWidgetAnimation creation API details (FMovieSceneSequence, etc.)
- CommonUI module dependency management (conditional linking)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Requirements
- `.planning/REQUIREMENTS.md` — EDITOR-01 through EDITOR-05, CUI-01 through CUI-04

### Prior Phase Decisions
- `.planning/phases/03-layer-mapping-widget-blueprint-generation/03-CONTEXT.md` — D-08 (skip+warn pattern), D-05/D-06/D-07 (anchor heuristics)
- `.planning/phases/05-layer-effects-blend-modes/05-CONTEXT.md` — D-01 (hidden layers as Collapsed)
- `.planning/phases/06-advanced-layout/06-CONTEXT.md` — D-05 (MaxSmartObjectDepth setting)

### Existing Code
- `Source/PSD2UMG/Public/PSD2UMGSetting.h` — Current settings class to extend
- `Source/PSD2UMG/Public/Factories/PsdImportFactory.h` — Current import factory to modify
- `Source/PSD2UMG/Private/Factories/PsdImportFactory.cpp` — Import flow to intercept with preview dialog
- `Source/PSD2UMG/PSD2UMG.Build.cs` — Module dependencies (needs CommonUI conditional)

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `UPSD2UMGSettings` — Already has 8 properties in Project Settings > Plugins > PSD2UMG. Adding 4 more follows the same pattern.
- `UPsdImportFactory` — Intercepts .psd import with ImportPriority = Default + 100. Preview dialog inserts between parse and generate steps.
- `FWidgetBlueprintGenerator::Generate()` — Current generation entry point. Reimport needs a parallel `Update()` path.
- `FPsdParser::ParseFile()` → `FPsdDocument` — Layer tree is already available for building the preview tree UI.

### Established Patterns
- Settings via `UDeveloperSettings` with `EditAnywhere, config, BlueprintReadWrite` properties
- Skip + warn on failures (never abort import)
- Layer name prefix/suffix convention for widget type dispatch
- `FLayerMappingRegistry` priority-based dispatch — CommonUI mapper would register at higher priority than default button mapper when enabled

### Integration Points
- Import factory `FactoryCreateNew()` — insert preview dialog before `FWidgetBlueprintGenerator::Generate()`
- Content Browser extender — register `FContentBrowserMenuExtender_SelectedAssets` for context menu
- `FReimportHandler` interface — implement for reimport support
- Build.cs — conditional CommonUI dependency: `if (Target.bBuildDeveloperTools) { PrivateDependencyModuleNames.Add("CommonUI"); }`

</code_context>

<specifics>
## Specific Ideas

No specific requirements — open to standard approaches.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 07-editor-ui-preview-settings*
*Context gathered: 2026-04-10*

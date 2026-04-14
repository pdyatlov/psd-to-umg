---
gsd_state_version: 1.0
milestone: v1.0
milestone_name: milestone
status: verifying
stopped_at: Completed 09-04-PLAN.md (migration guide + README rewrite) — Phase 9 complete
last_updated: "2026-04-14T15:36:19.376Z"
last_activity: 2026-04-14
progress:
  total_phases: 9
  completed_phases: 9
  total_plans: 31
  completed_plans: 31
  percent: 97
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-04-07)

**Core value:** A designer drops a PSD into Unreal Editor and gets a correctly structured, immediately usable Widget Blueprint -- with no Python dependency, no manual tweaking, and no loss of layer intent.
**Current focus:** Phase 09 — unified-layer-naming-convention-tag-based

## Current Position

Phase: 09 (unified-layer-naming-convention-tag-based) — EXECUTING
Plan: 4 of 4
Status: Phase complete — ready for verification
Last activity: 2026-04-14

Progress: [██████████] 97%

## Performance Metrics

**Velocity:**

- Total plans completed: 0
- Average duration: -
- Total execution time: 0 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| - | - | - | - |

**Recent Trend:**

- Last 5 plans: -
- Trend: -

*Updated after each plan completion*
| Phase 01-ue5-port P01 | 5m | 2 tasks | 8 files |
| Phase 01-ue5-port P02 | 15m | 2 tasks | 6 files |
| Phase 02-c-psd-parser P01 | human-verify cycle | 2 tasks + checkpoint | 7 files |
| Phase 02-c-psd-parser P02 | ~5m | 2 tasks | 4 files |
| Phase 02-c-psd-parser P03 | 20m | 2 tasks | 3 files |
| Phase 02-c-psd-parser P04 | 8m | 2 tasks | 4 files |
| Phase 02-c-psd-parser P05 | ~90m | 3 tasks + fix cycle | 4 files |
| Phase 03-layer-mapping-widget-blueprint-generation P01 | 10m | 3 tasks | 8 files |
| Phase 03-layer-mapping-widget-blueprint-generation P02 | 5m | 3 tasks | 8 files |
| Phase 03-layer-mapping-widget-blueprint-generation P03 | 5 | 2 tasks | 3 files |
| Phase 04 P01 | 3m | 2 tasks | 4 files |
| Phase 05-layer-effects-blend-modes P02 | 10m | 2 tasks | 2 files |
| Phase 06-advanced-layout P01 | 10 | 2 tasks | 5 files |
| Phase 06-advanced-layout P02 | 10m | 2 tasks | 9 files |
| Phase 06-advanced-layout P03 | 5m | 1 tasks | 1 files |
| Phase 07 P01 | 5m | 2 tasks | 3 files |
| Phase 07 P02 | 10m | 2 tasks | 3 files |
| Phase 07 P03 | 15 | 2 tasks | 3 files |
| Phase 07-editor-ui-preview-settings P04 | 8m | 2 tasks | 6 files |
| Phase 07 P05 | 15m | 2 tasks | 7 files |
| Phase 08-testing-documentation-release P01 | 5m | 1 tasks | 1 files |
| Phase 08 P03 | 5m | 1 tasks | 1 files |
| Phase 09-unified-layer-naming-convention-tag-based P01 | 25m | 2 tasks | 6 files |
| Phase 09-unified-layer-naming-convention-tag-based P02 | 35m | 4 tasks | 21 files |
| Phase 09-unified-layer-naming-convention-tag-based P03 | 3h | 4 tasks | 6 files |
| Phase 09-unified-layer-naming-convention-tag-based P04 | 5m | 2 tasks | 2 files |

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table.
Recent decisions affecting current work:

- PhotoshopAPI pre-built as static lib with /MD CRT (no CMake at UBT time)
- Trust PhotoshopAPI text API directly (no rasterize fallback, no manual TySh parsing)
- OpenImageIO ships from Phase 2 onward (needed for smart objects in Phase 6)
- [Phase 02-c-psd-parser]: ParseFile uses LayeredFile<bpp8_t>::read(filesystem::path); no in-memory stream overload exists, so plan 02-04 must spill byte buffer to temp file
- [Phase 02-c-psd-parser]: Text extraction uses style_run_*(0) and paragraph_run_justification(0); multi-run flattened with Warning diagnostic; pt->px is 1:1 (UMG DPI scaling deferred to Phase 4 TEXT-01)
- [Phase 02-c-psd-parser P05]: PhotoshopAPI text fill color array is ARGB, not RGBA — undocumented upstream quirk, discovered empirically via debug log. Phase 4 must verify channel order for stroke/shadow/per-run colors before trusting them.
- [Phase 03-layer-mapping-widget-blueprint-generation]: AllMappers.h private header declares all 15 mapper classes; .cpps provide out-of-line implementations — avoids ODR, keeps mappers internal
- [Phase 03-layer-mapping-widget-blueprint-generation]: UWidgetBlueprintFactory::FactoryCreateNew used for WBP creation (canonical editor path); FAnchorCalculator suffix table ordered longest-first; bComputed flag distinguishes fixed vs quadrant-derived stretch anchors
- [Phase 04]: Stroke color ARGB assumed (same as fill color); TEXT-04 shadow fields deferred
- [Phase 05-layer-effects-blend-modes]: Invisible layers created as Collapsed (not skipped) to preserve layer count and allow runtime show/hide
- [Phase 06-advanced-layout]: F9SliceImageLayerMapper at priority 150; FVariantsSuffixMapper at 200; default 9-slice margin 16px; bracket [L,T,R,B] stripped in TryParseSuffix
- [Phase 06-advanced-layout]: ExtractImagePixels templated for SmartObjectLayer; thread_local depth+path tracking in FSmartObjectImporter; FSmartObjectLayerMapper at priority 150
- [Phase 06-advanced-layout]: AlignmentTolerancePx = 6px; heuristic requires ALL children to align (conservative guard); HBox sorted left-to-right, VBox top-to-bottom
- [Phase 07]: Category grouping uses pipe-separator (PSD2UMG|General) for UDeveloperSettings sub-sections
- [Phase 07]: SPsdImportPreviewDialog: widget type inferred from EPsdLayerType + name prefixes (no FindMapper API)
- [Phase 07]: Import factory shows SPsdImportPreviewDialog modal via EditorAddModalWindow before WBP generation; cancel skips generation
- [Phase 07]: FCommonUIButtonLayerMapper at priority 210; CanMap returns false when bUseCommonUI is false; Map returns nullptr if CommonUI module not loaded; AnimationBindings.Add used (no BindPossessableObject)
- [Phase 07]: FReimportManager auto-registers on FReimportHandler construction; module stores TUniquePtr for lifetime
- [Phase 07]: Orphan widgets (deleted PSD layers) kept in WidgetTree per D-07 — Update() never removes them
- [Phase 08]: DOC-03 (CHANGELOG) skipped per D-06; DOC-02 merged inline into README Layer Naming section; fixture PSDs replace example project per D-08
- [Phase 09-unified-layer-naming-convention-tag-based]: FLayerTagParser implemented as pure function with OutDiagnostics out-param; ParsedTags populated as post-pass after ConvertLayerRecursive so D-02 default-type sees final EPsdLayerType
- [Phase 09-unified-layer-naming-convention-tag-based]: @9s shorthand defaults to {16,16,16,16, bExplicit=false} matching F9SliceImageLayerMapper historical default (D-07)
- [Phase 09-unified-layer-naming-convention-tag-based]: [09-02 Task 1] Reimport identity-key strategy: option-a — match by ParsedTags.CleanName. Accepts one-time orphaning of pre-Phase-9 WBPs (D-15 hard cutover). Migration guide (Plan 09-04) must document delete-and-reimport for legacy assets.
- [Phase 09-unified-layer-naming-convention-tag-based]: [09-02] FSwitcherLayerMapper retained at priority 199 dispatching on bIsVariants (parallel to FVariantsSuffixMapper at 200); the grammar has no @switcher tag so this avoids dead-coding the legacy class without behavioural drift.
- [Phase 09-unified-layer-naming-convention-tag-based]: [09-02] FProgressLayerMapper falls back from FindChildByState(Bg) to FindChildByState(Normal) so single-untagged-image progress bars still produce a background brush.
- [Phase 09-unified-layer-naming-convention-tag-based]: [09-03] Effects.psd and Typography.psd retained pre-Phase-9 names — their  separators are descriptive, not legacy dispatch prefixes; grammar has no matching @-tag so no rename needed
- [Phase 09-unified-layer-naming-convention-tag-based]: [09-03] @9s tag attaches to the pixel layer, not its containing group (F9SliceImageLayerMapper consumes per-image)
- [Phase 09-unified-layer-naming-convention-tag-based]: [09-04] Migration guide documents delete-and-reimport as the supported path for pre-Phase-9 WBPs (option-a ParsedTags.CleanName identity from Plan 02 surfaced as designer-facing guidance)
- [Phase 09-unified-layer-naming-convention-tag-based]: [09-04] Button_NN retained in README as Phase-9 parser auto-name (D-21), not legacy dispatch; benign grep hit

### Roadmap Evolution

- Phase 9 added: Unified layer naming convention (tag-based)

### Pending Todos

None yet.

### Blockers/Concerns

- Phase 2 highest risk: PhotoshopAPI transitive dep linking (blosc2, libdeflate, zlib-ng, OpenImageIO). Must validate early.
- Phase 3: Widget Blueprint ConstructWidget > compile > save sequence must be validated before building full mapper.
- PhotoshopAPI text API is new (v0.9.0) -- edge cases with CJK, mixed-font runs unknown.

## Session Continuity

Last session: 2026-04-14T15:36:19.371Z
Stopped at: Completed 09-04-PLAN.md (migration guide + README rewrite) — Phase 9 complete
Resume file: None

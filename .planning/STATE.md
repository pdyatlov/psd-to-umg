---
gsd_state_version: 1.0
milestone: v1.1
milestone_name: Import Fidelity Fixes
status: verifying
stopped_at: Completed 16-03-PLAN.md — Phase 16 Rich Text Multiple Text Runs DONE
last_updated: "2026-04-22T11:53:28.324Z"
last_activity: 2026-04-22
progress:
  total_phases: 2
  completed_phases: 1
  total_plans: 2
  completed_plans: 2
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-04-07)

**Core value:** A designer drops a PSD into Unreal Editor and gets a correctly structured, immediately usable Widget Blueprint -- with no Python dependency, no manual tweaking, and no loss of layer intent.
**Current focus:** Phase 16 — rich-text-multiple-text-runs

## Current Position

Phase: 16
Plan: Not started
Status: Phase complete — ready for verification
Last activity: 2026-04-22

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
| Phase 04.1-text-layer-effects-dispatch P01 | 20m | 2 tasks | 5 files |
| Phase 04.1-text-layer-effects-dispatch P02 | 25m | 2 tasks | 4 files |
| Phase 10-panel-child-attachment P01 | 2m | 1 tasks | 1 files |
| Phase 10-panel-child-attachment P02 | 2 | 1 tasks | 1 files |
| Phase 10-panel-child-attachment P03 | human-in-the-loop + scope trim | 2 tasks | 2 files |
| Phase 12-text-property-fidelity P01 | 45 | 3 tasks | 3 files |
| Phase 12-text-property-fidelity P02 | 30 | 6 tasks | 4 files |
| Phase 13-gradient-layers P02 | 15 | 3 tasks | 3 files |
| Phase 13-gradient-layers P03 | ~90m | 6 tasks | 8 files |
| Phase 14-shape-vector-layers P01 | 15 | 3 tasks | 3 files |
| Phase 14-shape-vector-layers P01 | 30 | 4 tasks | 3 files |
| Phase 14-shape-vector-layers P02 | 4 | 1 tasks | 1 files |
| Phase 14-shape-vector-layers P03 | ~60m | 3 tasks | 5 files |
| Phase 15 P01 | 3m | 3 tasks | 2 files |
| Phase 16-rich-text-multiple-text-runs P02 | 10m | 1 tasks | 1 files |

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
- [Phase 04.1-text-layer-effects-dispatch]: DPI conversion for shadow offset applied in FTextLayerMapper (D-09 Option B) matching OutlineSize pattern; RouteTextEffects helper in Internal namespace clears bHasDropShadow after routing (D-13 guard)
- [Phase 04.1-text-layer-effects-dispatch]: lfx2 discriminator at m_Data[4..7] (version=0x10), not [0..3] as documented; 8-byte prefix confirmed empirically
- [Phase 04.1-text-layer-effects-dispatch]: Hand-rolled lfx2 descriptor walker with std::function recursive skip for Objc/VlLs; FLinearColor::FromSRGBColor for RGBC channels (no ARGB swizzle)
- [Phase 10-panel-child-attachment]: PopulateCanvas renamed to PopulateChildren(UPanelWidget*); double-guard pattern Cast<UCanvasPanel>+Cast<UCanvasPanelSlot>; FX-01/02/03 moved outside slot guard to apply on all parent types
- [Phase 10-panel-child-attachment]: UpdateCanvas signature generalized to UPanelWidget*; D-11 clear-and-rebuild for non-canvas reimport via ClearChildren+PopulateChildren; canvas diff-update path byte-identical (PANEL-05)
- [Phase 12-text-property-fidelity]: TEXT-F-01: * 0.75f formula confirmed correct — PhotoshopAPI returns designer_pt * 4/3 and mapper inverts it; formula removal skipped
- [Phase 12-text-property-fidelity]: Overlay wins over fill: RouteTextEffects routes ColorOverlayColor onto Text.Color after fill is set, matching Photoshop render order
- [Phase 12-text-property-fidelity]: D-13 guard extended to Color Overlay: bHasColorOverlay cleared after routing so FX-03 generator block never fires for text layers
- [Phase 13-gradient-layers]: SoCo descriptor offset confirmed as 4 (PSD spec: 4-byte version=16 prefix precedes the descriptor). TryParseAt(4) first; TryParseAt(0) and TryParseAt(8) as fallbacks. Empirically verified: solid_gray renders correct gray tint.
- [Phase 13-gradient-layers]: SolidFill layers get no pixel extraction — FSolidFillLayerMapper (Plan 13-03) produces zero-texture UImage; FX-03 tints via ColorOverlayColor
- [Phase 13-gradient-layers]: get_channel shim iterates Layer<T>::m_UnparsedImageData — vendored-header patch following Phase 12 unparsed_tagged_blocks() precedent on Layer.h
- [Phase 13-gradient-layers]: PhotoshopAPI classifies gradient and solid-color fill layers as AdjustmentLayer<T>, NOT ShapeLayer<T>. RTTI-only dispatch silently fails. Tag-based detection (adjSolidColor/adjGradient from unparsed_tagged_blocks()) must precede all dynamic_pointer_cast calls. get_channel added to AdjustmentLayer<T> only to avoid C2385 ambiguity on SmartObjectLayer (inherits both Layer<T> and ImageDataMixin).
- [Phase 14-shape-vector-layers]: Shape enum value is distinct from SolidFill (D-03) for semantic clarity; drawn vector shapes (ShapeLayer+vscg) are separate from fill-layer solid-color fills (AdjustmentLayer+SoCo)
- [Phase 14-shape-vector-layers]: FindShapeLayer declared as static member inside FPsdParserShapeSpec BEGIN block -- not reusing Phase 13 FindGradLayer which is scoped to FPsdParserGradientSpec
- [Phase 14-shape-vector-layers]: ShapeLayer PSD layer record bounds = full canvas extent; actual shape geometry in vogk/vscg descriptor — SHAPE-02 Bounds assertion deferred to Plan 14-02
- [Phase 14-shape-vector-layers]: vscg offset confirmed as 4 (identical to SoCo per PSD spec 4-byte version prefix); TryParseAt(4) first, resolves Open Question 1
- [Phase 14-shape-vector-layers]: Type enum value string is 'solidColorLayer'; 'gradientFill' and any other value short-circuit to false, resolves Open Question 2
- [Phase 14-shape-vector-layers]: Dual Clr/FlCl acceptance in ScanShapeFillColor handles all known Photoshop version variants per Pitfall 3
- [Phase 14-shape-vector-layers]: vscg m_Data[0..3] IS the class ID ('SoCo'=solid, 'GdFl'=gradient) — NOT a version prefix. Descriptor starts at offset 8. There is no "Type" enum item inside the descriptor body; type discrimination is done via the upfront class ID check. TryParseAt(8) is primary for vscg (unlike SoCo where offset 4 wins).
- [Phase 15]: D-01/D-02: Group shadow uses null brush (NoDrawType) — no texture asset, solid tint, sized to bounds; ZOrder = main - 1
- [Phase 15]: D-04/D-05/D-06: Canvas group overlay uses fill anchors (0,0)-(1,1); non-canvas group uses AddChild best-effort; deferred via DeferredOverlayPanel local to ensure LAST child placement after recursion
- [Phase 16-rich-text-multiple-text-runs]: Multi-run block placed after stroke block inside existing try{} scope; RawSpans.Num() > 1 threshold after sentinel strip required to commit to Spans; ASCII-only FString::Mid slicing documented as TODO for non-ASCII
- [Phase 16-rich-text-multiple-text-runs]: FRichTextLayerMapper at priority 110; FTextLayerMapper::CanMap narrowed to Spans.Num() <= 1 for compile-time mutual exclusion
- [Phase 16-rich-text-multiple-text-runs]: DataTable persisted at {WbpPath}/{CleanName}_RichStyles/DT_{CleanName}_RichStyles to avoid Pitfall 6 package conflicts with WBP

### Roadmap Evolution

- Phase 9 added: Unified layer naming convention (tag-based)
- Phases 11-12 added: v1.1 Import Fidelity Fixes

### Pending Todos

None yet.

### Blockers/Concerns

- TEXT-F-01: pt→px formula needs verification. Decision log says "1:1" but user reports 24.45→30 inflation. Likely a 72/96 DPI ratio being applied somewhere (24.45 * 96/72 ≈ 32.6; 24.45 * 4/3 ≈ 32.6 — actual 30 suggests a different multiplier). Needs code inspection of FTextLayerMapper before planning.
- TEXT-F-03: ARGB channel-order bug already documented for fill color (Phase 02-c-psd-parser P05 decision). Phase 12 must apply the same ARGB→RGBA swap confirmed empirically.

## Session Continuity

Last session: 2026-04-22T11:50:11.219Z
Stopped at: Completed 16-03-PLAN.md — Phase 16 Rich Text Multiple Text Runs DONE
Resume file: None

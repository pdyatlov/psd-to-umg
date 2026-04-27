# Roadmap: PSD2UMG

## Overview

PSD2UMG transforms Photoshop files into fully functional UMG Widget Blueprints inside UE 5.7. The roadmap moves from porting the existing UE4 codebase, through building a native C++ PSD parser, to implementing the full layer-to-widget mapping pipeline, then layering on text, effects, layout, editor UI, and CommonUI support, culminating in testing and release. Each phase delivers a verifiable capability on top of the previous one.

## Shipped Milestones

- [x] **v1.0** (shipped 2026-04-15) — [archive](milestones/v1.0-ROADMAP.md) • [audit](milestones/v1.0-MILESTONE-AUDIT.md)
- [x] **v1.0.1** (shipped 2026-04-17) — [archive](milestones/v1.0.1-ROADMAP.md)

## Phases

<details>
<summary>✅ v1.0 MVP (Phases 1-9) — SHIPPED 2026-04-15</summary>

See [milestones/v1.0-ROADMAP.md](milestones/v1.0-ROADMAP.md) for full phase details.

</details>

<details>
<summary>✅ v1.0.1 Panel Child Attachment Hotfix (Phase 10) — SHIPPED 2026-04-17</summary>

- [x] Phase 10: Panel Child Attachment (3/3 plans) — completed 2026-04-17

See [milestones/v1.0.1-ROADMAP.md](milestones/v1.0.1-ROADMAP.md) for full phase details.

</details>

## v1.1 Import Fidelity Fixes (Phases 11-12)

- [x] **Phase 11: Import Dialog Hidden-Layer Filtering** - Skipped/deferred (2026-04-21)
- [x] **Phase 12: Text Property Fidelity** - Font size, alignment, and base color correctly preserved from PSD through to UTextBlock (completed 2026-04-17)

## Phase Details

### Phase 11: Import Dialog Hidden-Layer Filtering
**Goal**: Users can see which PSD layers are hidden and control which layers are included in the generated Widget Blueprint via import dialog checkboxes
**Depends on**: Nothing (targets existing SPsdImportPreviewDialog + FWidgetBlueprintGenerator)
**Requirements**: HIDDEN-01, HIDDEN-02, FILTER-01, FILTER-02
**Success Criteria** (what must be TRUE):
  1. Importing a PSD with hidden layers shows those layers with their checkbox unchecked by default in the preview dialog
  2. Hidden layers are visually distinct from visible layers in the dialog tree (dimmed row, eye-closed icon, or [hidden] label)
  3. Unchecking a layer in the dialog results in that layer's widget not appearing in the generated WBP
  4. Unchecking a group layer also excludes all of its children from the generated WBP
**Plans**: 2 plans
Plans:
- [ ] 11-01-PLAN.md — Data model + bVisible init fix (FPsdLayerTreeItem fields, BuildTreeRecursive, LayerName bug)
- [ ] 11-02-PLAN.md — UI eye-icon column in OnGenerateRow

### Phase 12: Text Property Fidelity
**Goal**: Text layers import with correct font size, paragraph alignment, and fill color — what the designer set in Photoshop is what appears in UMG
**Depends on**: Nothing (targets existing FTextLayerMapper)
**Requirements**: TEXT-F-01, TEXT-F-02, TEXT-F-03
**Success Criteria** (what must be TRUE):
  1. A text layer with 24.45 pt font in PSD produces a UTextBlock with the correct UMG font size (not inflated to 30)
  2. A center-aligned paragraph in PSD produces a UTextBlock with ETextJustify::Center after compile+save
  3. A gray text layer in PSD produces a UTextBlock with a gray ColorAndOpacity (not red or another corrupted color)
**Plans**: 2 plans
Plans:
- [x] 12-01-PLAN.md — Font size fidelity: confirm raw SizePx, remove * 0.75f, update test expectations (TEXT-F-01)
- [x] 12-02-PLAN.md — Alignment + fill color: fixture update, parser fallbacks, new spec assertions (TEXT-F-02, TEXT-F-03)

## v1.2 Layer Fidelity Expansion (Phases 13-17)

- [x] **Phase 13: Gradient Layers** - Gradient fill layers imported as pre-rendered TC_BC7 UImage widgets; solid fills as zero-texture UImage tinted via FX-03 (completed 2026-04-21)
- [x] **Phase 14: Shape/Vector Layers** - Photoshop solid-color shape layers imported as UImage with solid-color brush (completed 2026-04-22)
- [x] **Phase 15: Group Effects** - Effects applied to group layers (drop shadow, color overlay) propagated to the group's container widget (completed 2026-04-22)
- [x] **Phase 16: Rich Text / Multiple Text Runs** - Text layers with mixed styles (bold/italic/color spans) imported as URichTextBlock with inline style definitions (completed 2026-04-22)
- [x] **Phase 16.1: LayerTag Fix + Requirements Traceability** - FLayerTagParser type-inference fix for Gradient/SolidFill/Shape layers; HIDDEN-02 row dimming; REQUIREMENTS.md extended with v1.2 requirements and corrected traceability (completed 2026-04-22)
- [x] **Phase 17: Automated Font Matching** - Photoshop font names resolved to UE font assets automatically via a configurable name-mapping table with fuzzy fallback (completed 2026-04-22)
- [ ] **Phase 17.1: Button+Variants State Wiring Validation** (INSERTED) — Verify `@button`+`@variants` can co-exist on one layer, and that all four UE Button child states (`@state:normal/hover/pressed/disabled`) wire correctly
- [x] **Phase 17.2: Button State Text Animation** (INSERTED) — When `@button` `@state:*` groups contain text layers with different ColorAndOpacity across states, generate UWidgetAnimation color tracks and wire OnHovered/OnPressed delegates via K2 Blueprint graph nodes (completed 2026-04-27)

### Phase 13: Gradient Layers
**Goal**: Gradient fill layers from Photoshop import as usable UMG widgets — either pre-rendered as Texture2D or as a native gradient widget if available
**Depends on**: Phase 2 (parser), Phase 3 (mapper pipeline)
**Requirements**: GRAD-01, GRAD-02
**Success Criteria** (what must be TRUE):
  1. A linear gradient fill layer in PSD produces a valid UMG widget (UImage with pre-rendered texture or equivalent) after import
  2. Gradient color stops and direction are preserved with no visible banding at standard resolutions
**Plans**: 3 plans
Plans:
- [x] 13-01-PLAN.md — Fixture + RED spec stubs + EPsdLayerType::Gradient/SolidFill enum (Wave 1)
- [x] 13-02-PLAN.md — ShapeLayer.h get_channel shim + ConvertLayerRecursive dispatch + ScanSolidFillColor (Wave 2)
- [x] 13-03-PLAN.md — FFillLayerMapper + FSolidFillLayerMapper + registry + TC_BC7 + visual verify (Wave 3)

### Phase 14: Shape/Vector Layers
**Goal**: Photoshop drawn vector/shape layers (Rectangle/Ellipse/Pen Tool) with solid-color fill import as UImage widgets with a matching solid SlateBrush — preserving position, size, and fill color. Refines the Phase 13 ShapeLayer dispatch with a vscg (vecStrokeContentData) Type-field byte-walk to distinguish solid-fill drawn shapes from gradient-fill drawn shapes; stroke rendering and pattern fills deferred to later phases.
**Depends on**: Phase 2 (parser), Phase 3 (mapper pipeline), Phase 13 (ShapeLayer branch + FSolidFillLayerMapper template)
**Requirements**: SHAPE-01, SHAPE-02
**Success Criteria** (what must be TRUE):
  1. A solid-color rectangle shape layer in PSD produces a UImage with a solid SlateBrush of the correct color after import
  2. Position and size of the shape widget match the PSD layer bounds within 1px
**Plans**: 3 plans
Plans:
- [x] 14-01-PLAN.md — ShapeLayers.psd fixture + EPsdLayerType::Shape enum + RED FPsdParserShapeSpec stubs (Wave 1)
- [x] 14-02-PLAN.md — ScanShapeFillColor vscg byte-walker + 3-way ShapeLayer dispatch (SoCo / vscg / Gradient) (Wave 2)
- [x] 14-03-PLAN.md — FShapeLayerMapper + registry registration + SPsdImportPreviewDialog label + visual verify (Wave 3)

### Phase 15: Group Effects
**Goal**: Layer effects (drop shadow, color overlay) applied to Photoshop group layers are reflected on the generated container widget
**Depends on**: Phase 4.1 (effects dispatch), Phase 3 (group mapping)
**Requirements**: GRPFX-01, GRPFX-02
**Success Criteria** (what must be TRUE):
  1. A group layer with a drop shadow effect produces a container widget with a matching drop shadow applied
  2. A group layer with a color overlay produces a container widget with the overlay color applied
**Plans**: 1 plan
Plans:
- [x] 15-01-PLAN.md — FX-04 drop shadow + FX-03 color overlay extended to EPsdLayerType::Group with post-recursion overlay placement; 4 new automation spec cases (GRPFX-01, GRPFX-02)

### Phase 16: Rich Text / Multiple Text Runs
**Goal**: Text layers containing multiple style runs (mixed font size, weight, color within one layer) import as URichTextBlock with correct inline markup
**Depends on**: Phase 12 (text property fidelity)
**Requirements**: RICH-01, RICH-02
**Success Criteria** (what must be TRUE):
  1. A text layer with two differently-colored spans produces a URichTextBlock with both colors represented in inline markup
  2. A text layer with bold and normal weight runs produces a URichTextBlock with correct weight styling per span
**Plans**: 3 plans
Plans:
- [x] 16-01-PLAN.md — RichText.psd fixture + FPsdTextRunSpan struct + FPsdTextRun::Spans field + RED parser/mapper spec stubs (Wave 1)
- [x] 16-02-PLAN.md — ExtractSingleRunText multi-run extraction loop populating OutLayer.Text.Spans with sentinel stripping (Wave 2)
- [x] 16-03-PLAN.md — FRichTextLayerMapper at priority 110 + persistent UDataTable companion asset + FTextLayerMapper predicate narrowing (Wave 3)

### Phase 16.1: LayerTag Fix + Requirements Traceability
**Goal**: Close two audit gaps before milestone re-audit — fix the FLayerTagParser type-inference hole that silently drops fill/shape layers tagged `@image`, add HIDDEN-02 visual row dimming, and extend REQUIREMENTS.md with all v1.2 requirements and correct traceability status.
**Depends on**: Phase 13, Phase 14, Phase 11 (all code complete; this phase closes documentation and correctness gaps)
**Requirements**: GRAD-01, GRAD-02, SHAPE-01, SHAPE-02, HIDDEN-02
**Success Criteria** (what must be TRUE):
  1. `FLayerTagParser::Parse` maps `EPsdLayerType::Gradient`, `EPsdLayerType::SolidFill`, `EPsdLayerType::Shape` to `EPsdTagType::Image` in the D-02 default-type inference switch
  2. Hidden-layer rows in `SPsdImportPreviewDialog` render the layer-name text with reduced opacity (e.g., `ColorAndOpacity(FLinearColor(1,1,1,0.4f))`) when `!Item->bLayerVisible`
  3. REQUIREMENTS.md includes GRAD-01/02, SHAPE-01/02, GRPFX-01/02, RICH-01/02 with correct phase assignments and up-to-date traceability status
**Plans**: 1 plan
Plans:
- [x] 16.1-01-PLAN.md — FLayerTagParser 3-case fix + SPsdImportPreviewDialog dimming + REQUIREMENTS.md v1.2 extension

### Phase 17: Automated Font Matching
**Goal**: Photoshop font family names are automatically resolved to UE font assets via a configurable mapping table, with fuzzy fallback for common name variants
**Depends on**: Phase 12 (text pipeline)
**Requirements**: FONT-01, FONT-02
**Success Criteria** (what must be TRUE):
  1. A font named "Roboto-Bold" in PSD resolves to the correct UE font asset without any manual configuration if the asset exists in the project
  2. A font with no exact match logs a warning and falls back to the project default font rather than silently producing no font
**Plans**: 2 plans
Plans:
- [x] 17-01-PLAN.md — Enum AutoDiscovered + RED spec + REQUIREMENTS.md FONT-01/02 entries (Wave 1)
- [x] 17-02-PLAN.md — AssetRegistry auto-discovery cache + PsdImportFactory invalidation hook + FONT-01 close-out (Wave 2)

### Phase 17.1: Button+Variants State Wiring Validation (INSERTED)
**Goal**: Confirm that a PSD layer tagged with both `@button` and `@variants` is parsed and mapped correctly — no tag conflict, correct widget type — and that child layers tagged `@state:normal`, `@state:hover`, `@state:pressed`, `@state:disabled` are wired to the four UE Button widget appearance slots
**Depends on**: Phase 17 (tag parser), Phase 3 (mapper pipeline)
**Requirements**: BTN-STATE-01, BTN-STATE-02
**Success Criteria** (what must be TRUE):
  1. A layer tagged `@button @variants` produces a UButton widget (not a generic UImage or conflict error)
  2. Child layers `@state:normal`, `@state:hover`, `@state:pressed`, `@state:disabled` each map to the corresponding `Normal`/`Hovered`/`Pressed`/`Disabled` appearance slot on the UButton
  3. No assertion or tag-conflict log error fires when both `@button` and `@variants` are present on the same layer
  4. Missing state children (e.g. only `@state:normal` present) produce a warning but do not abort import
**Plans**: 2 plans
Plans:
- [x] 17.1-01-PLAN.md — Wave 0 RED: FButtonLayerMapperSpec.cpp + REQUIREMENTS.md BTN-STATE-01/02 entries (Wave 1)
- [ ] 17.1-02-PLAN.md — GREEN: FVariantsSuffixMapper HasType guard (D-01) + FButtonLayerMapper aggregate missing-slots warning (D-03) + REQUIREMENTS close-out (Wave 2)

### Phase 17.2: Button State Text Animation (INSERTED)
**Goal**: When a @button layer contains @state:* child groups that include text layers with different ColorAndOpacity values across states, auto-generate UWidgetAnimation color tracks and wire UButton event delegates (OnHovered/OnUnhovered/OnPressed/OnReleased) to PlayAnimation/ReverseAnimation calls via K2 Blueprint graph nodes injected into the WBP Event Graph — so the generated WBP works at runtime without any designer hand-wiring.
**Depends on**: Phase 17 (tag parser + text pipeline), Phase 17.1 (@button state-slot wiring foundation), Phase 3 (mapper pipeline)
**Requirements**: BTN-ANIM-01, BTN-ANIM-02, BTN-ANIM-03
**Success Criteria** (what must be TRUE):
  1. FPsdWidgetAnimationBuilder::CreateColorAnim produces a UWidgetAnimation with one UMovieSceneColorTrack on ColorAndOpacity, four R/G/B/A channel linear keys at frames 0 and DurationSec*24000
  2. For a @button PSD with @state:normal + @state:hover text children of different colors, the generator creates an animation named {CleanName}_Hover bound to the inner text widget; symmetric for @state:pressed → {CleanName}_Pressed
  3. @state:* layers direct-children of a UButton are NOT added as widget children by PopulateChildren (D-08/D-09); UButton's single child comes from @state:normal non-@background content (text only in this phase, per D-05)
  4. WBP Event Graph contains UK2Node_ComponentBoundEvent for OnHovered/OnUnhovered/OnPressed/OnReleased, each wired (then-pin → exec-pin) to UK2Node_CallFunction targeting UUserWidget::PlayAnimation or ReverseAnimation; InAnimation pin fed by UK2Node_VariableGet referencing the corresponding animation
  5. In PIE, hovering / pressing the generated button produces a visible text color transition; unhovering / releasing reverses it
  6. Disabled state is intentionally NOT animated — UE 5.7 UButton has no OnDisabled delegate (D-02 limitation, documented in VERIFICATION.md)
**Plans**: 4 plans
Plans:
- [x] 17.2-01-PLAN.md — Wave 1 RED: FButtonStateTextAnimSpec.cpp + REQUIREMENTS.md BTN-ANIM-01/02/03 entries
- [x] 17.2-02-PLAN.md — Wave 2 GREEN: CreateColorAnim + FButtonLayerMapper bIsVariable + @state:normal content + PopulateChildren skip guard; spec 3/4 GREEN
- [x] 17.2-03-PLAN.md — Wave 3 GREEN: BlueprintGraph dep + generator integration (TraverseButtonLayers + BuildButtonStateAnimations + InjectButtonEventGraphWiring + two-compile sequence); spec 4/4 GREEN; REQUIREMENTS BTN-ANIM-01/02 Complete
- [x] 17.2-04-PLAN.md — Wave 4 human verify: end-to-end PIE test + 17.2-VERIFICATION.md + REQUIREMENTS BTN-ANIM-03 Complete

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 11. Import Dialog Hidden-Layer Filtering | 0/2 | Skipped | 2026-04-21 |
| 12. Text Property Fidelity | 2/2 | Complete   | 2026-04-17 |
| 13. Gradient Layers | 3/3 | Complete | 2026-04-21 |
| 14. Shape/Vector Layers | 3/3 | Complete | 2026-04-22 |
| 15. Group Effects | 1/1 | Complete    | 2026-04-22 |
| 16. Rich Text / Multiple Text Runs | 3/3 | Complete    | 2026-04-22 |
| 16.1. LayerTag Fix + Requirements Traceability | 1/1 | Complete    | 2026-04-22 |
| 17. Automated Font Matching | 2/2 | Complete    | 2026-04-22 |
| 17.1. Button+Variants State Wiring Validation | 1/2 | In Progress|  |
| 17.2. Button State Text Animation | 4/4 | Complete | 2026-04-27 |
| 18. Phase 11 Verification Backfill | 0/TBD | Pending | — |
| 19. Integration Stability Fixes | 0/TBD | Pending | — |

## v1.1 / v1.2 Gap Closure (Phases 18-19)

- [ ] **Phase 18: Phase 11 Verification Backfill** — Execute Phase 11 plans, produce VERIFICATION.md, document headless path limitation, close HIDDEN-01/FILTER-01/FILTER-02
- [ ] **Phase 19: Integration Stability Fixes** — Raise fill/shape mapper priorities to 101, add reimport cache invalidation

### Phase 18: Phase 11 Verification Backfill
**Goal**: Formally close HIDDEN-01, FILTER-01, and FILTER-02 by executing the already-written Phase 11 plans in UE Editor, producing a VERIFICATION.md, and documenting the headless import path limitation.
**Depends on**: Phase 11 (code complete; this phase verifies and documents)
**Requirements**: HIDDEN-01, FILTER-01, FILTER-02
**Gap Closure**: Closes gaps from v1.1/v1.2 audit
**Success Criteria** (what must be TRUE):
  1. Phase 11 plans 11-01 and 11-02 executed and confirmed working in UE Editor
  2. VERIFICATION.md exists for Phase 11 with all criteria passing
  3. Headless path limitation (bShowPreviewDialog=false skips filtering) formally documented
  4. REQUIREMENTS.md HIDDEN-01, FILTER-01, FILTER-02 marked Complete
  5. ROADMAP Phase 11 plans checked [x]
**Plans**: TBD

### Phase 19: Integration Stability Fixes
**Goal**: Eliminate the mapper priority collision (non-stable sort risk for gradient/shape/fill layers) and reimport cache leak (fonts added between reimports invisible until engine restart).
**Depends on**: Phase 13, Phase 14, Phase 17
**Requirements**: GRAD-01, SHAPE-01, FONT-01 (integration hardening)
**Gap Closure**: Closes MAPPER_PRIORITY_COLLISION and REIMPORT_CACHE_LEAK integration gaps from v1.1/v1.2 audit
**Success Criteria** (what must be TRUE):
  1. FFillLayerMapper, FSolidFillLayerMapper, FShapeLayerMapper registered at priority 101 (above FImageLayerMapper at 100)
  2. FPsdReimportHandler::Reimport calls FFontResolver::InvalidateDiscoveryCache() after FWidgetBlueprintGenerator::Update
  3. Existing gradient, shape, and font tests pass with no regressions
**Plans**: TBD

## v1.3+ Backlog

Candidate work (from deferred registry):
- frameFXMulti VlLs stroke format (newer Photoshop)
- Image-layer stroke rendering (D-12 — data already populated)
- lrFX channel-order visual confirm for color overlay/shadow (needs host .uproject)
- GitHub Actions CI (D-04)
- CHANGELOG.md (D-06)
- Phase 2 VERIFICATION.md backfill (optional)
- Runtime module (RUN-01)
- Figma export JSON input (FMT-01)
- URichTextBlock mixed-style runs (TEXT-V2-01)
- Custom PSD thumbnail in Content Browser (MKT-01)
- Toolbar button "Re-import from PSD" in Widget Blueprint Editor (MKT-02)
- Fab/Epic Marketplace submission packaging (MKT-03)
- Configurable non-canvas slot properties (alignment, padding, orientation) — deferred from v1.0.1
- Overlay/Canvas/Nested spec coverage — test-coverage gap only (implementation is complete)
- Phase 14 deferred: Stroke rendering (`vstk` -> UMG border/outline effect); Pattern fill layers (`PtFl`); Vector path export (SVG / UMG custom shape widget)

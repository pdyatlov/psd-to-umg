# Phase 22: Stroke Rendering - Context

**Gathered:** 2026-04-29
**Status:** Ready for planning

<domain>
## Phase Boundary

Three stroke rendering requirements implemented:
1. **STROKE-01** — lfx2 `bHasStroke` path: image layers emit a stroke sibling UImage (sized +2×StrokePx, offset -StrokePx, tinted StrokeColor, ZOrder = main - 1). Canvas-only; mirrors FX-04 drop-shadow pattern exactly.
2. **STROKE-02** — `ScanVstkStroke()` parses `vstk` (vecStrokeData) tagged block at **byte offset 0** (CP-01) and writes `bHasVectorStroke`, `VectorStrokeSize`, `VectorStrokeColor` to `FPsdLayerEffects`. Must NOT write to `bHasStroke` (CP-02).
3. **STROKE-03** — vstk path: `FWidgetBlueprintGenerator` new FX-05 block checks `bHasVectorStroke` on Shape layers and emits a sibling UImage using the same geometry as STROKE-01.

No new mapper API changes. No new widget types. No new fixture PSD.

</domain>

<decisions>
## Implementation Decisions

### STROKE-03 Architecture
- **D-01:** Stroke sibling creation for both lfx2 (STROKE-01) and vstk (STROKE-03) paths lives in `FWidgetBlueprintGenerator`, NOT in `FShapeLayerMapper::Map()`. STROKE-03 is a new FX-05 block in the generator that checks `bHasVectorStroke` on Shape-type layers — identical pattern to FX-04 (drop shadow). `FShapeLayerMapper` does not create siblings; the "reads bHasVectorStroke" language in REQUIREMENTS means the generator reads the field, consistent with how FX-04 reads `bHasDropShadow`. No mapper API changes required.

### DrawType::Border Rule
- **D-02:** Always sibling UImage for both STROKE-01 and STROKE-03. No DrawType::Border logic. Shape layers produce UImages tinted via ColorOverlayColor (no texture file); the stroke geometry is a sized+offset sibling UImage with StrokeColor tint at ZOrder = main - 1, matching STROKE-01 exactly. This keeps one consistent implementation across both stroke paths.

### Double-Emit Guard
- **D-03:** vstk wins. `ScanVstkStroke()` clears `bHasStroke` when it sets `bHasVectorStroke` on a Shape layer. Guard lives at parse time, not at emit time. When both fields were potentially set (lfx2 raster stroke + vstk vector stroke on the same shape), the vstk value takes precedence. Pattern mirrors the D-13-style text outline guard (bHasStroke cleared after routing to outline fields). The generator never sees both `bHasStroke` and `bHasVectorStroke` simultaneously on a Shape layer.

### Fixture Strategy
- **D-04:** No new stroke fixture PSD. Validation uses ButtonStyles.psd for non-regression (success criterion 4 — no strokes present, all existing layers import correctly). Positive stroke assertions (STROKE-01, STROKE-03 sibling creation) are specification-only (unit-level assertions against parsed `FPsdLayerEffects` fields if feasible without a fixture, otherwise deferred until a fixture is available). Research to determine if spec-level stubs can exercise the generator FX-05 block without a PSD.

### Claude's Discretion
- Whether STROKE-01 (lfx2) and STROKE-03 (vstk) are the same FX block with a combined condition, or two sequential blocks that both emit via the same helper.
- Whether to add a shared `EmitStrokeSibling(UCanvasPanel*, UCanvasPanelSlot*, const FPsdLayer*, UWidgetTree*)` helper used by both paths.
- Plan count and split (researcher/planner decide based on dependency graph; STROKE-02 must precede STROKE-03, STROKE-01 is independent).

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Core parser file
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — `ScanShapeFillColor` pattern (~line 1445) as the model for `ScanVstkStroke()`; tagged block iteration via `unparsed_tagged_blocks()` (~line 687, ~line 1213, ~line 1450); existing lfx2 stroke set (~line 1811)

### Type definitions
- `Source/PSD2UMG/Public/Parser/PsdTypes.h` — `FPsdLayerEffects` (existing `bHasStroke`, `StrokeSize`, `StrokeColor` at lines 120-122; new `bHasVectorStroke`, `VectorStrokeSize`, `VectorStrokeColor` fields to add)

### Generator FX pattern (MANDATORY — STROKE-01/03 must mirror this exactly)
- `Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp` — FX-04 drop shadow block (~lines 305-366): sibling UImage creation, ZOrder = main - 1, canvas-only guard, `MakeUniqueObjectName`, `CanvasParent->AddChildToCanvas`

### Shape mapper (STROKE-03 consumer)
- `Source/PSD2UMG/Private/Mapper/FShapeLayerMapper.cpp` — current Map() implementation; confirms mappers return single UWidget*, no side-channel

### PhotoshopAPI enum (STROKE-02 block key)
- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/Util/Enum.h` — `TaggedBlockKey::vecStrokeData` (line 822); `TaggedBlockKey::vecStrokeContentData` (line 823) for reference

### Requirements + pitfalls (MANDATORY)
- `.planning/REQUIREMENTS.md` — STROKE-01, STROKE-02, STROKE-03 specs; CP-01 (vstk offset=0), CP-02 (bHasVectorStroke not bHasStroke)
- `.planning/research/SUMMARY.md` — synthesized pitfall details

### Prior phase decisions
- `.planning/phases/21-parser-correctness-fixes/21-CONTEXT.md` — D-06/D-07 (FXFMT-01 VlLs branch): model for adding a new tagged-block parse path
- `.planning/phases/15-group-layer-effects/15-CONTEXT.md` (if exists) — D-01/D-02: group shadow null-brush pattern; ZOrder = main - 1

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- FX-04 drop shadow block (`FWidgetBlueprintGenerator.cpp:305-366`): copy/adapt for FX-05 stroke block — identical sibling UImage pattern
- `ScanShapeFillColor()` (`PsdParser.cpp:1445`): model for `ScanVstkStroke()` — same `unparsed_tagged_blocks()` iteration, same descriptor walker structure
- `bHasStroke` / `StrokeSize` / `StrokeColor` on `FPsdLayerEffects` already exist — `bHasVectorStroke` / `VectorStrokeSize` / `VectorStrokeColor` are new parallel fields to add

### Established Patterns
- Tagged block iteration: `for (const auto& Block : InLayer->unparsed_tagged_blocks())` — already used for adjSolidColor, vecStrokeContentData, fxLayer
- `TaggedBlockKey::vecStrokeData` enum value already in PhotoshopAPI `Enum.h:822` — no vendored header patch needed
- Canvas-only guard for sibling creation: `if (CanvasParent)` block wrapping the FX-04 drop shadow; FX-05 stroke uses the same guard
- D-13-style guard: `bHasStroke = false` after routing (PsdParser.cpp:1842); `ScanVstkStroke()` does the same for Shape layers (clears bHasStroke)

### Integration Points
- `FPsdLayerEffects` struct: add 3 new fields alongside existing stroke fields
- `ScanVstkStroke()`: new static function in PsdParser.cpp, called in `ConvertLayerRecursive` after existing lfx2 parse block
- FX-05 block in `FWidgetBlueprintGenerator::PopulateCanvas` (or equivalent): after FX-04 drop shadow block, before FX-03 color overlay block OR in a logical FX sequence position

</code_context>

<specifics>
## Specific Notes

- CP-01 (offset=0): `ScanVstkStroke()` must parse the `vstk` descriptor at byte offset 0, NOT 4 or 8. This differs from `SoCo` (offset 4) and `vscg` (offset 8). Research must confirm by inspecting a real `vstk` block or PSD spec.
- CP-02 (separate field): `bHasVectorStroke` is the vstk field; `bHasStroke` is the lfx2 field. They are never aliased. Parser must respect this ownership boundary.
- D-03 guard scope: "clears bHasStroke on Shape layers only" — Image layers with lfx2 stroke should NOT be affected by ScanVstkStroke(). Guard is conditional on layer type == Shape.
- ButtonStyles.psd fixture already exists at `Source/PSD2UMG/Tests/Fixtures/ButtonStyles.psd` (new in git status) — spec writer should use it for the regression assertion (success criterion 4).

</specifics>

<deferred>
## Deferred Ideas

- Positive stroke fixture (Stroke.psd): deferred until user can provide a PSD with lfx2 image stroke and vstk shape stroke layers.
- DrawType::Border approach for shape strokes: intentionally not used per D-02; could revisit if sibling-image approximation is visually wrong.
- frameFXMulti VlLs stroke rendering: FXFMT-01 (Phase 21) unlocked parsing of VlLs-origin effects data; stroke emission for VlLs-origin vstk data is v1.3+ backlog.
- Inside/outside/center stroke alignment precision: sibling-image approximation is center-aligned; per REQUIREMENTS.md out-of-scope for v1.3.

</deferred>

---

*Phase: 22-stroke-rendering*
*Context gathered: 2026-04-29*

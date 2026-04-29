# v1.3 Advanced Effects — Requirements

**Milestone:** v1.3 Advanced Effects
**Status:** Defining
**Date:** 2026-04-28

---

## Scope

Close the visual delta between PSD and UMG by implementing stroke rendering, pattern fill
import, lrFX format correctness, and non-ASCII rich text robustness.

---

## Requirements

### Stroke Rendering

| ID | Requirement | Priority |
|----|-------------|----------|
| STROKE-01 | Image/shape layers with lfx2 `bHasStroke` set emit a stroke sibling UImage (size + 2×StrokePx, offset -StrokePx, tinted StrokeColor, ZOrder = main - 1). Canvas-only; mirrors existing drop-shadow pattern. No texture required. | Must |
| STROKE-02 | `ScanVstkStroke()` parses the `vstk` (vecStrokeData) tagged block at **byte offset 0** (not 4 or 8) and writes new fields `bHasVectorStroke`, `VectorStrokeSize`, `VectorStrokeColor` to `FPsdLayerEffects`. Must NOT write to `bHasStroke` (already owned by lfx2 for all layer types — CP-02). | Must |
| STROKE-03 | `FShapeLayerMapper` reads `bHasVectorStroke`; when set, emits stroke geometry (sibling UImage or DrawType::Border depending on texture presence). | Must |

### Effects Format

| ID | Requirement | Priority |
|----|-------------|----------|
| FXFMT-01 | `ParseFrFXDescriptor` adds a `VlLs` branch so Photoshop CC 2014+ effects (stored as a `VlLs` list under `"FrFX"` instead of a single `Objc`) are parsed rather than silently discarded via `SkipValueAfterOsType`. | Must |

### Pattern Fill

| ID | Requirement | Priority |
|----|-------------|----------|
| PTFL-01 | `ConvertLayerRecursive` detects the `adjPattern` (`TaggedBlockKey::adjPattern`) tagged block and sets `EPsdLayerType::PatternFill` (new enum value, parallel to `SolidFill`/`Gradient`). Without this the layer falls to `Unknown` and is silently skipped. | Must |
| PTFL-02 | `FPatternFillLayerMapper` (new mapper, priority 101) returns a UImage backed by PhotoshopAPI's composited `RGBAPixels`. If `RGBAPixels` is empty, falls back to `bFlattenComplexEffects` flatten path and logs a warning. | Must |

### Rich Text & Typography

| ID | Requirement | Priority |
|----|-------------|----------|
| RTXT-01 | Fix `Utf8ToFString` call in PSD text span extraction: pass explicit `FullUtf8.size()` (not implicit `strlen`) and strip any trailing `\0` sentinel before conversion. Add `RichTextCJK.psd` fixture with a CJK/emoji multi-run layer to make slicing correctness empirical. | Must |

### lrFX Channel Order

| ID | Requirement | Priority |
|----|-------------|----------|
| LFXC-01 | `ExtractLfx2*` functions add a ColorSpace branch: ColorSpace=1 (HSB) converts via `FLinearColor::HSVToLinearRGB`; ColorSpace=2 (CMYK) or other non-RGB spaces log `UE_LOG Warning` and use a best-effort identity mapping. ColorSpace=0 (RGB) unchanged. | Must |
| LFXC-02 | Human UAT: visual confirm that color overlay and drop shadow render with correct RGBC channel order on a real UE 5.7 host project. No code change expected (code verified correct for RGB); task closes the open empirical question. | Should |

---

## Out of Scope (v1.3)

- Material-based tiling for pattern fills (composited PNG is always visually correct for v1.3)
- Inside/outside/center stroke precision (sibling-image approximation acceptable)
- CMYK/Lab lrFX full conversion (warn path sufficient)
- URichTextBlock multi-run beyond first dominant run improvements (separate initiative)
- frameFXMulti VlLs stroke rendering (FXFMT-01 unlocks parsing; stroke emission for VlLs-origin data is post-v1.3 unless trivial)

---

## Dependencies

- STROKE-01 depends on nothing (lfx2 data already in place since Phase 4.1)
- STROKE-02 depends on nothing (new parser path, no overlap with STROKE-01)
- STROKE-03 depends on STROKE-02
- FXFMT-01 depends on nothing (isolated branch in ParseFrFXDescriptor)
- PTFL-01 depends on nothing
- PTFL-02 depends on PTFL-01
- RTXT-01 depends on nothing
- LFXC-01 depends on nothing
- LFXC-02 depends on LFXC-01 (UAT after code fix)

---

## Traceability

Continuing from v1.2 (25 requirements, Phases 13-20).
Research basis: `.planning/research/SUMMARY.md` (synthesized 2026-04-28).
Critical pitfalls: CP-01 (vstk offset=0), CP-02 (bHasVectorStroke not bHasStroke),
CP-03 (VlLs branch), CP-04 (PtFl has no Clr key), CP-05 (Utf8ToFString null-sentinel).

| Requirement | Phase | Status |
|-------------|-------|--------|
| RTXT-01 | Phase 21 / Phase 21-01 | Complete — sentinel strip at Content scalar + FullUtf8 callsites; FPsdParserCJKSpec added (2026-04-28) |
| LFXC-01 | Phase 21 | Complete |
| LFXC-02 | Phase 21 | Deferred — code correct (ConvertLfx2Color RGB path byte-identical, HSB via HSVToLinearRGB); visual UAT on real UE 5.7 host deferred to pre-v1.3 session (see 21-04-UAT-LOG.md) |
| FXFMT-01 | Phase 21 | Complete |
| STROKE-02 | Phase 22 | Pending |
| STROKE-01 | Phase 22 | Pending |
| STROKE-03 | Phase 22 | Pending |
| PTFL-01 | Phase 23 | Pending |
| PTFL-02 | Phase 23 | Pending |

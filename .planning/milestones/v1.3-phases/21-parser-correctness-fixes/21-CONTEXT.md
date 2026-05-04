# Phase 21: Parser Correctness Fixes - Context

**Gathered:** 2026-04-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Four parser-level defects resolved:
1. **RTXT-01** — `Utf8ToFString` null-sentinel truncation: strip trailing `\0` bytes from `FullUtf8` before conversion; add `RichTextCJK.psd` fixture to empirically validate non-ASCII slicing.
2. **LFXC-01** — lrFX ColorSpace branch: add ColorSpace dispatch to `sofi` and `dsdw` parsers (HSB→RGB via `HSVToLinearRGB`; CMYK→warn+identity; RGB unchanged).
3. **LFXC-02** — Human UAT: visual confirm color overlay and drop shadow render correctly on a real UE 5.7 host project. Non-blocking — ships as a separate task after code changes.
4. **FXFMT-01** — `ParseFrFXDescriptor` VlLs branch: iterate `FrFX` → `VlLs` list items (Photoshop CC 2014+ format) and **fully extract** stroke fields (`bHasStroke`, `StrokeSize`, `StrokeColor`), same depth as the existing `Objc` branch.

No new mappers, no new widget types, no reimport changes belong here.

</domain>

<decisions>
## Implementation Decisions

### CJK Fixture (RTXT-01)
- **D-01:** User provides a real `RichTextCJK.psd` file with at least one CJK (or emoji) multi-run text layer. File lands in `Source/PSD2UMG/Tests/Fixtures/` **before execution starts**. Plans assume fixture present; no execution checkpoint needed.
- **D-02:** Spec (`FPsdParserRichTextCJKSpec` or folded into existing rich-text spec) validates: (a) null-sentinel stripped — no garbage/truncation at import, (b) run boundary slicing correct for multi-byte characters.

### Utf8ToFString Null-Sentinel Fix (RTXT-01)
- **D-03:** Fix is at the `FullUtf8` call site in `PsdParser.cpp` (~line 485) — strip trailing `\0` bytes from the `std::string` before passing to `Utf8ToFString`. The `Utf8ToFString` helper itself (`c_str()` path at line 75) is not changed; the sentinel strip is a local pre-process.
- **D-04:** `FString::Mid` indexing by TCHAR is correct for CJK once the UTF-8→TCHAR conversion is clean (TCHAR = UTF-16 code unit; `style_run_lengths` = UTF-16 code units → boundaries align). No change needed to the Mid slicing loop beyond the sentinel fix.

### lrFX ColorSpace Branch (LFXC-01)
- **D-05:** Both `sofi` (~line 704) and `dsdw` (~line 750) in `ExtractLfx2Effects` get a `switch (ColorSpace)` dispatch **after** the channel reads:
  - `ColorSpace == 0` (RGB): current path — `FLinearColor(C0, C1, C2, A)` unchanged.
  - `ColorSpace == 1` (HSB): PS stores H in [0..65535] (→ 0..1 after `/65535.f`), S and B same. Convert: `FLinearColor::MakeFromHSV8` or `FMath::ColorFromHSVf`. Researcher to confirm exact UE API; keep H×360, S×100, B×100 semantics.
  - `ColorSpace == 2` (CMYK) or any other: `UE_LOG(LogPSD2UMG, Warning, ...)` + best-effort identity pass-through (use C0/C1/C2 as-is). Do NOT crash or zero out the color.

### FXFMT-01 — VlLs Branch Depth
- **D-06:** Full extraction. When outer loop hits `FrFX` with `VlLs` ostype, iterate each list item as a descriptor and extract `enab`, `Sz`, color components into `FStrokeInfo` — same fields the `Objc` branch populates. This lets Phase 22 consume VlLs-origin stroke data without re-opening PsdParser.cpp.
- **D-07:** VlLs list items are `Objc`-typed descriptors. Outer VlLs handler iterates item count, reads the ostype of each item, and dispatches to the same inner FrFX parse block already used for `Objc`. Research must confirm item structure from PSD spec before planning.

### LFXC-02 — Human UAT
- **D-08:** Non-blocking. RTXT-01, LFXC-01, FXFMT-01 constitute Phase 21 completion. LFXC-02 is logged as a standalone UAT task after code ships; it does NOT gate Phase 22 start.

### Claude's Discretion
- Whether to add a shared `ConvertLfx2Color(ColorSpace, C0, C1, C2, A) → FLinearColor` helper (used by both sofi and dsdw), or inline the dispatch in each block.
- Whether the CJK spec is a new `FPsdParserCJKSpec` file or appended to the existing rich-text spec in `FRichTextLayerMapper` tests.
- Plan count and split (researcher / planner decide based on dependency graph).

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Core parser file
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — `Utf8ToFString` (~line 75), multi-run span extraction (~lines 480-610), `sofi`/`dsdw` effect parsers (~lines 692-787), `ParseFrFXDescriptor` outer loop (~lines 965-1080)

### Type definitions
- `Source/PSD2UMG/Public/Parser/PsdTypes.h` — `FPsdLayerEffects` (stroke fields, ColorOverlay/DropShadow fields), `FPsdTextRunSpan`

### Rich-text mapper (downstream consumer of span fix)
- `Source/PSD2UMG/Private/Mapper/FRichTextLayerMapper.cpp` — consumes `OutLayer.Text.Spans`; confirms what a correct RawSpans array must look like

### Requirements + pitfalls (MANDATORY — contains critical offsets and format notes)
- `.planning/REQUIREMENTS.md` — RTXT-01, LFXC-01, LFXC-02, FXFMT-01 specs; CP-03 (VlLs branch), CP-05 (Utf8ToFString null-sentinel)
- `.planning/research/SUMMARY.md` — synthesized 2026-04-28; contains pitfall details and format notes for VlLs, vstk, lrFX ColorSpace

### Prior phase decisions (lrFX walker provenance)
- `.planning/phases/04.1-text-layer-effects-dispatch/04.1-CONTEXT.md` — D-07: 8-byte prefix confirmed empirically; VlLs already in SkipValueAfterOsType inner lambda (not outer loop — FXFMT-01 fixes the outer loop)

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `Utf8ToFString` helper at PsdParser.cpp:75 — unchanged; caller strips the sentinel before passing in
- `ParseFrFXDescriptor` inner `SkipValueAfterOsType` lambda — already handles `VlLs` for skip; outer loop (line 983) needs the new `VlLs` extraction branch alongside the existing `Objc` branch
- `FLinearColor::FromSRGBColor` already used in ARGB→RGBA span color path (line 532) — same import available for ColorSpace conversion

### Established Patterns
- Effect parser error handling: `try/catch(...)` → `OutDiag.AddWarning` → continue; no rethrow. ColorSpace conversion failure must follow same pattern.
- `VlLs` skip already present at PsdParser.cpp:943 inside `SkipValueAfterOsType` inner lambda — proof VlLs is a known ostype; outer extraction branch mirrors the inner skip structure.
- Fixture placement: `Source/PSD2UMG/Tests/Fixtures/` — existing files: `ButtonStyles.psd`, `RichText.psd`, etc.

### Integration Points
- `ExtractLfx2Effects(...)` at ~line 635: ColorSpace fix lives inside `sofi` and `dsdw` blocks within this function
- `ParseFrFXDescriptor` outer loop starts at line ~965; new VlLs branch goes alongside the existing `if (ItemKey == "FrFX" && strcmp(OsType, "Objc") == 0)` block at line 983

</code_context>

<specifics>
## Specific Notes

- `RichTextCJK.psd` provided by user before execution. Researcher does not need to generate a synthetic file.
- LFXC-02 UAT is intentionally out of automated test scope — visual comparison on a real host project only.
- CP-05 from REQUIREMENTS.md: the null-sentinel is a PhotoshopAPI quirk where `text()` may include a trailing `\0` in the returned `std::string`. Stripping at call site (not inside `Utf8ToFString`) keeps the helper generic.

</specifics>

<deferred>
## Deferred Ideas

- frameFXMulti VlLs stroke rendering: FXFMT-01 unlocks parsing; stroke emission for VlLs-origin data is Phase 22+ work.
- CMYK/Lab full lrFX color conversion: warn+identity path sufficient for v1.3; full conversion post-v1.3.
- `RichTextCJK.psd` multi-run emoji edge cases beyond null-sentinel: deferred to post-v1.3 if empirical tests expose gaps.

</deferred>

---

*Phase: 21-parser-correctness-fixes*
*Context gathered: 2026-04-28*

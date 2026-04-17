# Phase 4: Text, Fonts & Typography — Context

**Gathered:** 2026-04-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Make `UTextBlock` output match the designer's intended typography: correct font (via settings mapping), weight/style, color, outline, shadow, and multi-line wrap. Extends the basic text support already shipped in Phase 3.

**In scope:**
- Font mapping: Photoshop PostScript name → UE `UFont` asset via `UPSD2UMGSettings::FontMap` (field already exists)
- Default font fallback via `UPSD2UMGSettings::DefaultFont` (field already exists)
- Bold/italic applied via `TypefaceFontName` on `FSlateFontInfo`
- Text outline (stroke) via `FSlateFontInfo::OutlineSettings` — best-effort if PhotoshopAPI exposes it
- Text shadow via `UTextBlock::SetShadowOffset` + `SetShadowColorAndOpacity` — best-effort
- Multi-line wrap: enable `AutoWrapText` when the PSD layer is a paragraph (text box with explicit width), not a point text
- Parser extension: add fields to `FPsdTextRun` for weight/style flags, outline, shadow, text-box width (additive-only)

**Out of scope:**
- TEXT-01 (DPI × 0.75 conversion) — already shipped in Phase 3's `FTextLayerMapper`
- Multi-run text styling (per-character weight/color) — still deferred; Phase 4 continues the "first run flattens" pattern from Phase 2
- URichTextBlock support — v2 requirement (TEXT-V2-01)
- Letter spacing and line height — v2 requirement (TEXT-V2-02)
- Manual TySh PSD descriptor parsing — rejected (see D-12)
- Font file discovery / auto-registration — designers configure FontMap manually

</domain>

<decisions>
## Implementation Decisions

### Font Mapping Lookup (TEXT-05)
- **D-01:** Lookup strategy is **exact PostScript name match** against `FontMap` keys, with a **case-insensitive fallback** retry before falling through to `DefaultFont`. Example: parser extracts `"Arial-BoldMT"` → `FontMap.Find("Arial-BoldMT")` → if miss, iterate keys with `Equals(..., ESearchCase::IgnoreCase)` → if still miss, use `DefaultFont`.
- **D-02:** The lookup happens inside `FTextLayerMapper::Map` (or a helper `FFontResolver::Resolve`). Settings are read once per call via `UPSD2UMGSettings::Get()`.

### Font Fallback Chain (TEXT-05)
- **D-03:** Fallback chain when no mapping found:
  1. `FontMap` exact match
  2. `FontMap` case-insensitive match
  3. `DefaultFont` (if configured and non-null)
  4. Engine default font (the same `FSlateFontInfo` currently returned by `UTextBlock::GetFont()` before any modification)
- **D-04:** At step 4, emit `UE_LOG(LogPSD2UMG, Warning, TEXT("Font '%s' not found in FontMap and no DefaultFont configured; using engine default"), *PsFontName)`. Text widget is still created and readable; designer sees the gap in Output Log.
- **D-05:** Never abort the text layer on font resolution failure. D-08 from Phase 3 (skip + warn) does not apply to font resolution — the widget is always created.

### Bold/Italic Resolution (TEXT-02)
- **D-06:** Primary source: **PhotoshopAPI text style flags**. Add `bool bBold` and `bool bItalic` fields to `FPsdTextRun`. The researcher must verify which PhotoshopAPI call exposes these (likely `TextLayer::style()` or the TySh descriptor wrapper).
- **D-07:** Fallback when PhotoshopAPI does not expose flags (or returns false for a layer that is visually bold): **parse PostScript name suffix**. Suffix table: `-Bold`, `-BoldMT`, `-Italic`, `-It`, `-Oblique`, `-BoldItalic`, `-BoldItalicMT`. Applied after font lookup; the resolved `UFont` is queried for matching `TypefaceFontName` entries.
- **D-08:** Apply via `FSlateFontInfo.TypefaceFontName = FName("Bold")` (or `"Italic"`, `"Bold Italic"`, etc., matching the UE font asset's typeface names). If the resolved font doesn't have the requested typeface, log warning and fall through to the default typeface.

### Outline & Shadow (TEXT-03, TEXT-04)
- **D-09:** Outline (`FSlateFontInfo::OutlineSettings`) and shadow (`SetShadowOffset` + `SetShadowColorAndOpacity`) are **best-effort**. The researcher must check whether PhotoshopAPI v0.9 exposes text layer effect data (outline color/width, drop shadow offset/color).
- **D-10:** If PhotoshopAPI exposes the data: extract into new `FPsdTextRun` fields (`FLinearColor OutlineColor`, `float OutlineSize`, `FVector2D ShadowOffset`, `FLinearColor ShadowColor`) and apply in `FTextLayerMapper`.
- **D-11:** If PhotoshopAPI does not expose the data: TEXT-03 and TEXT-04 are delivered as "no-op — defaults remain in effect". The requirements are documented as partial delivery in `04-VERIFICATION.md` and a follow-up decimal phase (4.1) can close the gap later via TySh parsing or a future PhotoshopAPI release.
- **D-12:** **Manual TySh descriptor parsing is out of scope for Phase 4.** Too risky, too much work for a partial delivery. Revisit if/when gap closure is triggered.

### Multi-Line Wrap (TEXT-06)
- **D-13:** Enable `AutoWrapText = true` only when the PSD text layer is a **paragraph (text box with explicit width)**, not a point text. Point text layers keep `AutoWrapText = false` so short labels (button text, titles) don't wrap unexpectedly.
- **D-14:** Parser must distinguish the two cases. Add `bool bHasExplicitWidth` field to `FPsdTextRun` (alternative names acceptable — the researcher should check whether PhotoshopAPI exposes a "paragraph" vs "point" flag, or whether the text layer bounds indicate this). When `bHasExplicitWidth == true`, the mapper sets `AutoWrapText = true` and sizes the `UCanvasPanelSlot` to match the bounds width so wrapping has somewhere to wrap.

### Parser Extension Scope
- **D-15:** `FPsdTextRun` is extended additively in Phase 4. New fields (tentative):
  ```cpp
  struct FPsdTextRun
  {
      // Existing (Phase 2)
      FString Content;
      FString FontName;
      float SizePx = 0.f;
      FLinearColor Color = FLinearColor::White;
      TEnumAsByte<ETextJustify::Type> Alignment = ETextJustify::Left;
      // Phase 4 additions
      bool bBold = false;
      bool bItalic = false;
      bool bHasExplicitWidth = false;
      // Phase 4, best-effort (zero when not available)
      FLinearColor OutlineColor = FLinearColor::Transparent;
      float OutlineSize = 0.f;
      FVector2D ShadowOffset = FVector2D::ZeroVector;
      FLinearColor ShadowColor = FLinearColor::Transparent;
  };
  ```
- **D-16:** Struct shape is **advisory**, not locked. The planner may rename or restructure fields based on what PhotoshopAPI actually exposes. No existing consumers depend on the new fields (only `FTextLayerMapper` reads them, and it is rewritten in Phase 4).

### Relationship to Phase 3
- **D-17:** `FTextLayerMapper` (created in Phase 3) is rewritten in Phase 4 to use the new font resolver, apply weight/style, and handle outline/shadow/wrap. Phase 3's basic behavior (content + DPI size + color + alignment) remains — it's the baseline this phase builds on.
- **D-18:** TEXT-01 (DPI conversion) is already delivered in Phase 3 and should be marked satisfied in the Phase 4 plan without duplicating work. The `SizePx * 0.75f` line in `FTextLayerMapper` stays.

### Claude's Discretion
- Whether font resolution is a free function, static helper, or a class (`FFontResolver`)
- Exact name of the suffix-stripping helper and the suffix table location
- Whether to unit-test the font resolver in isolation or only via the end-to-end spec
- How to order the fields in the extended `FPsdTextRun` struct
- Whether `OutlineColor` / `ShadowColor` default to `Transparent` or `Black` with zero alpha

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project
- `CLAUDE.md` — UE 5.7.4 / C++20 / no Python / Editor-only / Win64 only
- `.planning/PROJECT.md` — Core value, requirements, evolution rules
- `.planning/REQUIREMENTS.md` — TEXT-01..06 acceptance criteria
- `.planning/ROADMAP.md` — Phase 4 goal and success criteria

### Phase 2 (parser foundation)
- `Source/PSD2UMG/Public/Parser/PsdTypes.h` — `FPsdTextRun`, `FPsdLayer`, `FPsdDocument` (Phase 4 extends `FPsdTextRun` additively)
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — text layer extraction entry point (new fields populated here)
- `Source/PSD2UMG/Private/Parser/PsdParserInternal.h` — PhotoshopAPI PIMPL boundary

### Phase 3 (mapper baseline)
- `Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp` — current basic text mapper (content + DPI + color + alignment). Phase 4 rewrites this.
- `.planning/phases/03-layer-mapping-widget-blueprint-generation/03-CONTEXT.md` — D-03 (Phase 3 text coverage), D-04 (font mapping deferred to Phase 4)
- `.planning/phases/03-layer-mapping-widget-blueprint-generation/03-04-SUMMARY.md` — verified state of Phase 3

### Settings
- `Source/PSD2UMG/Public/PSD2UMGSetting.h` — `FontMap: TMap<FString, TSoftObjectPtr<UFont>>` and `DefaultFont: TSoftObjectPtr<UFont>` already exist. No new settings fields needed.
- `Source/PSD2UMG/Private/PSD2UMGSetting.cpp` — settings defaults

### UE APIs (for research phase)
- `FSlateFontInfo` — `Size`, `FontObject`, `TypefaceFontName`, `OutlineSettings` fields
- `FFontOutlineSettings` — `OutlineSize`, `OutlineColor`
- `UTextBlock::SetFont`, `SetShadowOffset`, `SetShadowColorAndOpacity`, `SetAutoWrapText`
- `UFont::CompositeFont` / `FTypefaceEntry` — how to query available TypefaceFontNames

### Upstream
- PhotoshopAPI v0.9 text API — researcher must determine what bold/italic/outline/shadow fields are exposed (repo: https://github.com/EmilDohne/PhotoshopAPI, issue #126 about text layers)

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `UPSD2UMGSettings::FontMap` — already typed as `TMap<FString, TSoftObjectPtr<UFont>>`; just needs a lookup helper
- `UPSD2UMGSettings::DefaultFont` — already typed as `TSoftObjectPtr<UFont>`; used as fallback
- `FTextLayerMapper` — existing Phase 3 mapper provides the baseline content + size + color + alignment path; extended in Phase 4
- `FPsdTextRun` — Phase 2 POD struct; additively extended with new fields
- `LogPSD2UMG` — existing log category, used for all Phase 4 warnings

### Established Patterns
- PIMPL boundary: all PhotoshopAPI calls stay inside `Source/PSD2UMG/Private/Parser/PsdParserInternal.h`. Parser extension must follow this pattern — new fields populated inside the .cpp, never leaking PhotoshopAPI types into public headers.
- Diagnostic logging via `UE_LOG(LogPSD2UMG, Warning, ...)` for non-fatal issues (D-08 from Phase 3). Phase 4 continues this for font resolution warnings.
- `TSoftObjectPtr<UFont>::LoadSynchronous()` is the accepted pattern for loading UFont assets from soft refs in editor code.

### Integration Points
- `FTextLayerMapper::Map` — rewritten in Phase 4 to call font resolver, apply weight/style/outline/shadow/wrap
- `PsdParser.cpp` text layer branch — extended to populate new `FPsdTextRun` fields
- No changes needed to `FWidgetBlueprintGenerator`, `FLayerMappingRegistry`, or `UPsdImportFactory` — Phase 4 stays inside the existing mapper architecture

</code_context>

<specifics>
## Specific Ideas

- Phase 3 already set up `FSlateFontInfo` manipulation correctly (get → modify → set). Phase 4 extends this with `TypefaceFontName`, `OutlineSettings`, and the font object swap.
- `FFontOutlineSettings` needs both `OutlineSize` (in pixels) and `OutlineColor`. PhotoshopAPI's stroke effect uses points; if extracted, apply the same × 0.75 DPI conversion as font size.
- When setting a new `UFont` via `FontInfo.FontObject = ResolvedFont`, the previous `TypefaceFontName` may become invalid — reset it before applying the new weight.
- `TypefaceFontName` matching is case-sensitive and uses `FName`. Common UE font typeface names: `"Regular"`, `"Bold"`, `"Italic"`, `"Bold Italic"`, `"Black"`, `"Light"`. The resolver should check which names the resolved font actually has before assigning.

</specifics>

<deferred>
## Deferred Ideas

- **Manual TySh PSD descriptor parsing** — rejected for Phase 4, revisit in a gap-closure phase (4.1) if PhotoshopAPI proves insufficient
- **URichTextBlock / multi-run styling** — v2 requirement (TEXT-V2-01)
- **Letter spacing, line height** — v2 requirement (TEXT-V2-02)
- **Font file auto-discovery / Project font import** — designers maintain FontMap manually
- **Hard-error on missing font mapping** — rejected; always fall back to engine default with warning
- **Strip-suffix font lookup as primary strategy** — rejected; exact + case-insensitive is more predictable

</deferred>

---

*Phase: 04-text-fonts-typography*
*Context gathered: 2026-04-09 via /gsd:discuss-phase*

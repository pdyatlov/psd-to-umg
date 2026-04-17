# Phase 4: Text, Fonts & Typography — Research

**Researched:** 2026-04-09
**Domain:** PhotoshopAPI v0.9 text layer API + UE 5.7 Slate typography (FSlateFontInfo / UTextBlock)
**Confidence:** HIGH (primary unknown — PhotoshopAPI v0.9 text surface — resolved by direct inspection of vendored headers)

## Summary

The vendored PhotoshopAPI headers at `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LayerTypes/TextLayer/` expose a very rich text API — far more than Phase 3 used. Every style-run field needed for Phase 4 is reachable through the `TextLayerStyleRunMixin` (`style_run_faux_bold`, `style_run_faux_italic`, `style_run_stroke_flag`, `style_run_stroke_color`, `style_run_outline_width`, `style_run_underline`, `style_run_strikethrough`) and every shape/box field through `TextLayerShapeMixin` (`is_box_text`, `is_point_text`, `box_bounds`, `box_width`, `box_height`). Font PostScript name is already in use via `font_postscript_name(i)`.

**However:** drop-shadow data is a *layer effect* (stored in the `lfx2` tagged block), not a text style, and PhotoshopAPI v0.9's base `Layer.h` exposes **no layer-effect read API**. A project-wide grep found no `shadow`, `effect`, `lfx2`, or `DropShadow` accessor on any layer type. TEXT-04 must therefore ship as a **partial delivery** per D-11 of the CONTEXT: shadow fields plumb through `FPsdTextRun` but stay zero, and the mapper applies nothing. This is the correct, evidence-driven outcome and does not require TySh/lfx2 parsing (rejected by D-12).

**Primary recommendation:** Extend `FPsdTextRun` with the fields CONTEXT D-15 already sketches, plumb all of them from `TextLayerStyleRunMixin` / `TextLayerShapeMixin` inside `PsdParser.cpp`, and rewrite `FTextLayerMapper` to (1) resolve fonts via a new `FFontResolver` helper, (2) apply bold/italic via `FSlateFontInfo::TypefaceFontName`, (3) apply outline via `FSlateFontInfo::OutlineSettings`, (4) enable `AutoWrapText` only when `bHasExplicitWidth`. Ship TEXT-04 as partial (no-op) and document the gap.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Font mapping lookup (TEXT-05):**
- **D-01:** Exact PostScript name match against `FontMap`, then case-insensitive fallback, then `DefaultFont`.
- **D-02:** Lookup happens inside `FTextLayerMapper::Map` (or helper `FFontResolver::Resolve`). Settings read once per call via `UPSD2UMGSettings::Get()`.

**Font fallback chain (TEXT-05):**
- **D-03:** Chain = FontMap exact → FontMap case-insensitive → `DefaultFont` → engine default (whatever `UTextBlock::GetFont()` returns unmodified).
- **D-04:** At step 4, `UE_LOG(LogPSD2UMG, Warning, TEXT("Font '%s' not found in FontMap and no DefaultFont configured; using engine default"), *PsFontName)`.
- **D-05:** Never abort the text layer on font resolution failure. Phase 3's D-08 (skip + warn) does NOT apply.

**Bold/italic resolution (TEXT-02):**
- **D-06:** Primary source = PhotoshopAPI text style flags. Add `bBold` and `bItalic` to `FPsdTextRun`.
- **D-07:** Fallback when PhotoshopAPI doesn't expose flags (or returns false for a visually bold layer): parse PostScript name suffix. Suffixes: `-Bold`, `-BoldMT`, `-Italic`, `-It`, `-Oblique`, `-BoldItalic`, `-BoldItalicMT`.
- **D-08:** Apply via `FSlateFontInfo.TypefaceFontName = FName("Bold")` etc. If resolved font lacks the requested typeface, log warning and fall through to default typeface.

**Outline & shadow (TEXT-03, TEXT-04):**
- **D-09:** Best-effort. Researcher must confirm what PhotoshopAPI v0.9 exposes.
- **D-10:** If exposed: extract into `FLinearColor OutlineColor`, `float OutlineSize`, `FVector2D ShadowOffset`, `FLinearColor ShadowColor`.
- **D-11:** If NOT exposed: deliver as "no-op — defaults remain in effect"; document partial delivery in VERIFICATION.md; defer gap closure to a decimal phase (4.1).
- **D-12:** Manual TySh descriptor parsing is OUT OF SCOPE for Phase 4.

**Multi-line wrap (TEXT-06):**
- **D-13:** Enable `AutoWrapText = true` only for paragraph (box) text, not point text.
- **D-14:** Parser must distinguish the two cases. Add `bHasExplicitWidth` field to `FPsdTextRun`. When true, mapper sets `AutoWrapText = true` and sizes `UCanvasPanelSlot` to match bounds width.

**Parser extension scope:**
- **D-15:** `FPsdTextRun` extended additively with `bBold`, `bItalic`, `bHasExplicitWidth`, `OutlineColor`, `OutlineSize`, `ShadowOffset`, `ShadowColor`.
- **D-16:** Struct shape is advisory; planner may rename/restructure based on what PhotoshopAPI exposes.

**Relationship to Phase 3:**
- **D-17:** `FTextLayerMapper` is rewritten in Phase 4. Phase 3 baseline (content + DPI size + color + alignment) remains.
- **D-18:** TEXT-01 (DPI conversion) already delivered in Phase 3; mark satisfied without duplicating work. The `* 0.75f` line stays.

### Claude's Discretion
- Whether font resolution is free function / static helper / `FFontResolver` class
- Name of suffix-stripping helper and suffix table location
- Whether to unit-test font resolver in isolation or only via end-to-end spec
- Field ordering in extended `FPsdTextRun`
- Whether `OutlineColor` / `ShadowColor` default to `FLinearColor::Transparent` or `FLinearColor(0,0,0,0)`

### Deferred Ideas (OUT OF SCOPE)
- Manual TySh PSD descriptor parsing (revisit in gap-closure 4.1 if needed)
- URichTextBlock / multi-run styling (TEXT-V2-01)
- Letter spacing, line height (TEXT-V2-02)
- Font file auto-discovery / Project font import
- Hard-error on missing font mapping
- Strip-suffix font lookup as *primary* strategy (rejected; exact + case-insensitive is primary)
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| TEXT-01 | DPI conversion (PS pt × 0.75 → UMG) | ✅ Already delivered in Phase 3 `FTextLayerMapper.cpp` line `Layer.Text.SizePx * 0.75f`. Phase 4 marks satisfied; no new work. |
| TEXT-02 | Bold/italic via `TypefaceFontName` | `TextLayerStyleRunMixin::style_run_faux_bold(i)` and `style_run_faux_italic(i)` both return `std::optional<bool>`. Parser plumbs to `FPsdTextRun::bBold/bItalic`. Mapper applies via `FSlateFontInfo::TypefaceFontName = "Bold"`/`"Italic"`/`"Bold Italic"`. Suffix fallback (D-07) covers fonts whose bold is baked into the PostScript name rather than a faux flag. |
| TEXT-03 | Text outline (stroke) via `OutlineSettings` | `style_run_stroke_flag(i)` + `style_run_stroke_color(i)` (returns `std::vector<double>`, 4 components, **same ARGB quirk as fill per Phase 2 P05 decision log**) + `style_run_outline_width(i)` (returns points, apply × 0.75 DPI conversion). Mapper sets `FSlateFontInfo::OutlineSettings` only when `bStrokeEnabled && OutlineWidth > 0`. |
| TEXT-04 | Text shadow via `SetShadowOffset` + `SetShadowColorAndOpacity` | ❌ **NOT EXPOSED**. PhotoshopAPI v0.9 has no layer-effect / `lfx2` / drop-shadow reader. Base `Layer.h` grep returns no shadow/effect API. Ship as **partial delivery**: `FPsdTextRun::ShadowOffset/ShadowColor` fields exist but stay zero; mapper skips the setter; `04-VERIFICATION.md` records the gap; a future phase 4.1 can close via TySh/lfx2 parsing or a PhotoshopAPI upgrade. D-11 explicitly anticipates this branch. |
| TEXT-05 | Font mapping system | `UPSD2UMGSettings::FontMap` and `DefaultFont` already exist. New `FFontResolver` helper implements the D-03 chain. `TSoftObjectPtr<UFont>::LoadSynchronous()` is the accepted load path in editor code. |
| TEXT-06 | Multi-line `AutoWrapText` when text box width defined | `TextLayerShapeMixin::is_box_text()` / `is_point_text()` / `box_width()` (returns `std::optional<double>`). Parser sets `bHasExplicitWidth = Text->is_box_text()`. Mapper enables `AutoWrapText` only when true, and sizes the `UCanvasPanelSlot` to the layer bounds width. |
</phase_requirements>

## Standard Stack

### Core (already vendored / present)
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| PhotoshopAPI | v0.9.x (vendored) | Text layer extraction | Already integrated in Phase 2; no upgrade needed. Rich style-run mixin covers all Phase 4 fields except layer effects. |
| UE 5.7 Slate (`FSlateFontInfo`, `FFontOutlineSettings`) | 5.7.4 | UMG font rendering | Canonical UE path; already used by Phase 3 for basic font size. |
| UE 5.7 UMG (`UTextBlock`, `UCanvasPanelSlot`) | 5.7.4 | Widget binding | Phase 3 baseline. |

**No new third-party libraries.** Phase 4 is pure C++ additions on top of the existing stack.

**Installation:** N/A — all dependencies already present.

### PhotoshopAPI Text API — Verified Surface

From direct inspection of `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LayerTypes/TextLayer/`:

**`TextLayerFontMixin.h` (already partially used in Phase 2):**
- `font_postscript_name(size_t font_index) → std::optional<std::string>` — PostScript name (e.g. `"Arial-BoldMT"`) ✅
- `font_name(i)` — alias for `font_postscript_name`
- `primary_font_name() → std::optional<std::string>` — convenience for "the first font used"
- `used_font_names() → std::vector<std::string>` — all fonts in the layer
- `set_font_postscript_name(i, name)` — write-side (not needed this phase)

**`TextLayerStyleRunMixin.h` (Phase 4 expands usage):**

| PhotoshopAPI call | Returns | Phase 4 use |
|---|---|---|
| `style_run_count()` | `size_t` | Diagnostic; Phase 4 still flattens to run 0 |
| `style_run_font_size(i)` | `optional<double>` (points) | Already used — Phase 3 baseline |
| `style_run_font(i)` | `optional<int32_t>` (font index into layer's font list) | Resolve then look up via `font_postscript_name(returned_index)` if we later want per-run fonts; Phase 4 sticks with run 0 only |
| `style_run_faux_bold(i)` | `optional<bool>` | → `FPsdTextRun::bBold` (primary per D-06) |
| `style_run_faux_italic(i)` | `optional<bool>` | → `FPsdTextRun::bItalic` (primary per D-06) |
| `style_run_fill_color(i)` | `optional<vector<double>>` (ARGB!) | Already used — Phase 3 baseline; **ARGB quirk** logged in STATE.md |
| `style_run_fill_flag(i)` | `optional<bool>` | Diagnostic — true when fill is enabled |
| `style_run_stroke_flag(i)` | `optional<bool>` | → gate for TEXT-03 outline application |
| `style_run_stroke_color(i)` | `optional<vector<double>>` | → `FPsdTextRun::OutlineColor` **ASSUME ARGB ORDER** (same fill-color quirk applies; verify empirically during implementation) |
| `style_run_outline_width(i)` | `optional<double>` (points) | → `FPsdTextRun::OutlineSize` (apply × 0.75 DPI conversion to get UMG pixels) |
| `style_run_underline(i)` | `optional<bool>` | Ignore in Phase 4 (out of scope); noted for reference |
| `style_run_strikethrough(i)` | `optional<bool>` | Ignore in Phase 4 (out of scope) |
| `style_run_leading(i)`, `style_run_tracking(i)`, `style_run_kerning(i)` | Various | Deferred — TEXT-V2-02 |

**`TextLayerShapeMixin.h` (new in Phase 4):**

| PhotoshopAPI call | Returns | Phase 4 use |
|---|---|---|
| `shape_type()` | `optional<TextLayerEnum::ShapeType>` (0 = PointText, 1 = BoxText) | Primary signal |
| `is_point_text()` | `bool` | Diagnostic |
| `is_box_text()` | `bool` | **Primary: sets `FPsdTextRun::bHasExplicitWidth`** |
| `box_bounds()` | `optional<array<double, 4>>` `{top, left, bottom, right}` (API order; on-disk is top/left/right/bottom) | Available if we want exact box rect |
| `box_width()` | `optional<double>` | → apply × 0.75 DPI? **NO — box_width is in pixels already** (text-space ≈ canvas pixels; confirm against fixture). Planner should cross-check against an actual fixture during implementation. |
| `box_height()` | `optional<double>` | Same note |

**`TextLayerParagraphRunMixin.h` (already used):**
- `paragraph_run_justification(i)` — already used in Phase 2

**What is NOT exposed anywhere in `LayerTypes/`:**
- Drop shadow (layer effect — `lfx2` block): **no accessor**. Base `Layer.h` grep for `shadow|effect|DropShadow` returned only unrelated comments.
- Inner shadow, glow, bevel, gradient overlay, color overlay: **no accessor** on any layer type.
- Character-level font weight names (only faux-bold / faux-italic flags; real weight comes from picking a different PostScript name).

This confirms D-06 (use faux_bold/faux_italic as primary), D-07 (suffix fallback is necessary because real-bold fonts report `faux_bold == false`), D-11 (shadow ships as no-op), and D-12 (no TySh parsing).

### UE 5.7 Typography APIs

| API | Signature | Phase 4 use |
|---|---|---|
| `FSlateFontInfo::FontObject` | `TObjectPtr<const UObject>` | Assign to resolved `UFont*` |
| `FSlateFontInfo::TypefaceFontName` | `FName` | Assign `"Bold"` / `"Italic"` / `"Bold Italic"` / `"Regular"` after checking `UFont::CompositeFont::DefaultTypeface.Fonts` |
| `FSlateFontInfo::Size` | `float` | Phase 3 already sets this |
| `FSlateFontInfo::OutlineSettings` | `FFontOutlineSettings` | Set `OutlineSize` (int32, pixels) and `OutlineColor` (FLinearColor) |
| `FFontOutlineSettings::OutlineSize` | `int32` (pixels) | Outline size rounded from `PSOutlineWidth * 0.75f` |
| `FFontOutlineSettings::OutlineColor` | `FLinearColor` | From stroke color |
| `UTextBlock::SetFont(FSlateFontInfo)` | Setter | Primary apply path |
| `UTextBlock::SetAutoWrapText(bool)` | Setter | Phase 4 new |
| `UTextBlock::SetShadowOffset(FVector2D)` | Setter | **Skipped** (TEXT-04 partial) |
| `UTextBlock::SetShadowColorAndOpacity(FLinearColor)` | Setter | **Skipped** (TEXT-04 partial) |
| `UFont::CompositeFont::DefaultTypeface` | `FCompositeFont::DefaultTypeface: FTypeface` with `.Fonts: TArray<FTypefaceEntry>` where each entry has a `FName Name` | Iterate to check if requested typeface exists |
| `TSoftObjectPtr<UFont>::LoadSynchronous()` | Returns `UFont*` | Safe in editor code during `FactoryCreateBinary`; Phase 3 already uses this pattern elsewhere |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|---|---|---|
| `FSlateFontInfo::OutlineSettings` | Material outline via `FFontOutlineSettings::OutlineMaterial` | Overkill for Phase 4 — color-only outline is the Photoshop semantic; materials are a v2 polish concern |
| `TSoftObjectPtr<UFont>::LoadSynchronous()` | `LoadObject<UFont>(nullptr, *Path)` | Both work in editor code; soft pointer is the project-consistent pattern |
| `FauxBold/FauxItalic` as primary | PostScript suffix as primary | Rejected per D-06: flags are explicit designer intent; suffix is a fallback for real-bold fonts |
| Parse `lfx2` tagged block manually for drop shadow | Rely on future PhotoshopAPI release | D-12 rejects manual parsing; defer to 4.1 |

## Architecture Patterns

### Recommended File Layout

```
Source/PSD2UMG/
├── Public/Parser/PsdTypes.h         # FPsdTextRun additively extended (7 new fields)
├── Private/Parser/PsdParser.cpp     # ExtractSingleRunText extended to populate new fields
├── Private/Mapper/
│   ├── AllMappers.h                 # Unchanged (FTextLayerMapper already declared)
│   ├── FTextLayerMapper.cpp         # REWRITTEN: uses FFontResolver, applies weight/outline/wrap
│   └── FontResolver.h/.cpp          # NEW: FFontResolver::Resolve + suffix stripping helpers
```

The `FFontResolver` split is suggested (not locked — D discretion) because it (a) unit-tests cleanly without UE boot and (b) keeps `FTextLayerMapper.cpp` focused on widget assembly.

### Pattern 1: Parser Extension Behind PIMPL
**What:** New `FPsdTextRun` fields are populated inside `PsdParser.cpp`'s `ExtractSingleRunText` by calling the appropriate mixin methods directly on the `std::shared_ptr<TextLayer<PsdPixelType>>`.
**When to use:** Any time we pull additional PhotoshopAPI data into the UE-side POD.
**Example:**
```cpp
// Source: vendored TextLayerStyleRunMixin.h, TextLayerShapeMixin.h
// Inside ExtractSingleRunText(Text, OutLayer, OutDiag) in PsdParser.cpp
if (auto FauxBold = Text->style_run_faux_bold(0); FauxBold.has_value())
{
    OutLayer.Text.bBold = *FauxBold;
}
if (auto FauxItalic = Text->style_run_faux_italic(0); FauxItalic.has_value())
{
    OutLayer.Text.bItalic = *FauxItalic;
}
if (auto StrokeFlag = Text->style_run_stroke_flag(0); StrokeFlag.value_or(false))
{
    if (auto StrokeWidth = Text->style_run_outline_width(0); StrokeWidth.has_value())
    {
        OutLayer.Text.OutlineSize = static_cast<float>(*StrokeWidth); // points; mapper converts
    }
    if (auto StrokeColor = Text->style_run_stroke_color(0);
        StrokeColor.has_value() && StrokeColor->size() >= 4)
    {
        // ARGB order — same quirk as fill_color (see Phase 2 P05 decision log in STATE.md)
        const double A = (*StrokeColor)[0];
        const double Rd = (*StrokeColor)[1];
        const double Gd = (*StrokeColor)[2];
        const double Bd = (*StrokeColor)[3];
        OutLayer.Text.OutlineColor = FLinearColor::FromSRGBColor(FColor(
            static_cast<uint8>(FMath::Clamp(Rd, 0.0, 1.0) * 255.0),
            static_cast<uint8>(FMath::Clamp(Gd, 0.0, 1.0) * 255.0),
            static_cast<uint8>(FMath::Clamp(Bd, 0.0, 1.0) * 255.0),
            static_cast<uint8>(FMath::Clamp(A,  0.0, 1.0) * 255.0)));
    }
}
OutLayer.Text.bHasExplicitWidth = Text->is_box_text();
```

### Pattern 2: Font Resolver Fallback Chain

```cpp
// Inside FFontResolver::Resolve (new helper)
UFont* FFontResolver::Resolve(const FString& PsFontName, const UPSD2UMGSettings& Settings, FName& OutTypefaceName)
{
    OutTypefaceName = NAME_None;

    // 1. Exact match
    if (const TSoftObjectPtr<UFont>* Hit = Settings.FontMap.Find(PsFontName))
    {
        return Hit->LoadSynchronous();
    }
    // 2. Case-insensitive match
    for (const auto& Kvp : Settings.FontMap)
    {
        if (Kvp.Key.Equals(PsFontName, ESearchCase::IgnoreCase))
        {
            return Kvp.Value.LoadSynchronous();
        }
    }
    // 3. DefaultFont
    if (!Settings.DefaultFont.IsNull())
    {
        if (UFont* Fallback = Settings.DefaultFont.LoadSynchronous())
        {
            return Fallback;
        }
    }
    // 4. Engine default (null — caller leaves UTextBlock::GetFont() untouched)
    UE_LOG(LogPSD2UMG, Warning,
        TEXT("Font '%s' not found in FontMap and no DefaultFont configured; using engine default"),
        *PsFontName);
    return nullptr;
}
```

### Pattern 3: Typeface Selection

```cpp
// After resolving UFont*, pick Bold/Italic typeface from its CompositeFont
static FName PickTypeface(const UFont* Font, bool bBold, bool bItalic)
{
    if (!Font || !Font->CompositeFont) return NAME_None;
    const FName Desired = bBold && bItalic ? FName("Bold Italic")
                        : bBold            ? FName("Bold")
                        : bItalic          ? FName("Italic")
                        :                    FName("Regular");
    for (const FTypefaceEntry& Entry : Font->CompositeFont->DefaultTypeface.Fonts)
    {
        if (Entry.Name == Desired) return Desired;
    }
    // Log + fall through
    UE_LOG(LogPSD2UMG, Warning,
        TEXT("Font '%s' has no typeface '%s'; using Regular/default"),
        *Font->GetName(), *Desired.ToString());
    return NAME_None;
}
```

### Pattern 4: Suffix Fallback for Real-Bold Fonts

```cpp
// When faux_bold/faux_italic are both false but the PostScript name says otherwise.
// Example: "Arial-BoldMT" → bBold=true at parse or at mapper discretion.
static void InferBoldItalicFromSuffix(const FString& PsName, bool& bBold, bool& bItalic)
{
    static const TArray<TPair<const TCHAR*, TPair<bool,bool>>> Suffixes = {
        { TEXT("-BoldItalicMT"),  {true,  true}  },
        { TEXT("-BoldItalic"),    {true,  true}  },
        { TEXT("-BoldMT"),        {true,  false} },
        { TEXT("-Bold"),          {true,  false} },
        { TEXT("-Italic"),        {false, true}  },
        { TEXT("-It"),            {false, true}  },
        { TEXT("-Oblique"),       {false, true}  },
    };
    for (const auto& Pair : Suffixes)
    {
        if (PsName.EndsWith(Pair.Key))
        {
            bBold = bBold || Pair.Value.Key;
            bItalic = bItalic || Pair.Value.Value;
            return;
        }
    }
}
```

Ordering matters: longest-first (`-BoldItalicMT` before `-BoldMT` before `-Bold`), mirrors the FAnchorCalculator suffix-table pattern established in Phase 3.

### Pattern 5: AutoWrapText + CanvasPanelSlot Sizing (TEXT-06)

```cpp
// Inside FTextLayerMapper::Map after widget construction
if (Layer.Text.bHasExplicitWidth)
{
    TextWidget->SetAutoWrapText(true);
    // The UCanvasPanelSlot is set up by the generator, not the mapper.
    // Phase 3's FWidgetBlueprintGenerator already sizes the slot from Layer.Bounds.
    // Width from Layer.Bounds.Width() already constrains the wrap width — no extra work needed
    // PROVIDED the generator honours bounds width (verify during implementation against fixture).
}
else
{
    TextWidget->SetAutoWrapText(false); // explicit for clarity even though false is default
}
```

**IMPORTANT nuance (answers research Q6):** `UTextBlock` inside a `UCanvasPanelSlot` *does* honour the slot's explicit size for wrap width when `AutoWrapText == true`. The mapper does not need to add a `USizeBox`. Phase 3's generator already sets `CanvasPanelSlot->SetSize(FVector2D(Bounds.Width(), Bounds.Height()))`. Verify against the Phase 4 fixture — if wrap width is wrong, add a `USizeBox` wrap or set `WrapTextAt` explicitly as a remediation.

### Anti-Patterns to Avoid

- **Don't set `FontInfo.FontObject` and then reuse the old `TypefaceFontName`** — the old name may not exist on the new font. Reset `TypefaceFontName = NAME_None` before choosing a new one.
- **Don't assume PhotoshopAPI stroke color is RGBA.** Phase 2 P05 proved `fill_color` is ARGB. Assume the same for `stroke_color` until empirically disproven. Log the raw values on the first debug run against the fixture.
- **Don't multiply `box_width()` by 0.75.** Box bounds are in canvas-pixel-adjacent text space, not points. Font *size* is in points (× 0.75); box width is not.
- **Don't hand-roll lfx2 parsing for shadow.** D-12 rejects this.
- **Don't abort the text layer on font resolution failure.** D-05.
- **Don't apply the suffix fallback before the PhotoshopAPI flag check** — flags are primary per D-06; suffix only fills the gap when flags are both false.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---|---|---|---|
| Drop-shadow effect extraction | Manual `lfx2` tagged-block parser | Wait for PhotoshopAPI upgrade or ship as no-op | D-12; huge effort for partial delivery; high risk of corrupting descriptor data |
| Font file discovery | Scan `C:\Windows\Fonts` and auto-register | `FontMap` manual config | Designers maintain the map; auto-discovery is out of scope per CONTEXT deferred |
| Typeface name enumeration | Custom FCompositeFont walker | `Font->CompositeFont->DefaultTypeface.Fonts` array | UE engine type already provides it |
| DPI conversion table | Custom pt→px math | `* 0.75f` (already in Phase 3) | Phase 3 shipped this; D-18 says don't duplicate |
| TextBox vs PointText detection | Parse raw EngineData | `TextLayerShapeMixin::is_box_text()` | Mixin exists and is exactly what we need |

**Key insight:** Phase 4 is 90% plumbing. Every field we need is already exposed by PhotoshopAPI mixins (except shadow, which is correctly deferred). The only new code is the font resolver + suffix helper + mapper rewrite.

## Runtime State Inventory

*Not applicable — Phase 4 is additive code-only; no renames, no data migrations, no runtime state to update. No stored data / live service config / OS-registered state / secrets / build artifacts are touched.*

## Common Pitfalls

### Pitfall 1: Stroke color ARGB quirk (HIGH confidence — extrapolated from verified fill_color quirk)
**What goes wrong:** Implementing `stroke_color` as RGBA produces wrong-channel outlines (red becomes alpha, blue becomes green, etc.).
**Why it happens:** PhotoshopAPI text color storage is ARGB, not RGBA. Phase 2 P05 verified this empirically for `fill_color`; Phase 4 must re-verify for `stroke_color` using the same methodology.
**How to avoid:** First implementation pass logs raw `style_run_stroke_color(0)` values against a pure-red stroke fixture. Expected: `[1.0, 1.0, 0.0, 0.0]` if ARGB; `[1.0, 0.0, 0.0, 1.0]` if RGBA.
**Warning signs:** Stroke renders as transparent or wrong color.

### Pitfall 2: Faux-bold is false on real-bold fonts
**What goes wrong:** Designer uses "Arial Bold" (real bold font, not faux-bolded Arial Regular); `style_run_faux_bold(0)` returns `false`; mapper applies Regular typeface; text renders thin.
**Why it happens:** FauxBold/FauxItalic flags only fire when Photoshop is synthesizing boldness from a non-bold font. Real weight is encoded in the font selection itself.
**How to avoid:** D-07 suffix fallback. Run after the flag check, only when flags are false.
**Warning signs:** Bold-named text renders at Regular weight in the Widget Blueprint preview.

### Pitfall 3: Typeface name mismatch on resolved UFont
**What goes wrong:** Mapper assigns `TypefaceFontName = FName("Bold")` but the resolved `UFont` asset only has entries named `"Bold MT"` or `"Black"`. Slate silently falls back to Regular but no warning is emitted.
**Why it happens:** `TypefaceFontName` is an `FName` — no fuzzy matching. Designers' custom UFont assets may use any naming convention.
**How to avoid:** Before assigning, iterate `Font->CompositeFont->DefaultTypeface.Fonts` and verify a matching name exists. Log warning via D-04 pattern if not. See Pattern 3.
**Warning signs:** Bold-marked text renders at Regular weight, no log output.

### Pitfall 4: Box-text bounds × DPI double-conversion
**What goes wrong:** Someone adds `* 0.75f` to `box_width()` by analogy with font size; wrap width becomes 75% of intended.
**Why it happens:** Font size is in points (needs DPI), but box bounds are in canvas pixels (already match UMG space).
**How to avoid:** Strict rule — only `SizePx` and `OutlineSize` get the `× 0.75f` treatment. Everything else from PhotoshopAPI is already pixels.
**Warning signs:** Paragraph text wraps noticeably earlier than the visible PSD layer bounds.

### Pitfall 5: `TSoftObjectPtr::LoadSynchronous()` returning null silently
**What goes wrong:** `FontMap` entry references a UFont asset that was renamed/deleted; `LoadSynchronous()` returns nullptr; mapper proceeds with null font and the `UTextBlock::SetFont` call with null FontObject reverts to engine default without warning.
**Why it happens:** Soft pointers don't throw on broken references.
**How to avoid:** Check `ResolvedFont != nullptr` before assigning; fall through to the next tier of D-03 chain if null.
**Warning signs:** Specific fonts silently render as default Roboto.

### Pitfall 6: AutoWrapText on point text swallowing short labels
**What goes wrong:** Setting `AutoWrapText = true` unconditionally causes single-word button labels to wrap character-by-character when the canvas slot is narrower than expected.
**Why it happens:** Point text in Photoshop has no width constraint; applying wrap forces UMG to use the slot width.
**How to avoid:** D-13 gating. Enable wrap ONLY when `bHasExplicitWidth == true` (i.e. `is_box_text()`).
**Warning signs:** Button labels in imported PSDs render vertically one letter per line.

## Code Examples

### Canonical font application (answers research Q2 + Q4)

```cpp
// Source: UE 5.7 Slate + inspection of vendored headers
// Inside FTextLayerMapper::Map, rewritten for Phase 4
UWidget* FTextLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument&, UWidgetTree* Tree)
{
    UTextBlock* TextWidget = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*Layer.Name));
    TextWidget->SetText(FText::FromString(Layer.Text.Content));

    // Phase 3 baseline (unchanged - TEXT-01 already delivered)
    const float UmgSize = Layer.Text.SizePx * 0.75f;
    FSlateFontInfo FontInfo = TextWidget->GetFont();
    FontInfo.Size = FMath::RoundToInt(UmgSize);

    // Phase 4: font resolution (TEXT-05)
    const UPSD2UMGSettings& Settings = *UPSD2UMGSettings::Get();
    FName TypefaceName;
    UFont* ResolvedFont = FFontResolver::Resolve(Layer.Text.FontName, Settings, TypefaceName);
    if (ResolvedFont)
    {
        FontInfo.FontObject = ResolvedFont;
        FontInfo.TypefaceFontName = NAME_None; // reset — may be stale from engine default

        // Phase 4: bold/italic (TEXT-02) — flags primary, suffix fallback
        bool bBold = Layer.Text.bBold;
        bool bItalic = Layer.Text.bItalic;
        if (!bBold && !bItalic)
        {
            InferBoldItalicFromSuffix(Layer.Text.FontName, bBold, bItalic);
        }
        const FName Typeface = PickTypeface(ResolvedFont, bBold, bItalic);
        if (!Typeface.IsNone())
        {
            FontInfo.TypefaceFontName = Typeface;
        }
    }

    // Phase 4: outline (TEXT-03) — best effort, real API, with ARGB caveat
    if (Layer.Text.OutlineSize > 0.f)
    {
        FontInfo.OutlineSettings.OutlineSize = FMath::RoundToInt(Layer.Text.OutlineSize * 0.75f);
        FontInfo.OutlineSettings.OutlineColor = Layer.Text.OutlineColor;
    }

    TextWidget->SetFont(FontInfo);
    TextWidget->SetColorAndOpacity(FSlateColor(Layer.Text.Color));
    TextWidget->SetJustification(Layer.Text.Alignment);

    // Phase 4: wrap (TEXT-06)
    TextWidget->SetAutoWrapText(Layer.Text.bHasExplicitWidth);

    // Phase 4: shadow (TEXT-04) — PARTIAL DELIVERY, no-op per D-11
    // PhotoshopAPI v0.9 exposes no drop-shadow accessor. Fields in FPsdTextRun remain zero.
    // Gap closure deferred to a follow-up phase 4.1 if/when PhotoshopAPI adds lfx2 support.

    return TextWidget;
}
```

### Parser extension (answers research Q1 in code)

```cpp
// Source: vendored TextLayerStyleRunMixin.h + TextLayerShapeMixin.h
// Inside ExtractSingleRunText in PsdParser.cpp
try { if (auto V = Text->style_run_faux_bold(0);   V.has_value()) OutLayer.Text.bBold   = *V; } catch(...) {}
try { if (auto V = Text->style_run_faux_italic(0); V.has_value()) OutLayer.Text.bItalic = *V; } catch(...) {}

try
{
    if (auto Flag = Text->style_run_stroke_flag(0); Flag.value_or(false))
    {
        if (auto Width = Text->style_run_outline_width(0); Width.has_value())
        {
            OutLayer.Text.OutlineSize = static_cast<float>(*Width); // points — mapper applies × 0.75
        }
        if (auto Color = Text->style_run_stroke_color(0); Color.has_value() && Color->size() >= 4)
        {
            // ARGB by analogy with fill_color (Phase 2 P05). Verify empirically.
            OutLayer.Text.OutlineColor = FLinearColor::FromSRGBColor(FColor(
                static_cast<uint8>(FMath::Clamp((*Color)[1], 0.0, 1.0) * 255.0),
                static_cast<uint8>(FMath::Clamp((*Color)[2], 0.0, 1.0) * 255.0),
                static_cast<uint8>(FMath::Clamp((*Color)[3], 0.0, 1.0) * 255.0),
                static_cast<uint8>(FMath::Clamp((*Color)[0], 0.0, 1.0) * 255.0)));
        }
    }
} catch(...) { OutDiag.AddWarning(OutLayer.Name, TEXT("Stroke extraction threw; using defaults.")); }

try { OutLayer.Text.bHasExplicitWidth = Text->is_box_text(); } catch(...) {}
```

Note: the existing `ExtractSingleRunText` already has a single catch-all at the bottom; individual try/catch blocks shown above are illustrative. The planner may consolidate under the existing outer try.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|---|---|---|---|
| PSD text parsing via psd_tools (Python) | PhotoshopAPI v0.9 C++ API | Phase 2 (2026-04-08) | Native, Python-free; Phase 4 exploits the richer API |
| Set font via `UFont` UPROPERTY in Blueprint | `FSlateFontInfo::FontObject` set from code | UE 4.15+ | Runtime / editor-code path is fine; UTextBlock::SetFont is canonical |
| Manually parse TySh descriptor for text props | PhotoshopAPI `TextLayerStyleRunMixin` + `TextLayerShapeMixin` | PhotoshopAPI v0.9 (early 2026) | Massively reduces Phase 4 risk; makes D-12 correct |

**Deprecated/outdated:**
- UE4 `FEditorStyle` text styles → UE5 `FAppStyle` (already handled in Phase 1)
- `UTextBlock::bEnableShadow` boolean → shadow is now offset-based; there is no separate enable flag. `SetShadowColorAndOpacity(FLinearColor(0,0,0,0))` effectively disables it.

## Open Questions

1. **Is PhotoshopAPI `style_run_stroke_color` really ARGB?**
   - What we know: `style_run_fill_color` is ARGB (Phase 2 P05 empirical verification).
   - What's unclear: Stroke color uses the same engine-data path, so almost certainly the same order, but not yet verified.
   - Recommendation: First implementation task logs raw stroke color values against a red-stroke fixture. Adjust channel mapping if disproven.

2. **Does `UCanvasPanelSlot` width correctly constrain `AutoWrapText` wrap width?**
   - What we know: Phase 3 generator already sizes canvas slots to bounds.
   - What's unclear: Whether `UTextBlock` inside that slot picks up the width automatically, or whether we need a `USizeBox` wrapper / explicit `WrapTextAt`.
   - Recommendation: Ship without SizeBox. If fixture verification shows wrong wrap width, add a remediation task (wrap in SizeBox OR call `SetWrapTextAt(Bounds.Width())`).

3. **What typeface names do Epic's shipped UFonts actually use?**
   - What we know: UE default fonts (`Roboto`, `DroidSans`) use `"Regular"`, `"Bold"`, `"Italic"`, `"Bold Italic"`, `"Black"`, `"Light"`.
   - What's unclear: Designers importing Google Fonts may create UFont assets with different conventions (`"Medium"`, `"SemiBold"`, `"ExtraBold"`).
   - Recommendation: Document supported typeface names in CONVENTIONS.md (Phase 8) and log a clear warning on mismatch. Don't auto-map.

## Environment Availability

*Skip — Phase 4 has no new external dependencies. PhotoshopAPI and UE 5.7.4 toolchain already present from Phases 1-3.*

## Validation Architecture

### Test Framework
| Property | Value |
|---|---|
| Framework | UE Automation Spec (`FAutomationTestBase` / `DEFINE_SPEC`) — already used in Phase 2 (`FPsdParserSpec`) and Phase 3 (mapping spec) |
| Config file | None — UE automation specs are declared inline via macros in `.cpp` files |
| Quick run command | `"<UE>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<ProjectPath>" -ExecCmds="Automation RunTests PSD2UMG.Text" -unattended -nopause -nullrhi` |
| Full suite command | `"<UE>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<ProjectPath>" -ExecCmds="Automation RunTests PSD2UMG" -unattended -nopause -nullrhi` |

Pure-C++ unit tests (no UE boot) are not yet set up in the project (TEST-01 is Phase 8). For Phase 4, the font resolver's suffix-stripping helper can still be tested as a UE automation spec (`PSD2UMG.Text.FontResolver`) — it doesn't require UE types beyond `FString`.

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|---|---|---|---|---|
| TEXT-01 | DPI × 0.75 conversion | regression — already covered by existing Phase 3 behavior | `Automation RunTests PSD2UMG.Mapping.Text` | ✅ existing Phase 3 spec |
| TEXT-02 | Bold/italic TypefaceFontName applied | integration (spec) | `Automation RunTests PSD2UMG.Text.BoldItalic` | ❌ Wave 0 |
| TEXT-02 (fallback) | Suffix parser: `-BoldMT` → bBold=true | unit (spec, no UE boot needed logically but runs as spec) | `Automation RunTests PSD2UMG.Text.FontResolver.Suffix` | ❌ Wave 0 |
| TEXT-02 (font resolver) | Exact / case-insensitive / default chain | unit (spec with mock settings) | `Automation RunTests PSD2UMG.Text.FontResolver.Chain` | ❌ Wave 0 |
| TEXT-03 | Outline applied via `FSlateFontInfo::OutlineSettings` when stroke enabled | integration (spec) | `Automation RunTests PSD2UMG.Text.Outline` | ❌ Wave 0 |
| TEXT-04 | Shadow — PARTIAL delivery, no-op | manual only + documented gap | verify VERIFICATION.md lists partial-delivery status | ❌ Wave 0 (documentation) |
| TEXT-05 | Font mapping system resolves PostScript name to `UFont` | integration (spec) — set up test settings, call resolver | `Automation RunTests PSD2UMG.Text.FontResolver.Chain` | ❌ Wave 0 |
| TEXT-06 | Paragraph text → AutoWrapText=true; point text → false | integration (spec) | `Automation RunTests PSD2UMG.Text.Wrap` | ❌ Wave 0 |
| TEXT-06 (visual) | Wrapping actually looks right in the editor | manual-only | Open generated WBP in UMG Designer; visual check | — |

### Sampling Rate
- **Per task commit:** `Automation RunTests PSD2UMG.Text` (quick)
- **Per wave merge:** `Automation RunTests PSD2UMG` (full plugin suite)
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] Extend `MultiLayer.psd` fixture (Phase 2) OR add `Typography.psd` fixture containing:
  - 1× point text layer in a plain font (baseline — Phase 3 behavior)
  - 1× point text layer in a *real-bold* font (e.g. `Arial-BoldMT`) — exercises suffix fallback
  - 1× point text layer with faux-bold on a plain font — exercises flag path
  - 1× box/paragraph text layer with explicit width — exercises TEXT-06
  - 1× text layer with a stroke (outline) applied — exercises TEXT-03 + ARGB stroke-color verification
- [ ] `Source/PSD2UMG/Private/Tests/FTextPipelineSpec.cpp` — covers TEXT-02, TEXT-03, TEXT-05, TEXT-06 end-to-end against the fixture
- [ ] `Source/PSD2UMG/Private/Tests/FFontResolverSpec.cpp` — unit tests for resolver chain + suffix table (PSD2UMG.Text.FontResolver.*)
- [ ] Helper to build a mock `UPSD2UMGSettings` with test `FontMap` entries in the spec

## Project Constraints (from CLAUDE.md)

- **UE 5.7.4 only** — use FAppStyle, PlatformAllowList, C++20 (`CppStandard = CppStandardVersion.Cpp20`)
- **No Python at runtime** — do not reintroduce `PythonScriptPlugin` dependency
- **Editor-only module** (`Type = Editor`, `LoadingPhase = PostEngineInit`) — Phase 4 mapper code must stay editor-side
- **PhotoshopAPI PIMPL boundary** — all PhotoshopAPI includes stay inside `PsdParser.cpp` under `THIRD_PARTY_INCLUDES_START/END` with Windows macro push/pop dance; **new parser fields do NOT leak PhotoshopAPI types into `Public/`**
- **Win64 primary target** — Phase 2 dropped Mac; Phase 4 does not revive it
- **Unreal naming** — `U`-prefix UObjects (`UTextBlock`), `F`-prefix structs (`FPsdTextRun`, `FSlateFontInfo`), `F`-prefix non-UObject helpers (`FFontResolver` or free functions)
- **Comments** — Allman braces, tabs; copyright header on new files matching existing pattern (`// Copyright 2018-2021 - John snow wind` for consistency with the fork)
- **TEXT() macro** for all string literals in C++ code
- **Log category** `LogPSD2UMG` for all warnings per established pattern
- **GSD workflow enforcement** — all file edits under a GSD command (this phase is already inside `/gsd:plan-phase` research)

## Sources

### Primary (HIGH confidence)
- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LayerTypes/TextLayer/TextLayerStyleRunMixin.h` — full style-run API (read above, lines 1-312)
- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LayerTypes/TextLayer/TextLayerShapeMixin.h` — box-text vs point-text API (read above, lines 1-515)
- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LayerTypes/TextLayer/TextLayerFontMixin.h` (grep-verified entry points)
- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LayerTypes/Layer.h` — grep-verified **no** layer-effect / shadow API
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — Phase 2 extraction pattern; ARGB fill-color quirk documented in-file at lines 236-250
- `Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp` — Phase 3 baseline to be rewritten
- `Source/PSD2UMG/Public/PSD2UMGSetting.h` — `FontMap`, `DefaultFont` already present
- `.planning/phases/04-text-fonts-typography/04-CONTEXT.md` — locked decisions
- `.planning/STATE.md` — Phase 2 P05 ARGB quirk decision log (line: "PhotoshopAPI text fill color array is ARGB, not RGBA")
- UE 5.7 engine headers (assistant training + UE5 continuity): `FSlateFontInfo::TypefaceFontName`, `FFontOutlineSettings`, `UTextBlock::SetFont/SetAutoWrapText/SetShadowOffset`, `FCompositeFont::DefaultTypeface`

### Secondary (MEDIUM confidence)
- Assumption that `stroke_color` follows same ARGB order as `fill_color` — extrapolated from P05 finding; needs runtime verification
- Assumption that `box_width()` is in pixels (not points) and does not need × 0.75 — inferred from Photoshop's text-space conventions; needs runtime verification against fixture

### Tertiary (LOW confidence)
- None for this phase. The decisive unknown (PhotoshopAPI text API surface) is HIGH confidence via direct header inspection.

## Metadata

**Confidence breakdown:**
- PhotoshopAPI text API surface: **HIGH** — verified by direct reading of vendored headers
- PhotoshopAPI lack of drop-shadow API: **HIGH** — verified by exhaustive grep across `LayerTypes/`
- UE 5.7 FSlateFontInfo / FFontOutlineSettings / UTextBlock shape: **HIGH** — stable UE5 public API
- Stroke color channel order (ARGB): **MEDIUM** — extrapolated from verified fill-color behavior, not independently verified
- Box width units: **MEDIUM** — inferred from Photoshop conventions and mixin doc comments
- AutoWrapText + CanvasPanelSlot width interaction: **MEDIUM** — standard UMG behavior but worth verifying against fixture
- Font typeface name conventions on third-party UFont assets: **LOW** — varies per asset; documented as open question

**Research date:** 2026-04-09
**Valid until:** 2026-05-09 (30 days — all findings anchored to vendored headers that won't change without an explicit PhotoshopAPI upgrade)

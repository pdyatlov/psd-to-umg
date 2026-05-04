# Phase 21: Parser Correctness Fixes - Research

**Researched:** 2026-04-28
**Domain:** PSD parser C++ — UTF-8/UTF-16 text slicing, lrFX ColorSpace dispatch, lfx2/FrFX VlLs extraction
**Confidence:** HIGH

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**RTXT-01 — Null-sentinel strip**
- D-01: User provides `RichTextCJK.psd` before execution. Plans assume fixture present.
- D-02: Spec (`FPsdParserRichTextCJKSpec` or folded into existing rich-text spec) validates: (a) null-sentinel stripped, (b) run boundary slicing correct for multi-byte characters.
- D-03: Fix at the `FullUtf8` call site in `PsdParser.cpp` (~line 485) — strip trailing `\0` bytes from the `std::string` before passing to `Utf8ToFString`. The `Utf8ToFString` helper (~line 75) is NOT changed.
- D-04: `FString::Mid` indexing by TCHAR is correct for CJK once the UTF-8→TCHAR conversion is clean (TCHAR = UTF-16 code unit on Win64; `style_run_lengths` = UTF-16 code units → boundaries align). No change needed to the Mid slicing loop beyond the sentinel fix.

**LFXC-01 — lrFX ColorSpace branch**
- D-05: Both `sofi` (~line 704) and `dsdw` (~line 750) in `ExtractLfx2Effects` get a `switch (ColorSpace)` dispatch after the channel reads:
  - `ColorSpace == 0` (RGB): current path unchanged.
  - `ColorSpace == 1` (HSB): convert via UE's `FLinearColor::HSVToLinearRGB`.
  - `ColorSpace == 2` (CMYK) or other: `UE_LOG(LogPSD2UMG, Warning, ...)` + best-effort identity pass-through. Do NOT crash or zero out.

**FXFMT-01 — VlLs branch depth**
- D-06: Full extraction. When outer loop hits `FrFX` with `VlLs` ostype, iterate each list item and extract `enab`, `Sz`, color components into `FStrokeInfo` — same fields as the `Objc` branch.
- D-07: VlLs list items are `Objc`-typed descriptors. Outer VlLs handler iterates item count, reads the ostype of each item, dispatches to the same inner FrFX parse block used for `Objc`.

**LFXC-02 — Human UAT**
- D-08: Non-blocking. RTXT-01, LFXC-01, FXFMT-01 constitute Phase 21 completion. LFXC-02 is logged as a standalone UAT task after code ships; does NOT gate Phase 22 start.

### Claude's Discretion
- Whether to add a shared `ConvertLfx2Color(ColorSpace, C0, C1, C2, A) → FLinearColor` helper (used by both sofi and dsdw), or inline the dispatch in each block.
- Whether the CJK spec is a new `FPsdParserCJKSpec` file or appended to the existing rich-text spec in `FRichTextLayerMapper` tests.
- Plan count and split (researcher / planner decide based on dependency graph).

### Deferred Ideas (OUT OF SCOPE)
- frameFXMulti VlLs stroke rendering: FXFMT-01 unlocks parsing; stroke emission for VlLs-origin data is Phase 22+ work.
- CMYK/Lab full lrFX color conversion: warn+identity path sufficient for v1.3; full conversion post-v1.3.
- `RichTextCJK.psd` multi-run emoji edge cases beyond null-sentinel: deferred to post-v1.3 if empirical tests expose gaps.
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| RTXT-01 | Strip trailing `\0` sentinel from `FullUtf8` before `Utf8ToFString`; add `RichTextCJK.psd` fixture; validate non-ASCII slicing in spec. | Exact call site identified (PsdParser.cpp:485); null-sentinel mechanism confirmed; `std::string::erase` strip pattern documented below. |
| LFXC-01 | Add ColorSpace branch to `sofi` and `dsdw` parsers: HSB via `FLinearColor::HSVToLinearRGB`; CMYK warn+identity. | UE 5.7 `Color.cpp` `HSVToLinearRGB` source confirmed: H∈[0,360], S∈[0,1], V∈[0,1]. Photoshop scale factors documented below. |
| LFXC-02 | Human UAT: visual confirm color overlay and drop shadow on real UE 5.7 host project. | Non-blocking — no code change. UAT task template documented in Architecture Patterns. |
| FXFMT-01 | `ParseFrFXDescriptor` VlLs branch: iterate list items as Objc descriptors, extract stroke fields at same depth as Objc branch. | VlLs item structure confirmed from PSD spec and existing `SkipValueAfterOsType` inner lambda. Exact refactor pattern documented below. |
</phase_requirements>

---

## Summary

Phase 21 fixes four isolated parser-level defects in `PsdParser.cpp`. None require new types, new mappers, or changes outside the parser and its test specs. All four changes are self-contained, low-risk C++ edits with clear call sites identified from prior research.

The most architecturally significant change is FXFMT-01: factoring the `FrFX Objc` body inside `ParseFrFXDescriptor` into a reusable lambda so both the existing `Objc` path and the new `VlLs` path can call it. This is a minor refactor of an already-isolated ~80-line function.

RTXT-01 is a one-liner strip at the `FullUtf8` call site plus a new spec that loads `RichTextCJK.psd` (user-provided). LFXC-01 adds a ColorSpace switch block after each channel read in `sofi` and `dsdw`; the UE `HSVToLinearRGB` API is confirmed present in UE 5.7 and takes `FLinearColor(H_0_360, S_0_1, V_0_1)`. LFXC-02 is a human UAT task with no code change.

**Primary recommendation:** Implement all three code changes (RTXT-01, LFXC-01, FXFMT-01) in a single plan wave; gate the phase on green spec runs. Log LFXC-02 as a standalone deferred UAT checklist item.

---

## Standard Stack

### Core (already present — no new dependencies)

| Library / API | Where Used | Notes |
|---------------|------------|-------|
| `FLinearColor::HSVToLinearRGB()` | LFXC-01 sofi/dsdw | **Confirmed in UE 5.7** `Color.cpp:411`. Instance method, no params. Takes `FLinearColor(H,S,V)` where H∈[0,360], S∈[0,1], V∈[0,1]. Returns linear RGB `FLinearColor`. |
| `std::string::erase` / `find_last_not_of` | RTXT-01 sentinel strip | Standard C++ — strip trailing `\0` bytes from `FullUtf8` before `Utf8ToFString`. |
| `UE_LOG(LogPSD2UMG, Warning, ...)` | LFXC-01 CMYK path | Existing log macro in this TU. |
| UE Automation Spec (`BEGIN_DEFINE_SPEC`) | RTXT-01 spec | Pattern established in `PsdParserSpec.cpp`, `FTextEffectsSpec.cpp`. |

### No New Packages Required

All dependencies are already in the plugin. No `npm install`, no CMake changes, no new ThirdParty libs.

---

## Architecture Patterns

### Recommended Project Structure (unchanged)

```
Source/PSD2UMG/
├── Private/Parser/PsdParser.cpp   — all three code changes land here
├── Tests/
│   ├── PsdParserSpec.cpp           — RTXT-01 CJK spec added here OR new file
│   └── Fixtures/
│       ├── RichText.psd            — existing ASCII multi-run fixture
│       └── RichTextCJK.psd         — NEW (user-provided before execution)
```

### Pattern 1: Null-Sentinel Strip (RTXT-01)

**What:** Strip trailing `\0` bytes from the PhotoshopAPI `text()` return value before passing to `Utf8ToFString`.

**Where:** `PsdParser.cpp` line ~485, inside the multi-run span extraction block. The `Content` scalar field at line ~204 calls `Utf8ToFString(*Content)` directly — that path also needs the strip to avoid a garbage `\0` character appearing as `Content` for single-run layers.

**Code site — multi-run (line ~485):**
```cpp
// BEFORE:
const std::string FullUtf8 = Text->text().value_or("");

// AFTER (D-03: strip at call site, do NOT touch Utf8ToFString helper):
std::string FullUtf8 = Text->text().value_or("");
// Strip PhotoshopAPI null-sentinel (CP-05): text() may embed trailing \0 bytes.
// Utf8ToFString uses c_str() which stops at the first \0, so an embedded sentinel
// causes content truncation for multi-byte strings.
const size_t LastNonNull = FullUtf8.find_last_not_of('\0');
if (LastNonNull != std::string::npos)
    FullUtf8.erase(LastNonNull + 1);
else
    FullUtf8.clear(); // entire string was null bytes
```

**Code site — Content scalar (line ~204):**
```cpp
// BEFORE:
OutLayer.Text.Content = Utf8ToFString(*Content);

// AFTER:
std::string ContentStr = *Content;
const size_t LastNonNull = ContentStr.find_last_not_of('\0');
if (LastNonNull != std::string::npos) ContentStr.erase(LastNonNull + 1);
else ContentStr.clear();
OutLayer.Text.Content = Utf8ToFString(ContentStr);
```

**Why D-04 holds (no Mid change needed):** `Utf8ToFString` calls `UTF8_TO_TCHAR(In.c_str())` which produces a UTF-16 FString on Win64. The PSD spec states `style_run_lengths()` entries are UTF-16 code-unit counts. BMP CJK characters (U+0000–U+FFFF) are each 1 UTF-16 code unit and 1 TCHAR on Win64, so `FString::Mid(offset, len)` aligns correctly. The null-sentinel was the only correctness problem — its removal is sufficient for CJK validation. Supplementary-plane emoji (surrogate pairs) remain a deferred edge case (CONTEXT.md `<deferred>`).

### Pattern 2: ColorSpace Dispatch (LFXC-01)

**What:** After reading `ColorSpace`, `C0`, `C1`, `C2` in both `sofi` and `dsdw`, dispatch to the correct color-space conversion before constructing `FLinearColor`.

**UE 5.7 HSVToLinearRGB semantics (confirmed from `Color.cpp:411`):**
- Input: `FLinearColor(H, S, V, A)` where **H ∈ [0, 360]**, **S ∈ [0, 1]**, **V ∈ [0, 1]**.
- Photoshop stores HSB channel values as 16-bit integers in [0..65535]; C0/C1/C2 are already divided by 65535.f at read time → they are in [0, 1].
- Therefore: H_ue = C0 × 360.f, S_ue = C1, V_ue = C2.

**Recommended helper (Claude's Discretion — shared helper is cleaner):**
```cpp
// Inside namespace PSD2UMG::Parser::Internal — before ExtractLayerEffects
static FLinearColor ConvertLfx2Color(uint16 ColorSpace, float C0, float C1, float C2, float A,
                                      FPsdParseDiagnostics& OutDiag, const FString& LayerName)
{
    if (ColorSpace == 0) // RGB
    {
        return FLinearColor(C0, C1, C2, A);
    }
    else if (ColorSpace == 1) // HSB
    {
        // Photoshop H in [0..65535] -> [0..1] after /65535 read above.
        // UE HSVToLinearRGB expects H in [0..360], S in [0..1], V in [0..1].
        return FLinearColor(C0 * 360.f, C1, C2, A).HSVToLinearRGB();
    }
    else // CMYK (2), Lab (7), or unknown -- warn + identity pass-through
    {
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("Layer '%s' lrFX: unsupported ColorSpace=%u; color channel values used as-is."),
            *LayerName, static_cast<uint32>(ColorSpace));
        return FLinearColor(C0, C1, C2, A);
    }
}
```

**sofi block patch** (replaces current line ~720):
```cpp
// BEFORE:
OutLayer.Effects.ColorOverlayColor = Enabled ? FLinearColor(C0, C1, C2, A) : FLinearColor::White;

// AFTER:
OutLayer.Effects.ColorOverlayColor = Enabled
    ? ConvertLfx2Color(ColorSpace, C0, C1, C2, A, OutDiag, OutLayer.Name)
    : FLinearColor::White;
```

**dsdw block patch** (replaces current line ~775-777):
```cpp
// BEFORE:
OutLayer.Effects.DropShadowColor = ShadowEnabled
    ? FLinearColor(C0, C1, C2, ShadowA)
    : FLinearColor(0, 0, 0, 0);

// AFTER:
OutLayer.Effects.DropShadowColor = ShadowEnabled
    ? ConvertLfx2Color(ColorSpace, C0, C1, C2, ShadowA, OutDiag, OutLayer.Name)
    : FLinearColor(0, 0, 0, 0);
```

### Pattern 3: FrFX VlLs Branch (FXFMT-01)

**What:** Add a VlLs branch inside `ParseFrFXDescriptor`'s top-level descriptor loop so Photoshop CC 2014+ effects are extracted rather than silently skipped.

**PSD spec structure for FrFX VlLs (confirmed from existing `SkipValueAfterOsType` inner lambda and CP-03 in PITFALLS.md):**
```
"FrFX" key, ostype "VlLs":
  uint32 N          — number of list items
  for each item:
    char[4] itemOsType   — read 4 bytes (typically "Objc")
    [item payload]       — for "Objc": same structure as FrFX Objc descriptor
                           (SkipUnicodeString + classID + uint32 count + key/value pairs)
```

**Refactor strategy (extract shared lambda, D-07):**
```cpp
// Before the top-level walk loop, define a lambda that parses one FrFX Objc item:
auto ParseFrFXObjcItem = [&]() -> bool
{
    // ---- This body is MOVED from the existing "FrFX"/"Objc" branch ----
    SkipUnicodeString(); // class name
    ReadPsString();       // classID ('FrFX')
    uint32 FrFXCount = ReadU32BE();

    bool   bEnab   = false;
    double SzPx    = 0.0;
    double OpctPct = 100.0;
    double Rd = 0.0, Grn = 0.0, Bl = 0.0;

    for (uint32 j = 0; j < FrFXCount && CheckRemaining(8); ++j)
    {
        // ... [existing key/value parsing: enab, Sz  , Opct, Clr ] ...
        // identical to current inner loop at lines 995-1050
    }

    if (bEnab)
    {
        Out.bEnabled = true;
        Out.SizePx   = static_cast<float>(SzPx);
        const float A = FMath::Clamp(static_cast<float>(OpctPct / 100.0), 0.f, 1.f);
        Out.Color = FLinearColor::FromSRGBColor(
            FColor(
                static_cast<uint8>(FMath::Clamp(Rd  / 255.0, 0.0, 1.0) * 255.0),
                static_cast<uint8>(FMath::Clamp(Grn / 255.0, 0.0, 1.0) * 255.0),
                static_cast<uint8>(FMath::Clamp(Bl  / 255.0, 0.0, 1.0) * 255.0),
                static_cast<uint8>(A * 255.0)));
    }
    return bEnab;
};

// In the top-level walk loop:
// Existing Objc branch (call the extracted lambda):
if (ItemKey == "FrFX" && FCStringAnsi::Strcmp(OsType, "Objc") == 0)
{
    bFoundStroke = ParseFrFXObjcItem();
}
// NEW VlLs branch (D-06, D-07):
else if (ItemKey == "FrFX" && FCStringAnsi::Strcmp(OsType, "VlLs") == 0)
{
    const uint32 N = ReadU32BE();
    for (uint32 k = 0; k < N && CheckRemaining(4) && !bFoundStroke; ++k)
    {
        char ElemOT[5] = {};
        for (int c = 0; c < 4; ++c) ElemOT[c] = static_cast<char>(Data[Pos + c]);
        Pos += 4;
        if (FCStringAnsi::Strcmp(ElemOT, "Objc") == 0)
            bFoundStroke = ParseFrFXObjcItem();
        else
            SkipValueAfterOsType(ElemOT);
    }
}
else
{
    SkipValueAfterOsType(OsType);
}
```

**Note:** The existing `VlLs` handler inside `SkipValueAfterOsType` (line ~943) handles the INNER skip path; the new code above is for the OUTER top-level extraction path. These do not conflict.

### Pattern 4: CJK Spec Structure (RTXT-01)

**Recommended: fold into existing PsdParserSpec.cpp** (simpler, fewer files, fixture pattern already established)

```cpp
// In PsdParserSpec.cpp — new Describe block alongside existing ones:
Describe("ParseFile on RichTextCJK.psd", [this]()
{
    BeforeEach([this]()
    {
        // load RichTextCJK.psd from Fixtures/
    });

    It("Content has no trailing null byte (CP-05 sentinel strip)", [this]()
    {
        // Find the CJK layer; verify Content does not contain '\0'
        // TestFalse("no null in Content", Layer->Text.Content.Contains(TEXT("\0")));
    });

    It("Multi-run spans do not have empty or null-only Text", [this]()
    {
        // Verify each Span.Text has Len() > 0 and no '\0'
    });

    It("Span boundaries are CJK-clean (no mid-codepoint split)", [this]()
    {
        // Sum of Span.Text lengths equals full Content length
    });
});
```

**Alternative: new file `FPsdParserCJKSpec.cpp`** — only needed if the CJK tests would exceed ~100 lines or if the fixture BeforeEach would conflict. For Phase 21 scope, folding is recommended.

### Anti-Patterns to Avoid

- **Modifying `Utf8ToFString` itself:** D-03 is explicit — the fix is at the call site. The helper is `c_str()`-based and generic; altering it breaks the `FontName` and other call sites.
- **Changing `FString::Mid` slicing math:** D-04 is explicit — Mid is correct for CJK once the sentinel is stripped. Do not add UTF-16 conversion machinery; that is the deferred emoji path.
- **Using `FLinearColor::MakeFromHSV8`:** This takes `uint8` values (H scaled to 256, S and V to 255) and maps H differently (256 steps not 360 degrees). It is NOT equivalent for Photoshop's 16-bit HSB values. Use `FLinearColor(H_deg, S_01, V_01).HSVToLinearRGB()` directly.
- **Crashing or returning `FLinearColor::Black` for unknown ColorSpace:** CONTEXT.md D-05 is explicit — best-effort identity pass-through with a Warning log. Never crash or zero out.
- **Copying the entire existing Objc body into the VlLs branch:** Extract the lambda first. Duplicated code is a maintenance hazard and the planner's job is to produce clean tasks.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| HSB→RGB conversion | Custom trig formula | `FLinearColor(H*360, S, V).HSVToLinearRGB()` | UE 5.7 confirmed; handles edge cases (H≥360, S=0, etc.) |
| UTF-8 null-sentinel detection | Byte scanner loop | `std::string::find_last_not_of('\0')` | Standard C++; O(n) scan from end — the sentinel is always at the tail |
| UTF-16 code-unit extraction for CJK | Custom UTF-8 decoder | `UTF8_TO_TCHAR` via `Utf8ToFString` (after sentinel strip) | UE's TCHAR = UTF-16 code unit on Win64; BMP CJK are single code units; no decoder needed for Phase 21 scope |

---

## Common Pitfalls

### Pitfall 1: Applying α to HSVToLinearRGB input (CP-06 variant)

**What goes wrong:** `FLinearColor(C0 * 360.f, C1, C2, A).HSVToLinearRGB()` — if `A` is passed as the alpha channel of the input FLinearColor, `HSVToLinearRGB` preserves it untouched in the output's alpha. This is correct; the alpha does not participate in the H/S/V conversion math.

**Why it happens:** Developer fears alpha will corrupt the conversion. The UE implementation (`Color.cpp:411-443`) confirms: `return FLinearColor(R_rgb, G_rgb, B_rgb, A)` — alpha is passed through as-is.

**How to avoid:** Pass alpha as the 4th constructor arg and call `HSVToLinearRGB()`. The returned color's `.A` will equal the input's `.A`. Confirmed correct.

### Pitfall 2: FrFX Objc lambda captures `Out` by reference — returning bool vs mutating

**What goes wrong:** The extracted `ParseFrFXObjcItem` lambda mutates `Out` (the `FPsdStrokeInfo&` parameter of `ParseFrFXDescriptor`) and returns a bool. The bool drives the `bFoundStroke` flag. If the lambda is defined before `Out` is referenced, the capture chain is fine — but if `bFoundStroke` is also captured, there is a double-mutation risk.

**How to avoid:** The lambda should return `bool` (stroke was enabled). The caller sets `bFoundStroke = ParseFrFXObjcItem()`. Do not capture `bFoundStroke` inside the lambda.

### Pitfall 3: Content scalar path also needs sentinel strip (easy to miss)

**What goes wrong:** The sentinel strip is added only at the `FullUtf8` site inside the multi-run block (~line 485), but `OutLayer.Text.Content = Utf8ToFString(*Content)` at ~line 204 calls `Utf8ToFString` with the raw `text()` value. A CJK layer with a single run will have a corrupted `Content` string even after the fix.

**How to avoid:** Apply the sentinel strip to BOTH call sites. The CONTEXT.md D-03 says "fix is at the `FullUtf8` call site" but the same PhotoshopAPI `text()` quirk affects the scalar `Content` path. RTXT-01's spec should assert `Content` has no null byte.

**Warning signs:** `Content.Contains(TEXT("\0"))` returns true in the CJK spec.

### Pitfall 4: VlLs item count loop off-by-one with `!bFoundStroke` guard

**What goes wrong:** The outer VlLs loop has `&& !bFoundStroke`. If `ParseFrFXObjcItem()` sets `Out.bEnabled = false` and returns false, the loop continues to the next item. If the first item has `bEnab = true`, `bFoundStroke` is set and the loop stops. This is correct for Photoshop's single-stroke model. If a PSD has multiple stroke items in the list (unusual), only the first enabled one wins — matching the Objc branch's behavior.

**How to avoid:** Keep the `!bFoundStroke` guard. This matches CONTEXT.md D-06: "same fields as the existing Objc branch."

### Pitfall 5: Photoshop ColorSpace=1 H channel is truly 0..65535 (not pre-divided)

**What goes wrong:** At `sofi` line ~705: `C0 = static_cast<float>(ReadU16()) / 65535.f` — the division is already applied. So `C0` is in [0, 1] when it reaches the ColorSpace dispatch. When the existing PITFALLS.md CP-06 prevention snippet shows `C0 * 360.f`, it means multiply the already-divided value by 360 to get degrees. Do not divide by 65535 again.

**How to avoid:** Confirm the read path: `ReadU16()` reads a raw 16-bit value; dividing by 65535.f maps [0..65535] → [0..1]. For HSB, multiplying back by 360 gives degrees. The helper `ConvertLfx2Color` receives `C0` already in [0, 1].

---

## Code Examples

### RTXT-01: Sentinel Strip (verified from codebase + CP-05 spec)
```cpp
// PsdParser.cpp — multi-run site (~line 485):
std::string FullUtf8 = Text->text().value_or("");
{
    const size_t Last = FullUtf8.find_last_not_of('\0');
    if (Last != std::string::npos)
        FullUtf8.erase(Last + 1);
    else
        FullUtf8.clear();
}
const FString FullText = Utf8ToFString(FullUtf8);
```

### LFXC-01: HSVToLinearRGB usage (UE 5.7 Color.cpp confirmed)
```cpp
// H in [0..360], S in [0..1], V in [0..1], A passed through
const FLinearColor Result = FLinearColor(C0 * 360.f, C1, C2, A).HSVToLinearRGB();
// Source: UE_5.7/Engine/Source/Runtime/Core/Private/Math/Color.cpp:411
```

### FXFMT-01: VlLs outer loop (from SkipValueAfterOsType pattern + CP-03 spec)
```cpp
else if (ItemKey == "FrFX" && FCStringAnsi::Strcmp(OsType, "VlLs") == 0)
{
    const uint32 N = ReadU32BE();
    for (uint32 k = 0; k < N && CheckRemaining(4) && !bFoundStroke; ++k)
    {
        char ElemOT[5] = {};
        for (int c = 0; c < 4; ++c) ElemOT[c] = static_cast<char>(Data[Pos + c]);
        Pos += 4;
        if (FCStringAnsi::Strcmp(ElemOT, "Objc") == 0)
            bFoundStroke = ParseFrFXObjcItem();
        else
            SkipValueAfterOsType(ElemOT);
    }
}
```

---

## State of the Art

| Old Behavior | Phase 21 Behavior | When Changed | Impact |
|--------------|-------------------|--------------|--------|
| `text()` value passed raw to `Utf8ToFString` | Sentinel `\0` stripped before conversion | Phase 21 | CJK text no longer truncated at null byte |
| `sofi` / `dsdw` always construct `FLinearColor(C0,C1,C2,A)` regardless of ColorSpace | Switch on ColorSpace; HSB converted via `HSVToLinearRGB` | Phase 21 | Correct hue for HSB-mode overlay and shadow colors |
| `FrFX` key with `VlLs` ostype silently skipped via `SkipValueAfterOsType` | VlLs items iterated; stroke fields populated for CC 2014+ files | Phase 21 | Photoshop CC 2014–2025 stroke effects now parse |

---

## Open Questions

1. **Does `RichTextCJK.psd` contain supplementary-plane (emoji) characters?**
   - What we know: D-04 says `FString::Mid` is correct for BMP CJK after the sentinel strip. Emoji (U+1F600+) are supplementary-plane and remain a deferred edge case.
   - What's unclear: the user has not specified whether `RichTextCJK.psd` contains emoji or only BMP CJK.
   - Recommendation: spec assertions should verify that `Content` and `Span.Text` strings contain no null bytes and that span lengths sum to total content length. If the fixture has emoji, add a note that this validates BMP CJK only and emoji accuracy is deferred per CONTEXT.md.

2. **Is the `Content` scalar also affected by the null-sentinel?**
   - What we know: The same `Text->text()` PhotoshopAPI call is used at both the scalar path (~line 204) and the multi-run path (~line 485). The null-sentinel is a PhotoshopAPI quirk on the return value, not call-site-specific.
   - Recommendation: Apply the sentinel strip to the scalar `Content` path too, and add a spec assertion. This is in scope for RTXT-01 (the requirement says "strip any trailing `\0` sentinel before conversion" generically).

3. **Will the first VlLs item always be `Objc`-typed?**
   - What we know: D-07 says "VlLs list items are Objc-typed descriptors" per PSD spec. The existing `SkipValueAfterOsType("VlLs")` already skips Objc items, and CP-03 shows the expected structure.
   - What's unclear: whether Photoshop ever writes non-Objc items in the FrFX VlLs list (e.g., a nested VlLs).
   - Recommendation: the `else SkipValueAfterOsType(ElemOT)` fallback handles unknown item types gracefully. The plan should include this fallback as written in CP-03.

---

## Environment Availability

Step 2.6: SKIPPED (no external dependencies — all changes are C++ edits to existing source files; UE 5.7 already installed and confirmed by prior phases).

---

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | UE Automation Spec (`BEGIN_DEFINE_SPEC` / `END_DEFINE_SPEC`) |
| Config file | `PSD2UMG.uplugin` + UE Test Runner (no separate config file) |
| Quick run command | Editor command line: `-ExecCmds="Automation RunTests PSD2UMG"` |
| Full suite command | Same — all specs run under `PSD2UMG.` prefix |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| RTXT-01 | `Content` has no null byte after import of CJK layer | unit | `Automation RunTests PSD2UMG.Parser` | New spec block — Wave 0 gap |
| RTXT-01 | Span.Text fields no null byte | unit | `Automation RunTests PSD2UMG.Parser` | New spec block — Wave 0 gap |
| RTXT-01 | Sum of span lengths = Content length | unit | `Automation RunTests PSD2UMG.Parser` | New spec block — Wave 0 gap |
| LFXC-01 | sofi ColorSpace=1 (HSB) produces correct hue (manual fixture only) | manual-only | n/a — requires HSB Effects PSD fixture not in scope | Not automated |
| LFXC-02 | Visual UAT on real UE 5.7 host | manual-only | n/a — human visual comparison | Not automated |
| FXFMT-01 | FrFX VlLs stroke parsed (requires a VlLs-format PSD fixture) | manual-only | n/a — no VlLs-format fixture in scope | Not automated |

**Note on LFXC-01 / FXFMT-01 automation:** These require fixture PSDs saved by Photoshop CC 2014+ with HSB effects and VlLs stroke format respectively. No such fixtures exist in `Fixtures/` today. The code changes can be reviewed for correctness via code inspection; automated regression coverage is deferred until a conforming fixture is available. The plan should note this gap explicitly.

### Sampling Rate
- **Per task commit:** `Automation RunTests PSD2UMG.Parser` (covers RTXT-01 CJK spec)
- **Per wave merge:** Full `PSD2UMG.` suite
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps

- [ ] `Source/PSD2UMG/Tests/Fixtures/RichTextCJK.psd` — user-provided; plan must gate task execution on presence check
- [ ] New `Describe("ParseFile on RichTextCJK.psd")` block in `PsdParserSpec.cpp` (or new `FPsdParserCJKSpec.cpp`) — covers RTXT-01

*(Existing `PsdParserSpec.cpp`, `FTextEffectsSpec.cpp` infrastructure cover all other automated requirements.)*

---

## Sources

### Primary (HIGH confidence)
- `C:\Dev\psd-to-umg\Source\PSD2UMG\Private\Parser\PsdParser.cpp` — `Utf8ToFString` at line 75; `ExtractSingleRunText` with `Content` scalar at line ~204; multi-run extraction block at lines 480-608; `ExtractLayerEffects` with `sofi` at lines 692-721 and `dsdw` at lines 722-778; `ParseFrFXDescriptor` with `SkipValueAfterOsType` inner lambda at lines 908-963 and outer Objc branch at lines 982-1066.
- `C:\Dev\psd-to-umg\Source\PSD2UMG\Public\Parser\PsdTypes.h` — `FPsdLayerEffects`, `FPsdTextRunSpan`, `FPsdTextRun` struct definitions.
- `C:\Dev\psd-to-umg\Source\PSD2UMG\Private\Mapper\FRichTextLayerMapper.cpp` — `CanMap` (Spans.Num() > 1), `BuildMarkup`, `EscapeMarkup` — confirms what correct span extraction must produce for mapper to work.
- `C:\Program Files\Epic Games\UE_5.7\Engine\Source\Runtime\Core\Private\Math\Color.cpp` (lines 411-443) — `FLinearColor::HSVToLinearRGB` implementation confirmed: H∈[0,360], S∈[0,1], V∈[0,1], alpha pass-through.
- `C:\Program Files\Epic Games\UE_5.7\Engine\Source\Runtime\Core\Public\Math\Color.h` (line 339) — `HSVToLinearRGB` declaration confirmed in UE 5.7.
- `.planning/REQUIREMENTS.md` — RTXT-01, LFXC-01, LFXC-02, FXFMT-01 specs; CP-03, CP-05 pitfall detail.
- `.planning/research/PITFALLS.md` — CP-05 null-sentinel, CP-06 lrFX ColorSpace, CP-03 VlLs outer loop, MP-04 surrogate sanitization.
- `.planning/phases/21-parser-correctness-fixes/21-CONTEXT.md` — locked decisions D-01 through D-08.

### Secondary (MEDIUM confidence)
- `.planning/research/SUMMARY.md` — synthesized stack and pitfall overview (2026-04-28).
- `.planning/STATE.md` — accumulated decisions: Phase 04.1 D-07 (8-byte prefix), Phase 16 multi-run slicing ASCII-only note.

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all APIs verified against UE 5.7 source in local install.
- Architecture: HIGH — all call sites pinpointed to exact line numbers in existing code; patterns confirmed from prior phase implementations.
- Pitfalls: HIGH — derived from direct codebase analysis in PITFALLS.md (2026-04-28) plus confirmed UE source.

**Research date:** 2026-04-28
**Valid until:** 2026-05-28 (stable domain — UE 5.7 APIs, C++ standard library)

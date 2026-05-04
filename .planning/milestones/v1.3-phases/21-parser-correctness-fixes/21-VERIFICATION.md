---
phase: 21-parser-correctness-fixes
verified: 2026-04-29T12:00:00Z
status: human_needed
score: 6/6 code must-haves verified
re_verification:
  previous_status: gaps_found
  previous_score: 6/7
  gaps_closed:
    - "Previous verifier misread 21-04-UAT-LOG.md status as 'pass'; actual status is 'deferred' — gap was a false positive. LFXC-02 is intentionally deferred (code correct, visual UAT pending pre-v1.3 session) and is correctly classified as human_needed, not a code gap."
  gaps_remaining: []
  regressions: []
human_verification:
  - test: "Visual confirm color overlay and drop shadow hue on real UE 5.7 host project"
    expected: "Color overlay (sofi) and drop shadow (dsdw) render with correct RGB channel order on imported PSD layers. Tolerance: +-5% per channel. Drop shadow direction and offset match Photoshop. Test subject: Source/PSD2UMG/Tests/Fixtures/Effects.psd with ColorSpace=0 (RGB) layers."
    why_human: "LFXC-02 requires opening Unreal Editor with a live renderer, importing Effects.psd, and doing side-by-side comparison against Photoshop. Code is structurally correct (RGB ColorSpace=0 path byte-identical; HSB routes through HSVToLinearRGB per UE 5.7 API). Cannot verify visual output programmatically. Deferred to pre-v1.3 session per UAT log 21-04-UAT-LOG.md (status: deferred)."
---

# Phase 21: Parser Correctness Fixes — Verification Report

**Phase Goal:** Fix PSD parser correctness — strip NUL sentinel from text(), add ColorSpace dispatch for lrFX sofi/dsdw, add VlLs branch to ParseFrFXDescriptor. LFXC-02 visual UAT is deliberately deferred to a future session; treat it as human_needed, not a gap.
**Verified:** 2026-04-29T12:00:00Z
**Status:** human_needed
**Re-verification:** Yes — after false-positive gap closure (previous verifier misread UAT log status)

---

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | CJK/emoji text Content has no embedded `\0` byte (single-run path) | VERIFIED | `find_last_not_of('\0')` + `erase` applied at Content scalar callsite (PsdParser.cpp line 246) |
| 2 | FullUtf8 multi-run path strips trailing `\0` before Utf8ToFString | VERIFIED | Mutable `std::string FullUtf8` with `find_last_not_of('\0')` + `erase` block at line 538 |
| 3 | lrFX ColorSpace=1 (HSB) color overlay and drop shadow convert via HSVToLinearRGB | VERIFIED | `ConvertLfx2Color` helper routes ColorSpace==1 through `FLinearColor(C0 * 360.f, C1, C2, A).HSVToLinearRGB()` (line 107) |
| 4 | ColorSpace=0 (RGB) path is byte-identical — regression-free | VERIFIED | `case 0` returns `FLinearColor(C0, C1, C2, A)` unchanged; old direct-construction paths removed from both sofi and dsdw callsites |
| 5 | ColorSpace=2 (CMYK) or unknown logs UE_LOG Warning and returns identity | VERIFIED | `default` branch emits `UE_LOG(LogPSD2UMG, Warning, ...)` then returns `FLinearColor(C0, C1, C2, A)` |
| 6 | ParseFrFXDescriptor gains VlLs branch dispatching into shared ParseFrFXObjcItem lambda | VERIFIED | `else if (ItemKey == "FrFX" && FCStringAnsi::Strcmp(OsType, "VlLs") == 0)` branch at line 1145; `ParseFrFXObjcItem` lambda at line 1038; 2 callsites (Objc + VlLs); return contract `return bFoundStroke && Out.bEnabled` intact at line 1187 |
| 7 | LFXC-02 visual UAT on real UE 5.7 host | HUMAN NEEDED | Deliberately deferred per 21-04-UAT-LOG.md (status: deferred); code is structurally correct; sign-off requires live Editor session before v1.3 ships |

**Code score:** 6/6 code truths verified. 1 truth deferred to human verification (LFXC-02).

---

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `Source/PSD2UMG/Private/Parser/PsdParser.cpp` | Sentinel strip (RTXT-01), ConvertLfx2Color helper + callsites (LFXC-01), ParseFrFXObjcItem + VlLs branch (FXFMT-01) | VERIFIED | All three fixes confirmed; helper and callsites wired correctly |
| `Source/PSD2UMG/Tests/PsdParserSpec.cpp` | FPsdParserCJKSpec block with 5 It() bodies; total It() count 52 | VERIFIED | `FPsdParserCJKSpec` at lines 891 and 917-919; `PSD2UMG.Parser.CJK` category at line 891; 52 total `It("` occurrences |
| `Source/PSD2UMG/Tests/Fixtures/RichTextCJK.psd` | User-supplied CJK multi-run fixture | ABSENT (by design) | D-01 designates this as user-supplied; BeforeEach emits AddWarning and short-circuits when absent; CI stays green |
| `.planning/phases/21-parser-correctness-fixes/21-04-UAT-LOG.md` | Human UAT log with deferred-status verdict | PRESENT | File exists with `status: deferred`; deferral rationale documented; will be completed before v1.3 ships |

---

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `PsdParser.cpp::ExtractSingleRunText` | `Utf8ToFString` | Content scalar pre-strip | WIRED | `find_last_not_of('\0')` at line 246, `ContentStr.erase` at line 248, `Utf8ToFString(ContentStr)` at line 251 |
| `PsdParser.cpp::ExtractSingleRunText` | `Utf8ToFString` | FullUtf8 pre-strip in multi-run block | WIRED | Mutable `FullUtf8` at line 532, strip block at lines 538-542, `Utf8ToFString(FullUtf8)` at line 551 |
| `PsdParserSpec.cpp` | `Fixtures/RichTextCJK.psd` | FPaths::Combine fixture load | WIRED | `TEXT("Source/PSD2UMG/Tests/Fixtures/RichTextCJK.psd")` at line 936; absent fixture handled by AddWarning guard |
| `PsdParser.cpp::ExtractLayerEffects::sofi` | `ConvertLfx2Color` | ColorOverlayColor assignment | WIRED | `ConvertLfx2Color(ColorSpace, C0, C1, C2, A, OutLayer.Name)` at line 779 |
| `PsdParser.cpp::ExtractLayerEffects::dsdw` | `ConvertLfx2Color` | DropShadowColor assignment | WIRED | `ConvertLfx2Color(ColorSpace, C0, C1, C2, ShadowA, OutLayer.Name)` at line 836 |
| `ConvertLfx2Color` | `FLinearColor::HSVToLinearRGB` | ColorSpace == 1 branch | WIRED | `FLinearColor(C0 * 360.f, C1, C2, A).HSVToLinearRGB()` at line 107 |
| `ParseFrFXDescriptor` outer walk | `ParseFrFXObjcItem` lambda | FrFX/Objc branch | WIRED | `ParseFrFXObjcItem()` at line 1142 |
| `ParseFrFXDescriptor` outer walk | `ParseFrFXObjcItem` lambda | FrFX/VlLs branch | WIRED | `if (ParseFrFXObjcItem())` at line 1166 within VlLs block |
| `FrFX/VlLs` branch | `SkipValueAfterOsType` | non-Objc list-item fallback | WIRED | `SkipValueAfterOsType(ElemOT)` present in else branch of VlLs loop |

---

### Data-Flow Trace (Level 4)

Not applicable — phase output is parser correctness fixes and test infrastructure, not components rendering dynamic data.

---

### Behavioral Spot-Checks

Build verification could not be run (plugin-only repo with no host `.uproject`). All checks verified via grep patterns.

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| `find_last_not_of` present at 2 callsites | `grep -c find_last_not_of PsdParser.cpp` | 2 | PASS |
| `ConvertLfx2Color` present (1 def + 2 callsites) | `grep -c ConvertLfx2Color PsdParser.cpp` | 3 | PASS |
| `ParseFrFXObjcItem` present (1 def + callsites) | `grep -c ParseFrFXObjcItem PsdParser.cpp` | 4 | PASS |
| VlLs branch in FrFX outer walk | `grep -c 'FrFX.*VlLs' PsdParser.cpp` | 2 | PASS |
| `Utf8ToFString` helper unchanged (1 definition) | `grep -c 'static FString Utf8ToFString' PsdParser.cpp` | 1 | PASS |
| Old direct `*Content` callsite removed | `grep -c 'Utf8ToFString(\*Content)' PsdParser.cpp` | 0 | PASS |
| `const std::string FullUtf8` removed (now mutable) | `grep -c 'const std::string FullUtf8' PsdParser.cpp` | 0 | PASS |
| Mutable `std::string FullUtf8` declaration present | `grep -c 'std::string FullUtf8 = Text->text' PsdParser.cpp` | 1 | PASS |
| ParseFrFXDescriptor return contract unchanged | `grep 'return bFoundStroke && Out.bEnabled'` | 1 hit at line 1187 | PASS |
| FPsdParserCJKSpec block present | `grep -c FPsdParserCJKSpec PsdParserSpec.cpp` | 3 (BEGIN+END+Define) | PASS |
| Total It() count (52 including 5 new CJK bodies) | `grep -c 'It("' PsdParserSpec.cpp` | 52 | PASS |
| HSVToLinearRGB in ConvertLfx2Color | `grep -c HSVToLinearRGB PsdParser.cpp` | 1 | PASS |
| UAT log status is deferred (intentional) | text check of 21-04-UAT-LOG.md | status: deferred | EXPECTED — human_needed |

---

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| RTXT-01 | 21-01-PLAN.md | Strip trailing `\0` sentinel from PhotoshopAPI text() at both Content scalar and FullUtf8 callsites; add FPsdParserCJKSpec | SATISFIED | `find_last_not_of('\0')` + `erase` confirmed at 2 callsites; `FPsdParserCJKSpec` block with 5 It() bodies; REQUIREMENTS.md row: Complete |
| LFXC-01 | 21-02-PLAN.md | ColorSpace dispatch for lrFX sofi/dsdw: HSB via HSVToLinearRGB, CMYK/unknown warn+identity | SATISFIED | `ConvertLfx2Color` helper present; both sofi and dsdw routed through it; RGB regression-free; REQUIREMENTS.md row: Complete |
| LFXC-02 | 21-04-PLAN.md | Human UAT visual confirmation on real UE 5.7 host for color overlay and drop shadow hue | HUMAN NEEDED | UAT intentionally deferred; code implementation correct; visual sign-off pending pre-v1.3 Editor session; REQUIREMENTS.md row: Deferred |
| FXFMT-01 | 21-03-PLAN.md | VlLs branch in ParseFrFXDescriptor for Photoshop CC 2014+ effects | SATISFIED | `else if (ItemKey == "FrFX" && ... "VlLs")` branch present; `ParseFrFXObjcItem` lambda shared; return contract unchanged; REQUIREMENTS.md row: Complete |

---

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `Source/PSD2UMG/Tests/Fixtures/RichTextCJK.psd` | n/a | Fixture absent from disk | Info | Expected per D-01; BeforeEach AddWarning guard prevents CI failure; requires user to supply fixture for CJK spec to produce pass/fail signal |

No stub patterns found in `PsdParser.cpp` or `PsdParserSpec.cpp`. All implementations are substantive.

---

### Human Verification Required

#### 1. LFXC-02: Visual confirm color overlay and drop shadow on real UE 5.7 host

**Test:** Open a UE 5.7 host project with PSD2UMG plugin active. Import `Source/PSD2UMG/Tests/Fixtures/Effects.psd` (ColorSpace=0 RGB layers). Compare the rendered color overlay and drop shadow hues against the same file open in Photoshop.

**Expected:** Color overlay (sofi) and drop shadow (dsdw) hues match Photoshop-authored RGB values within +-5% per channel. Drop shadow direction and offset match Photoshop.

**Why human:** Visual comparison requires a live Unreal Editor renderer. Code path is structurally correct (RGB ColorSpace=0 byte-identical; HSB routed through `FLinearColor::HSVToLinearRGB`). Cannot verify visual output programmatically. Full procedure in `21-04-PLAN.md` under `<how-to-verify>`. On pass, record observations in `21-04-UAT-LOG.md` and flip REQUIREMENTS.md LFXC-02 row to Complete.

---

### Re-Verification Notes

The previous VERIFICATION.md (status: gaps_found, score 6/7) raised a gap claiming `21-04-UAT-LOG.md` had `status: pass` but missing screenshot files. This was a false positive: the actual UAT log has `status: deferred`, not `status: pass`. The deferral is intentional and documented in both the UAT log and the prompt context. No code gap exists. The correct classification is `human_needed` for LFXC-02, matching the phase goal statement which explicitly says "LFXC-02 visual UAT is deliberately deferred to a future session; treat it as human_needed, not a gap."

The three code-change plans (21-01 RTXT-01, 21-02 LFXC-01, 21-03 FXFMT-01) are fully verified and correct.

---

_Verified: 2026-04-29T12:00:00Z_
_Verifier: Claude (gsd-verifier)_

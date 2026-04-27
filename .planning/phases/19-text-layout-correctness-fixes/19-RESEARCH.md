# Phase 19: Text + Layout Correctness Fixes — Research

**Researched:** 2026-04-27
**Domain:** UMG Text properties (ETextTransformPolicy, ColorAndOpacity), PhotoshopAPI text layer APIs, HBox/VBox child ordering
**Confidence:** HIGH

---

## Summary

Phase 19 targets three correctness requirements. Two require new code (TXT-CAPS-01, LAYOUT-ORDER-01). One (TXT-FX-01) is already implemented by the Phase 12 RouteTextEffects routing chain but was never formally verified or marked Complete in REQUIREMENTS.md — its task is verification + REQUIREMENTS update, not a code fix.

**TXT-FX-01 finding:** The overlay routing is correct and exercised by existing tests. `RouteTextEffects` (PsdParser.cpp:1695) copies `Effects.ColorOverlayColor` into `Text.Color` and clears `bHasColorOverlay`. `FTextLayerMapper` then uses `Text.Color` for `SetColorAndOpacity`. Two existing automated tests pin this behavior (PsdParserSpec "text_overlay_gray" at line 340, FTextPipelineSpec "text_overlay_gray" at line 157). Task is: confirm tests pass GREEN, update REQUIREMENTS.md TXT-FX-01 to Complete.

**TXT-CAPS-01 finding:** PhotoshopAPI exposes `style_run_font_caps(i)` which returns `std::optional<TextLayerEnum::FontCaps>`. `FontCaps::AllCaps = 2`. The parser currently reads `FauxBold`/`FauxItalic` from style runs but never reads `FontCaps`. Adding `style_run_font_caps(0)` to `ExtractSingleRunText` and storing `bAllCaps` on `FPsdTextRun` enables `FTextLayerMapper` to call `SetTextTransformPolicy(ETextTransformPolicy::ToUpper)` when the flag is set.

**LAYOUT-ORDER-01 finding:** PhotoshopAPI returns `Group->layers()` in top-to-bottom order (index 0 = topmost layer in Photoshop panel). `PopulateChildren` iterates 0..N and calls `Parent->AddChild(Widget)` in that order. For HBox/VBox, `AddChild` appends to the end of the child list, so the first call (topmost layer) becomes slot 0 = leftmost (HBox) / topmost (VBox). This is already the CORRECT intended order. The bug is that the existing `Panels.psd` fixture or test fixture was authored with the children in the wrong PSD order, OR the requirement description ("currently reversed") reflects behavior that existed before Phase 10's `PopulateChildren` refactor but was never re-verified. **Action for planning: add a test that asserts `GetChildAt(0)` identity by name (not just count) against the known PSD layer order, and run it to confirm whether the order is actually correct or reversed today.** If reversed, the fix is `Algo::Reverse(Layer.Children)` after `ConvertLayerRecursive` collects group children (parser side), or reversing the iteration order in `PopulateChildren` for non-canvas parents.

**Primary recommendation:** Three separate plans — (1) TXT-FX-01 verify + REQUIREMENTS update, (2) TXT-CAPS-01 parser + mapper + spec, (3) LAYOUT-ORDER-01 empirical order check + spec + fix if needed.

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| TXT-FX-01 | Color Overlay on text layers routes to UTextBlock::ColorAndOpacity, taking priority over fill color | Already implemented via RouteTextEffects in PsdParser.cpp:1726-1729; FTextLayerMapper.cpp:129-132; confirmed by PsdParserSpec + FTextPipelineSpec; needs REQUIREMENTS.md Close-out |
| TXT-CAPS-01 | All Caps text transformation sets UTextBlock::TextTransformPolicy = ETextTransformPolicy::ToUpper | PhotoshopAPI: style_run_font_caps(0) returns FontCaps::AllCaps=2; FPsdTextRun needs bAllCaps field; ExtractSingleRunText needs to read it; FTextLayerMapper needs SetTextTransformPolicy |
| LAYOUT-ORDER-01 | HBox/VBox children in same visual order as Photoshop layer stack (topmost PSD layer = first child slot) | PopulateChildren iterates index 0..N → AddChild appends in that order; PhotoshopAPI returns layers top-to-bottom (verified via LayeredFileImpl.h rbegin reverse); need empirical test to confirm actual runtime order before writing a fix |
</phase_requirements>

---

## Standard Stack

### Core (all already in project)
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| PhotoshopAPI | vendored | `style_run_font_caps(i)` for All Caps | Already used for all text property extraction |
| UTextBlock | UE 5.7 | `SetTextTransformPolicy(ETextTransformPolicy::ToUpper)` | Native UMG text widget |
| UHorizontalBox / UVerticalBox | UE 5.7 | Child ordering via `AddChild` | Already mapped in FSimplePrefixMappers |

### Relevant APIs (verified in UE 5.7 source)

**ETextTransformPolicy** (SlateCore/Public/Styling/SlateTypes.h:309):
```cpp
enum class ETextTransformPolicy : uint8
{
    None    = 0,  // default
    ToLower = 1,
    ToUpper = 2,
};
```

**UTextBlock::SetTextTransformPolicy** (UMG/Public/Components/TextBlock.h:197):
```cpp
UMG_API void SetTextTransformPolicy(ETextTransformPolicy InTransformPolicy);
```

**TextLayerEnum::FontCaps** (PhotoshopAPI/LayeredFile/LayerTypes/TextLayer/TextLayerEnum.h:96):
```cpp
enum class FontCaps : int32_t
{
    Normal    = 0,
    SmallCaps = 1,
    AllCaps   = 2,
};
```

**TextLayerStyleRunMixin::style_run_font_caps(i)** (TextLayerStyleRunMixin.h:58):
```cpp
std::optional<TextLayerEnum::FontCaps> style_run_font_caps(const size_t i) const;
```

**Installation:** No new packages needed. All APIs are already in the project.

---

## Architecture Patterns

### TXT-FX-01 — Verify only (no code change)

The implementation path is:
1. `ExtractLayerEffects` populates `FPsdLayerEffects::bHasColorOverlay` + `ColorOverlayColor` from the `sofi` lrFX block
2. `RouteTextEffects` (PsdParser.cpp:1695) — for text layers — copies `ColorOverlayColor` into `Text.Color` and clears `bHasColorOverlay`
3. `FTextLayerMapper::Map` uses `Text.Color` for `SetColorAndOpacity`

The FTextLayerMapper code at line 129-132 currently reads:
```cpp
const FLinearColor& TextColor = Layer.Effects.bHasColorOverlay
    ? Layer.Effects.ColorOverlayColor
    : Layer.Text.Color;
TextWidget->SetColorAndOpacity(FSlateColor(TextColor));
```
Since `bHasColorOverlay` is always false at mapper time (cleared by RouteTextEffects), this is dead code in the true branch. `Text.Color` already holds the overlay color. The behavior is correct. The dead-code guard is harmless but confusing — the planner may optionally clean it up.

Existing test coverage pins this:
- `PSD2UMG.Parser.Typography` → "text_overlay_gray" → `Text.Color` is gray (not white fill)
- `PSD2UMG.TextPipeline` → Typography fixture end-to-end → `text_overlay_gray` UTextBlock ColorAndOpacity is gray

Task: run tests, confirm GREEN, mark TXT-FX-01 Complete in REQUIREMENTS.md.

### TXT-CAPS-01 — Parser + FPsdTextRun + Mapper

**Step 1: Add `bAllCaps` to FPsdTextRun** (PsdTypes.h):
```cpp
// TXT-CAPS-01: Photoshop All Caps transformation from character panel.
// True when style_run_font_caps returns FontCaps::AllCaps for the dominant run.
bool bAllCaps = false;
```

**Step 2: Extract in ExtractSingleRunText** (PsdParser.cpp, after bBold/bItalic block ~line 366):
```cpp
// TXT-CAPS-01 — All Caps / FontCaps.
if (auto Caps = Text->style_run_font_caps(DominantRunIdx); Caps.has_value())
{
    OutLayer.Text.bAllCaps = (*Caps == TextLayerEnum::FontCaps::AllCaps);
}
```

**Step 3: Apply in FTextLayerMapper::Map** (after SetJustification):
```cpp
// TXT-CAPS-01 — All Caps text transform.
if (Layer.Text.bAllCaps)
{
    TextWidget->SetTextTransformPolicy(ETextTransformPolicy::ToUpper);
}
```

**Include needed** in FTextLayerMapper.cpp: `#include "Framework/Text/TextLayout.h"` — but `ETextTransformPolicy` is in `SlateCore/Styling/SlateTypes.h` which is transitively included via `Components/TextBlock.h`. Verify at compile time; no new explicit include likely needed.

### LAYOUT-ORDER-01 — Empirical + Sort

**Root cause analysis:**

PhotoshopAPI `build_layer_hierarchy_recursive` iterates PSD layer records from `rbegin()` (PSD stores layers bottom-to-top; reversing gives top-to-bottom visual order). Therefore `Group->layers()[0]` = topmost layer in Photoshop layer panel.

`ConvertLayerRecursive` appends children sequentially:
```cpp
for (const auto& Child : Children)
{
    FPsdLayer& ChildOut = OutLayer.Children.AddDefaulted_GetRef();
    ConvertLayerRecursive(Child, ChildOut, OutDiag, Lfx2Map);
}
```
So `FPsdLayer::Children[0]` = topmost PSD layer.

`PopulateChildren` iterates 0..N and calls `Parent->AddChild(Widget)` for each. `AddChild` appends to the end of the child list, making the topmost PSD layer the **first** child (slot 0). For VBox this is the topmost visual slot; for HBox this is the leftmost slot.

**This appears to be the CORRECT behavior.** The STATE.md decision "HBox sorted left-to-right, VBox top-to-bottom" was a planned approach for auto-detection (Phase 6) that was never needed after tag-based tagging; no sort was ever implemented.

**The requirement says "currently the order is reversed."** There are two possible explanations:
1. The requirement was written based on observed behavior before Phase 10's PopulateChildren refactor, and the behavior may now be correct.
2. PhotoshopAPI's `rbegin` reversal may not fully restore visual order — the reversed hierarchy reconstruction could itself be reversed relative to UI display.

**Recommended plan action:** Write a RED spec test that asserts `VBox->GetChildAt(0)->GetName() == "ItemA"` (topmost PSD layer) using the existing `Panels.psd` fixture. Run it. If RED → bug exists, fix is `Algo::Reverse(OutLayer.Children)` after the group children loop in `ConvertLayerRecursive`, OR iterate `TotalLayers-1` down to 0 in `PopulateChildren` for non-canvas parents. If GREEN → requirement was stale; just mark Complete and close.

**No sort by spatial position should be added** — the STATE.md decision "HBox sorted left-to-right, VBox top-to-bottom" was for the *auto-detection* heuristic (Phase 6, deprecated in Phase 9). Explicit `@hbox`/`@vbox` tags preserve PSD layer order; spatial sorting would break intentional ordering.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| All Caps text transform | Custom uppercase logic | `ETextTransformPolicy::ToUpper` + `SetTextTransformPolicy` | Native UMG handles locale-correct uppercase; custom logic misses Turkish i, etc. |
| Child index probing | Manual widget tree walk | `UPanelWidget::GetChildAt(int32)` + `GetChildrenCount()` | Built-in panel API, stable across UE versions |

---

## Common Pitfalls

### Pitfall 1: FontCaps FallBack for Multi-Run Layers (TXT-CAPS-01)
**What goes wrong:** `style_run_font_caps(0)` only reads run 0. Multi-run text layers have the dominant-run index (not necessarily 0). For the dominant-run path, use `DominantRunIdx`, not hardcoded 0.
**How to avoid:** Mirror the existing `style_run_faux_bold(DominantRunIdx)` pattern exactly.

### Pitfall 2: FTextLayerMapper vs FRichTextLayerMapper (TXT-CAPS-01)
**What goes wrong:** `FTextLayerMapper` is gated to single-run layers (`Spans.Num() <= 1`). Multi-run layers route to `FRichTextLayerMapper`. If a multi-run text layer uses All Caps, `FRichTextLayerMapper` also needs `SetTextTransformPolicy`. Phase 19 scope is `FTextLayerMapper` only — document the gap for `FRichTextLayerMapper` but don't implement it now.
**Warning signs:** No failing test for multi-run All Caps; defer explicitly.

### Pitfall 3: Dead-Code Guard in FTextLayerMapper (TXT-FX-01)
**What goes wrong:** The `Layer.Effects.bHasColorOverlay` check at line 129 looks like the authoritative overlay logic but is always false. If a future developer removes `RouteTextEffects` without updating the mapper, overlay will silently revert to white-fill.
**How to avoid:** Add a code comment (or optionally clean up the dead branch) so intent is clear.

### Pitfall 4: LAYOUT-ORDER-01 Spatial Sort vs Preservation Order
**What goes wrong:** Adding a spatial sort (left-to-right for HBox, top-to-bottom for VBox) would break PSD layers that are intentionally in reverse spatial order (e.g., a countdown list: "3, 2, 1" arranged visually top-to-bottom but authored in PSD bottom-to-top).
**How to avoid:** Preserve PSD layer stack order exactly. No spatial sort. Rely on the designer's intended PSD order.

### Pitfall 5: ETextTransformPolicy Include (TXT-CAPS-01)
**What goes wrong:** `ETextTransformPolicy` is defined in `SlateCore/Public/Styling/SlateTypes.h`. Including `Components/TextBlock.h` transitively pulls it in on most platforms, but a direct include may be needed for correctness.
**How to avoid:** Add `#include "Framework/Text/TextLayout.h"` or `#include "Styling/SlateTypes.h"` explicitly in FTextLayerMapper.cpp if the compiler errors on `ETextTransformPolicy::ToUpper`.

---

## Code Examples

Verified patterns from source:

### Reading FontCaps from PhotoshopAPI (verified in TextLayerStyleRunMixin.h:58)
```cpp
// Inside ExtractSingleRunText, after FauxBold/FauxItalic block:
if (auto Caps = Text->style_run_font_caps(DominantRunIdx); Caps.has_value())
{
    OutLayer.Text.bAllCaps = (*Caps == TextLayerEnum::FontCaps::AllCaps);
}
```

### Setting TextTransformPolicy in FTextLayerMapper (verified in TextBlock.h:197)
```cpp
// Inside FTextLayerMapper::Map, after SetJustification:
if (Layer.Text.bAllCaps)
{
    TextWidget->SetTextTransformPolicy(ETextTransformPolicy::ToUpper);
}
```

### Asserting VBox child order (spec pattern from FPanelAttachmentSpec.cpp)
```cpp
// TXT-CAPS-01 / LAYOUT-ORDER-01 spec pattern:
UVerticalBox* VBox = Cast<UVerticalBox>(FindWidgetByName(WBP->WidgetTree, FName(TEXT("VBoxGroup"))));
if (!TestNotNull(TEXT("VBoxGroup is VBox"), VBox)) return;
// Assert slot 0 = topmost PSD layer (ItemA):
UWidget* Slot0 = VBox->GetChildAt(0);
TestTrue(TEXT("VBox slot 0 = ItemA (topmost PSD layer)"), Slot0 && Slot0->GetName() == TEXT("ItemA"));
```

---

## State of the Art

| Old Approach | Current Approach | Impact |
|--------------|------------------|--------|
| FX-03 generator block applied Color Overlay to text (would double-render via UImage tint) | RouteTextEffects routes overlay into Text.Color; D-13 guard clears bHasColorOverlay so generator FX-03 block never fires for text | Text renders with overlay color via UTextBlock native ColorAndOpacity |
| No text transform support | Add bAllCaps + SetTextTransformPolicy(ToUpper) | Photoshop All Caps designers' intent preserved without upcasing content string |
| HBox/VBox auto-sort by spatial position (Phase 6, deprecated Phase 9) | Preserve PSD layer stack order | Designer controls order explicitly via layer panel position |

---

## Open Questions

1. **Is LAYOUT-ORDER-01 actually reversed today?**
   - What we know: PhotoshopAPI returns top-to-bottom, PopulateChildren adds in that order, AddChild appends → topmost PSD layer is slot 0.
   - What's unclear: The requirement says "currently reversed." This may be stale (written before Phase 10 refactor).
   - Recommendation: Plan 03 starts with a RED/GREEN spec that asserts child order. If the spec is GREEN on the first run, the order is already correct and the fix is only REQUIREMENTS.md Close-out.

2. **Should FRichTextLayerMapper also get SetTextTransformPolicy?**
   - What we know: TXT-CAPS-01 requirement targets UTextBlock only. Multi-run text routes to FRichTextLayerMapper.
   - What's unclear: Whether URichTextBlock supports SetTextTransformPolicy.
   - Recommendation: Defer to a future phase. Document as a known gap in the plan.

---

## Environment Availability

Step 2.6: SKIPPED (no external dependencies identified — all changes are C++ source edits within the UE build system, no external tools or services required).

---

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | UE Automation (FAutomationSpecBase) |
| Config file | none — UE discovers specs via EAutomationTestFlags |
| Quick run command | Run automation filter: `PSD2UMG.TextPipeline` + `PSD2UMG.Parser.Typography` |
| Full suite command | Run automation filter: `PSD2UMG` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| TXT-FX-01 | Color Overlay → ColorAndOpacity on UTextBlock | integration | `PSD2UMG.TextPipeline` Typography.psd "text_overlay_gray" | ✅ FTextPipelineSpec.cpp:158 |
| TXT-FX-01 | RouteTextEffects clears bHasColorOverlay | unit | `PSD2UMG.Parser.Typography` "text_overlay_gray" | ✅ PsdParserSpec.cpp:340 |
| TXT-CAPS-01 | bAllCaps set by parser for All Caps layer | unit | `PSD2UMG.Parser.Typography` new spec | ❌ Wave 0 |
| TXT-CAPS-01 | UTextBlock::TextTransformPolicy == ToUpper end-to-end | integration | `PSD2UMG.TextPipeline` Typography.psd "text_caps" | ❌ Wave 0 + fixture layer needed |
| LAYOUT-ORDER-01 | VBox slot 0 = topmost PSD layer (by name) | integration | `PSD2UMG.PanelAttachment` Panels.psd VBoxGroup slot-0 name | ❌ Wave 0 |
| LAYOUT-ORDER-01 | HBox slot 0 = topmost PSD layer (by name) | integration | `PSD2UMG.PanelAttachment` Panels.psd HBoxGroup slot-0 name | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** Run `PSD2UMG.TextPipeline` + `PSD2UMG.Parser.Typography` + `PSD2UMG.PanelAttachment`
- **Per wave merge:** Full `PSD2UMG` suite
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `Source/PSD2UMG/Tests/PsdParserSpec.cpp` — add spec for `text_caps` layer `bAllCaps == true` (TXT-CAPS-01 parser)
- [ ] `Source/PSD2UMG/Tests/FTextPipelineSpec.cpp` — add spec for `text_caps` UTextBlock `TextTransformPolicy == ToUpper` (TXT-CAPS-01 end-to-end)
- [ ] `Source/PSD2UMG/Tests/Fixtures/Typography.psd` — add `text_caps` layer with All Caps enabled
- [ ] `Source/PSD2UMG/Tests/FPanelAttachmentSpec.cpp` — add spec asserting `VBoxGroup->GetChildAt(0)->GetName() == "ItemA"` and HBoxGroup slot 0 name (LAYOUT-ORDER-01)
- Existing `Panels.psd` fixture has VBoxGroup with ItemA/ItemB/ItemC — no PSD edit needed if layer names are known; the test simply probes GetChildAt(0) and compares to the expected topmost-layer name

---

## Sources

### Primary (HIGH confidence)
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp:1695-1731` — RouteTextEffects implementation (direct code read)
- `Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp:126-132` — ColorAndOpacity path (direct code read)
- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LayerTypes/TextLayer/TextLayerStyleRunMixin.h:58` — `style_run_font_caps` API (direct header read)
- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LayerTypes/TextLayer/TextLayerEnum.h:96` — `FontCaps::AllCaps = 2` (direct header read)
- `C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/SlateCore/Public/Styling/SlateTypes.h:309` — `ETextTransformPolicy::ToUpper` definition (direct source read)
- `C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/UMG/Public/Components/TextBlock.h:197` — `SetTextTransformPolicy` API (direct source read)
- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/Impl/LayeredFileImpl.h:104-117` — rbegin layer ordering rationale (direct header read)
- `Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp:59-199` — PopulateChildren implementation (direct code read)

### Secondary (MEDIUM confidence)
- PSD format layer ordering: bottom-to-top in file; PhotoshopAPI reverses to top-to-bottom via `rbegin` — consistent with comment in LayeredFileImpl.h and known PSD spec behavior

### Tertiary (LOW confidence)
- LAYOUT-ORDER-01 "currently reversed" claim in REQUIREMENTS.md — unverified against current code; empirical test required before writing any fix

---

## Metadata

**Confidence breakdown:**
- TXT-FX-01 analysis: HIGH — direct code + tests read
- TXT-CAPS-01 API availability: HIGH — PhotoshopAPI header + UE source both verified
- LAYOUT-ORDER-01 current state: MEDIUM — PhotoshopAPI ordering logic confirmed; actual runtime order requires empirical spec

**Research date:** 2026-04-27
**Valid until:** 2026-05-27 (stable APIs)

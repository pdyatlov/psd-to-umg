---
phase: quick
plan: 260429-juj
type: execute
wave: 1
depends_on: []
files_modified:
  - Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp
  - Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp
autonomous: true
requirements: [TXT-OVERFLOW-01]

must_haves:
  truths:
    - "Paragraph text (bHasExplicitWidth=true) wraps at BoxWidthPx and does not overflow the bottom of its canvas slot"
    - "Point text (bHasExplicitWidth=false) renders at font-metric height rather than tight PhotoshopAPI pixel bounds"
    - "Stretch-anchored text layers (bStretchH or bStretchV) are unaffected by AutoSize — they still fill their margin-defined slot"
  artifacts:
    - path: "Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp"
      provides: "WrapTextAt(BoxWidthPx) replaces SetAutoWrapText for paragraph text"
      contains: "SetWrapTextAt"
    - path: "Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp"
      provides: "SetAutoSize(true) for non-stretch text canvas slots"
      contains: "SetAutoSize"
  key_links:
    - from: "FTextLayerMapper.cpp line 114"
      to: "UTextBlock::SetWrapTextAt"
      via: "Layer.Text.BoxWidthPx"
      pattern: "SetWrapTextAt"
    - from: "FWidgetBlueprintGenerator.cpp after SetLayout"
      to: "UCanvasPanelSlot::SetAutoSize"
      via: "!AnchorResult.bStretchH && !AnchorResult.bStretchV guard"
      pattern: "SetAutoSize\\(true\\)"
---

<objective>
Fix UTextBlock bottom overflow caused by Slate font metrics being taller than
PhotoshopAPI's tight pixel bounds stored in the layer record.

Purpose: When a canvas slot height equals PhotoshopAPI's layer height (tight visual
bounds) but Slate's line height (font metrics + leading) is taller, the UTextBlock
clips or overflows at the bottom. Two coordinated changes remove this mismatch.

Output: Two modified source files; plugin compiles; paragraph text wraps correctly
and point/paragraph text slots auto-size to Slate font metrics.
</objective>

<execution_context>
@$HOME/.claude/get-shit-done/workflows/execute-plan.md
@$HOME/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@.planning/PROJECT.md

<!-- Key interfaces the executor needs. -->
<interfaces>
From Source/PSD2UMG/Public/Parser/PsdTypes.h (FPsdTextRun fields):
```cpp
bool bHasExplicitWidth = false;   // true for paragraph/box text
float BoxWidthPx = 0.f;           // designer's explicit wrap width in PSD pixels
float BoxHeightPx = 0.f;          // designer's explicit box height (informational only)
```

From Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp (context lines 277-292):
```cpp
// TEXT-06 block (lines 277-285): sets Data.Offsets.Right = BoxWidthPx when
// bHasExplicitWidth && !bStretchH. Keep this block; AutoSize makes it
// informational only but it remains useful documentation/fallback.

CanvasSlot->SetLayout(Data);        // line 287 — insertion point is AFTER this call

// AnchorResult fields available in this scope:
bool AnchorResult.bStretchH;        // horizontal stretch anchor
bool AnchorResult.bStretchV;        // vertical stretch anchor
```

From Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp (line 114):
```cpp
// Current — replace this line:
TextWidget->SetAutoWrapText(Layer.Text.bHasExplicitWidth);
```
</interfaces>
</context>

<tasks>

<task type="auto">
  <name>Task 1: Replace SetAutoWrapText with SetWrapTextAt in FTextLayerMapper</name>
  <files>Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp</files>
  <action>
Replace line 114 (the TEXT-06 AutoWrapText call) with two statements:

```cpp
// TEXT-06 — paragraph (box) text: wrap at the designer's explicit width.
// SetWrapTextAt supplies a fixed pixel wrap boundary that functions correctly
// when the canvas slot uses AutoSize=true (see FWidgetBlueprintGenerator).
// SetAutoWrapText is NOT used because it relies on the slot width, which
// becomes undefined once AutoSize=true.  Point text (bHasExplicitWidth=false)
// leaves WrapTextAt at 0 (no wrap), matching pre-fix behaviour.
if (Layer.Text.bHasExplicitWidth && Layer.Text.BoxWidthPx > 0.f)
{
    TextWidget->SetWrapTextAt(Layer.Text.BoxWidthPx);
}
```

Remove the old single-line call:
  `TextWidget->SetAutoWrapText(Layer.Text.bHasExplicitWidth);`

No other changes to this file.
  </action>
  <verify>
    <automated>grep -n "SetWrapTextAt\|SetAutoWrapText" "Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp"</automated>
  </verify>
  <done>File contains SetWrapTextAt and does NOT contain SetAutoWrapText</done>
</task>

<task type="auto">
  <name>Task 2: Add SetAutoSize(true) for non-stretch text slots in FWidgetBlueprintGenerator</name>
  <files>Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp</files>
  <action>
After line 287 (`CanvasSlot->SetLayout(Data);`) insert the following block.
Do NOT modify or remove the existing TEXT-06 BoxWidthPx override block (lines 277-285) —
it is kept as documentation/fallback; AutoSize makes it ignored at render time.

```cpp
            // TXT-OVERFLOW-01 — AutoSize for non-stretch text canvas slots.
            // PhotoshopAPI layer bounds are tight visual pixel bounds; Slate's line
            // height (font metrics + leading) is typically taller.  SetAutoSize(true)
            // tells the canvas slot to use UTextBlock's desired size (driven by Slate
            // font metrics) rather than the Offsets.Right/Bottom values we set above.
            //
            //   Point text:     desired = content_width × font_line_height
            //   Paragraph text: desired = BoxWidthPx    × wrapped_text_height
            //                   (WrapTextAt set by FTextLayerMapper Task 1)
            //
            // Stretch-anchored slots already derive size from margin math; applying
            // AutoSize there would break the fill behaviour — guard is required.
            if (LayerPtr->Type == EPsdLayerType::Text
                && !AnchorResult.bStretchH
                && !AnchorResult.bStretchV)
            {
                CanvasSlot->SetAutoSize(true);
            }
```

Insert this block immediately after `CanvasSlot->SetLayout(Data);` (line 287) and
before `CanvasSlot->SetZOrder(...)` (line 289).  The comment on lines 291-293 that
says "Non-canvas layout groups…SetAutoSize would force…" remains correct for
non-canvas groups — it does NOT contradict this canvas-specific change, so leave
that comment in place.
  </action>
  <verify>
    <automated>grep -n "SetAutoSize\|TXT-OVERFLOW-01" "Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp"</automated>
  </verify>
  <done>File contains SetAutoSize(true) guarded by the EPsdLayerType::Text + !bStretchH + !bStretchV condition, inserted after SetLayout</done>
</task>

</tasks>

<verification>
After both tasks:
1. `grep -n "SetAutoWrapText" Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp` — must return nothing
2. `grep -n "SetWrapTextAt" Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp` — must show the new conditional call
3. `grep -n "SetAutoSize" Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp` — must show the guarded call
4. Plugin compiles without errors (full build or at minimum UBT dependency check)
</verification>

<success_criteria>
- FTextLayerMapper.cpp: SetAutoWrapText removed; SetWrapTextAt(BoxWidthPx) present inside bHasExplicitWidth guard
- FWidgetBlueprintGenerator.cpp: SetAutoSize(true) present inside EPsdLayerType::Text + !bStretchH + !bStretchV guard, positioned after SetLayout call
- No other production code changes
- Plugin compiles clean
</success_criteria>

<output>
After completion, update `.planning/STATE.md` Quick Tasks Completed table with:
| 260429-juj | Fix text bottom overflow: WrapTextAt + AutoSize for non-stretch text slots | 2026-04-29 | {commit} | [260429-juj-fix-text-bottom-overflow-ftextlayermappe](./quick/260429-juj-fix-text-bottom-overflow-ftextlayermappe/) |
</output>

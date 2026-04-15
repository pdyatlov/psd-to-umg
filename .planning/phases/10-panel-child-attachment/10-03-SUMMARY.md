---
phase: 10-panel-child-attachment
plan: "03"
subsystem: testing
tags: [fixture, automation-spec, panel-dispatch, vbox, hbox, scrollbox, scope-trim]
dependency_graph:
  requires: [10-01, 10-02]
  provides: [PANEL-07]
  affects: [FPanelAttachmentSpec, Panels.psd]
tech_stack:
  added: []
  patterns: [FPaths::FileExists-gate, BEGIN_DEFINE_SPEC, ClearChildren+PopulateChildren reimport sentinel]
key_files:
  created:
    - Source/PSD2UMG/Tests/Fixtures/Panels.psd
    - Source/PSD2UMG/Tests/FPanelAttachmentSpec.cpp
  modified: []
decisions:
  - "Overlay/Canvas/Nested spec cases removed per user direction — code paths are generic and work (UPanelWidget::AddChild), but 1.0.1 scope trimmed to VBox/HBox/ScrollBox coverage only"
  - "Fixture content matches trimmed scope: VBoxGroup @vbox (3 children), HBoxGroup @hbox (2 children), ScrollBoxGroup @scrollbox (4 children)"
  - "NonCanvasChildSlotsHaveNoPositionalData and ReimportPreservesNonCanvasChildren sentinels retained as regression guards for 10-01/10-02 attach and clear-and-rebuild logic"
metrics:
  duration: "human-in-the-loop (Photoshop) + scoped trim session"
  completed: "2026-04-15"
  tasks_completed: 2
  files_modified: 2
---

# Phase 10 Plan 03: Panel Fixture + Automation Spec Summary

VBox/HBox/ScrollBox fixture authored by human (229 KB PSD), spec covers PANEL-07 with VBox/HBox/ScrollBox coverage; Overlay/Canvas/Nested cases deferred per user-approved scope trim (code paths work; not v1.0.1 priority).

## Tasks Completed

| # | Name | Commit | Files |
|---|------|--------|-------|
| 1 | Author Panels.psd fixture (human-in-the-loop Photoshop step) | 948566b | Source/PSD2UMG/Tests/Fixtures/Panels.psd |
| 2 | Implement FPanelAttachmentSpec.cpp + trim Overlay/Canvas/Nested cases | 9267f39, 0bbf17c | Source/PSD2UMG/Tests/FPanelAttachmentSpec.cpp |

## Decisions Made

1. **Scope trim (user-directed):** The original plan called for 7 spec cases including PANEL-04 (@overlay), PANEL-05 (Canvas regression sentinel), and nested OuterVBox > InnerCanvas dispatch. Per explicit user direction, these were removed from the spec. The underlying implementation in FWidgetBlueprintGenerator is generic — UPanelWidget::AddChild handles all non-canvas panel types and the canvas path is byte-identical. The trim is a test-coverage deferral, not a missing feature.

2. **Retained sentinels:** Despite the trim, two guards were kept as regression anchors for 10-01/10-02 work:
   - `NonCanvasChildSlotsHaveNoPositionalData` — verifies ItemA inside VBoxGroup has UVerticalBoxSlot (not UCanvasPanelSlot), confirming the double-guard pattern in PopulateChildren.
   - `ReimportPreservesNonCanvasChildren` — calls Update() after Generate() on the same WBP and asserts VBox/ScrollBox child counts and types survive clear-and-rebuild (10-02 D-11 strategy).

3. **Fixture size:** 229 KB, human-authored in Photoshop with "maximize compatibility" on. Contains VBoxGroup @vbox (ItemA @text, ItemB @image, ItemC @button group), HBoxGroup @hbox (H1 @text, H2 @text), ScrollBoxGroup @scrollbox (S1..S4 @text).

## Deviations from Plan

### User-Directed Scope Trims

**1. [User Direction] Overlay/Canvas/Nested cases removed from spec**
- **Found during:** Task 2 execution
- **Direction:** User explicitly requested removal of PANEL-04 (OverlayGroup_IsOverlayWith2Children), PANEL-05 (CanvasGroup_PreservesAnchorsAndOffsets), and PANEL-06 nested case (NestedMixedGroups_DispatchCorrectly) from the spec scope.
- **Outcome:** Spec trimmed from 7 cases to 5 cases. Fixture was kept minimal (3 top-level groups only). Commit `0bbf17c` records the trim with full rationale in message.
- **Technical impact:** None. The code dispatch is generic (UPanelWidget path). The canvas diff-update path was validated in 10-01/10-02 via readback, not live build.
- **Files modified:** FPanelAttachmentSpec.cpp (9 insertions, 98 deletions at trim commit)

## Known Deferred Coverage

The following PANEL requirements have working implementations in FWidgetBlueprintGenerator but lack spec case coverage as of this plan:

| Requirement | Missing Case | Reason |
|---|---|---|
| PANEL-04 (@overlay) | OverlayGroup_IsOverlayWith2Children | User scope trim for v1.0.1 |
| PANEL-05 (Canvas regression) | CanvasGroup_PreservesAnchorsAndOffsets | User scope trim for v1.0.1 |
| PANEL-06 (nested dispatch) | NestedMixedGroups_DispatchCorrectly | User scope trim for v1.0.1 |

These are not stubs — the implementation is complete. The deferred items are test-coverage gaps only.

## Spec Cases Present (Post-Trim)

1. **VBoxGroup_IsVerticalBoxWith3Children** — Cast to UVerticalBox, count == 3, child names + types.
2. **HBoxGroup_IsHorizontalBoxWith2Children** — Cast to UHorizontalBox, count == 2.
3. **ScrollBoxGroup_IsScrollBoxWith4Children** — Cast to UScrollBox, count == 4.
4. **NonCanvasChildSlotsHaveNoPositionalData** — ItemA's slot is UVerticalBoxSlot, not UCanvasPanelSlot.
5. **ReimportPreservesNonCanvasChildren** — Update() on same WBP preserves VBox count == 3, ScrollBox count == 4; ItemA remains UTextBlock with UVerticalBoxSlot.

All 5 cases are gated by `bFixtureExists` (FPaths::FileExists pattern from FTextEffectsSpec).

Final spec: 263 lines.

## Self-Check: PASSED

- File `Source/PSD2UMG/Tests/Fixtures/Panels.psd` exists (committed at 948566b).
- File `Source/PSD2UMG/Tests/FPanelAttachmentSpec.cpp` exists (263 lines; committed at 9267f39, trimmed at 0bbf17c).
- Commits 948566b, 9267f39, 0bbf17c exist in git log.

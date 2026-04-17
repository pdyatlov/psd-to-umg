---
plan: 11-02
status: complete
phase: 11
subsystem: UI / import dialog
tags: [hidden-layers, eye-icon, visibility, import-dialog]
key-files:
  modified:
    - Source/PSD2UMG/Private/UI/SPsdImportPreviewDialog.cpp
decisions:
  - "EVisibility::Hidden used (not Collapsed) for visible layers to preserve 20px column width and keep rows horizontally aligned"
  - "FAppStyle::GetBrush(Layer.NotVisibleIcon16x) selected over Icons.EyeClosed — verified present in UE 5.7"
  - "Slot inserted as leftmost column (Col 0) before SCheckBox (Col 1)"
metrics:
  duration: 5m
  completed: 2026-04-17
  tasks: 1
  files_modified: 1
---

## What was built

Added a leading eye-closed icon column (Col 0) to every row in the `SPsdImportPreviewDialog` tree view. Hidden PSD layers (`bLayerVisible == false`) display the `Layer.NotVisibleIcon16x` brush icon with a tooltip; visible layers display an invisible but width-reserved (20px) slot so all rows stay horizontally aligned. No other slots or logic were touched.

## Key decisions

- `EVisibility::Hidden` is used when the layer is visible (icon present but invisible) rather than `EVisibility::Collapsed` — this is critical to maintain column alignment across all rows.
- `FAppStyle::GetBrush(TEXT("Layer.NotVisibleIcon16x"))` was chosen as specified; `Icons.EyeClosed` does not exist in UE 5.7 and was explicitly avoided.
- The slot was inserted before the existing `SCheckBox` slot, making it the leftmost column (Col 0).

## Verification

Grep results confirming all three required strings are present in the file:

```
575:                .Image(FAppStyle::GetBrush(TEXT("Layer.NotVisibleIcon16x")))
576:                .Visibility(Item->bLayerVisible ? EVisibility::Hidden : EVisibility::Visible)
577:                .ToolTipText(LOCTEXT("HiddenLayerTip", "This layer is hidden in Photoshop — unchecked by default."))
```

Commit: `5f56072`

## Human verification required

This plan requires human verification in Unreal Editor before being marked fully complete.

Steps:
1. Open Unreal Editor with PSD2UMG plugin loaded
2. Import a PSD with at least one hidden layer
3. Confirm hidden layers show eye-closed icon to the left of checkbox
4. Confirm visible layers show empty (but aligned) slot — same column width, no icon
5. Confirm hidden layers have checkbox UNCHECKED by default (from Plan 11-01)

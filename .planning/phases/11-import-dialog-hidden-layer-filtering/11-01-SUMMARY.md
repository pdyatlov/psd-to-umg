---
plan: 11-01
status: complete
---

## What was built

Fixed two latent bugs in the layer tree data model before the hidden-layer UI column is added in Plan 02. `FPsdLayerTreeItem` now carries a `DisplayName` field (tag-free CleanName for visual display) separate from `LayerName` (raw PSD name used as the SkippedLayerNames key), and a `bLayerVisible` field that records PSD source visibility independently of user intent. `BuildTreeRecursive` now initialises `bChecked` from `bParentChecked && Layer.bVisible` instead of the hardcoded `true`, and propagates the computed state into children via the new `bParentChecked` parameter.

## Key decisions

- `LayerName` stores `Layer.Name` (raw) so the factory collection loop can use it as an exact skip key matching what the PSD parser emits — changing it to CleanName would silently break exclusion for any tag-annotated layer.
- `DisplayName` replaces the in-tree text block binding (`Item->LayerName` → `Item->DisplayName`) so users see the clean, tag-stripped name while the raw name is preserved for programmatic use.
- `bLayerVisible` is stored as a read-only PSD fact so the upcoming visibility column (Plan 02) can display it without re-querying the document.

## Verification

```
grep -n "DisplayName\|bLayerVisible" PsdLayerTreeItem.h
20:    FString DisplayName;         // CleanName or raw Name — shown in tree row (Phase 11)
24:    bool bLayerVisible = true;   // PSD source truth — read-only after BuildTreeRecursive (Phase 11)

grep -n "bParentChecked" SPsdImportPreviewDialog.h
108:                                   bool bParentChecked = true);

grep -n "Layer\.Name\b" SPsdImportPreviewDialog.cpp
249:        Item->LayerName    = Layer.Name;
251:                           ? Layer.Name : Layer.ParsedTags.CleanName;

grep -n "bParentChecked && Layer\.bVisible" SPsdImportPreviewDialog.cpp
253:        Item->bChecked     = bParentChecked && Layer.bVisible;
```

All four checks passed. Commit: beb11e8

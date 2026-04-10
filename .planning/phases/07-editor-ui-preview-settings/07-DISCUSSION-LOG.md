# Phase 7: Editor UI, Preview & Settings - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-10
**Phase:** 07-editor-ui-preview-settings
**Areas discussed:** Import preview dialog, Reimport strategy, CommonUI & animations, Settings & context menu

---

## Import Preview Dialog

| Option | Description | Selected |
|--------|-------------|----------|
| Every PSD import | Dialog always shows before creating widgets | ✓ |
| Only via context menu | Drag-and-drop stays automatic, preview only on right-click | |
| Configurable | bShowPreviewDialog setting, default on | |

**User's choice:** Every PSD import
**Notes:** None

| Option | Description | Selected |
|--------|-------------|----------|
| Name + widget type badge | Checkbox, name, colored badge for widget type | ✓ |
| Name + badge + thumbnail | Same plus image thumbnail for image layers | |
| Name + badge + dimensions | Same plus pixel dimensions | |

**User's choice:** Name + widget type badge
**Notes:** None

| Option | Description | Selected |
|--------|-------------|----------|
| No — read-only badges | Preview for visibility control only | ✓ |
| Yes — dropdown per layer | Override widget type per layer | |

**User's choice:** No — read-only badges
**Notes:** None

| Option | Description | Selected |
|--------|-------------|----------|
| Editable in dialog | Output path with browse button, pre-filled from settings | ✓ |
| Always from settings | No override in dialog | |

**User's choice:** Editable in dialog
**Notes:** None

---

## Reimport Strategy

| Option | Description | Selected |
|--------|-------------|----------|
| Layer name | Match by PSD layer name | ✓ |
| Layer name + path | Match by full hierarchy path | |
| PSD layer ID | Internal Photoshop IDs | |

**User's choice:** Layer name
**Notes:** None

| Option | Description | Selected |
|--------|-------------|----------|
| Update PSD-sourced, preserve manual | Update position/size/image/text, keep Blueprint logic | ✓ |
| Full replace | Destroy and recreate matched widget | |
| Update nothing — add new only | Only add new layers | |

**User's choice:** Update PSD-sourced, preserve manual
**Notes:** None

| Option | Description | Selected |
|--------|-------------|----------|
| Keep orphaned widgets | Widgets from removed layers stay | ✓ |
| Remove orphaned widgets | Auto-delete unmatched widgets | |
| Mark as orphaned | Collapsed + _orphan suffix | |

**User's choice:** Keep orphaned widgets
**Notes:** None

| Option | Description | Selected |
|--------|-------------|----------|
| Yes — show preview | Same dialog with [new]/[changed]/[unchanged] annotations | ✓ |
| Silent reimport | Auto-apply with notification toast | |

**User's choice:** Yes — show preview
**Notes:** None

---

## CommonUI & Animations

| Option | Description | Selected |
|--------|-------------|----------|
| Global toggle in settings | bUseCommonUI in UPSD2UMGSettings | ✓ |
| Per-import toggle | Checkbox in preview dialog | |
| Layer name prefix | CButton_ for CommonUI | |

**User's choice:** Global toggle in settings
**Notes:** None

| Option | Description | Selected |
|--------|-------------|----------|
| Resolve at import | Look up UInputAction asset at import time | ✓ |
| Store name only | Save as metadata for manual binding | |
| You decide | Claude's discretion | |

**User's choice:** Resolve at import
**Notes:** None

| Option | Description | Selected |
|--------|-------------|----------|
| Simple opacity fade | _show: 0→1, _hide: 1→0, _hover: scale 1.05 | ✓ |
| Property snapshots only | Keyframes from variant layer properties | |
| You decide | Claude's discretion | |

**User's choice:** Simple opacity fade
**Notes:** None

| Option | Description | Selected |
|--------|-------------|----------|
| Auto-calculate from children | Sum children heights for content size | |
| No height calc — let UMG handle it | UMG calculates at runtime | ✓ |
| You decide | Claude's discretion | |

**User's choice:** No height calc — let UMG handle it
**Notes:** None

---

## Settings & Context Menu

**New settings selected (multi-select):**
- bShowPreviewDialog (default: true)
- bUseCommonUI (default: false)
- InputActionSearchPath
- Source DPI (default: 72)

| Option | Description | Selected |
|--------|-------------|----------|
| PSD files + existing WBPs | Context menu on both asset types | ✓ |
| PSD files only | Context menu on .psd textures only | |
| You decide | Claude's discretion | |

**User's choice:** PSD files + existing WBPs
**Notes:** None

---

## Claude's Discretion

- Dialog layout and Slate widget arrangement
- Change detection algorithm for reimport
- PSD source path metadata storage on Widget Blueprints
- UWidgetAnimation creation API details
- CommonUI module dependency management

## Deferred Ideas

None — discussion stayed within phase scope.

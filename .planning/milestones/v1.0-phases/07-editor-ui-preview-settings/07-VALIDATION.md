---
phase: 7
slug: editor-ui-preview-settings
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-10
---

# Phase 7 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | UE Automation (FAutomationTestBase) |
| **Config file** | Source/PSD2UMG/Tests/ (existing test directory) |
| **Quick run command** | `UE5 -RunTests PSD2UMG` |
| **Full suite command** | `UE5 -RunTests PSD2UMG -ExecCmds="Automation RunAll"` |
| **Estimated runtime** | ~30 seconds |

---

## Sampling Rate

- **After every task commit:** Compile check (UBT build)
- **After every plan wave:** Run full test suite
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 60 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 07-01-01 | 01 | 1 | EDITOR-01 | compile | UBT build | N/A | ⬜ pending |
| 07-02-01 | 02 | 1 | EDITOR-02,03 | manual | Launch editor, import PSD | N/A | ⬜ pending |
| 07-03-01 | 03 | 2 | EDITOR-04 | manual | Import, modify, reimport PSD | N/A | ⬜ pending |
| 07-04-01 | 04 | 2 | EDITOR-05 | manual | Right-click PSD in Content Browser | N/A | ⬜ pending |
| 07-05-01 | 05 | 3 | CUI-01,02 | manual | Enable CommonUI, import Button_ layer | N/A | ⬜ pending |
| 07-06-01 | 06 | 3 | CUI-03 | manual | Import _show/_hide/_hover variants | N/A | ⬜ pending |
| 07-07-01 | 07 | 3 | CUI-04 | manual | Import ScrollBox_ layer, check runtime | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

*Existing infrastructure covers all phase requirements. Phase 7 is primarily editor UI — validation is manual (launch editor, interact with dialogs).*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Settings appear in Project Settings | EDITOR-01 | UI presence check requires running editor | Open Project Settings > Plugins > PSD2UMG, verify all settings visible |
| Preview dialog shows on PSD import | EDITOR-02,03 | Slate dialog interaction | Import a .psd, verify dialog appears with layer tree and checkboxes |
| Reimport preserves manual edits | EDITOR-04 | Requires manual WBP editing then reimport | Import PSD, edit WBP manually, modify PSD, reimport, verify edits preserved |
| Context menu works | EDITOR-05 | Content Browser interaction | Right-click PSD texture, verify "Import as Widget Blueprint" appears |
| CommonUI button produced | CUI-01 | Requires CommonUI plugin enabled | Enable bUseCommonUI, import Button_ layer, verify UCommonButtonBase created |
| Input action binding | CUI-02 | Requires UInputAction asset | Import Button_Confirm[IA_Confirm], verify binding on widget |
| Animation generation | CUI-03 | Visual verification in animation editor | Import _show/_hide/_hover variants, verify UWidgetAnimation created |
| ScrollBox content | CUI-04 | Runtime layout verification | Import ScrollBox_ layer, play in editor, verify scroll works |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending

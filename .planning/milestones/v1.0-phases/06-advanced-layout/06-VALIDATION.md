---
phase: 6
slug: advanced-layout
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-10
---

# Phase 6 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | UE Automation (FAutomationTestBase) |
| **Config file** | Source/PSD2UMG/Tests/ directory |
| **Quick run command** | `UE5 -RunTests="PSD2UMG"` |
| **Full suite command** | `UE5 -RunTests="PSD2UMG" -ExecCmds="Quit"` |
| **Estimated runtime** | ~30 seconds |

---

## Sampling Rate

- **After every task commit:** Compile check (`UBT Build`)
- **After every plan wave:** Run full automation spec
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 06-01-01 | 01 | 1 | LAYOUT-01 | unit | grep `ESlateBrushDrawType::Box` | ❌ W0 | ⬜ pending |
| 06-01-02 | 01 | 1 | LAYOUT-02 | unit | grep `_9s` suffix parsing | ❌ W0 | ⬜ pending |
| 06-02-01 | 02 | 1 | LAYOUT-04 | unit | grep `SmartObject` in PsdTypes.h | ❌ W0 | ⬜ pending |
| 06-02-02 | 02 | 1 | LAYOUT-05 | unit | grep rasterize fallback | ❌ W0 | ⬜ pending |
| 06-03-01 | 03 | 2 | LAYOUT-03 | integration | grep HBox/VBox heuristic | ❌ W0 | ⬜ pending |
| 06-04-01 | 04 | 2 | LAYOUT-06 | unit | grep `_variants` suffix | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] Existing test infrastructure (FPsdParserSpec, FWidgetBlueprintGenSpec) covers framework needs
- [ ] Test PSD fixtures may need Smart Object layers added

*Existing infrastructure covers most phase requirements — extend existing spec files.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| 9-slice visual rendering | LAYOUT-01 | Visual appearance in UMG Designer | Import PSD with _9s layer, verify Box draw mode in Designer |
| Smart Object child WBP opens | LAYOUT-04 | UMG Designer interactivity | Import PSD with Smart Object, open child WBP in Designer |
| HBox/VBox auto-detection | LAYOUT-03 | Visual layout correctness | Import PSD with aligned layers, verify HBox/VBox in widget tree |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending

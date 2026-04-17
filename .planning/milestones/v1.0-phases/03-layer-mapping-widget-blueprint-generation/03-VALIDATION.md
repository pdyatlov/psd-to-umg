---
phase: 3
slug: layer-mapping-widget-blueprint-generation
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-09
---

# Phase 3 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | UE Automation (FAutomationTestBase / FAutomationSpec) — editor-only |
| **Config file** | none — UE Session Frontend runs specs |
| **Quick run command** | `SessionFrontend → Automation → filter "PSD2UMG" → Run Selected` |
| **Full suite command** | Same — all PSD2UMG specs |
| **Estimated runtime** | ~30–60 seconds per spec run |

---

## Sampling Rate

- **After every task commit:** Manual compile check (UBT) + inspect Output Log for new warnings
- **After every plan wave:** Full UE Automation spec run against the phase fixture PSD
- **Before `/gsd:verify-work`:** Full suite must be green (zero fails in Session Frontend)
- **Max feedback latency:** ~60 seconds (compile + editor load + spec run)

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 03-01-01 | 01 | 1 | MAP-01 | manual | UBT compile | ❌ W0 | ⬜ pending |
| 03-01-02 | 01 | 1 | TEX-01,TEX-02 | spec | UE Automation | ❌ W0 | ⬜ pending |
| 03-02-01 | 02 | 1 | MAP-02,MAP-04 | spec | UE Automation | ❌ W0 | ⬜ pending |
| 03-02-02 | 02 | 1 | MAP-05,MAP-06 | spec | UE Automation | ❌ W0 | ⬜ pending |
| 03-02-03 | 02 | 1 | MAP-07..12 | spec | UE Automation | ❌ W0 | ⬜ pending |
| 03-03-01 | 03 | 2 | WBP-01,WBP-06 | spec | UE Automation | ❌ W0 | ⬜ pending |
| 03-03-02 | 03 | 2 | WBP-02,WBP-05 | spec | UE Automation | ❌ W0 | ⬜ pending |
| 03-04-01 | 04 | 2 | WBP-03,WBP-04 | spec | UE Automation | ❌ W0 | ⬜ pending |
| 03-05-01 | 05 | 3 | MAP-03,TEXT-01 | spec | UE Automation | ❌ W0 | ⬜ pending |
| 03-end   | E2E | 3 | All MAP+WBP+TEX | manual | Open WBP in UMG Designer | manual | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `Source/PSD2UMG/Tests/FLayerMappingSpec.cpp` — spec stubs covering mapper dispatch, texture import, WBP creation
- [ ] Test fixture PSD extended from `MultiLayer.psd` (Phase 2) to include Button_, Progress_, and HBox_ layer groups
- [ ] Build.cs updated with any new module deps required for WBP creation

*Wave 0 lays test infrastructure that later waves fill in.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| WBP opens in UMG Designer without errors | WBP-06 | Requires editor UI interaction | Drag test PSD → Content Browser → double-click generated WBP → verify it opens |
| Widget hierarchy is correct in Designer | WBP-01 | Visual inspection of panel tree | Check Hierarchy panel matches PSD group structure |
| Textures visible on UImage widgets | MAP-02 | Visual verification | Open WBP → Viewport → confirm images render |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending

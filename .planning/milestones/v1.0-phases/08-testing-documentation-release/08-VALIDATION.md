---
phase: 8
slug: testing-documentation-release
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-13
---

# Phase 8 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | UE FAutomationTestBase (Spec framework — DEFINE_SPEC / It / BeforeEach) |
| **Config file** | No config file — registered via IMPLEMENT_SIMPLE_AUTOMATION_TEST macros |
| **Quick run command** | Unreal Editor → Session Frontend → Automation tab → filter "PSD2UMG" → Run |
| **Full suite command** | Same — all PSD2UMG specs |
| **Estimated runtime** | ~30-60 seconds (UE editor must be open) |

---

## Sampling Rate

- **After every task commit:** Verify file exists and compiles (no editor required for file checks)
- **After every plan wave:** Run full PSD2UMG automation suite in UE Session Frontend
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 60 seconds (compile + run)

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | Status |
|---------|------|------|-------------|-----------|-------------------|--------|
| 8-01 | 01 | 1 | TEST-01 | automation spec | FPsdParserSpec — new It() blocks | ⬜ pending |
| 8-02 | 01 | 1 | TEST-02 | automation spec | FWidgetBlueprintGenSpec — new It() blocks | ⬜ pending |
| 8-03 | 02 | 2 | TEST-03 | fixture creation | File exists at Tests/Fixtures/SimpleHUD.psd | ⬜ pending |
| 8-04 | 02 | 2 | TEST-03 | fixture creation | File exists at Tests/Fixtures/ComplexMenu.psd | ⬜ pending |
| 8-05 | 02 | 2 | TEST-03 | fixture creation | File exists at Tests/Fixtures/Effects.psd | ⬜ pending |
| 8-06 | 03 | 3 | DOC-01 | file exists | README.md contains "## Installation" | ⬜ pending |
| 8-07 | 03 | 3 | DOC-02 | file exists | README.md contains layer naming cheat sheet table | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements — FAutomationTestBase specs already registered, no new test framework needed.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| PSD fixture imports successfully in UE | TEST-03 | Requires UE editor + import pipeline | Drag SimpleHUD.psd into Content Browser, verify WBP is created |
| README renders correctly on GitHub | DOC-01 | No automated markdown renderer in CI (CI skipped per D-04) | Preview README.md in browser |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending

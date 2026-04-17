---
phase: 5
slug: layer-effects-blend-modes
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-10
---

# Phase 5 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | FAutomationTestBase / Spec-style (UE5 automation) |
| **Config file** | Build.cs `bBuildDeveloperTools = true` + module type Editor |
| **Quick run command** | UE automation: `-ExecCmds="Automation RunTests PSD2UMG.Generator"` |
| **Full suite command** | UE automation: `-ExecCmds="Automation RunTests PSD2UMG"` |
| **Estimated runtime** | ~10 seconds |

---

## Sampling Rate

- **After every task commit:** Run `PSD2UMG.Generator` spec suite
- **After every plan wave:** Run full `PSD2UMG` automation suite
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 10 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 05-01-01 | 01 | 1 | FX-01 (struct), FX-03, FX-04 | unit spec | UE automation PSD2UMG.Generator | N/A (parser data struct, no direct test) | pending |
| 05-01-02 | 01 | 1 | FX-03, FX-04, FX-05 | unit spec | UE automation PSD2UMG.Generator | N/A (parser extraction, tested via Plan 02) | pending |
| 05-02-01 | 02 | 2 | FX-01, FX-02, FX-03, FX-04, FX-05 | unit spec | UE automation PSD2UMG.Generator | FWidgetBlueprintGenSpec.cpp (update + new) | pending |
| 05-02-02 | 02 | 2 | FX-01, FX-02, FX-03, FX-05 | unit spec | UE automation PSD2UMG.Generator | FWidgetBlueprintGenSpec.cpp (new tests) | pending |

*Status: pending / green / red / flaky*

---

## Wave 0 Requirements

- [ ] New `It("should apply opacity < 1.0 via SetRenderOpacity")` in `FWidgetBlueprintGenSpec.cpp`
- [ ] Update `It("should skip invisible layers")` -> assert 1 child with `Visibility==Collapsed`
- [ ] New `It("should apply color overlay tint to UImage brush")` in `FWidgetBlueprintGenSpec.cpp`
- [ ] New `It("should create shadow sibling for drop shadow layers")` — conditional on access path
- [ ] New `It("should flatten layer with complex effects when setting enabled")` in `FWidgetBlueprintGenSpec.cpp`
- [ ] `Test_Effects.psd` fixture in `Source/PSD2UMG/Tests/Fixtures/`

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Drop shadow visual fidelity | FX-04 | Visual offset accuracy requires UMG Designer inspection | Open generated WBP, verify shadow UImage is behind and offset from main widget |
| Flatten visual result | FX-05 | Composited image quality requires visual comparison with PSD | Compare flattened UImage with expected PSD layer+effects composite |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 10s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending

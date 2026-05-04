---
phase: 23
slug: pattern-fill-layers
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-05-04
---

# Phase 23 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Unreal Automation Spec (`BEGIN_DEFINE_SPEC` / `EAutomationTestFlags`) |
| **Config file** | Configured via UE Editor / Session Frontend — no separate config file |
| **Quick run command** | `-ExecCmds="Automation RunTests PSD2UMG.Mapper.PatternFillLayerMapper"` |
| **Full suite command** | `-ExecCmds="Automation RunTests PSD2UMG"` |
| **Estimated runtime** | ~5 seconds (mapper spec only; no PSD fixture needed) |

---

## Sampling Rate

- **After every task commit:** Run `PSD2UMG.Mapper.PatternFillLayerMapper`
- **After every plan wave:** Run full `PSD2UMG` suite
- **Before `/gsd:verify-work`:** Full suite must be green

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 23-01-01 | 01 | 1 | PTFL-01 | unit (spec stub, fixture-gated) | `PSD2UMG.Parser.PatternFill` | ❌ Wave 0 | ⬜ pending |
| 23-01-02 | 01 | 1 | PTFL-02 | unit | `PSD2UMG.Mapper.PatternFillLayerMapper` | ❌ Wave 0 | ⬜ pending |
| 23-01-03 | 01 | 1 | PTFL-02 (fallback) | unit | `PSD2UMG.Mapper.PatternFillLayerMapper` | ❌ Wave 0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `FPsdParserPatternSpec` block in `Source/PSD2UMG/Tests/PsdParserSpec.cpp` — covers PTFL-01 (fixture-gated; follows `FPsdParserCJKSpec` "fixture absent → AddWarning + short-circuit" pattern)
- [ ] `FPatternFillLayerMapperSpec` block — covers PTFL-02 positive case + empty-pixels fallback (synthetic `FPsdLayer`, no fixture needed); may live in `PsdParserSpec.cpp` or a new `FPatternFillLayerMapperSpec.cpp`

*Existing test infrastructure: framework present, `TestHelpers.h` available, `MakeTaggedTestLayer` helper verified*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Pattern fill layer renders correctly in UMG preview | PTFL-02 | No real PatternFill.psd fixture available this phase | Supply a PSD with a pattern fill layer; import via PSD2UMG; verify UImage widget appears at correct position/size in Widget Blueprint |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending

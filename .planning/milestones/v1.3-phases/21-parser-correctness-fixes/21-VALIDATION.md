---
phase: 21
slug: parser-correctness-fixes
status: planned
nyquist_compliant: true
wave_0_complete: false
created: 2026-04-28
updated: 2026-04-28
---

# Phase 21 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | UE Automation Spec (`BEGIN_DEFINE_SPEC` / `END_DEFINE_SPEC`) |
| **Config file** | `PSD2UMG.uplugin` + UE Test Runner (no separate config file) |
| **Quick run command** | `-ExecCmds="Automation RunTests PSD2UMG.Parser"` |
| **Full suite command** | `-ExecCmds="Automation RunTests PSD2UMG"` |
| **Estimated runtime** | ~30 seconds |

---

## Sampling Rate

- **After every task commit:** Run `Automation RunTests PSD2UMG.Parser`
- **After every plan wave:** Run `Automation RunTests PSD2UMG`
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** ~30 seconds

---

## Per-Task Verification Map

| Task ID    | Plan  | Wave | Requirement | Test Type     | Automated Command                                    | File Exists | Status     |
|------------|-------|------|-------------|---------------|------------------------------------------------------|-------------|------------|
| 21-01-T01  | 21-01 | 1    | RTXT-01     | unit          | `Automation RunTests PSD2UMG.Parser.CJK`             | W0 fixture  | ⬜ pending |
| 21-01-T02  | 21-01 | 1    | RTXT-01     | unit          | `Automation RunTests PSD2UMG.Parser`                 | ✅          | ⬜ pending |
| 21-02-T01  | 21-02 | 2    | LFXC-01     | build         | `Build.bat PSD2UMGEditor`                            | ✅          | ⬜ pending |
| 21-02-T02  | 21-02 | 2    | LFXC-01     | unit + manual | `Automation RunTests PSD2UMG`; HSB hue manual        | ✅ + manual | ⬜ pending |
| 21-03-T01  | 21-03 | 3    | FXFMT-01    | unit          | `Automation RunTests PSD2UMG`                        | ✅          | ⬜ pending |
| 21-03-T02  | 21-03 | 3    | FXFMT-01    | unit + manual | `Automation RunTests PSD2UMG`; VlLs PSD manual       | ✅ + manual | ⬜ pending |
| 21-04-T01  | 21-04 | 4    | LFXC-02     | manual-only   | n/a — human visual UAT on real UE 5.7 host           | manual      | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `Source/PSD2UMG/Tests/Fixtures/RichTextCJK.psd` — user-supplied before Plan 21-01 execution starts (CONTEXT.md D-01). Plan 21-01 Task 1 spec block degrades gracefully (AddWarning + skip) if missing, so absence does NOT fail CI but DOES leave RTXT-01 unverified.
- [x] New `Describe("ParseFile on RichTextCJK.psd")` Describe block in `PsdParserSpec.cpp` — covered by Plan 21-01 Task 1.

*Existing `PsdParserSpec.cpp` and `FTextEffectsSpec.cpp` infrastructure covers all other automated requirements.*

---

## Manual-Only Verifications

| Behavior                                                          | Requirement | Why Manual                                          | Test Instructions                                                                                                                                  |
|-------------------------------------------------------------------|-------------|-----------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| sofi ColorSpace=1 (HSB) overlay renders correct hue               | LFXC-01     | No HSB Effects PSD fixture in Fixtures/             | Import a PSD with HSB color overlay on UE 5.7 host; visually verify the resulting ColorOverlayColor matches Photoshop hue (within ±5% per channel). |
| FrFX VlLs stroke parsed (Photoshop CC 2014+ file)                 | FXFMT-01    | No VlLs-format PSD fixture in Fixtures/             | Import a CC 2014+ PSD with stroke effect; verify FPsdStrokeInfo.bEnabled = true via debugger or layer-effects log lines (LogPSD2UMG Verbose).      |
| Color overlay and drop shadow visual output correct for ColorSpace=0 | LFXC-02   | Human visual comparison on real host project        | Plan 21-04 — fully scripted as a checkpoint task; outcome recorded in 21-04-UAT-LOG.md.                                                            |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies (21-04-T01 uses an automated grep on the UAT log file as its `<automated>` gate after the human resumes).
- [x] Sampling continuity: no 3 consecutive tasks without automated verify (every task in 21-01/02/03 has a Build.bat or Automation RunTests verify; 21-04 has a Node grep verify on the UAT log).
- [x] Wave 0 covers all MISSING references (RichTextCJK.psd is the only Wave 0 dependency and is gated by Plan 21-01 with graceful skip).
- [x] No watch-mode flags (all commands run once and exit).
- [x] Feedback latency < 30s (all automated paths under 30s).
- [x] `nyquist_compliant: true` set in frontmatter.

**Approval:** approved — 2026-04-28 (planning agent)

---
phase: 4
slug: text-fonts-typography
status: draft
nyquist_compliant: false
wave_0_complete: true
created: 2026-04-09
---

# Phase 4 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

**Wave 0 note:** There is no separate Wave 0 stub plan. The Typography.psd fixture is created in Plan 01 Task 1 (Wave 1) and the FTextPipelineSpec.cpp E2E spec is created in Plan 03 Task 2 (Wave 3). Parser-level spec assertions are added in Plan 01 Task 2 alongside the parser extension. Each consumer task compiles its own test on first touch, so no scaffolding-only wave is needed. `wave_0_complete: true` because the validation infrastructure is fully defined by the plans themselves.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | UE Automation (FAutomationTestBase / FAutomationSpec) — editor-only |
| **Config file** | none — UE Session Frontend runs specs |
| **Quick run command** | `SessionFrontend → Automation → filter "PSD2UMG.Text" → Run Selected` |
| **Full suite command** | Same — all PSD2UMG specs including Phase 2/3 regression |
| **Estimated runtime** | ~30–60 seconds per spec run |

---

## Sampling Rate

- **After every task commit:** UBT compile check + grep structural checks on modified files
- **After every plan wave:** Full UE Automation spec run against Phase 4 fixture PSD
- **Before `/gsd:verify-work`:** Full suite green
- **Max feedback latency:** ~60 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 04-01-01 | 01 | 1 | — (fixture) | manual | Hand-crafted Typography.psd | created in-task | ⬜ pending |
| 04-01-02 | 01 | 1 | TEXT-02, TEXT-06 (parser) | spec | UE Automation (parser) | created in-task | ⬜ pending |
| 04-02-01 | 02 | 2 | TEXT-05 | spec | UE Automation (resolver) | created in-task | ⬜ pending |
| 04-02-02 | 02 | 2 | TEXT-02 (mapper) | spec | UE Automation (mapper) | created in-task | ⬜ pending |
| 04-03-1a | 03 | 3 | TEXT-03, TEXT-04 | spec | UE Automation (mapper) | created in-task | ⬜ pending |
| 04-03-1b | 03 | 3 | TEXT-06 (generator) | spec | UE Automation (generator) | created in-task | ⬜ pending |
| 04-03-02 | 03 | 3 | All TEXT-* e2e | spec | UE Automation (pipeline) | created in-task | ⬜ pending |
| 04-end   | E2E | 3 | All TEXT-* | manual | Open WBP in UMG Designer | manual | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

**Row 04-01-02 note:** Plan 01 delivers TEXT-02 and TEXT-06 at the PARSER level only (surfaces bBold/bItalic/bHasExplicitWidth/BoxWidthPx on FPsdTextRun). The MAPPER/GENERATOR-level delivery lives in 04-02-02 (bold/italic typeface application) and 04-03-1a/1b (wrap + slot sizing). Both plans jointly own these requirement IDs in their frontmatter.

---

## Wave 0 Requirements

None — this phase has no separate Wave 0 stub plan. See note at top of file.

Items previously listed as Wave 0 are now delivered inline:
- `Source/PSD2UMG/Tests/Fixtures/Typography.psd` — created in Plan 01 Task 1
- `Source/PSD2UMG/Tests/FTextPipelineSpec.cpp` — created in Plan 03 Task 2
- Minimal UFont test asset — covered by engine-default Roboto fallback per Plan 02

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Text renders with correct font in UMG Designer | TEXT-05 | Visual inspection | Drop Typography.psd → open generated WBP → verify each text widget uses the correct UFont asset |
| Bold / italic visually match PSD | TEXT-02 | Visual inspection | Compare PSD source to Designer viewport |
| Stroke outline visible | TEXT-03 | Visual inspection + may need fixture tuning | Red-stroke text layer should render with red outline in Designer |
| Paragraph wrap works | TEXT-06 | Visual inspection | Long-text paragraph layer wraps within its box width |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references (none needed; inline delivery)
- [x] No watch-mode flags
- [x] Feedback latency < 60s
- [ ] `nyquist_compliant: true` set in frontmatter (pending full spec review)

**Approval:** pending

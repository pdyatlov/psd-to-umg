---
phase: 22
slug: stroke-rendering
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-29
updated: 2026-04-29
---

# Phase 22 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | UE Automation Test (Spec API via `BEGIN_DEFINE_SPEC`) |
| **Config file** | None — engine discovers specs via EAutomationTestFlags |
| **Quick run command** | `PSD2UMG.Parser.*` specs via editor Automation window or CLI |
| **Full suite command** | All `PSD2UMG.*` specs in editor Automation window or CLI |
| **Estimated runtime** | ~30 seconds |

---

## Sampling Rate

- **After every task commit:** Run `PSD2UMG.Parser.*` to catch parser regressions
- **After every plan wave:** Run full `PSD2UMG.*` suite
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** ~30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 22-01-01 | 01 | 1 | STROKE-02 | unit (parser spec, positive) | `PSD2UMG.Parser.VstkStroke` | **Deferred per D-04** — no Stroke.psd fixture; positive vstk assertions deferred until fixture provided | ⬛ deferred |
| 22-01-02 | 01 | 1 | STROKE-02 | regression | `PSD2UMG.Parser.ButtonStyles` | ❌ Wave 0 | ⬜ pending |
| 22-02-01 | 02 | 2 | STROKE-01 | unit (generator spec — Image + Shape residual lfx2) | `PSD2UMG.Generator` (existing namespace; new It() blocks) | ❌ Wave 0 | ⬜ pending |
| 22-02-02 | 02 | 2 | STROKE-03 | unit (generator spec — Shape vstk) | `PSD2UMG.Generator` (existing namespace; new It() block) | ❌ Wave 0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky · ⬛ deferred*

### Deferred Test Rationale (22-01-01)

`PSD2UMG.Parser.VstkStroke` was originally listed as a Wave 0 deliverable but is now
**marked deferred per D-04** (CONTEXT.md): no positive stroke fixture is available. A
positive parser spec for ScanVstkStroke requires a PSD file containing a Shape layer with
a vstk (vecStrokeData) tagged block — none of the existing fixtures (ButtonStyles.psd,
SimpleHUD.psd, Effects.psd, etc.) carry one.

Plan 22-01 Task 2 instead delivers the **negative regression** spec (`PSD2UMG.Parser.ButtonStyles`)
which proves no spurious vstk activation occurs on a stroke-free fixture — necessary and
sufficient for STROKE-02 success criterion 4. The positive code path is exercised at the
generator layer via synthetic in-memory `FPsdLayerEffects` setup in
`Source/PSD2UMG/Tests/FWidgetBlueprintGenSpec.cpp` (plan 22-02 Task 2) which sets the
`bHasVectorStroke`/`VectorStrokeSize`/`VectorStrokeColor` fields directly without going
through the parser — adequate spec-level substitute permitted by D-04.

When a Stroke.psd fixture is later provided, add `FPsdParserVstkStrokeSpec` registered as
`PSD2UMG.Parser.VstkStroke` with positive It() blocks asserting:
- A Shape layer with a vstk block has `Effects.bHasVectorStroke == true`
- `Effects.VectorStrokeSize > 0` matches the PSD-authored stroke width
- `Effects.VectorStrokeColor.A == 1.0` and RGB matches authored color
- `Effects.bHasStroke == false` on the same Shape layer (D-03 guard verification)
- An Image layer with a hypothetical vstk block has `Effects.bHasVectorStroke == false`
  (Pitfall 4 — call site is type-scoped to Shape)

Until then, this test ID remains deferred. No stub spec is added to PsdParserSpec.cpp because
D-04 explicitly accepts the spec-level substitute path; a no-op stub would be dead code.

---

## Wave 0 Requirements

- [ ] `Source/PSD2UMG/Tests/PsdParserSpec.cpp` — add `FPsdParserButtonStylesSpec` for ButtonStyles.psd non-regression (success criterion 4: no `bHasVectorStroke` on any layer)
- [ ] `Source/PSD2UMG/Tests/FWidgetBlueprintGenSpec.cpp` — add four new It() blocks under existing `FWidgetBlueprintGenSpec::Define` for STROKE-01 (Image + Shape lfx2 residual), STROKE-03 (Shape vstk), and non-canvas guard

*Existing `FPsdParserEffectsSpec` and `FPsdParserSpec` require no changes. The deferred
`PSD2UMG.Parser.VstkStroke` (positive vstk parser spec) is NOT a Wave 0 deliverable — see
Per-Task Verification Map deferred rationale above.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Visual stroke visible on shape layer in UE editor | STROKE-03 | No fixture PSD with vstk block; D-04 defers positive fixture | Import a PSD with a shape+stroke when Stroke.psd fixture available; verify sibling UImage renders behind main widget |
| Image layer lfx2 stroke sibling renders correctly | STROKE-01 | No fixture PSD with lfx2 image stroke; D-04 | Same as above |
| Shape layer lfx2-residual stroke (no vstk) renders correctly | STROKE-01 (Shape residual case) | No fixture PSD with lfx2 stroke on a shape but no vstk block; D-04 | Same as above |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references (deferred items explicitly marked)
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending

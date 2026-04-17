---
phase: 9
slug: unified-layer-naming-convention-tag-based
status: draft
nyquist_compliant: true
wave_0_complete: true
created: 2026-04-14
---

# Phase 9 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | UE Automation (existing PSD2UMG spec suite from Phase 8) |
| **Config file** | `Source/PSD2UMG/PSD2UMG.Build.cs` (test deps), spec files under `Source/PSD2UMG/Private/Tests/` |
| **Quick run command** | `UnrealEditor-Cmd.exe <Project> -ExecCmds="Automation RunTests PSD2UMG.Parser+PSD2UMG.Mapper; quit" -unattended -nopause -log` |
| **Full suite command** | `UnrealEditor-Cmd.exe <Project> -ExecCmds="Automation RunTests PSD2UMG.+; quit" -unattended -nopause -log` |
| **Estimated runtime** | ~30s quick / ~120s full (rough — Phase 8 baseline) |

---

## Sampling Rate

- **After every task commit:** Run quick command (Parser + Mapper specs)
- **After every plan wave:** Run full suite
- **Before `/gsd:verify-work`:** Full suite must be green; manual smoke import of one retagged fixture PSD in editor
- **Max feedback latency:** ~30s

---

## Per-Task Verification Map

*Filled by gsd-planner during planning. Each task gets a row with its automated command and the spec file(s) it must turn green.*

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | Spec File | Status |
|---------|------|------|-------------|-----------|-------------------|-----------|--------|
| 09-01/T1 | 01 | 1 | TAG-01, TAG-03 | build | `Build.bat PSD2UMGEditor Win64 Development` | n/a (header+doc) | ⬜ pending |
| 09-01/T2 | 01 | 1 | TAG-01, TAG-07 | unit spec | `Automation RunTests PSD2UMG.Parser.LayerTagParser` | `Source/PSD2UMG/Private/Tests/FLayerTagParserSpec.cpp` | ⬜ pending |
| 09-02/T1 | 02 | 2 | TAG-02 | checkpoint:decision | n/a (human decision gate) | n/a | ⬜ pending |
| 09-02/T2 | 02 | 2 | TAG-02, TAG-08 | integration spec | `Automation RunTests PSD2UMG.Mapper` | mapper specs (all) + `FLayerTagParserSpec.cpp` CommonUI priority case | ⬜ pending |
| 09-02/T3 | 02 | 2 | TAG-02, TAG-07 | integration spec | `Automation RunTests PSD2UMG.Anchor PSD2UMG.WidgetBlueprintGen` | `FWidgetBlueprintGenSpec.cpp` (anchor heuristic + `@anchor:stretch-h` override case) | ⬜ pending |
| 09-02/T4 | 02 | 2 | TAG-02 | integration spec | `Automation RunTests PSD2UMG.Animation` | `FPsdWidgetAnimationBuilderSpec.cpp` (if exists) + AnimationBuilder coverage | ⬜ pending |
| 09-02/T6a | 02 | 2 | TAG-09 | unit spec + human-verify | `Automation RunTests PSD2UMG.Parser.LayerTagParser` (ReconstructTagChips case) + Task 6 dialog smoke | `FLayerTagParserSpec.cpp` + `SPsdImportPreviewDialog.cpp` | ⬜ pending |
| 09-02/T5 | 02 | 2 | TAG-02 | full suite | `Automation RunTests PSD2UMG.` | all specs under `Source/PSD2UMG/Private/Tests/` | ⬜ pending |
| 09-02/T6 | 02 | 2 | TAG-02 | checkpoint:human-verify | n/a (editor smoke) | n/a | ⬜ pending |
| 09-03/T1 | 03 | 3 | TAG-05 | artifact check | `ls .planning/phases/09-*/09-03-RETAG-CHECKLIST.md` | n/a (planning doc) | ⬜ pending |
| 09-03/T2 | 03 | 3 | TAG-05 | checkpoint:human-action | n/a (Photoshop manual retag) | n/a | ⬜ pending |
| 09-03/T3 | 03 | 3 | TAG-05 | full suite | `Automation RunTests PSD2UMG.` | `PsdParserSpec.cpp`, `FWidgetBlueprintGenSpec.cpp`, `FTextPipelineSpec.cpp`, `FFontResolverSpec.cpp` | ⬜ pending |
| 09-03/T4 | 03 | 3 | TAG-05 | checkpoint:human-verify | n/a (editor fixture smoke) | n/a | ⬜ pending |
| 09-04/T1 | 04 | 4 | TAG-04 | artifact grep | `grep -q "@button" Docs/Migration-PrefixToTag.md && grep -q "LayerTagGrammar" Docs/Migration-PrefixToTag.md` | n/a (docs) | ⬜ pending |
| 09-04/T2 | 04 | 4 | TAG-06 | artifact grep | `! grep -qE "Button_\|_anchor-\|\[IA_\|_9s\|_hovered" README.md` | n/a (docs) | ⬜ pending |

---

## Wave 0 Requirements

- [x] `Source/PSD2UMG/Private/Tests/FLayerTagParserSpec.cpp` — stub spec covering grammar EBNF cases (D-01 through D-21) — **owned by Plan 01 Task 2**
- [x] Test helper to construct synthetic `FParsedLayerTags` for unit-testing mappers without a real PSD — **owned by Plan 02 Task 5 (`MakeTaggedTestLayer`)**
- [x] Confirm Phase 8 spec runner config still picks up the new spec file (no Build.cs changes expected, but verify) — **owned by Plan 01 Task 2 build verify**

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Retagged fixture PSD imports cleanly in editor | D-24 | PSD binary regeneration is human-only; end-to-end import touches editor UI | Import each retagged fixture in UE editor, verify generated WBP matches Phase 8 baseline screenshots |
| Migration guide accuracy | D-23 | Doc review | Cross-check every old-syntax row in `Docs/Migration-PrefixToTag.md` against deleted code paths; confirm every deletion has a migration entry |
| README naming section reflects only new grammar | D-25 | Doc review | Read `README.md` lines 39-96; ensure no `Button_`, `_anchor-tl`, `_9s`, `[IA_X]`, `_show`/`_hide`/`_hover` examples remain |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 30s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending (sign-off date TBD)

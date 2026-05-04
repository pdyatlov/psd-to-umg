# Project Retrospective

*A living document updated after each milestone. Lessons feed forward into future planning.*

## Milestone: v1.0.1 — Panel Child Attachment Hotfix

**Shipped:** 2026-04-17
**Phases:** 1 (Phase 10) | **Plans:** 3 | **~32 commits**

### What Was Built
- `PopulateChildren(UPanelWidget*)` — refactored from `PopulateCanvas(UCanvasPanel*)` with panel-type dispatch for Generate path
- `UpdateCanvas(UPanelWidget*)` — reimport path generalized; clear-and-rebuild strategy for non-canvas groups
- `FPanelAttachmentSpec` — 5-case automation spec covering VBox/HBox/ScrollBox attach + slot-type assertion + reimport sentinel
- `Panels.psd` — 229 KB human-authored fixture with VBox/HBox/ScrollBox groups and mixed child types

### What Worked
- **Context doc (10-CONTEXT.md) + 3-plan breakdown**: The pre-planning separation of Generate path (10-01), Update path (10-02), and fixtures (10-03) made each plan execute in ~2 minutes with zero deviations.
- **Double-guard pattern (Cast<UCanvasPanel> + Cast<UCanvasPanelSlot>)**: Self-documenting and defensive even when the first cast guarantees the second; good signal for reviewers.
- **Scope trim process**: User-directed removal of Overlay/Canvas/Nested spec cases was handled cleanly — decisions logged in SUMMARY, deferred items listed in ROADMAP backlog. No ambiguity left.

### What Was Inefficient
- **Diagnostic commit sprawl**: The diag(10/20/30) and fix(generator) commits in the git log suggest the generator crash (duplicate widget names) was not anticipated during planning. This added ~10 commits of noise to a 3-task hotfix. Pre-flight readback of the generator's name-collision handling would have caught this.
- **Human-in-the-loop fixture step**: Panels.psd required leaving Claude to author in Photoshop. Not avoidable for a binary asset, but the spec design could be verified earlier to reduce back-and-forth.

### Patterns Established
- **`FPaths::FileExists` fixture gate**: All spec files now gate their It() blocks on fixture existence — established in Phase 5/6, reinforced here. This prevents false-failures in CI when binary fixtures aren't present.
- **Clear-and-rebuild for non-canvas reimport (D-11)**: Documented as the canonical strategy for panel types that carry no PSD slot state. Use `ClearChildren` + `PopulateChildren` for any non-canvas group on reimport.

### Key Lessons
1. **Name collision in the generator is a pre-existing hazard**: Any PSD with duplicate layer names at the same level will hit the widget-name conflict. This should be part of the standard pre-flight checklist when writing generator tests.
2. **Scope trims are cheaper than scope creep**: Removing 3 spec cases (Overlay/Canvas/Nested) kept the milestone focused. The deferred items are test-coverage gaps only — the implementation works. Log the trim explicitly so it doesn't look like a gap later.

### Cost Observations
- Model mix: ~balanced (sonnet for execution, opus for planning)
- Notable: Hotfix milestone completed end-to-end (plan → spec → fixture → archive) in approximately 1 session day

---

## Milestone: v1.3 — Advanced Effects

**Shipped:** 2026-05-04
**Phases:** 3 (Phases 21-23) | **Plans:** 8 | **~35 commits** | **38 files changed, 5,744 insertions, 1,697 deletions**
**Timeline:** 2026-04-28 → 2026-05-04 (7 days)

### What Was Built

- **Phase 21 (Parser Correctness):** CJK/emoji null-sentinel fix in `Utf8ToFString` + `FPsdParserCJKSpec`; `ConvertLfx2Color` HSB→RGB via `HSVToLinearRGB` for lrFX effects; `ParseFrFXDescriptor` VlLs branch for Photoshop CC 2014+ effects format; LFXC-02 UAT deferred (code correct, live-host confirm pending)
- **Phase 22 (Stroke Rendering):** `ScanVstkStroke` binary descriptor walker at offset 0 → `bHasVectorStroke/VectorStrokeSize/VectorStrokeColor`; lfx2 stroke sibling UImage for image layers (STROKE-01); vstk stroke sibling for shape layers (STROKE-03) with D-03 double-emit guard
- **Phase 23 (Pattern Fill):** `EPsdLayerType::PatternFill` enum + `adjPattern` detection in `ConvertLayerRecursive` via Adj-cast `ExtractImagePixels`; `FPatternFillLayerMapper` at priority 101 with 8-case CanMap spec and D-04 nullptr fallback

### What Worked

- **Detailed CONTEXT.md + RESEARCH.md per phase**: Critical pitfalls (CP-01 vstk offset=0, CP-02 separate bHasVectorStroke field, CP-04 PtFl no Clr key) were documented before execution. Zero pitfall-related rework during execution.
- **TDD fixture-gated spec pattern (D-05)**: Parser and mapper specs compile and run immediately; fixture absence triggers AddWarning + short-circuit rather than test failure. CI stays green without binary PSD assets.
- **Priority-101 fill/shape mapper tier**: The existing priority architecture from v1.2 meant adding PatternFill mapper required only one file (FPatternFillLayerMapper.cpp) + declaration + registration. No architectural change needed.
- **Wave-based parallel execution**: Plans 23-01 and 23-02 ran sequentially (dependency) but phases 21/22/23 planned independently. Each wave spawned fresh executor agents with isolated worktrees.

### What Was Inefficient

- **LFXC-02 deferred again**: Visual UAT on a live UE 5.7 host requires physical project setup that wasn't available. This is the second deferral. If not resolved in v1.4, it will become stale context.
- **22-02 SUMMARY one-liner was empty**: The executor agent's summary file for plan 22-02 had a malformed `One-liner:` field. Minor quality gap; doesn't affect function but degrades retrospective data.
- **No PatternFill.psd fixture**: D-05 explicitly deferred the positive fixture. The spec stubs are correct but the full validation path can't fire until the fixture is authored.

### Patterns Established

- **Adj-cast for fill-type layers (CP-04):** For any adjustment-layer tagged block with no Clr key (PtFl, and potentially future fill types), `dynamic_pointer_cast<AdjustmentLayer<PsdPixelType>>` + `ExtractImagePixels` is the correct pixel extraction path. Do NOT add a descriptor walker.
- **Double-emit guard pattern (D-03):** When two stroke sources can fire on the same layer (lfx2 + vstk), the vstk path wins and explicitly clears `bHasStroke` to prevent the lfx2 path from also firing. Establish winner-takes-all at parse time.
- **`FPsdParserPatternSpec` fixture-gate model:** Mirror `FPsdParserCJKSpec` exactly — same BeforeEach guard, same It() short-circuit, same AddWarning text format. All new parser specs should follow this model.

### Key Lessons

1. **Document critical pitfalls as CPs before planning**: The CP-01/02/03/04/05 system caught offset=0, field-collision, and no-Clr-key mistakes before any code was written. Investment in CONTEXT.md pays back immediately in execution.
2. **Fixture-gated specs are the right v1.3 pattern**: Binary PSD fixtures need human time to author. Spec stubs that compile and run (with graceful skip) let automated validation flow without blocking on asset creation.
3. **LFXC-02 visual UAT needs a physical host project**: Code-level verification is insufficient for color-correctness confirms. Prioritize this in v1.4 before more lrFX work builds on it.

### Cost Observations

- Model mix: sonnet for execution agents, sonnet for verification, opus in planning context
- Notable: 3 phases (8 plans, 14 tasks) executed in 7 days including planning, research, and archival

---

## Cross-Milestone Trends

### Process Evolution

| Milestone | Phases | Plans | Key Change |
|-----------|--------|-------|------------|
| v1.0 | 9 (+1 decimal) | 33 | Full greenfield build of the plugin |
| v1.0.1 | 1 | 3 | First hotfix milestone — tight scope, fast execution |
| v1.1 | 2 | 4 | Import fidelity fixes, text property parity |
| v1.2 | 8 (+3 decimal) | 27 | Layer fidelity expansion — gradients, shapes, effects, rich text, CommonUI |
| v1.3 | 3 | 8 | Advanced effects — stroke, pattern fill, parser correctness, format support |

### Top Lessons (Verified Across Milestones)

1. **Tight phase scoping + context docs = fast execution**: Phases with pre-written CONTEXT.md + RESEARCH.md execute with near-zero deviation. Pitfall documentation (CP-xx) prevents the most expensive class of rework.
2. **Binary fixture assets need human time budgeted explicitly**: PSD/texture fixtures can't be generated by Claude — plan for the human-in-the-loop step in phase estimates.
3. **Priority tiers prevent mapper collision**: The priority-101 fill/shape tier established in v1.2 let v1.3 add a new mapper type (PatternFill) with a single file and no architectural change. Worth investing in the tier system early.
4. **Fixture-gated spec stubs let CI stay green**: The AddWarning short-circuit pattern (FPsdParserCJKSpec model) is the correct pattern for specs that require user-supplied binary assets.

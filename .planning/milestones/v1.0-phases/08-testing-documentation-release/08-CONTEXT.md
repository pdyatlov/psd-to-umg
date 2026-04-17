# Phase 8: Testing, Documentation & Release - Context

**Gathered:** 2026-04-13
**Status:** Ready for planning

<domain>
## Phase Boundary

Make the plugin ship-ready: expand automated test coverage using existing FAutomationTestBase infrastructure, create new PSD test fixtures, write lean internal-focused documentation, and bundle example PSDs in TestData/. No CI/CD for v1.
</domain>

<decisions>
## Implementation Decisions

### Test Coverage (TEST-01, TEST-02)
- **D-01:** Test approach is **expand existing FAutomationTestBase specs** — add more cases to `FPsdParserSpec` and `FWidgetBlueprintGenSpec`. No standalone test binary, no pure-logic extraction. All tests run through UE Automation Framework.
- **D-02:** Edge cases to cover: empty PSD, malformed layer names, nested smart objects, DPI conversion values, anchor heuristic boundary conditions, prefix/suffix parsing edge cases, reimport change detection.
- **D-03:** No new test framework or test runner introduced — stay with what exists.

### CI/CD (DOC-04 / CI-01)
- **D-04:** **No CI/CD for v1.** Build and test steps documented in README (or CONTRIBUTING if it exists). Revisit after release. No GitHub Actions workflow files created.

### Documentation (DOC-01, DOC-02, DOC-03)
- **D-05:** Documentation is **internal-team focused** — lean README covering installation and usage. Assumes the reader is an Unreal Engine developer.
- **D-06:** Docs to create: `README.md` (installation, quick start, layer naming conventions, settings reference). Skip ARCHITECTURE.md, CONTRIBUTING.md, CHANGELOG.md for v1.
- **D-07:** README tone: concise, practical. No hand-holding for UE basics. Link to existing UE docs for engine concepts.

### Example Project / Test PSDs (TEST-03, DOC-02)
- **D-08:** No separate example UE project. Instead, **PSDs are bundled in `TestData/`** (plugin repo root or `Source/PSD2UMG/Tests/Fixtures/`). README explains: drag into your project's Content Browser and import.
- **D-09:** Existing fixtures kept as-is: `MultiLayer.psd`, `Typography.psd`.
- **D-10:** **Three new PSD fixtures to create:**
  - `SimpleHUD.psd` — basic HUD with image background, text labels, health bar (Progress_), button (Button_)
  - `ComplexMenu.psd` — nested groups, variant switcher (_variants), 9-slice panel (_9s), multiple button states
  - `Effects.psd` — color overlay, drop shadow, flatten fallback (layer with complex effect), opacity variation
  - `MobileUI.psd` — **skipped for v1**

### Claude's Discretion
- Exact test case names and assertion structure within FAutomationTestBase specs
- Whether fixture PSDs live in `Source/PSD2UMG/Tests/Fixtures/` or a new top-level `TestData/` directory
- README file structure and section ordering
- How to author new PSD fixtures (Photoshop, Affinity, or programmatically)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Requirements
- `.planning/REQUIREMENTS.md` — TEST-01, TEST-02, TEST-03, TEST-04, DOC-01, DOC-02, DOC-03, DOC-04

### Prior Phase Decisions
- `.planning/phases/02-c-psd-parser/02-CONTEXT.md` — parser decisions, PhotoshopAPI quirks (ARGB color order)
- `.planning/phases/03-layer-mapping-widget-blueprint-generation/03-CONTEXT.md` — mapper architecture, anchor heuristics
- `.planning/phases/07-editor-ui-preview-settings/07-CONTEXT.md` — reimport strategy (D-05 through D-08), full settings list

### Existing Test Files
- `Source/PSD2UMG/Tests/FWidgetBlueprintGenSpec.cpp` — existing integration spec to expand
- `Source/PSD2UMG/Tests/Fixtures/MultiLayer.psd` — existing multi-layer fixture
- `Source/PSD2UMG/Tests/Fixtures/Typography.psd` — existing typography fixture

### Codebase Reference
- `Source/PSD2UMG/PSD2UMG.Build.cs` — module structure and dependencies
- `Source/PSD2UMG/Public/` — all public headers (for README API surface documentation)
- `PSD2UMG.uplugin` — plugin descriptor (version, description for README)

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `FPsdParserSpec` — automation spec for parser; expand with edge-case PSD inputs
- `FWidgetBlueprintGenSpec` — automation spec for full pipeline; expand with new fixture PSDs
- All prior CONTEXT.md files document which requirements each phase fulfilled — use as checklist for README feature list

### Established Patterns
- Tests use `FAutomationTestBase` / Spec framework (`IMPLEMENT_SIMPLE_AUTOMATION_TEST` or `DEFINE_SPEC`)
- Fixture PSD files live in `Source/PSD2UMG/Tests/Fixtures/`
- Plugin version is in `PSD2UMG.uplugin` — source of truth for README version badge

### Integration Points
- README installation steps: copy plugin to `Plugins/` folder, enable in `.uproject`, rebuild
- Layer naming conventions to document: all prefixes (Button_, Progress_, HBox_, etc.), all suffixes (_9s, _variants, _anchor-*, _stretch-*, _fill)

</code_context>

<specifics>
## Specific Ideas

- README should include a **layer naming cheat sheet** — the prefix/suffix table is the core of the designer workflow
- New fixture PSDs should be **minimal but real** — authored to exercise specific code paths, not polished designs
- `Effects.psd` must include a layer that triggers `bFlattenComplexEffects` fallback (inner shadow or gradient overlay)

</specifics>

<deferred>
## Deferred Ideas

- CI/CD (GitHub Actions) — explicitly deferred to post-v1 (D-04)
- ARCHITECTURE.md — deferred (D-06)
- CONTRIBUTING.md — deferred (D-06)
- CHANGELOG.md — deferred (D-06)
- MobileUI.psd fixture — deferred (D-10)
- Separate example UE project — deferred (D-08)

</deferred>

---

*Phase: 08-testing-documentation-release*
*Context gathered: 2026-04-13*

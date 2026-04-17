# Phase 8: Testing, Documentation & Release - Research

**Researched:** 2026-04-13
**Domain:** UE5 Automation Testing (FAutomationTestBase), PSD fixture authoring, README documentation
**Confidence:** HIGH

## Summary

Phase 8 is a finishing phase: expand the existing FAutomationTestBase spec coverage, author three new PSD fixtures, and write a lean README. The test infrastructure is already established — four spec files exist, two fixture PSDs exist, and the pattern (BEGIN_DEFINE_SPEC / DEFINE_SPEC, BeforeEach, It, TestNotNull/TestEqual/TestTrue) is consistent across all files. No new framework is introduced.

The key planning challenge is that TEST-01 (unit tests with "no UE dependency") and TEST-02 (integration tests) are both fulfilled via FAutomationTestBase running inside the UE editor. D-01 locks this approach. Some "unit-like" tests (DPI conversion, anchor heuristics, prefix parsing) work against in-memory FPsdDocument / FPsdLayer structs with zero I/O — these are functionally unit tests even though they run in the editor context.

For documentation, only README.md is required. The layer naming cheat sheet is the highest-value section for the intended audience (UE developers who receive PSD files from designers).

**Primary recommendation:** Add new It() blocks to existing spec files rather than creating new spec files where possible. Only create new spec files when testing a distinct subsystem not covered by existing specs (e.g., FAnchorCalculator, FLayerMappingRegistry prefix dispatch).

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- D-01: Test approach is expand existing FAutomationTestBase specs — add more cases to FPsdParserSpec and FWidgetBlueprintGenSpec. No standalone test binary, no pure-logic extraction. All tests run through UE Automation Framework.
- D-02: Edge cases to cover: empty PSD, malformed layer names, nested smart objects, DPI conversion values, anchor heuristic boundary conditions, prefix/suffix parsing edge cases, reimport change detection.
- D-03: No new test framework or test runner introduced — stay with what exists.
- D-04: No CI/CD for v1. Build and test steps documented in README. No GitHub Actions workflow files created.
- D-05: Documentation is internal-team focused — lean README covering installation and usage. Assumes the reader is an Unreal Engine developer.
- D-06: Docs to create: README.md (installation, quick start, layer naming conventions, settings reference). Skip ARCHITECTURE.md, CONTRIBUTING.md, CHANGELOG.md for v1.
- D-07: README tone: concise, practical. No hand-holding for UE basics. Link to existing UE docs for engine concepts.
- D-08: No separate example UE project. PSDs are bundled in Source/PSD2UMG/Tests/Fixtures/. README explains: drag into your project's Content Browser and import.
- D-09: Existing fixtures kept as-is: MultiLayer.psd, Typography.psd.
- D-10: Three new PSD fixtures to create: SimpleHUD.psd, ComplexMenu.psd, Effects.psd. MobileUI.psd skipped.

### Claude's Discretion
- Exact test case names and assertion structure within FAutomationTestBase specs
- Whether fixture PSDs live in Source/PSD2UMG/Tests/Fixtures/ or a new top-level TestData/ directory
- README file structure and section ordering
- How to author new PSD fixtures (Photoshop, Affinity, or programmatically)

### Deferred Ideas (OUT OF SCOPE)
- CI/CD (GitHub Actions) — explicitly deferred to post-v1 (D-04)
- ARCHITECTURE.md — deferred (D-06)
- CONTRIBUTING.md — deferred (D-06)
- CHANGELOG.md — deferred (D-06)
- MobileUI.psd fixture — deferred (D-10)
- Separate example UE project — deferred (D-08)
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| TEST-01 | Unit tests (no UE dependency): FPsdParser, FLayerMappingRegistry, FAnchorCalculator, DPI conversion | Covered by adding It() blocks to existing specs using in-memory FPsdDocument structs (no file I/O = functionally unit tests within editor context per D-01) |
| TEST-02 | Integration tests (FAutomationTestBase): full PSD->WBP pipeline, reimport | FWidgetBlueprintGenSpec already has 7 integration tests; add reimport and edge-case scenarios; new fixture PSDs enable file-based integration tests |
| TEST-03 | Test PSD files: SimpleHUD, ComplexMenu, Typography (existing), Effects in Fixtures/ | Three new PSDs authored; bundled in Source/PSD2UMG/Tests/Fixtures/ per D-08 (MobileUI deferred) |
| TEST-04 | GitHub Actions CI | SKIPPED per D-04 |
| DOC-01 | README.md with quick start, installation, basic usage | README.md authored at repo root |
| DOC-02 | CONVENTIONS.md with layer naming convention reference | Merged into README.md per D-06 (no separate CONVENTIONS.md for v1) |
| DOC-03 | CHANGELOG.md | SKIPPED per D-06 |
| DOC-04 | Example project: 3-4 pre-imported PSD->WBP demonstrations | Replaced by fixture PSDs bundled in Tests/Fixtures/ per D-08 |
</phase_requirements>

## Standard Stack

### Core (already established — no new dependencies)
| Tool | Version | Purpose |
|------|---------|---------|
| FAutomationTestBase | UE 5.7 | Test base class for all automation tests |
| BEGIN_DEFINE_SPEC / DEFINE_SPEC | UE 5.7 | Spec-style test declaration (BDD-style It/Describe/BeforeEach) |
| EAutomationTestFlags | UE 5.7 | Filter flags: EditorContext + ProductFilter or EngineFilter |
| IPluginManager::FindPlugin | UE 5.7 | Resolve fixture file paths at runtime |

### Fixture Authoring
| Tool | Purpose | Confidence |
|------|---------|------------|
| Affinity Photo 2 | Author minimal PSD fixtures with precise layer control | MEDIUM — works, free trial |
| Adobe Photoshop | Canonical PSD author tool | HIGH — known to work with PhotoshopAPI |
| Python + psd-tools | Programmatic PSD generation for edge-case fixtures | MEDIUM — useful for empty/malformed PSDs |

**Installation:** No new packages. Tests run through UE Automation Framework via Session Frontend or command line.

## Architecture Patterns

### Existing Test File Structure
```
Source/PSD2UMG/Tests/
├── PsdParserSpec.cpp          # FPsdParserSpec (MultiLayer), FPsdParserTypographySpec
├── FWidgetBlueprintGenSpec.cpp # FWidgetBlueprintGenSpec (integration)
├── FTextPipelineSpec.cpp      # Typography pipeline (parse -> generate)
├── FFontResolverSpec.cpp      # Font resolution
└── Fixtures/
    ├── MultiLayer.psd         # 256x128, 3 layers: Title(text), Buttons(group), Background(image)
    └── Typography.psd         # 5 layers: text_regular, text_bold, text_italic, text_stroked, text_paragraph
```

### Pattern: Spec-Style Test (established in project)
```cpp
// Source: existing PsdParserSpec.cpp / FWidgetBlueprintGenSpec.cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(FMySpec, "PSD2UMG.MySubsystem",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
    // member variables here
END_DEFINE_SPEC(FMySpec)

void FMySpec::Define()
{
    Describe("Subsystem::Method", [this]()
    {
        BeforeEach([this]() { /* setup */ });

        It("does X when Y", [this]()
        {
            TestTrue(TEXT("condition"), expr);
            TestEqual(TEXT("value"), actual, expected);
            TestNotNull(TEXT("object"), ptr);
        });
    });
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

### Pattern: In-Memory "Unit" Tests (no file I/O)
For DPI conversion, anchor heuristics, and prefix parsing — construct FPsdDocument and FPsdLayer in-memory, call the logic, assert on FPsdDocument output or UWidget properties. These have no file dependency and complete in milliseconds.

```cpp
// DPI conversion: 72pt source -> assert UMG font size = 72 * (96.0/72.0) = 96
FPsdDocument Doc;
Doc.CanvasSize = FIntPoint(800, 600);
FPsdLayer& TextLayer = Doc.RootLayers.AddDefaulted_GetRef();
TextLayer.Text.SizePx = 72.f;  // 72pt at 72 DPI source
// After Generate(), TextBlock FontSize should be ~96 (72 * 96/72)
```

### Pattern: File-Based Integration Test
For new fixture PSDs, follow the PsdParserSpec pattern:
```cpp
BeforeEach([this]()
{
    TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("PSD2UMG"));
    FixturePath = FPaths::Combine(Plugin->GetBaseDir(),
        TEXT("Source/PSD2UMG/Tests/Fixtures/SimpleHUD.psd"));
    bParsed = PSD2UMG::Parser::ParseFile(FixturePath, Doc, Diag);
});
```

### Anti-Patterns to Avoid
- **Creating standalone test executables:** D-01 locks all tests to the UE Automation Framework.
- **New spec files for every new test case:** Prefer adding It() blocks to existing spec files when testing the same subsystem.
- **Hardcoded absolute paths in specs:** Always use IPluginManager::FindPlugin + GetBaseDir() to resolve fixture paths.
- **Testing private implementation details:** Test public behavior (FPsdDocument fields, UWidget properties) not internal PhotoshopAPI calls.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead |
|---------|-------------|-------------|
| Test discovery | Custom test runner | UE Automation Framework (Session Frontend, -ExecCmds="Automation RunTests PSD2UMG") |
| Fixture path resolution | Hardcoded string paths | IPluginManager::Get().FindPlugin(TEXT("PSD2UMG"))->GetBaseDir() |
| Async test coordination | Manual threading | FAutomationTestBase is synchronous within editor context; no async needed for these tests |

## Common Pitfalls

### Pitfall 1: EAutomationTestFlags filter mismatch
**What goes wrong:** Tests declared with EngineFilter don't show up in ProductFilter runs or vice versa.
**Why it happens:** FPsdParserSpec uses EngineFilter (file I/O, slower); FWidgetBlueprintGenSpec uses ProductFilter (in-memory, fast). Mixing causes tests to be skipped unexpectedly.
**How to avoid:** File-based tests (parse from fixture PSD) → EngineFilter. In-memory tests (construct FPsdDocument, call Generate) → ProductFilter.

### Pitfall 2: Fixture PSD not found at runtime
**What goes wrong:** AddError("Fixture PSD missing") and test reports false failure.
**Why it happens:** GetBaseDir() is correct but relative path component is wrong, or fixture not committed to git.
**How to avoid:** Verify fixture path with FPaths::FileExists in BeforeEach, AddError not TestFalse (so remaining Its are skipped). Confirm fixture files are committed (not .gitignored).

### Pitfall 3: WBP asset name collision between tests
**What goes wrong:** Second run fails because asset already exists at /Game/_Test/... from previous run.
**Why it happens:** Generate() creates UWidgetBlueprint assets in the content browser; sequential test runs see leftover assets.
**How to avoid:** Use unique asset names per test (append a GUID or use test-local paths like /Game/_AutoTest/). Or clean up in AfterEach using ObjectTools::DeleteAssets.

### Pitfall 4: Settings singleton mutation across tests
**What goes wrong:** Test that sets bFlattenComplexEffects = true leaves it set for subsequent tests.
**Why it happens:** UPSD2UMGSettings is a CDO singleton. Already present in existing spec (FWidgetBlueprintGenSpec line 170).
**How to avoid:** Save original value in BeforeEach, restore in AfterEach:
```cpp
bool bOriginal;
BeforeEach([this]() { bOriginal = UPSD2UMGSettings::Get()->bFlattenComplexEffects; });
AfterEach([this]()  { UPSD2UMGSettings::Get()->bFlattenComplexEffects = bOriginal; });
```

### Pitfall 5: README layer naming cheat sheet drift
**What goes wrong:** README documents prefixes/suffixes that don't match the actual mapper registry priority or name format.
**Why it happens:** Code evolves; README written once and forgotten.
**How to avoid:** Source the cheat sheet directly from AllMappers.h and FAnchorCalculator.cpp GSuffixes table. Cross-reference CONTEXT.md from each phase.

## New Test Cases Required (per D-02)

### For FWidgetBlueprintGenSpec (add to existing file)
| Case | What to assert |
|------|---------------|
| Empty PSD (no layers) | Generate returns non-null WBP with root CanvasPanel and 0 children |
| Malformed layer name (prefix only, e.g. "Button_") | Does not crash; maps to UButton with empty name |
| DPI conversion: 72pt source -> UMG font size | TextBlock FontSize ≈ 96 (72 × 96/72) when SourceDPI=72 |
| Reimport: unchanged layer -> Unchanged annotation | DetectChange returns EPsdChangeAnnotation::Unchanged |
| Reimport: moved layer -> Changed annotation | DetectChange returns EPsdChangeAnnotation::Changed |
| Reimport: new layer -> New annotation | DetectChange(layer, nullptr) returns EPsdChangeAnnotation::New |
| Anchor heuristic: top-left quadrant layer | Anchor = top-left (0,0) |
| Anchor heuristic: bottom-right quadrant layer | Anchor = bottom-right (1,1) |
| Anchor heuristic: centered layer | Anchor ≈ centered (0.5, 0.5) |
| _anchor-tl suffix override | Anchor forced to top-left regardless of position |
| _fill suffix override | Both anchors (0,0) and (1,1); offsets all 0 |
| Nested smart object (group inside group) | Children resolved recursively without stack overflow |
| Progress_ prefix group | Root child is UProgressBar |
| HBox_ prefix group | Root child is UHorizontalBox |
| _9s suffix | Image uses Box draw mode |
| _variants suffix | UWidgetSwitcher created |

### For FPsdParserSpec (add to existing file, requires SimpleHUD.psd + Effects.psd fixtures)
| Case | Fixture | What to assert |
|------|---------|---------------|
| Color overlay effect parsed | Effects.psd | bHasColorOverlay = true, ColorOverlayColor matches authored value |
| Drop shadow parsed | Effects.psd | bHasDropShadow = true, DropShadowColor and DropShadowOffset extracted |
| Complex effects flag | Effects.psd | bHasComplexEffects = true for inner shadow / gradient overlay layer |
| Progress_ layer present | SimpleHUD.psd | Layer named "Progress_Health" exists with correct type |
| Button_ layer present | SimpleHUD.psd | Layer named "Button_Start" exists |

## New PSD Fixtures (per D-10)

### SimpleHUD.psd — Spec contract
Canvas: 1920×1080

| Layer Name | Type | Purpose |
|------------|------|---------|
| Background | Image | Full-canvas BG, 1920×1080 |
| Progress_Health | Group | Contains: HealthBg (image), HealthFill (image) |
| Button_Start | Group | Contains: Normal (image), Hover (image) |
| Score | Text | Text content "00000", red color, 32pt |

Tests assert: 4 root layers, Progress_ and Button_ groups present, Score text content extractable.

### ComplexMenu.psd — Spec contract
Canvas: 1280×720

| Layer Name | Type | Purpose |
|------------|------|---------|
| Panel_9s | Group | 9-slice background panel |
| Menu_variants | Group | Contains: Slot0 (group), Slot1 (group) — widget switcher |
| Button_Play | Group | Button with Normal/Hover children |
| Button_Quit | Group | Button with Normal/Hover children |
| Title | Text | "Main Menu", left-aligned |

Tests assert: Panel_9s parsed, Menu_variants has 2 children, two Button_ groups present.

### Effects.psd — Spec contract
Canvas: 512×512

| Layer Name | Type | Effect | Purpose |
|------------|------|--------|---------|
| Overlay_Red | Image | Color Overlay (red, 80% opacity) | FX-03 test |
| Shadow_Box | Image | Drop Shadow (black, 70%, offset 5,5) | FX-04 test |
| Complex_Inner | Image/Group | Inner Shadow or Gradient Overlay | FX-05 flatten fallback |
| Opacity50 | Image | 50% opacity, no effects | FX-01 test |

Tests assert: bHasColorOverlay on Overlay_Red, bHasDropShadow on Shadow_Box, bHasComplexEffects on Complex_Inner.

## Fixture Authoring Strategy

**Recommended approach:** Affinity Photo 2 (or Adobe Photoshop) — both export valid PSDs readable by PhotoshopAPI.

Key constraints for PhotoshopAPI compatibility:
- Save as "Photoshop PSD" not "Large Document PSB"
- 8-bit per channel (PhotoshopAPI template type bpp8_t used in parser)
- Effects added via Layer Style panel (Photoshop / Affinity) — these encode as standard PSD descriptor blocks PhotoshopAPI can read
- Layer names must be exact (parser uses CaseSensitive matching in specs)

**For the empty/malformed edge cases:** These are in-memory tests (no actual PSD file needed) — construct FPsdDocument directly in the It() block.

## README Structure (per D-05, D-06, D-07)

Recommended sections (concise, UE-developer audience):

1. **PSD2UMG** (one-line description + badge: UE 5.7, License MIT)
2. **Quick Start** (4 steps: copy plugin, enable, drag PSD, widget appears)
3. **Installation** (copy to Plugins/, regenerate project files, enable in .uproject)
4. **Layer Naming Cheat Sheet** (the core designer-facing table — prefixes + suffixes)
5. **Plugin Settings** (Project Settings → Plugins → PSD2UMG, describe each field)
6. **Running Tests** (Session Frontend path, command-line invocation)
7. **Known Limitations** (effects fidelity, Win64 only for v1, UE 5.7+ only)

### Layer Naming Cheat Sheet (source of truth: code)

Prefixes (from mapper registry — verified against AllMappers.h):

| Prefix | UMG Widget | Notes |
|--------|-----------|-------|
| Button_ | UButton (or UCommonButtonBase if CommonUI mode on) | Normal/Hovered/Pressed/Disabled child images |
| Progress_ | UProgressBar | Background/Fill child images |
| HBox_ | UHorizontalBox | Children laid out horizontally |
| VBox_ | UVerticalBox | Children laid out vertically |
| Overlay_ | UOverlay | Z-stack children |
| ScrollBox_ | UScrollBox | Scrollable container |
| Slider_ | USlider | |
| CheckBox_ | UCheckBox | |
| Input_ | UEditableTextBox | |
| List_ | UListView | |
| Tile_ | UTileView | |
| Switcher_ | UWidgetSwitcher | |

Suffixes (from FAnchorCalculator.cpp GSuffixes table):

| Suffix | Effect |
|--------|--------|
| _9s | Box draw mode (9-slice); optional [L,T,R,B] margins |
| _variants | UWidgetSwitcher (one child per variant) |
| _anchor-tl | Force anchor top-left |
| _anchor-tr | Force anchor top-right |
| _anchor-bl | Force anchor bottom-left |
| _anchor-br | Force anchor bottom-right |
| _anchor-c | Force anchor centered |
| _stretch-h | Stretch horizontally |
| _stretch-v | Stretch vertically |
| _fill | Fill entire parent |
| _show | Animation variant (show) |
| _hide | Animation variant (hide) |
| _hover | Animation variant (hover) |

## Running Tests

**Session Frontend:** Window → Developer Tools → Session Frontend → Automation → filter "PSD2UMG" → Run

**Command line (headless editor):**
```
UnrealEditor.exe MyProject.uproject -ExecCmds="Automation RunTests PSD2UMG;Quit" -log -nosplash -nullrhi
```

**Filter by subsystem:**
- `PSD2UMG.Parser.*` — parser specs
- `PSD2UMG.Generator.*` — widget blueprint gen specs
- `PSD2UMG.Typography.*` — text pipeline specs

## Environment Availability

Step 2.6: SKIPPED for test spec authoring (no external CLI dependencies). PSD fixture authoring requires Photoshop or Affinity Photo on the developer machine — this is a one-time human task, not an automated build step.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | UE Automation Framework (FAutomationTestBase / Spec DSL) — UE 5.7 |
| Config file | None — tests auto-discovered by WITH_DEV_AUTOMATION_TESTS macro |
| Quick run command | Session Frontend: filter "PSD2UMG.Generator", Run Selected |
| Full suite command | Session Frontend: filter "PSD2UMG", Run All / or command-line above |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| TEST-01 | DPI conversion, anchor heuristics, prefix parsing (in-memory) | unit (editor) | Automation RunTests PSD2UMG.Generator | Add to FWidgetBlueprintGenSpec.cpp |
| TEST-01 | FLayerMappingRegistry prefix dispatch | unit (editor) | Automation RunTests PSD2UMG.Generator | Add to FWidgetBlueprintGenSpec.cpp |
| TEST-02 | Reimport: DetectChange annotation | integration | Automation RunTests PSD2UMG.Generator | Add to FWidgetBlueprintGenSpec.cpp |
| TEST-02 | Full pipeline: SimpleHUD.psd → WBP | integration | Automation RunTests PSD2UMG.Parser | Add parser spec for SimpleHUD |
| TEST-02 | Effects pipeline: Effects.psd → WBP | integration | Automation RunTests PSD2UMG.Parser | Add parser spec for Effects |
| TEST-03 | Fixture files present and parseable | integration | Automation RunTests PSD2UMG.Parser.* | ❌ Wave 0: author 3 new PSDs |
| DOC-01 | README.md exists and is complete | manual | Human review | ❌ Wave 0: create README.md |

### Wave 0 Gaps
- [ ] `Source/PSD2UMG/Tests/Fixtures/SimpleHUD.psd` — required by TEST-03 and new parser integration tests
- [ ] `Source/PSD2UMG/Tests/Fixtures/ComplexMenu.psd` — required by TEST-03
- [ ] `Source/PSD2UMG/Tests/Fixtures/Effects.psd` — required by TEST-03 and FX effect assertions
- [ ] `README.md` (repo root) — required by DOC-01

## Sources

### Primary (HIGH confidence)
- Existing codebase: `Source/PSD2UMG/Tests/PsdParserSpec.cpp`, `FWidgetBlueprintGenSpec.cpp`, `FTextPipelineSpec.cpp` — established patterns verified by reading
- `Source/PSD2UMG/Public/Generator/FWidgetBlueprintGenerator.h` — API surface for DetectChange, Update
- `Source/PSD2UMG/Public/PSD2UMGSetting.h` — settings fields for README documentation
- `PSD2UMG.uplugin` — version (1.0), engine (5.7), platforms (Win64)
- `.planning/phases/08-testing-documentation-release/08-CONTEXT.md` — user decisions (D-01 through D-10)

### Secondary (MEDIUM confidence)
- UE5 automation test documentation (from project knowledge): EAutomationTestFlags, BEGIN_DEFINE_SPEC, Describe/It/BeforeEach pattern — consistent with observed code

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — existing test infrastructure observed directly in codebase
- Architecture: HIGH — patterns extracted directly from 4 existing spec files
- Pitfalls: HIGH — asset collision and settings mutation pitfalls observed in existing code (line 170 of FWidgetBlueprintGenSpec)
- New fixture contracts: MEDIUM — layer names and counts are planning proposals; fixture author must match these exactly

**Research date:** 2026-04-13
**Valid until:** Stable — no fast-moving dependencies

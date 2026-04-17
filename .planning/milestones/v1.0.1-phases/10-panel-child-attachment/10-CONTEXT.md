# Phase 10: Panel Child Attachment — Context

**Gathered:** 2026-04-15
**Status:** Ready for planning
**Source:** Captured inline from v1.0.1 hotfix scoping conversation (no /gsd:discuss-phase run).

<domain>
## Phase Boundary

Close the v1.0 gap where non-canvas group tags (`@vbox`, `@hbox`, `@scrollbox`, `@overlay`) silently drop all children. Root cause: `FWidgetBlueprintGenerator.cpp:253-260` hard-casts the recursion parent to `UCanvasPanel`; if the mapper constructed a non-canvas panel (VBox, HBox, ScrollBox, Overlay), the cast returns null and `PopulateCanvas` is never called. Even if the cast were relaxed, `PopulateCanvas` itself calls `Parent->AddChildToCanvas(Widget)` (lines 88, 230, 455) — a Canvas-only API.

**In scope:**
- Dispatch child attachment on parent panel type.
- `UCanvasPanel` → `AddChildToCanvas` + existing slot positioning (absolute PSD coords, anchors, z-order) — unchanged.
- `UVerticalBox` / `UHorizontalBox` / `UScrollBox` / `UOverlay` → `UPanelWidget::AddChild(UWidget*)`. Children in PSD z-order. No PSD positional data on the resulting slots; slots get engine defaults.
- Fixture PSD `Panels.psd` + `FPanelAttachmentSpec` asserting child counts > 0 and widget-class-per-child for nested non-canvas groups.
- Diagnostic warning (`UE_LOG(LogPSD2UMG, Warning, ...)`) whenever an attachment fails due to unknown parent-panel type — no more silent drops.

**Out of scope:**
- Configurable slot properties (HBox child alignment, Overlay padding, ScrollBox orientation, UniformGridPanel layout). Default engine slots only.
- Image-layer stroke rendering (still deferred per v1.0 D-12).
- Any new type-tag additions.
- Smart-object panel wrappers.

</domain>

<decisions>
## Implementation Decisions

### Attachment Dispatch
- **D-01:** Refactor `FWidgetBlueprintGenerator::PopulateCanvas` into a generic `PopulateChildren(UPanelWidget* Parent, ...)`. Inside, branch on the concrete panel type:
  - `UCanvasPanel` → `AddChildToCanvas(Child)` → returns `UCanvasPanelSlot*` → apply existing position / anchor / z-order logic.
  - Any other `UPanelWidget` subclass → `Parent->AddChild(Child)` → slot stays default.
- **D-02:** The recursion site at line 253 replaces the `Cast<UCanvasPanel>` with `Cast<UPanelWidget>` and calls the generic populate. Canvas path is byte-identical (`UCanvasPanel` derives from `UPanelWidget` so the cast still succeeds).
- **D-03:** For non-canvas panels, **skip all positional application** (line 88 `SetAnchors/SetOffsets/SetZOrder`, line 230 `SetZOrder`, line 455 slot setup). These are Canvas-slot APIs. Children stack by `AddChild` insertion order.

### Child Ordering
- **D-04:** PSD z-order maps to **slot insertion order**. PSD layers are stored top-to-bottom in `Layer.Children` (top of Photoshop panel = index 0). For Canvas this used to translate to `SetZOrder(Index)`; for VBox/HBox it translates to `AddChild` call order. Keep the same Children iteration pattern; the ordering is implicit.
- **D-05:** No reverse or sort — accept the existing `Layer.Children` order. If a designer's PSD has reversed z-order intent, they re-order layers in Photoshop. Predictable and matches `@canvas` behavior.

### Drop-Shadow Sibling (Phase 5 D-08)
- **D-06:** On non-canvas parents, the "sibling `UImage` drop-shadow behind main widget" pattern at lines 216-243 is **skipped**. Warn once: `"Drop shadow on layer '%s' inside non-canvas parent '%s' — no-op (canvas-only sibling pattern)."`. Text layers still get their per-layer `SetShadowOffset` from 04.1 routing (that path is on the text widget itself, independent of parent type).

### Diagnostic / No-Silent-Drops
- **D-07:** If `Cast<UPanelWidget>` at the recursion site fails (e.g., mapper returned something unexpected), emit `UE_LOG(LogPSD2UMG, Error, "Group layer '%s' mapped to non-panel widget '%s'; %d children dropped"`, ...)`.  Previously this was silent.
- **D-08:** When attachment succeeds but no children are processed (empty group), no log change — matches current quiet success.

### Mapper Contract
- **D-09:** Existing mappers already return the correct widget class for each tag. No mapper changes required. Verified via `FSimplePrefixMappers.cpp` (`FVBoxLayerMapper`, `FHBoxLayerMapper`, `FScrollBoxLayerMapper`, `FOverlayLayerMapper`) — each returns a concrete `UPanelWidget` subclass.

### Reimport Path
- **D-10:** The Update/reimport code at `FWidgetBlueprintGenerator.cpp:366+` (`Parent->AddChildToCanvas` at line 455, canvas cast at line 531) also needs the same treatment. Symmetric refactor: extract the recursion into a shared helper used by both `Generate` and `Update`. If reimport complexity grows, split into two plans; otherwise one plan.
- **D-11:** If an existing non-canvas panel is reimported and its child set changes, the reimport logic for non-canvas is simpler than canvas (no per-slot position to preserve — just rebuild the ordered child list). Plan may decide to detach all children and re-add, or diff-and-patch. Planner chooses; document the choice.

### Fixture & Automation
- **D-12:** New fixture `Source/PSD2UMG/Tests/Fixtures/Panels.psd`. Structure:
  - Root canvas containing:
    - A group named `VBoxGroup @vbox` with 3 child layers (`ItemA @text`, `ItemB @image`, `ItemC @button`).
    - A group `HBoxGroup @hbox` with 2 child layers.
    - A group `ScrollBoxGroup @scrollbox` with 4 child layers (enough to imply scrolling semantics, even without runtime scroll test).
    - A group `OverlayGroup @overlay` with 2 stacked child layers.
    - A group `CanvasGroup @canvas` with 2 child layers — regression anchor (Canvas path unchanged).
    - A nested case: `OuterVBox @vbox` containing `InnerCanvas @canvas` containing `NestedItem @text`. Proves recursion works across panel-type transitions.
- **D-13:** Automation spec `FPanelAttachmentSpec`. Per case, assert:
  - Widget class matches (`Cast<UVerticalBox>`, etc.).
  - `GetChildrenCount()` matches expected.
  - Individual child names + widget classes.
  - For `CanvasGroup`: anchor + offsets preserved (regression sentinel).
  - For the nested case: `InnerCanvas` children still get anchor/offsets; `OuterVBox` children do not.
- **D-14:** Fixture-gated with `FPaths::FileExists` (same pattern as `FTextEffectsSpec`). Human-authored PSD via Photoshop checkpoint (no CLI can write the full feature set reliably).

### Scope Guards
- **D-15:** No changes to:
  - Tag parser / EBNF (Phase 9).
  - Layer-effects routing (Phase 4.1, 5).
  - 9-slice handling (Phase 6).
  - Smart-object import (Phase 6).
  - CommonUI / anim wiring (Phase 7).
- **D-16:** No PhotoshopAPI surface changes.
- **D-17:** No public header struct shape changes. `FPsdLayer` / `FPsdLayerEffects` untouched.

### Claude's Discretion
- Whether to do the Update/reimport path refactor in the same plan or a second plan (D-10 trade-off).
- Whether to name the new helper `PopulateChildren`, `AttachChildWidget`, or similar.
- Exact log wording / severity thresholds (within `LogPSD2UMG` conventions).
- Whether the nested `@vbox → @canvas → children` test asserts position data exhaustively or spot-checks one field.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project / Milestone
- `CLAUDE.md` — UE 5.7.4 / C++20 / no Python / Editor-only / Win64 only
- `.planning/PROJECT.md` — v1.0.1 milestone goals
- `.planning/REQUIREMENTS.md` — PANEL-01..PANEL-07 (all assigned to Phase 10)
- `.planning/ROADMAP.md` — Phase 10 section

### Generator (the bug site)
- `Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp` — `PopulateCanvas` (line ~31), recursion cast (line ~253), Update/reimport mirror (line ~366, 455, 531)
- `Source/PSD2UMG/Public/Generator/FWidgetBlueprintGenerator.h` — public signatures
- `Source/PSD2UMG/Private/Mapper/FSimplePrefixMappers.cpp` — `FVBoxLayerMapper`, `FHBoxLayerMapper`, `FScrollBoxLayerMapper`, `FOverlayLayerMapper` (all return concrete `UPanelWidget` subclasses)

### Phase 9 (Tag Parser)
- `Source/PSD2UMG/Private/Parser/FLayerTagParser.cpp` — confirms `@vbox`, `@hbox`, `@scrollbox`, `@overlay` parse to correct `EPsdTagType` values
- `.planning/phases/09-unified-layer-naming-convention-tag-based/DESIGN.md` — tag → widget mapping table

### Phase 3 (Original Mapper + Generator)
- `.planning/phases/03-layer-mapping-widget-blueprint-generation/03-CONTEXT.md` — original canvas-centric decisions (to preserve for D-05 canvas behavior)

### UE APIs
- `UPanelWidget::AddChild(UWidget*)` — generic child attachment (returns `UPanelSlot*`)
- `UCanvasPanel::AddChildToCanvas(UWidget*)` — returns `UCanvasPanelSlot*`
- `UVerticalBox / UHorizontalBox / UScrollBox / UOverlay` — all derive from `UPanelWidget`; slot types `UVerticalBoxSlot`, `UHorizontalBoxSlot`, `UScrollBoxSlot`, `UOverlaySlot`

### Tests
- `Source/PSD2UMG/Tests/FTextEffectsSpec.cpp` — fixture-gate pattern to replicate
- `Source/PSD2UMG/Tests/FSimplePrefixMappersSpec.cpp` (if exists) — existing mapper coverage reference

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `FWidgetBlueprintGenerator::PopulateCanvas` — the body stays largely intact; only the parent type generalizes and the slot-application branches become canvas-only.
- All panel mappers in `FSimplePrefixMappers.cpp` already return the correct `UPanelWidget` subclass. No change needed.
- `FTextEffectsSpec` fixture-gate pattern (`FPaths::FileExists` → skip vs assert) is the template for `FPanelAttachmentSpec`.

### Established Patterns
- PIMPL boundary for PhotoshopAPI (unchanged; we're not touching parser code).
- Fixture PSD committed as binary under `Source/PSD2UMG/Tests/Fixtures/`.
- Spec assertions via `UE_BEGIN_SPEC` framework, one `Describe` per feature group.
- Log categories: `LogPSD2UMG` with `Log` / `Warning` / `Error` severities.

### Integration Points
- `Generate` path (line ~267): `RootCanvas` creation → `PopulateCanvas` → recursion. Only the recursive inner branch needs the refactor; root is always Canvas.
- `Update` path (line ~366+): same recursion shape; must be refactored in lockstep or behavior diverges between first-import and reimport.

</code_context>

<specifics>
## Specific Ideas

- Extract the current recursion body from `PopulateCanvas` into `ProcessLayer(Registry, Tree, UPanelWidget* Parent, const FPsdLayer& Layer, ...)`. Then `PopulateCanvas` becomes a thin wrapper that iterates `Layer.Children` and calls `ProcessLayer`. Non-canvas recursion calls the same `ProcessLayer` with a different `Parent`.
- The existing `UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(Widget)` pattern becomes:
  ```cpp
  UPanelSlot* Slot = nullptr;
  if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(Parent)) { Slot = Canvas->AddChildToCanvas(Widget); /* apply position */ }
  else { Slot = Parent->AddChild(Widget); /* engine defaults */ }
  ```
- Reimport diff logic for non-canvas: simplest safe approach = clear-and-rebuild when any child delta detected. Optimizing diff-and-patch for non-canvas is v1.1 territory.

</specifics>

<deferred>
## Deferred Ideas

- Configurable slot properties per tag (e.g. `@hbox-align:center`, `@overlay-pad:8,8,8,8`, `@scrollbox:horizontal`). Requires new modifier tags in Phase 9 parser — v1.1 scope.
- `UUniformGridPanel`, `UGridPanel`, `UWrapBox` type tags. Future.
- Diff-and-patch reimport for non-canvas (preserve identity of unchanged children). Future optimization.
- Tag-driven scroll direction for `@scrollbox`.

</deferred>

---

*Phase: 10-panel-child-attachment*
*Context captured: 2026-04-15 inline (milestone v1.0.1 hotfix)*

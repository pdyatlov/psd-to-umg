# Phase 9: Unified Layer Naming Convention (Tag-Based) - Research

**Researched:** 2026-04-14
**Domain:** Compiler/parser design + UE5 C++20 mapper architecture refactor
**Confidence:** HIGH (all findings grounded in actual repo source — no external library research needed)

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Tag Syntax**
- **D-01:** Layer name format is `WidgetName @tag @tag:value @tag:v1,v2,…` — clean name first, then `@`-prefixed tags separated by whitespace.
- **D-02:** Exactly one **type tag** required per layer (`@button`, `@image`, `@text`, `@progress`, `@hbox`, `@vbox`, `@overlay`, `@scrollbox`, `@slider`, `@checkbox`, `@input`, `@list`, `@tile`, `@smartobject`, `@canvas`). Group layers without a type tag default to `@canvas`. Pixel layers without a type tag default to `@image`. Text layers without a type tag default to `@text`.
- **D-03:** Tag values use `:` separator. Multi-value tags use comma: `@9s:16,16,16,16`, `@anchor:tl`. No spaces inside a single tag.
- **D-04:** Tags case-insensitive (names + string values). Tag values referencing UE asset names (e.g., `@ia:IA_Confirm`) preserve case for lookup.

**Tag Inventory**
- **D-05:** Type tags as listed in D-02. **Dropped:** `@switcher` (replaced by `@variants` modifier).
- **D-06:** Anchor consolidated under `@anchor:tl|tc|tr|cl|c|cr|bl|bc|br|stretch-h|stretch-v|fill`. Auto-anchor heuristics still apply when absent.
- **D-07:** 9-slice tag: `@9s:L,T,R,B`. `@9s` (zero-margin shorthand) is **planner's discretion** — fallback is to require explicit margins.
- **D-08:** `@variants` on a group → `UWidgetSwitcher` (replaces `Switcher_` and `_variants`).
- **D-09:** Input action: `@ia:IA_ActionName`.
- **D-10:** Animation tags: `@anim:show`, `@anim:hide`, `@anim:hover` on a child group inside a CommonUI button.
- **D-11:** SmartObject: `@smartobject:TypeName` (planner verifies mapping).

**State Brushes**
- **D-12:** Child layers use `@state:name` (`hover|pressed|disabled|normal|fill|bg`) uniformly across all parent widget types. Centralized lookup.
- **D-13:** When parent needs a state child but none has matching `@state:*`, fall back to first child with no state tag (preserves "first child = normal").

**Case + Compatibility**
- **D-14:** All tags case-insensitive. No exceptions.
- **D-15:** **No backwards compatibility.** Old `Button_X`, `_anchor-tl`, `_9s`, `[IA_X]`, `[L,T,R,B]`, `_show`/`_hide`/`_hover`, `_hovered`/`_pressed`, `_fill`/`_bg` removed entirely.

**Conflict / Unknown**
- **D-16:** Tags order-free.
- **D-17:** Conflicting tags → last wins, warn with layer path.
- **D-18:** Unknown tags → warn + ignore + continue.
- **D-19:** Multiple type tags → log error, last wins, continue.

**Name Extraction**
- **D-20:** Widget name = text before first `@`-token, trimmed, internal spaces → underscores. Special chars passed through.
- **D-21:** Empty name → `<TypeTag>_<LayerIndex>` (e.g., `Button_07`). Warn.

**Deliverables**
- **D-22:** `Docs/LayerTagGrammar.md` — formal EBNF spec.
- **D-23:** `Docs/Migration-PrefixToTag.md` — old→new mapping.
- **D-24:** Retag Phase 8 fixture PSDs (`Source/PSD2UMG/Tests/Fixtures/*.psd`); update specs.
- **D-25:** Update `README.md` naming cheat sheet.

### Claude's Discretion

- Internal C++ representation of parsed layer name (struct vs map of tags).
- Where central tag parser lives (`Source/PSD2UMG/Private/Parser/FLayerTagParser.{h,cpp}` is obvious).
- Whether `@9s` shorthand without margins is supported.
- Exact wording of warning/error messages.
- Performance — runs once per layer at import time, no special optimization needed.
- Whether to expose tag registry to Blueprint/Python (probably not — internal C++).
- How `@smartobject:TypeName` value maps to current `FSmartObjectImporter` behavior.

### Deferred Ideas (OUT OF SCOPE)

- Tag registry exposed to Blueprint/Python.
- Custom designer-defined tags / project-specific extensions.
- PSD layer metadata / custom properties as alternative to name parsing.
- Linter / auto-fixer tool to convert old PSDs.
- Auto-anchor heuristic vs explicit `@anchor` conflict review (preserve current behavior).
- Adding new widget capabilities during refactor.

</user_constraints>

<phase_requirements>
## Phase Requirements

This phase has no dedicated requirement IDs in `REQUIREMENTS.md` (Phase 9 was added post-v1 closure as a refactor). It supersedes the layer-naming aspects of MAP-05/06/07/08/09/10/11/12, WBP-04, LAYOUT-01/02/06, CUI-02, CUI-03 — but does NOT change the widget-creation behavior, only the syntax that triggers it.

| Pseudo-ID | Behavior | Research Support |
|-----------|----------|------------------|
| TAG-01 | One central parser produces `FParsedLayerTags` per layer | API design in §Standard Stack |
| TAG-02 | All 12+ callsites consume parsed struct, not raw `Layer.Name` | Callsite Inventory below |
| TAG-03 | Grammar formalized in `Docs/LayerTagGrammar.md` | EBNF in §Architecture Patterns |
| TAG-04 | Migration guide ships at `Docs/Migration-PrefixToTag.md` | Migration Map below |
| TAG-05 | Phase 8 fixture PSDs retagged; specs updated | §Migration Scope |
| TAG-06 | README naming section rewritten | §Migration Scope |
| TAG-07 | Last-wins conflict + unknown-tag warnings logged | §Architecture Patterns |
| TAG-08 | State-child lookup centralized via `FindChildByState` | §Standard Stack |

</phase_requirements>

## Project Constraints (from CLAUDE.md)

- **UE 5.7.4** APIs only (FAppStyle, PlatformAllowList, AssetRegistry path-style includes).
- **C++20** (Build.cs uses `CppStandardVersion.Cpp20`). TStringView, TOptional, structured bindings, designated init, `consteval` available.
- **No Python at runtime.** All parsing in C++.
- **Editor-only** module, LoadingPhase `PostEngineInit`. No shipping module.
- **Allman braces, tabs, `TEXT()` macro, `FString`/`FName`** Unreal idioms.
- **GSD workflow:** Edit/Write only via GSD command — planner produces wave/task plans for `/gsd:execute-phase`.

## Summary

Phase 9 is a **refactor with one architectural win**: replace ~12 scattered `StartsWith`/`EndsWith`/`FindChar` calls across mappers with a single `FLayerTagParser` that produces an `FParsedLayerTags` value object consumed by every mapper, the anchor calculator, the animation builder, the SmartObject importer, and the preview dialog's type-inference helper. The grammar (`Name @tag @tag:value`) is well-defined by the user's 25 locked decisions; no external research was required — the work is entirely internal: design the value object, write the parser, fan out the consumer rewrites, update fixtures and specs, ship docs.

The biggest risk is **fixture regeneration**: PSDs are binary and hand-authored (per phase 08-02 plan). The four spec files (`PsdParserSpec.cpp`, `FWidgetBlueprintGenSpec.cpp`, `FTextPipelineSpec.cpp`, `FFontResolverSpec.cpp`) assert against literal layer names like `"Progress_Health"`, `"Button_Start"`, `"Menu_variants"`, `"Background_fill"`, `"Icon_anchor-tl"`, `"Panel_9s"` — every one must be updated, and the matching layer in the binary PSD must be renamed via Photoshop (no generation script exists).

**Primary recommendation:** Plan as 4 waves: (1) parser + struct + grammar doc, (2) parallel mapper/animation/anchor/preview rewrites, (3) fixture PSD retag + spec rewrite, (4) README + migration doc. Compile-time gate after wave 1 (parser unit-testable headless); integration spec gate after wave 3.

## Standard Stack

### Core
| Component | Location | Purpose |
|-----------|----------|---------|
| `FLayerTagParser` (new) | `Source/PSD2UMG/Private/Parser/FLayerTagParser.{h,cpp}` | Parse `Layer.Name` → `FParsedLayerTags` |
| `FParsedLayerTags` (new) | Same header | Value object every consumer reads from |
| `FPsdLayer.ParsedTags` (new field) | `Source/PSD2UMG/Public/Parser/PsdTypes.h` | Populated once during parse, immutable downstream |
| `LogPSD2UMG` (existing) | `Source/PSD2UMG/Private/PSD2UMGLog.h` | Warning/error channel for unknown/conflicting/empty-name diagnostics |

### Recommended `FLayerTagParser` API (header sketch)

```cpp
// Source/PSD2UMG/Private/Parser/FLayerTagParser.h
#pragma once

#include "CoreMinimal.h"

UENUM()
enum class EPsdTagType : uint8
{
    None,
    Button, Image, Text, Progress,
    HBox, VBox, Overlay, ScrollBox,
    Slider, CheckBox, Input,
    List, Tile, SmartObject, Canvas
};

UENUM()
enum class EPsdAnchorTag : uint8
{
    None,
    TL, TC, TR, CL, C, CR, BL, BC, BR,
    StretchH, StretchV, Fill
};

UENUM()
enum class EPsdStateTag : uint8
{
    None, Normal, Hover, Pressed, Disabled, Fill, Bg
};

UENUM()
enum class EPsdAnimTag : uint8
{
    None, Show, Hide, Hover
};

struct FPsdNineSliceMargin
{
    float L = 0.f, T = 0.f, R = 0.f, B = 0.f;
    bool bExplicit = false; // false = @9s shorthand (planner: support or reject per D-07)
};

/** Parsed result. One per layer. Default-constructible = "no tags found". */
struct FParsedLayerTags
{
    FString CleanName;                       // D-20: name before first @-token, trimmed, spaces→underscores
    EPsdTagType Type = EPsdTagType::None;    // D-02 default-from-layer-type applied by parser caller
    EPsdAnchorTag Anchor = EPsdAnchorTag::None;
    EPsdStateTag State = EPsdStateTag::None;
    EPsdAnimTag Anim = EPsdAnimTag::None;
    TOptional<FPsdNineSliceMargin> NineSlice;
    TOptional<FString> InputAction;          // @ia:IA_Confirm — case preserved per D-04
    TOptional<FString> SmartObjectTypeName;  // @smartobject:TypeName — case preserved
    bool bIsVariants = false;                // @variants
    TArray<FString> UnknownTags;             // for D-18 warning emission

    bool HasType() const { return Type != EPsdTagType::None; }
};

class FLayerTagParser
{
public:
    /**
     * Parse a single layer name. Pure function — no logging side-effects from the parser
     * itself; caller logs based on result fields (UnknownTags, conflicts captured in
     * diagnostics). Caller passes the layer's source EPsdLayerType so D-02 default-type
     * inference can be applied here in one place.
     */
    static FParsedLayerTags Parse(
        FStringView LayerName,
        EPsdLayerType SourceLayerType,
        int32 LayerIndex,        // for D-21 fallback name "Button_07"
        FString& OutDiagnostics  // multi-line warnings/errors for caller to UE_LOG
    );

    /** Helper used by Button/Progress/future stateful mappers — D-12, D-13. */
    static const FPsdLayer* FindChildByState(
        const TArray<FPsdLayer>& Children,
        EPsdStateTag DesiredState);
};
```

**Why this shape:**
- **Single struct, not `TMap<FName,FString>`** — every consumer needs typed access; map lookups invite typos and re-parse cost. Strong enums catch invalid states at compile time.
- **`TOptional` for value-bearing tags** (`@9s`, `@ia`, `@smartobject`) — distinguishes "absent" from "present-with-empty-value".
- **`UnknownTags` array, not silent drop** — D-18 requires logging; collect at parse time, log at integration time so parser stays pure / testable headlessly (matches Phase 2 PIMPL pattern).
- **`OutDiagnostics` string out-param** — keeps parser testable without UE_LOG dependency; caller (likely `FLayerTagParser::Parse` invoker in `PsdParser.cpp`) emits to `LogPSD2UMG`.
- **`SourceLayerType` argument** — D-02 default-type rules (Group→Canvas, Pixel→Image, Text→Text) live in one place, not duplicated per consumer.
- **`LayerIndex`** — D-21 fallback name needs index; pushing this to parser keeps callers simple.
- **`FindChildByState` static helper on the same class** — D-12 and D-13 are one-liner lookups; centralizing avoids re-introducing per-mapper string matching.

### Population Site

`PsdParser.cpp::Parser::ParseFile()` should call `FLayerTagParser::Parse()` for every extracted layer immediately after `OutLayer.Name` is populated. Result lives on `FPsdLayer.ParsedTags`. After this phase, **no consumer should call `Layer.Name.StartsWith()` or similar** for dispatch decisions — `Layer.Name` survives only as a designer-friendly identity string for reimport name-keying (per Phase 7 D-05).

## Architecture Patterns

### Refactored Project Structure (delta from current)

```
Source/PSD2UMG/
├── Public/Parser/
│   └── PsdTypes.h               # MODIFIED — add ParsedTags field on FPsdLayer
├── Private/Parser/
│   ├── PsdParser.cpp            # MODIFIED — call FLayerTagParser::Parse during layer extraction
│   ├── FLayerTagParser.h        # NEW
│   └── FLayerTagParser.cpp      # NEW
├── Private/Mapper/
│   ├── *.cpp                    # ALL REWRITTEN — read Layer.ParsedTags, not Layer.Name
│   └── AllMappers.h             # likely unchanged (interface stable)
├── Private/Generator/
│   ├── FAnchorCalculator.cpp    # MODIFIED — drop suffix table; read ParsedTags.Anchor
│   └── FWidgetBlueprintGenerator.cpp # likely unchanged (delegates to calculator + registry)
├── Private/Animation/
│   └── FPsdWidgetAnimationBuilder.cpp # MODIFIED — drop _show/_hide/_hover; read ParsedTags.Anim
└── Private/UI/
    └── SPsdImportPreviewDialog.cpp # MODIFIED — InferWidgetTypeName reads ParsedTags.Type

Source/PSD2UMG/Tests/
├── Fixtures/*.psd               # ALL RETAGGED in Photoshop (4 files: SimpleHUD, ComplexMenu, Effects, Typography; MultiLayer probably needs review)
├── PsdParserSpec.cpp            # MODIFIED — assert tag-renamed names
├── FWidgetBlueprintGenSpec.cpp  # MODIFIED — synthetic FPsdLayer.Name strings rewritten
└── (new) FLayerTagParserSpec.cpp # NEW — pure unit tests for parser

Docs/
├── LayerTagGrammar.md           # NEW (D-22)
└── Migration-PrefixToTag.md     # NEW (D-23)

README.md                        # MODIFIED (D-25) — replace lines ~39-96 naming cheat sheet
```

### Pattern 1: Mapper Before/After (canonical example — `FButtonLayerMapper`)

**Before** (current `FButtonLayerMapper.cpp`):
```cpp
bool FButtonLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::Group && Layer.Name.StartsWith(TEXT("Button_"));
}

UWidget* FButtonLayerMapper::Map(const FPsdLayer& Layer, ...)
{
    UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass(), FName(*Layer.Name));
    // ... loops Children, calls Child.Name.ToLower().EndsWith("_hovered") etc.
}
```

**After**:
```cpp
bool FButtonLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.ParsedTags.Type == EPsdTagType::Button;
}

UWidget* FButtonLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree)
{
    UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass(), FName(*Layer.ParsedTags.CleanName));
    FButtonStyle Style = FButtonStyle::GetDefault();

    // D-12: state lookup centralized via helper
    auto ApplyState = [&](EPsdStateTag State, void(*Apply)(FButtonStyle&, const FSlateBrush&)) {
        if (const FPsdLayer* Child = FLayerTagParser::FindChildByState(Layer.Children, State))
        {
            FSlateBrush Brush = MakeBrushFor(*Child, Doc);
            Apply(Style, Brush);
        }
    };
    ApplyState(EPsdStateTag::Normal,   [](auto& S, auto& B){ S.SetNormal(B); });
    ApplyState(EPsdStateTag::Hover,    [](auto& S, auto& B){ S.SetHovered(B); });
    ApplyState(EPsdStateTag::Pressed,  [](auto& S, auto& B){ S.SetPressed(B); });
    ApplyState(EPsdStateTag::Disabled, [](auto& S, auto& B){ S.SetDisabled(B); });

    // D-13 fallback for normal: if no @state:normal child, FindChildByState returns the first
    // child with State==None (implemented inside the helper).

    Btn->SetStyle(Style);
    return Btn;
}
```

This same pattern applies to **every other mapper**:
- `CanMap` collapses to one enum compare.
- `Map` reads `ParsedTags.CleanName` (never `Layer.Name`).
- State / value tags read from `ParsedTags`.
- Child dispatch uses `FindChildByState`.

### Pattern 2: Anchor Calculator simplification

`FAnchorCalculator::TryParseSuffix` currently does suffix-table linear scan + bracket stripping. After Phase 9:

```cpp
FAnchorResult FAnchorCalculator::Calculate(const FPsdLayer& Layer, const FIntRect& Bounds, const FIntPoint& CanvasSize)
{
    const FParsedLayerTags& Tags = Layer.ParsedTags;
    if (Tags.Anchor != EPsdAnchorTag::None)
    {
        return ResolveExplicitAnchor(Tags.Anchor, Bounds, CanvasSize, Tags.CleanName);
    }
    FAnchorResult Result;
    Result.Anchors = QuadrantAnchor(Bounds, CanvasSize, Result.bStretchH, Result.bStretchV);
    Result.CleanName = Tags.CleanName;
    return Result;
}
```

The entire `GSuffixes` table goes away. `ResolveExplicitAnchor` is a small switch.

**Note:** `FAnchorCalculator::Calculate` signature changes from `(FString, FIntRect, FIntPoint)` to `(const FPsdLayer&, ...)`. Two call sites in `FWidgetBlueprintGenerator.cpp` (lines 251, 534, 602 per current code) need updating — straightforward.

### EBNF Grammar (for `Docs/LayerTagGrammar.md` per D-22)

```ebnf
layer-name      = clean-name { whitespace tag } ;
clean-name      = { non-at-char } ;          (* trimmed; internal spaces → '_' downstream *)
non-at-char     = ? any Unicode codepoint except '@' ? ;

tag             = "@" tag-name [ ":" tag-value ] ;
tag-name        = identifier ;
identifier      = letter { letter | digit | "-" } ;
letter          = "A".."Z" | "a".."z" ;
digit           = "0".."9" ;

tag-value       = value-atom { "," value-atom } ;
value-atom      = { value-char } ;
value-char      = ? any non-whitespace, non-comma char except '@' ? ;
                  (* allows ':' inside? NO — colon already separates name:value;
                     planner ruling: forbid additional colons in value to keep grammar LL(1) *)

whitespace      = " " | "\t" ;               (* one or more *)
```

**Edge cases the planner must spec:**

| Edge case | Resolution |
|-----------|------------|
| Empty layer name (`@button` only) | `CleanName` = `""` → caller substitutes `<TypeTag>_<Index>` per D-21, warn |
| Layer named only whitespace before first `@` | Treat as empty → D-21 fallback |
| Literal `@` in clean name | Impossible by grammar (first `@` ends clean name). Designers must avoid `@` in widget names; if encountered before any tag-like token, parser still treats first `@` as tag start |
| Multiple `@@` | First `@` opens tag, second `@` is start of an empty tag-name → treated as malformed tag; emit unknown-tag warning, ignore |
| Tag with no name (`@:value`) | Empty tag name → unknown-tag warning, ignore |
| Tag with trailing `:` (`@anchor:`) | Empty value → unknown-tag warning, treat as `@anchor` only (which itself is unknown) |
| Trailing whitespace after last tag | Strip silently |
| Two type tags (`@button @image`) | D-19: log error, last wins → `EPsdTagType::Image` |
| Two anchor tags (`@anchor:tl @anchor:c`) | D-17: log warn, last wins → `Anchor = C` |
| Two `@9s` tags with different margins | D-17: warn, last wins |
| Comma in value with trailing comma (`@9s:1,2,3,4,`) | Strip empty token, error if resulting count != 4 |
| Tag name with hyphen (`@anchor:stretch-h`) | Allowed in value, NOT in tag-name (only `@anchor` is the name); value `stretch-h` parses as one atom |
| Mixed case (`@BUTTON @Anchor:TL`) | D-04, D-14: lowercase tag-name + value before enum lookup; `@ia:IA_Confirm` value preserved verbatim |

### Anti-patterns to Avoid

- **Re-parsing on every consumer call.** `Parse()` runs once in `PsdParser.cpp`; result is cached on `FPsdLayer`.
- **Silent unknown-tag drop.** D-18 requires warning. Collect in `UnknownTags` and log at integration boundary.
- **Per-mapper state-child matching.** D-12 explicitly requires the helper. Don't let a consumer re-introduce `Child.Name.EndsWith(...)`.
- **Modifying `Layer.Name` in-place** to strip tags. `Layer.Name` should remain the verbatim PSD layer name (it's the Phase 7 reimport identity key). Use `ParsedTags.CleanName` for widget names.

### Reimport Identity Key — IMPORTANT

Phase 7 D-05 says reimport matches widgets by **PSD layer name** (the verbatim string). After Phase 9, the widget's `FName` is `ParsedTags.CleanName`. **The reimport key must be the verbatim `Layer.Name`, not `CleanName`** — otherwise renaming a tag (e.g., adding `@anchor:c`) would orphan the widget. The planner must verify `FWidgetBlueprintGenerator::Update()` and `CollectWidgetsByName` use the right identity. Currently `CollectWidgetsByName` uses `W->GetName()` (the clean widget name) — this is already a known fragility; Phase 9 may need a `WidgetIdentity` mapping table or an annotation slot tag. **Flag this for planner review.** (See Risk #4.)

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| String tokenizing | Custom char-by-char loop | `FString::ParseIntoArray(Tokens, TEXT(" "), /*bCullEmpty=*/true)` for whitespace split, then per-token check for `'@'` prefix | UE-idiomatic, handles tabs + multi-space correctly |
| Tag name → enum lookup | Long if/else chain | `static const TMap<FString, EPsdTagType>` initialized once | Maintainability, easy to extend |
| Case-insensitive compare | `Lhs.ToLower() == Rhs.ToLower()` | `Lhs.Equals(Rhs, ESearchCase::IgnoreCase)` | Avoids string allocation per compare |
| Substring stripping for clean name | Manual `Mid()` chains | `FString::Split(TEXT("@"), &Left, &Right)` then `Left.TrimStartAndEnd()` | Already-tested in Unreal core |
| EBNF doc | ASCII art ad-hoc | Standard EBNF (ISO 14977) syntax | Reviewable, future tooling friendly |

## Runtime State Inventory

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | None — no DB, no persisted state outside `.uasset` files. Existing user-imported WBPs would have widget `FName`s like `Button_Start` (built from old `Layer.Name`). After Phase 9 retags PSDs, reimport produces widgets like `Start` (the new clean name). | **Code edit + designer action.** No automatic data migration possible — designer reimports from retagged PSDs; existing WBPs become orphans (or designer manually renames widgets). Document this in migration guide. |
| Live service config | None — plugin is editor-only, no external services. | None. |
| OS-registered state | None. | None. |
| Secrets/env vars | None. | None. |
| Build artifacts | `Binaries/Win64/UnrealEditor-PSD2UMG.dll` and `Intermediate/Build/` will rebuild on next compile — no stale state. PSD fixture binaries (`Tests/Fixtures/*.psd`) are content-state requiring designer retag. | Rebuild on compile; manual PSD edit in Photoshop for fixtures. |

## Common Pitfalls

### Pitfall 1: Conflict-resolution log spam during fixture migration
**What goes wrong:** During fixture retag, a designer leaves an old prefix mistakenly intact alongside new tags (`Button_Start @button`). Parser sees clean-name `Button_Start` and `Type=Button` — works correctly but designer wonders why widget is named `Button_Start` not `Start`.
**Why it happens:** Old prefix is now just part of the clean name; no error.
**How to avoid:** Migration guide includes a "lint your PSD" checklist: search layer names for `_`-suffixes that look like old anchor/state syntax.
**Warning signs:** Widget names in generated WBP retain old prefix.

### Pitfall 2: `FName` invalidation from special chars in CleanName
**What goes wrong:** D-20 says special chars pass through. `FName(*"My/Widget")` triggers UE name validation failure → silent name mangling or crash.
**Why it happens:** UE `FName` rejects `/`, `.`, etc.
**How to avoid:** Phase 9 plan includes a validation pass in `FLayerTagParser::Parse` or in the consumer that wraps clean-name in `FName::IsValidXName` check; emit clear error per D-20 ("invalid name … pointing back at the offending layer").
**Warning signs:** Spec test failures with mangled widget names.

### Pitfall 3: `FCommonUIButtonLayerMapper` and `FButtonLayerMapper` priority overlap
**What goes wrong:** Both currently match `Button_` prefix; CommonUI wins via priority 210 > 200 + the `bUseCommonUI` gate. After tag refactor, both will match `@button`. Same priority gate works — but verify the new `CanMap` keeps the gate.
**Why it happens:** Easy to drop the `bUseCommonUI` check during refactor.
**How to avoid:** Add unit test asserting CommonUI mapper returns false from `CanMap` when setting is off.

### Pitfall 4: `FAnchorCalculator::Calculate` signature change ripples
**What goes wrong:** Callsites in `FWidgetBlueprintGenerator.cpp` (lines 251, 534, 602) all pass `LayerName/Layer.Name`. Need to pass `Layer` (or `ParsedTags`).
**Why it happens:** Three call sites; easy to miss one.
**How to avoid:** Plan task explicitly enumerates the three line numbers.

### Pitfall 5: Tests assert against `Layer.Name` in synthetic `FPsdLayer` construction
**What goes wrong:** `FWidgetBlueprintGenSpec.cpp` constructs `FPsdLayer` in-memory with `.Name = TEXT("Button_Start")` then expects `FButtonLayerMapper` to match. After refactor, mapper checks `ParsedTags.Type` — which is never populated in the synthetic test path because nobody called the parser.
**Why it happens:** Synthetic test bypasses the parser pipeline.
**How to avoid:** Either (a) make the spec call `FLayerTagParser::Parse` after constructing the synthetic layer to populate `ParsedTags`, or (b) provide a `FPsdLayer::FromTaggedName(...)` test helper. Plan task should specify which.

## Code Examples

### Tokenizing a layer name

```cpp
// Source: this research, derived from Unreal FString patterns
FParsedLayerTags FLayerTagParser::Parse(FStringView LayerName, EPsdLayerType SourceLayerType, int32 LayerIndex, FString& OutDiagnostics)
{
    FParsedLayerTags Out;

    const FString Source(LayerName);
    int32 FirstAt = INDEX_NONE;
    Source.FindChar(TEXT('@'), FirstAt);

    FString CleanRaw = (FirstAt == INDEX_NONE) ? Source : Source.Left(FirstAt);
    Out.CleanName = CleanRaw.TrimStartAndEnd().Replace(TEXT(" "), TEXT("_"));

    if (FirstAt != INDEX_NONE)
    {
        FString Rest = Source.Mid(FirstAt);
        TArray<FString> Tokens;
        Rest.ParseIntoArrayWS(Tokens); // splits on any whitespace, culls empty
        for (const FString& Token : Tokens)
        {
            if (!Token.StartsWith(TEXT("@"))) continue; // malformed, skip
            ApplyTokenToTags(Token, Out, OutDiagnostics);
        }
    }

    // D-02 default-type inference
    if (Out.Type == EPsdTagType::None)
    {
        switch (SourceLayerType)
        {
            case EPsdLayerType::Group:  Out.Type = EPsdTagType::Canvas; break;
            case EPsdLayerType::Image:  Out.Type = EPsdTagType::Image;  break;
            case EPsdLayerType::Text:   Out.Type = EPsdTagType::Text;   break;
            default: break;
        }
    }

    // D-21 empty-name fallback
    if (Out.CleanName.IsEmpty())
    {
        Out.CleanName = FString::Printf(TEXT("%s_%02d"), *TypeTagToString(Out.Type), LayerIndex);
        OutDiagnostics += FString::Printf(TEXT("Layer with empty name → '%s'\n"), *Out.CleanName);
    }

    return Out;
}
```

### State-child lookup helper

```cpp
const FPsdLayer* FLayerTagParser::FindChildByState(const TArray<FPsdLayer>& Children, EPsdStateTag DesiredState)
{
    // D-12: explicit @state match first
    for (const FPsdLayer& Child : Children)
    {
        if (Child.ParsedTags.State == DesiredState) return &Child;
    }
    // D-13: fall back to first child with no state tag (only for "Normal" or unspecified-default cases)
    if (DesiredState == EPsdStateTag::Normal)
    {
        for (const FPsdLayer& Child : Children)
        {
            if (Child.ParsedTags.State == EPsdStateTag::None && Child.Type == EPsdLayerType::Image)
            {
                return &Child;
            }
        }
    }
    return nullptr;
}
```

## Callsite Inventory (Exhaustive)

Every place in `Source/PSD2UMG/` where layer-name parsing currently happens. Each becomes a planner task to rewrite against `ParsedTags`.

| # | File | Lines | Pattern Used | Tag Equivalent | Action |
|---|------|-------|--------------|----------------|--------|
| 1 | `Private/Mapper/FButtonLayerMapper.cpp` | 18 | `Layer.Name.StartsWith("Button_")` | `ParsedTags.Type == Button` | Rewrite CanMap |
| 2 | `Private/Mapper/FButtonLayerMapper.cpp` | 47-64 | `Child.Name.ToLower().EndsWith("_hovered")` etc. — 4 state branches | `FindChildByState(Children, Hover/Pressed/Disabled/Normal)` | Rewrite Map state loop |
| 3 | `Private/Mapper/FCommonUIButtonLayerMapper.cpp` | 31 | `Layer.Name.StartsWith("Button_", IgnoreCase)` | `ParsedTags.Type == Button` | Rewrite CanMap (preserve `bUseCommonUI` gate) |
| 4 | `Private/Mapper/FCommonUIButtonLayerMapper.cpp` | 55-98 | `LayerName.FindChar('[')` + `.FindChar(']')` → IA name extraction | `ParsedTags.InputAction.GetValue()` | Drop bracket parsing, read TOptional |
| 5 | `Private/Mapper/FProgressLayerMapper.cpp` | 18 | `Layer.Name.StartsWith("Progress_")` | `ParsedTags.Type == Progress` | Rewrite CanMap |
| 6 | `Private/Mapper/FProgressLayerMapper.cpp` | 47-56 | `Child.Name.ToLower().EndsWith("_fill")` / `_foreground` / fallback bg | `FindChildByState(Children, Fill/Bg)` | Rewrite Map state loop |
| 7 | `Private/Mapper/FSimplePrefixMappers.cpp` | 27, 40, 53, 66, 79, 92, 105, 118, 134, 150 | 10 × `Layer.Name.StartsWith("Hbox_"/"VBox_"/"Overlay_"/...)`  | `ParsedTags.Type == HBox/VBox/...` | Rewrite all 10 CanMap methods |
| 8 | `Private/Mapper/F9SliceImageLayerMapper.cpp` | 24-25 | `Layer.Name.Contains("_9slice"/"_9s", CaseSensitive)` | `ParsedTags.NineSlice.IsSet()` | Rewrite CanMap |
| 9 | `Private/Mapper/F9SliceImageLayerMapper.cpp` | 51-78 | `FindLastChar('[')` + `[L,T,R,B]` parsing | `ParsedTags.NineSlice->{L,T,R,B,bExplicit}` | Drop bracket parsing |
| 10 | `Private/Mapper/FVariantsSuffixMapper.cpp` | 18 | `Layer.Name.EndsWith("_variants")` | `ParsedTags.bIsVariants` | Rewrite CanMap |
| 11 | `Private/Mapper/FSmartObjectLayerMapper.cpp` | 19 | `Layer.Type == EPsdLayerType::SmartObject` (type-based, not name-based — but `@smartobject:TypeName` still needs handling) | `ParsedTags.Type == SmartObject` OR `ParsedTags.SmartObjectTypeName.IsSet()` (planner verifies which path is canonical) | Rewrite CanMap; consume TypeName value if present (currently SmartObject "TypeName" lookup doesn't actually exist in `FSmartObjectLayerMapper.cpp` or `FSmartObjectImporter.cpp` — only `Layer.Type` and `Layer.SmartObjectFilePath` are used. **D-11's "type identifier" appears to be a forward-looking field with no current consumer.** Planner should confirm intent: probably parser stores it on `ParsedTags.SmartObjectTypeName` for future use, no behavioral change in Phase 9.) |
| 12 | `Private/Generator/FAnchorCalculator.cpp` | 16-37 | `GSuffixes` table: `_9slice`, `_9s`, `_variants`, `_anchor-tl`...`_anchor-c`, `_stretch-h/v`, `_fill` (15 entries) | `ParsedTags.Anchor` enum | Delete entire suffix table; replace `TryParseSuffix` with enum switch |
| 13 | `Private/Generator/FAnchorCalculator.cpp` | 63-126 | `TryParseSuffix` body — `EndsWith` loop + bracket strip | Same — collapsed into switch | Rewrite |
| 14 | `Private/Generator/FAnchorCalculator.cpp` | 131-154 | `Calculate(FString LayerName, ...)` signature | `Calculate(const FPsdLayer& Layer, ...)` | Change signature |
| 15 | `Private/Generator/FWidgetBlueprintGenerator.cpp` | 251, 534, 602 | 3 callers of `FAnchorCalculator::Calculate(LayerName, ...)` | Pass `Layer` reference | Update three call sites |
| 16 | `Private/Animation/FPsdWidgetAnimationBuilder.cpp` | 145-159 | `LayerLower.EndsWith("_show"/"_hide"/"_hover")` 3 branches | `ParsedTags.Anim == Show/Hide/Hover` | Rewrite `ProcessAnimationVariants` |
| 17 | `Private/UI/SPsdImportPreviewDialog.cpp` | 39-55 | `Name.StartsWith("Button_"/"Btn_"/"Progress_"/"List_"/"Tile_", IgnoreCase)` | `ParsedTags.Type` switch | Rewrite `InferWidgetTypeName` |
| 18 | `Private/Parser/PsdParser.cpp` | (insert during layer extraction, around line 540-560) | n/a — currently no parsing | **NEW:** call `FLayerTagParser::Parse` for every extracted layer; populate `OutLayer.ParsedTags` | Add call |
| 19 | `Private/Mapper/FImageLayerMapper.cpp` | 14, 29 | `Layer.Type == Image`; `FName(*Layer.Name)` for widget name | `ParsedTags.Type == Image`; `FName(*ParsedTags.CleanName)` | Mostly same; switch to CleanName for widget naming |
| 20 | `Private/Mapper/FTextLayerMapper.cpp` | 17-18, 24 | `Layer.Type == Text`; `FName(*Layer.Name)` | Same swap | Switch to CleanName |
| 21 | `Private/Mapper/FGroupLayerMapper.cpp` | 11-18 | `Layer.Type == Group`; `FName(*Layer.Name)` | `ParsedTags.Type == Canvas`; `FName(*ParsedTags.CleanName)` | Switch dispatch + name |

**Note on `FCommonUIButtonLayerMapper.cpp` line 43** — `const FString CleanName = Layer.Name;` — already named `CleanName`; refactor to `Layer.ParsedTags.CleanName`.

**Smart-object name lookup** — search confirmed: no existing code reads a "SmartObject type name" from layer name. D-11 introduces a new tag (`@smartobject:TypeName`) whose value the parser captures in `ParsedTags.SmartObjectTypeName` for forward use; Phase 9 has no consumer mapping behavior change beyond storing it. Planner should confirm with user that this is the intended interpretation.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Per-mapper `StartsWith`/`EndsWith` | One central `FLayerTagParser` returning typed struct | Phase 9 | Single source of truth for layer-name semantics |
| Bracket params `[IA_X]`, `[L,T,R,B]` | `@ia:IA_X`, `@9s:L,T,R,B` | Phase 9 | Uniform syntax, no nested bracket parsing |
| Suffix-table linear scan in `FAnchorCalculator` | Enum switch | Phase 9 | Faster, type-safe, deletable when adding new anchors |
| Implicit "first child = normal" | `FindChildByState` helper enforcing D-13 | Phase 9 | Rule lives in one place |

**Deprecated/removed:**
- `Button_`, `Progress_`, `HBox_`, `VBox_`, `Overlay_`, `ScrollBox_`, `Slider_`, `CheckBox_`, `Input_`, `List_`, `Tile_`, `Switcher_` prefixes — D-15 hard removal.
- `_anchor-*`, `_stretch-*`, `_fill`, `_9s`, `_9slice`, `_variants` suffixes — D-15.
- `[IA_X]`, `[L,T,R,B]` bracket forms — D-15.
- `_show`, `_hide`, `_hover`, `_hovered`, `_pressed`, `_disabled`, `_normal`, `_fill`, `_bg`, `_foreground`, `_background` child suffixes — D-15.
- `Btn_` prefix (used in preview dialog only, line 40) — D-15 implicit removal.

## Migration Scope (D-24, D-25)

### Fixture PSDs requiring retag

Located at `Source/PSD2UMG/Tests/Fixtures/`:

| File | Created by | Retag Source |
|------|-----------|--------------|
| `MultiLayer.psd` | Phase 02-05 (hand-authored) | Manual Photoshop |
| `Typography.psd` | Phase 04-01 (hand-authored) | Manual Photoshop |
| `SimpleHUD.psd` | Phase 08-02 (hand-authored) | Manual Photoshop |
| `ComplexMenu.psd` | Phase 08-02 (hand-authored) | Manual Photoshop |
| `Effects.psd` | Phase 08-02 (hand-authored) | Manual Photoshop |

**Critical:** No generation scripts exist. PSDs must be opened in Photoshop and layers renamed by hand. PhotoshopAPI is read-only in this codebase (used by parser; no write support wired in). **This is a manual designer task** — flag in plan as "human-required" wave with a checklist of every layer name needing change.

For each PSD, the migration checklist must include:
- Every group layer named `Button_*` → `Button @button` (or whatever clean name + `@button`)
- Every group with `_anchor-tl` etc. → strip suffix, add `@anchor:tl`
- Every `_9s`/`_9slice` image → rename + `@9s` (or `@9s:L,T,R,B` if margin bracket present)
- Every state child (`*_hovered`, `*_pressed`, `*_fill`, `*_bg`) → rename + `@state:hover` etc.
- Every `_variants` group → `@variants`
- Every `[IA_X]` bracket → strip, add `@ia:IA_X`
- Every animation child (`*_show`, `*_hide`, `*_hover`) → `@anim:show` etc.

### Spec files requiring rewrite

| File | Affected lines (approximate) | Pattern |
|------|------------------------------|---------|
| `Tests/PsdParserSpec.cpp` | 360-373 (Progress_Health, Button_Start asserts) | Rewrite to assert against retagged names |
| `Tests/FWidgetBlueprintGenSpec.cpp` | 380-386 (`Button_`), 401-407 (`Progress_Health`), 422-428 (`HBox_Row`), 443-449 (`Menu_variants`), 470-476 (`Background_fill`), 498-504 (`Icon_anchor-tl`), 642-648 (`Panel_9s`) — all synthetic `Layer.Name = TEXT("...")` constructions | Rewrite synthetic name + populate `ParsedTags` (see Pitfall 5) |
| `Tests/FTextPipelineSpec.cpp` | (verify — likely no prefix asserts but check) | Probably minor |
| `Tests/FFontResolverSpec.cpp` | (likely none — font resolver doesn't touch layer-name dispatch) | Verify only |

**New spec required:** `Tests/FLayerTagParserSpec.cpp` — pure unit tests covering grammar edge cases enumerated above (empty name, conflicts, unknown tags, multiple type tags, case sensitivity, value preservation for `@ia:`, `@9s` margin parse). Headless (no UE asset deps), runs fast.

### Documentation deliverables

| File | Status | Content |
|------|--------|---------|
| `Docs/LayerTagGrammar.md` | NEW (D-22) | Formal EBNF (above), full tag inventory with table, case rules, conflict/unknown rules, name extraction algorithm, ~10 worked examples |
| `Docs/Migration-PrefixToTag.md` | NEW (D-23) | Old → new mapping table for every removed prefix/suffix/bracket; before/after PSD layer-name examples; "lint your PSD" checklist |
| `README.md` | MODIFIED (D-25) | Replace lines ~39-96 (current naming cheat sheet) with new tag-based cheat sheet — keep it terse, link to `Docs/LayerTagGrammar.md` for formal grammar |

## Risk Register (Top 5)

1. **PSD fixture regeneration is manual.** Five binary PSDs, each with ~10-30 layers, must be opened in Photoshop and renamed by a human. No automation possible without adding write support to PhotoshopAPI vendor code (out of scope per CONTEXT.md deferred ideas). **Mitigation:** Plan a dedicated wave with a per-PSD checklist; consider using a single small "retag verification" PSD first to validate the new grammar before touching Phase 8 fixtures.

2. **Reimport identity-key fragility.** Phase 7 D-05 keys reimport on layer name. If the parser strips tags from the widget `FName` (`CleanName`), but reimport still expects the verbatim PSD name, every reimport produces all-orphans. **Mitigation:** Plan task explicitly covers `FWidgetBlueprintGenerator::Update()` and decides identity strategy — likely store original `Layer.Name` as widget metadata or always match by `CleanName` (and document that adding/removing a tag now breaks reimport identity, which is acceptable given D-15 hard cutover).

3. **`FAnchorCalculator::Calculate` signature change ripples.** Three call sites in the generator. **Mitigation:** Plan task lists all three line numbers; compile error catches missed sites immediately.

4. **Synthetic test layers bypass parser.** `FWidgetBlueprintGenSpec.cpp` constructs `FPsdLayer` in-memory with `.Name = "Button_Start"` — never invokes `FLayerTagParser`. After refactor, `ParsedTags` is default-constructed and mappers see `Type == None`. **Mitigation:** Either auto-populate `ParsedTags` in tests (call `FLayerTagParser::Parse` after constructing) or provide a `FPsdLayer::FromTaggedName` test helper. Plan task must specify approach and update every synthetic-layer test site.

5. **Anchor-heuristic + explicit `@anchor:stretch-h` interaction.** Current behavior: explicit suffix overrides quadrant heuristic. After refactor, same logic applies (`Calculate` checks `ParsedTags.Anchor` first). **But:** the auto-row/column detection in `FWidgetBlueprintGenerator::PopulateCanvas` (lines 101-182) wraps children in HBox/VBox before per-child anchor calculation runs. A child with `@anchor:stretch-h` inside an auto-detected row may behave oddly because the parent is no longer a CanvasPanel. **Mitigation:** Add a spec test for this combination; document expected behavior in grammar doc.

## Open Questions

1. **`@9s` shorthand without margins** — D-07 explicitly defers to planner. Recommendation: **support shorthand** (`@9s` alone) with sensible 16px default (matches current `F9SliceImageLayerMapper` line 76 default), to keep designer ergonomics. Document the default in the grammar spec. Reject if user prefers strict.
2. **`@smartobject:TypeName` semantics** — no current code consumes a "type name" for SmartObjects; mapper dispatches solely on `Layer.Type == SmartObject`. Recommend parsing and storing the value for forward use, no behavioral change in Phase 9. Verify with user before planning.
3. **Reimport identity key after tag refactor** — see Risk #2. Needs explicit user decision before Wave 2 mapper rewrite.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Unreal `FAutomationTestBase` / Spec macros (existing) |
| Config file | None — specs are `Source/PSD2UMG/Tests/*.cpp` files compiled into the editor module |
| Quick run command | Editor: Window → Test Automation → run `PSD2UMG.*` filter; CLI: `UnrealEditor-Cmd.exe <project> -ExecCmds="Automation RunTests PSD2UMG; quit" -unattended` |
| Full suite command | Same with `PSD2UMG.` prefix runs all PSD2UMG specs |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|--------------|
| TAG-01 | Parser produces correct struct for canonical input | unit | `Automation RunTests PSD2UMG.LayerTagParser` | Wave 0 (NEW) |
| TAG-01 | Empty name → fallback `Button_NN` | unit | same | Wave 0 |
| TAG-01 | Conflict → last wins, diagnostic emitted | unit | same | Wave 0 |
| TAG-01 | Unknown tag → captured in `UnknownTags`, no failure | unit | same | Wave 0 |
| TAG-01 | Case-insensitive tag match; value case preserved for `@ia:` | unit | same | Wave 0 |
| TAG-02 | Each mapper's `CanMap` returns true for matching `Type` tag | integration | `Automation RunTests PSD2UMG.WidgetBlueprintGen` | Updated existing |
| TAG-02 | `FindChildByState` finds explicit and falls back to first untagged | unit | `Automation RunTests PSD2UMG.LayerTagParser` | Wave 0 |
| TAG-05 | SimpleHUD.psd → expected widget tree after retag | integration | `Automation RunTests PSD2UMG.PsdParser` | Updated existing |
| TAG-07 | Conflicting tags log warning + continue | integration | manual (verify Output Log) — also unit asserts diagnostic string | partial |

### Sampling Rate
- **Per task commit:** Run `LayerTagParser` spec + any spec touching the rewritten file.
- **Per wave merge:** Full `PSD2UMG.*` suite.
- **Phase gate:** All specs green; manual smoke import of 2 retagged fixtures into running editor.

### Wave 0 Gaps
- [ ] `Source/PSD2UMG/Tests/FLayerTagParserSpec.cpp` — pure-unit grammar coverage (covers TAG-01)
- [ ] No new framework install needed — automation test base already in use

## Sources

### Primary (HIGH confidence — direct repo source read)
- `.planning/phases/09-unified-layer-naming-convention-tag-based/09-CONTEXT.md` — locked decisions
- `.planning/REQUIREMENTS.md` — existing requirement IDs Phase 9 supersedes
- `.planning/STATE.md` — phase position
- `.planning/ROADMAP.md` — Phase 9 entry confirmed "[To be planned]"
- `.planning/phases/03-layer-mapping-widget-blueprint-generation/03-CONTEXT.md` — D-02/D-05/D-06/D-07 superseded
- `.planning/phases/06-advanced-layout/06-CONTEXT.md` — D-01/D-02/D-03/D-13/D-14/D-15 superseded
- `.planning/phases/07-editor-ui-preview-settings/07-CONTEXT.md` — D-09/D-10/D-11 superseded; D-05 reimport identity key (informs Risk #2)
- `Source/PSD2UMG/Private/Mapper/*.cpp` (10 files) — current parsing logic and signatures
- `Source/PSD2UMG/Private/Generator/FAnchorCalculator.cpp` — suffix table to delete
- `Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp` — anchor-calc call sites + auto-row/column logic (Risk #5)
- `Source/PSD2UMG/Private/Generator/FSmartObjectImporter.cpp` — confirms no current `@smartobject:TypeName` consumer
- `Source/PSD2UMG/Private/Animation/FPsdWidgetAnimationBuilder.cpp` lines 145-159 — `_show`/`_hide`/`_hover` detection
- `Source/PSD2UMG/Private/UI/SPsdImportPreviewDialog.cpp` lines 34-69 — preview dialog inference
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — confirms no current name-parsing in parser; `OutLayer.Name` populated then untouched
- `Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp` — registration order (priority bands 50/100/150/200/210)
- `Source/PSD2UMG/Tests/*.cpp` — spec assertion patterns (existing) and `Source/PSD2UMG/Tests/Fixtures/*.psd` fixture inventory
- `CLAUDE.md` — UE 5.7 / C++20 / Editor-only constraints

### Secondary (MEDIUM confidence)
- None — phase is internal refactor, no external libraries researched.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — design grounded in actual repo struct usage; UE5 idioms (`TOptional`, `TStringView`, `FString::ParseIntoArrayWS`) verified in adjacent code.
- Architecture: HIGH — refactor plan derived from direct read of every callsite.
- Pitfalls: HIGH — Pitfalls 4 + 5 caught from concrete line-number reading, not speculation.
- Migration scope: HIGH — fixture file list exact (Glob); spec line numbers exact (Grep).
- Risk #2 (reimport identity): MEDIUM — based on reading Phase 7 D-05 + Phase 9 D-15; needs user confirmation.

**Research date:** 2026-04-14
**Valid until:** Until any of: (a) PSD2UMG mapper interface changes, (b) Phase 7 reimport contract changes, (c) PhotoshopAPI write support added (would change fixture migration plan).

---

*Phase: 09-unified-layer-naming-convention-tag-based*
*Researched: 2026-04-14*

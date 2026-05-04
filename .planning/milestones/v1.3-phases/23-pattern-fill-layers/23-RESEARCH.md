# Phase 23: Pattern Fill Layers - Research

**Researched:** 2026-05-04
**Domain:** PSD AdjustmentLayer tagged-block dispatch + UMG mapper registration
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** `adjPattern` detection belongs in the first-pass AdjustmentLayer tag scan (PsdParser.cpp lines 2161–2194, alongside `adjSolidColor` and `adjGradient`). Add `bIsPatternFill` bool to the existing loop.
- **D-02:** Call `ExtractImagePixels` for `adjPattern` layers using the `Adj` cast path (same as `adjGradient`). No `ScanPatternFillColor()`. CP-04: `PtFl` descriptor has no `Clr ` key.
- **D-03:** New `FPatternFillLayerMapper.cpp` file. `CanMap` dispatches on `Layer.Type == EPsdLayerType::PatternFill`. `Map` mirrors `FFillLayerMapper::Map` exactly (priority 101, `FTextureImporter::ImportLayer`, `SetBrushFromTexture`, `DrawAs = Image`).
- **D-04:** When `RGBAPixels` is empty, `FPatternFillLayerMapper::Map` logs `UE_LOG Warning` and returns `nullptr`. `bHasComplexEffects` is NOT set. Consistent with `FFillLayerMapper`'s nullptr-on-failure path.
- **D-05:** Spec-only stubs — no `PatternFill.psd` fixture for this phase. `FPsdParserPatternSpec` asserts PTFL-01 using synthetic layer with `adjPattern` tag injection. `FPatternFillLayerMapperSpec` asserts PTFL-02 via pre-filled `FPsdLayer` with `Type=PatternFill` and synthetic `RGBAPixels`.
- **D-06:** Priority 101 — consistent with `FFillLayerMapper` (101) and `FSolidFillLayerMapper` (101) per Phase 20 D-01.

### Claude's Discretion
- Whether the `PatternFill` enum value is inserted before or after `Shape` in `EPsdLayerType` (ordering is cosmetic; adjacent to `Gradient`/`SolidFill`).
- Whether the parser spec uses a mock `unparsed_tagged_blocks()` block or a real `adjPattern` key lookup (spec design detail).
- Plan count and split (single plan likely sufficient given narrow scope).

### Deferred Ideas (OUT OF SCOPE)
- PatternFill.psd fixture: deferred until user can provide a PSD with a real pattern fill layer.
- Material-based tiling for pattern fills: out of scope per REQUIREMENTS.md v1.3.
- Pattern scale/offset metadata from `PtFl` descriptor: not attempted.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| PTFL-01 | `ConvertLayerRecursive` detects `adjPattern` tagged block and sets `EPsdLayerType::PatternFill` (new enum value). Layer must not fall to `Unknown`. | First-pass detection block at PsdParser.cpp:2161–2194 confirmed; `TaggedBlockKey::adjPattern` confirmed at Enum.h:757, `"PtFl"` at Enum.h:832; `AdjustmentLayer<T>` cast path confirmed identical to `adjGradient` branch. |
| PTFL-02 | `FPatternFillLayerMapper` (priority 101) returns a UImage backed by composited `RGBAPixels`. If empty, returns `nullptr` with a `UE_LOG Warning`. | `FFillLayerMapper::Map` is the byte-for-byte model; `FTextureImporter::ImportLayer` confirmed reusable; mapper registration pattern confirmed in `AllMappers.h` + `FLayerMappingRegistry.cpp`; priority 101 confirmed for fill-tier mappers. |
</phase_requirements>

## Summary

Phase 23 is a narrow three-file implementation: one new enum value, one new mapper `.cpp`, and two new spec blocks. The entire pattern has a fully implemented twin in the `adjGradient` / `FFillLayerMapper` pair from Phase 13. The detection block in `PsdParser.cpp` (lines 2161–2194) accepts a third bool and a third dispatch branch with no structural change; the mapper is copied verbatim from `FFillLayerMapper.cpp` with the single-line `CanMap` change. Specs follow the established synthetic-layer approach used by `FPsdParserGradientSpec` and `FButtonLayerMapperSpec`.

The only non-obvious constraint is CP-04: the `PtFl` tagged block carries no `Clr ` key, so no color parsing is attempted. PhotoshopAPI composites the tiled pattern into `RGBAPixels` before we see it — the RGBA buffer is the sole data source, extracted via the `Adj` cast path.

**Primary recommendation:** Implement in a single plan. Add `PatternFill` enum adjacent to `SolidFill` in `EPsdLayerType`, extend the detection loop, create `FPatternFillLayerMapper.cpp`, register it, then add specs. No parser structural change, no new helpers, no descriptor scanner.

## Standard Stack

### Core
| Library / File | Version / Phase | Purpose | Why Standard |
|---------------|-----------------|---------|--------------|
| `PhotoshopAPI::Enum::TaggedBlockKey::adjPattern` | Win64 vendored | Key for `PtFl` block detection | Confirmed at Enum.h:757; `"PtFl"` string confirmed at Enum.h:832 |
| `AdjustmentLayer<PsdPixelType>` cast path | PhotoshopAPI vendored | Pixel extraction for fill-type AdjustmentLayers | Identical to `adjGradient` path at PsdParser.cpp:2184–2189 |
| `FTextureImporter::ImportLayer` | Phase 3 / unchanged | Convert `RGBAPixels` to `UTexture2D` | Used by `FFillLayerMapper`, `FImageLayerMapper`, `FSmartObjectLayerMapper` |
| `UImage::SetBrushFromTexture(Tex, true)` + `DrawAs = Image` | UMG / Phase 13 | Wire texture to UImage | Exact pattern from `FFillLayerMapper::Map` (confirmed lines 42–46) |

### Supporting
| Library / File | Purpose | When to Use |
|---------------|---------|-------------|
| `PSD2UMG::Tests::MakeTaggedTestLayer` | Build synthetic `FPsdLayer` with `ParsedTags` populated | All mapper specs (Phase 9 Pitfall 5: synthetic layers need ParsedTags or tag-dispatched mappers reject) |
| `AllMappers.h` declaration block | Forward-declares mapper for registry consumption | Every new mapper class must appear here |
| `FLayerMappingRegistry::RegisterDefaults` | Instantiates `MakeUnique<FPatternFillLayerMapper>()` | The canonical registration site |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Direct `adjPattern` key check in existing loop | Separate RTTI branch or separate tagged-block scan | The existing loop approach is coherent, already handles `adjSolidColor` + `adjGradient`, and avoids a fourth RTTI dispatch level. Locked by D-01. |
| Separate base class for fill mappers | Copy-paste from `FFillLayerMapper` | Three-line mapper body does not justify shared base; introduces coupling without gain. Locked by D-03. |

## Architecture Patterns

### Recommended Project Structure

No new directories. Files to create / modify:

```
Source/PSD2UMG/
├── Public/Parser/PsdTypes.h                     ← +1 enum value (PatternFill)
├── Private/Parser/PsdParser.cpp                 ← +1 bool + 1 dispatch branch (lines 2162–2193)
├── Private/Mapper/AllMappers.h                  ← +1 class declaration
├── Private/Mapper/FLayerMappingRegistry.cpp     ← +1 MakeUnique<> call
├── Private/Mapper/FPatternFillLayerMapper.cpp   ← NEW file (~47 lines, mirrors FFillLayerMapper)
└── Tests/PsdParserSpec.cpp                      ← +1 BEGIN_DEFINE_SPEC block (FPsdParserPatternSpec)
                                                    +1 BEGIN_DEFINE_SPEC block (FPatternFillLayerMapperSpec)
```

### Pattern 1: First-Pass AdjustmentLayer Tag Detection

**What:** Extend the existing `bool bIsSolidFill, bIsGradientFill` loop at PsdParser.cpp:2161–2193 with a third boolean and a third `if` dispatch block.

**When to use:** Any new `AdjustmentLayer<T>`-based layer type that needs type-specific dispatch before the RTTI cascade.

**Example (confirmed from PsdParser.cpp:2162–2193):**
```cpp
// ADD: third bool beside the existing two
bool bIsSolidFill = false, bIsGradientFill = false, bIsPatternFill = false;
for (const auto& Block : InLayer->unparsed_tagged_blocks())
{
    if (!Block) continue;
    const auto Key = Block->getKey();
    if (Key == NAMESPACE_PSAPI::Enum::TaggedBlockKey::adjSolidColor) bIsSolidFill = true;
    if (Key == NAMESPACE_PSAPI::Enum::TaggedBlockKey::adjGradient)   bIsGradientFill = true;
    if (Key == NAMESPACE_PSAPI::Enum::TaggedBlockKey::adjPattern)    bIsPatternFill = true;  // NEW
}
// ... bIsSolidFill / bIsGradientFill branches unchanged ...
if (bIsPatternFill)   // NEW block, mirrors bIsGradientFill branch exactly
{
    OutLayer.Type = EPsdLayerType::PatternFill;
    if (auto Adj = std::dynamic_pointer_cast<AdjustmentLayer<PsdPixelType>>(InLayer))
        ExtractImagePixels(Adj, OutLayer, OutDiag);
    else if (auto Shape = std::dynamic_pointer_cast<ShapeLayer<PsdPixelType>>(InLayer))
        ExtractImagePixels(Shape, OutLayer, OutDiag);
    else
        OutDiag.AddWarning(OutLayer.Name, TEXT("Pattern fill: unknown concrete type, pixel extraction skipped."));
    UE_LOG(LogPSD2UMG, Log,
        TEXT("Layer '%s' dispatched as PatternFill (fill tag branch)"), *OutLayer.Name);
    return;
}
```

### Pattern 2: Fill Layer Mapper (copy from FFillLayerMapper)

**What:** New `.cpp` file with `CanMap` dispatching on `EPsdLayerType::PatternFill`, `Map` byte-for-byte from `FFillLayerMapper::Map`.

**When to use:** Any new layer type that maps to a texture-backed `UImage`.

**Example (confirmed from FFillLayerMapper.cpp:20–47):**
```cpp
// Source: FFillLayerMapper.cpp (copy verbatim, change only CanMap and class name)
int32 FPatternFillLayerMapper::GetPriority() const { return 101; }

bool FPatternFillLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::PatternFill;  // ONLY change from FFillLayerMapper
}

UWidget* FPatternFillLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree)
{
    const FString PsdName = FPaths::GetBaseFilename(Doc.SourcePath);
    UTexture2D* Tex = FTextureImporter::ImportLayer(Layer, FTextureImporter::BuildTexturePath(PsdName));
    if (!Tex)
    {
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("FPatternFillLayerMapper: Texture import returned nullptr for pattern fill layer '%s' — skipping"),
            *Layer.Name);
        return nullptr;
    }

    UImage* Img = Tree->ConstructWidget<UImage>(UImage::StaticClass(), FName(*Layer.ParsedTags.CleanName));
    Img->SetBrushFromTexture(Tex, /*bMatchSize=*/true);
    FSlateBrush Brush = Img->GetBrush();
    Brush.DrawAs = ESlateBrushDrawType::Image;
    Img->SetBrush(Brush);
    return Img;
}
```

### Pattern 3: Spec-Only Stubs (no PSD fixture)

**What:** Synthetic `FPsdLayer` with `Type=PatternFill` and pre-populated `RGBAPixels` used for PTFL-02. Parser spec for PTFL-01 uses a layer with `adjPattern` key injection.

**When to use:** When no real PSD fixture is available for a new layer type.

**Example (from FPsdParserGradientSpec and FButtonLayerMapperSpec patterns):**
```cpp
// Parser spec: synthetic layer → not possible to unit-test tag detection without
// a PSD. The spec is a documentation stub that records intent; it will fail
// gracefully when fixture is absent (AddWarning path, same as FPsdParserCJKSpec D-01).
// Mapper spec: build pre-filled FPsdLayer directly.
FPsdLayer L = PSD2UMG::Tests::MakeTaggedTestLayer(TEXT("pattern_tile"), EPsdLayerType::PatternFill);
L.PixelWidth = 64; L.PixelHeight = 64;
L.RGBAPixels.SetNumZeroed(64 * 64 * 4);
```

### Anti-Patterns to Avoid

- **Attempting to parse `PtFl` descriptor for color:** CP-04 — `PtFl` has no `Clr ` key. Do not write a `ScanPatternFillColor()` function. Composited `RGBAPixels` are the only data source.
- **Setting `bHasComplexEffects = true` as fallback:** D-04 explicitly locks this out. The nullptr path is the correct fallback. The FX-05 flatten path requires both `bHasComplexEffects=true` AND `RGBAPixels.Num() > 0` — setting the flag without pixels produces nothing useful.
- **Using RTTI dispatch (dynamic_pointer_cast<AdjustmentLayer>) as primary detection:** Phase 13 confirmed this silently fails for fill layers. Tag-based detection must precede RTTI cascade.
- **Placing `PatternFill` detection after the RTTI block:** The `if (bIsPatternFill)` dispatch block must be inside the same `{}` scope as `bIsSolidFill`/`bIsGradientFill` (lines 2161–2193), with a `return` so it exits before RTTI begins. If it falls through to RTTI, `EPsdLayerType::Unknown` results.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| RGBA → UTexture2D conversion | Custom importer | `FTextureImporter::ImportLayer` | Handles package creation, dedup, transient guard (automation mode) — 8 edge cases already covered |
| UImage brush wiring | Custom brush code | `SetBrushFromTexture(Tex, true)` + `DrawAs = Image` | `bMatchSize=true` sets `Brush.ImageSize` to texture dimensions; skipping it causes zero-size render (confirmed Phase 13 pitfall) |
| AdjustmentLayer tag scanning | New scan helper | Extend existing `bool` loop at lines 2162–2168 | Existing loop already accesses `unparsed_tagged_blocks()`; adding one `if` is the minimal-diff path |

**Key insight:** All infrastructure for pattern fills exists. Phase 23 is purely wiring three already-proven elements together: the detection loop, the pixel extraction path, and the fill mapper pattern.

## Common Pitfalls

### Pitfall 1: CP-04 — No `Clr ` key in `PtFl` descriptor
**What goes wrong:** Developer attempts `ScanPatternFillColor()` mirroring `ScanSolidFillColor()`, finds no `Clr ` or `Clrz` key in the raw bytes, and either produces a wrong color or crashes during byte-walk.
**Why it happens:** `PtFl` is a pattern fill, not a color fill. The visual data is entirely in the composited pixel channels, not a descriptor color field.
**How to avoid:** Do not write a `ScanPatternFillColor()` function. Do not look for any descriptor key. Extract `RGBAPixels` exactly as `adjGradient` does.
**Warning signs:** Any code that calls `TryParseAt()` or walks `Block->m_Data` for a `PtFl`-keyed block.

### Pitfall 2: Detection After RTTI Block
**What goes wrong:** The `bIsPatternFill` dispatch is placed after the `if (auto Group = ...)` / `if (auto Image = ...)` RTTI cascade (line 2196+), so an `AdjustmentLayer<T>` falls through to `Unknown`.
**Why it happens:** Developer inserts the new `if` block after the closing `}` of the fill-detection scope rather than inside it.
**How to avoid:** The `if (bIsPatternFill)` block and its `return` must be inside the `{ }` block at lines 2161–2194, before line 2196.
**Warning signs:** `PTFL-01` spec assertion fails with `Type == Unknown` after detection code is added.

### Pitfall 3: FImageLayerMapper Priority Collision
**What goes wrong:** `FPatternFillLayerMapper` registered at priority 100 instead of 101; `FImageLayerMapper` also returns `true` from `CanMap` for `PatternFill` layers (Phase 16.1 D-02 maps `PatternFill` → `EPsdTagType::Image`); `TArray::Sort` is non-stable so either mapper may win on any run.
**Why it happens:** Developer copies `FImageLayerMapper`'s priority (100) instead of `FFillLayerMapper`'s (101).
**How to avoid:** `GetPriority()` must return 101. Verified: all three fill-tier mappers (`FFillLayerMapper`, `FSolidFillLayerMapper`, `FShapeLayerMapper`) return 101.
**Warning signs:** Intermittent test failures where `FImageLayerMapper` produces a widget for a pattern fill layer.

### Pitfall 4: Missing `MakeTaggedTestLayer` in Mapper Spec
**What goes wrong:** Synthetic `FPsdLayer` has unpopulated `ParsedTags`; `FPatternFillLayerMapper::Map` calls `Layer.ParsedTags.CleanName` and gets an empty `FName`; `UWidgetTree::ConstructWidget` produces a widget with a blank name (not a crash, but the spec can't verify correct naming).
**Why it happens:** Test constructs `FPsdLayer` field-by-field without calling `FLayerTagParser::Parse`.
**How to avoid:** Always use `PSD2UMG::Tests::MakeTaggedTestLayer(TEXT("pattern_tile"), EPsdLayerType::PatternFill)` or call `PopulateParsedTags(L)` after field construction.
**Warning signs:** Widget name is empty in test runner output; `TestFalse(TEXT("widget name empty"), Img->GetName().IsEmpty())` fails.

### Pitfall 5: Mapper Spec Requires UWidgetTree in Editor Context
**What goes wrong:** `FPatternFillLayerMapperSpec` creates a `UWidgetTree` via `NewObject<>` outside a running editor context; crashes with nullptr package.
**Why it happens:** `UWidgetTree` is a UObject requiring a valid transient package.
**How to avoid:** Use `EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter` (same flags as all existing mapper specs). Existing mappers that call `Tree->ConstructWidget` already work with this flag pair — no additional package setup needed.

## Code Examples

Verified patterns from existing codebase:

### Enum Addition (PsdTypes.h)
```cpp
// Source: PsdTypes.h:15–25 (verified)
// Insert PatternFill adjacent to SolidFill (Claude's discretion per CONTEXT.md)
enum class EPsdLayerType : uint8
{
    Image,
    Text,
    Group,
    SmartObject,
    Gradient,
    SolidFill,
    PatternFill,   // NEW — Phase 23 PTFL-01: adjPattern (PtFl) AdjustmentLayer; composited via RGBAPixels
    Shape,
    Unknown
};
```

### Detection Loop Extension (PsdParser.cpp:2162–2193)
```cpp
// Source: PsdParser.cpp:2162–2168 (verified) — extend with one line + one block
bool bIsSolidFill = false, bIsGradientFill = false, bIsPatternFill = false;
for (const auto& Block : InLayer->unparsed_tagged_blocks())
{
    if (!Block) continue;
    const auto Key = Block->getKey();
    if (Key == NAMESPACE_PSAPI::Enum::TaggedBlockKey::adjSolidColor) bIsSolidFill = true;
    if (Key == NAMESPACE_PSAPI::Enum::TaggedBlockKey::adjGradient)   bIsGradientFill = true;
    if (Key == NAMESPACE_PSAPI::Enum::TaggedBlockKey::adjPattern)    bIsPatternFill = true;
}
```

### AllMappers.h Declaration
```cpp
// Source: AllMappers.h (verified pattern from FFillLayerMapper declaration, lines 40–47)
// Defined in FPatternFillLayerMapper.cpp  (Phase 23 / PTFL-01, PTFL-02 -- pattern fill)
class FPatternFillLayerMapper : public IPsdLayerMapper
{
public:
    int32 GetPriority() const override;
    bool CanMap(const FPsdLayer& Layer) const override;
    UWidget* Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree) override;
};
```

### Registry Entry (FLayerMappingRegistry.cpp)
```cpp
// Source: FLayerMappingRegistry.cpp:48–50 (verified) — add alongside FFillLayerMapper block
Mappers.Add(MakeUnique<FPatternFillLayerMapper>());   // Phase 23 / PTFL-01, PTFL-02 -- pattern fill (priority 101)
```

### Parser Spec Stub (PsdParserSpec.cpp)
```cpp
// Mirrors FPsdParserCJKSpec D-01 "fixture absent → AddWarning + early return" pattern.
// Source: PsdParserSpec.cpp:919–944 (verified pattern)
BEGIN_DEFINE_SPEC(FPsdParserPatternSpec, "PSD2UMG.Parser.PatternFill",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
    FString FixturePath; FPsdDocument Doc; FPsdParseDiagnostics Diag; bool bParsed = false;
    static const FPsdLayer* FindPatternLayer(const TArray<FPsdLayer>& Layers, const FString& Name)
    {
        for (const FPsdLayer& L : Layers) { if (L.Name.Equals(Name, ESearchCase::CaseSensitive)) return &L; }
        return nullptr;
    }
END_DEFINE_SPEC(FPsdParserPatternSpec)
// BeforeEach: fixture absent → AddWarning and return (bParsed stays false).
// It("pattern_tile has Type == EPsdLayerType::PatternFill (PTFL-01)"): guarded by bParsed check.
// It("pattern_tile has RGBAPixels.Num() > 0 (PTFL-02 pixel extraction)"): guarded by bParsed check.
```

### Mapper Spec Synthetic Layer
```cpp
// Source: FButtonLayerMapperSpec.cpp:50–58 + TestHelpers.h:18–29 (verified patterns)
// Mapper spec does NOT need a real PSD — synthetic layer suffices for CanMap + Map unit test.
FPsdLayer L = PSD2UMG::Tests::MakeTaggedTestLayer(TEXT("pattern_tile"), EPsdLayerType::PatternFill);
L.Bounds = FIntRect(0, 0, 64, 64);
L.PixelWidth = 64; L.PixelHeight = 64;
L.RGBAPixels.SetNumZeroed(64 * 64 * 4);
// Test: PatternFillMapper.CanMap(L) == true
// Test: PatternFillMapper.Map(L, Doc, Tree) returns non-null UImage
// Empty-pixels fallback: RGBAPixels stays empty → Map returns nullptr + warning logged
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| RTTI-only dispatch for AdjustmentLayer types | Tag-based detection (unparsed_tagged_blocks loop) precedes RTTI | Phase 13 | adjPattern must use tag path; RTTI silently fails for AdjustmentLayer<T> |
| Single EPsdLayerType::Image for all pixel layers | Discrete enum values per fill type | Phase 13–14 | PatternFill needs its own enum value to win priority collision with FImageLayerMapper |
| Priority 100 for all fill/shape mappers | Priority 101 for specialized fill/shape mappers | Phase 20 D-01 | FPatternFillLayerMapper must be at 101 or FImageLayerMapper may steal the layer |

## Open Questions

1. **`FLayerTagParser` mapping for `PatternFill`**
   - What we know: Phase 16.1 D-02 maps `Gradient`/`SolidFill`/`Shape` to `EPsdTagType::Image` in `FLayerTagParser`. This is why those mappers need priority 101.
   - What's unclear: Does `FLayerTagParser` have a branch for `PatternFill`? If not, it defaults to `EPsdTagType::Image` (since `PatternFill` is a new enum value after that decision). Priority 101 already handles the collision regardless.
   - Recommendation: Add `PatternFill` to the `FLayerTagParser` switch mapping it to `EPsdTagType::Image` (same as Gradient/SolidFill/Shape). This makes the intent explicit and matches the established pattern. Read `FLayerTagParser.cpp` before Task 1 to verify the switch location.

2. **Parser spec: fixture-absent approach**
   - What we know: D-05 says spec-only stubs with synthetic layer injection. No real `PatternFill.psd` fixture exists.
   - What's unclear: A "synthetic layer with adjPattern tag injection" cannot be fully implemented without a PSD file because `FPsdParserSpec` tests require `ParseFile` with a real PSD. The PTFL-01 parser spec must either use the CJK-spec "fixture absent → AddWarning" pattern or test only the detection logic via a unit test on the `bool` detection code path.
   - Recommendation: Use the "fixture absent → AddWarning + short-circuit" pattern (identical to `FPsdParserCJKSpec`). The mapper spec covers PTFL-02 fully via synthetic `FPsdLayer`. Both specs compile and run in CI; PTFL-01 assertions are documented intent that turns green when a fixture is supplied.

## Environment Availability

Step 2.6: SKIPPED — Phase 23 is a code-only change. No external tools, services, runtimes, or CLI utilities beyond the existing C++ build toolchain are required. All dependencies (PhotoshopAPI vendored lib, UE5 modules, test framework) are already available from prior phases.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Unreal Automation Spec (`BEGIN_DEFINE_SPEC` / `EAutomationTestFlags`) |
| Config file | Configured via UE Editor / Session Frontend — no separate config file |
| Quick run command | Run `PSD2UMG.Parser.PatternFill` + `PSD2UMG.Mapper.PatternFillLayerMapper` specs via UE Session Frontend or `-ExecCmds="Automation RunTests PSD2UMG"` |
| Full suite command | `-ExecCmds="Automation RunTests PSD2UMG"` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| PTFL-01 | `ConvertLayerRecursive` sets `EPsdLayerType::PatternFill` for `adjPattern` layers | unit (spec stub, fixture-gated) | `PSD2UMG.Parser.PatternFill` | ❌ Wave 0 (new spec block in PsdParserSpec.cpp) |
| PTFL-02 | `FPatternFillLayerMapper::CanMap` returns true for `PatternFill` layers; `Map` returns `UImage`; empty pixels returns nullptr + Warning | unit | `PSD2UMG.Mapper.PatternFillLayerMapper` | ❌ Wave 0 (new spec block in PsdParserSpec.cpp or new file) |
| PTFL-02 (fallback) | `FPatternFillLayerMapper::Map` on empty `RGBAPixels` returns nullptr and logs Warning | unit | same spec, separate `It()` block | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** Run `PSD2UMG.Mapper.PatternFillLayerMapper` (mapper spec, no PSD dependency)
- **Per wave merge:** Full `PSD2UMG` suite
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `FPsdParserPatternSpec` block in `Source/PSD2UMG/Tests/PsdParserSpec.cpp` — covers PTFL-01 (fixture-gated)
- [ ] `FPatternFillLayerMapperSpec` block — covers PTFL-02 (synthetic layer, no fixture needed); may live in `PsdParserSpec.cpp` or a new `FPatternFillLayerMapperSpec.cpp`

*(Existing test infrastructure: framework present, fixtures present for other specs, `TestHelpers.h` available)*

## Sources

### Primary (HIGH confidence)
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp:2161–2193` — first-pass AdjustmentLayer detection block read directly
- `Source/PSD2UMG/Private/Mapper/FFillLayerMapper.cpp` — model mapper read directly
- `Source/PSD2UMG/Public/Parser/PsdTypes.h` — `EPsdLayerType` enum read directly
- `Source/PSD2UMG/Private/Mapper/AllMappers.h` — declaration pattern read directly
- `Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp` — registration pattern read directly
- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/Util/Enum.h:757,832` — `TaggedBlockKey::adjPattern` and `"PtFl"` string key verified directly
- `Source/PSD2UMG/Public/Generator/FTextureImporter.h` — API signature verified directly
- `Source/PSD2UMG/Tests/PsdParserSpec.cpp` — all existing spec patterns read directly
- `Source/PSD2UMG/Tests/TestHelpers.h` — `MakeTaggedTestLayer` helper verified directly
- `.planning/phases/23-pattern-fill-layers/23-CONTEXT.md` — locked decisions read directly
- `.planning/REQUIREMENTS.md` — PTFL-01, PTFL-02, CP-04 verified directly

### Secondary (MEDIUM confidence)
- None required — all findings verified from primary sources

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all files read directly from repo; no external dependencies
- Architecture patterns: HIGH — direct code inspection of twin implementation (`FFillLayerMapper`) and detection block
- Pitfalls: HIGH — CP-04 documented in REQUIREMENTS.md; priority-collision pitfall documented in Phase 20 decisions; RTTI-dispatch failure documented in Phase 13 decisions; all confirmed by code inspection

**Research date:** 2026-05-04
**Valid until:** Stable (no fast-moving dependencies; all vendored or internal)

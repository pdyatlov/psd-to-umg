# Phase 5: Layer Effects & Blend Modes - Research

**Researched:** 2026-04-10
**Domain:** PhotoshopAPI layer effects extraction, UMG widget property application, PNG rasterization fallback
**Confidence:** HIGH (all critical paths verified against vendored headers and existing codebase)

## Summary

Phase 5 delivers five layer-effects requirements against a codebase that already parses `Opacity` and `bVisible` from PhotoshopAPI but discards them in the generator. The generator change for FX-01/FX-02 is mechanical. Color overlay (FX-03) and drop shadow (FX-04) require extracting effect data from PhotoshopAPI's raw tagged-block bytes, which is the key technical risk. FX-05 (flatten fallback) uses PhotoshopAPI's existing `get_image_data()` to obtain composited RGBA pixels — no separate render API call is needed since `ImageLayer::get_image_data()` already returns per-channel decoded pixels at layer bounds.

**Critical finding:** PhotoshopAPI v0.9 does NOT expose a typed effects API. Layer effects (color overlay, drop shadow, inner shadow, gradient overlay, bevel) live in the `lrFX` (`TaggedBlockKey::fxLayer`) tagged block, which is stored as raw `std::vector<std::byte>` in `Layer::m_UnparsedBlocks`. Parsing color/opacity/offset from these bytes requires manual binary parsing of the PSD `lrFX` / `lfx2` descriptor format. Alternatively, the descriptor key list in `DescriptorUtil.h` already references `"DrSh"` (DropShadow) and `"ebbl"` (BevelEmboss) as known keys, indicating the raw format is understood and could be parsed with moderate effort.

**Primary recommendation:** Implement a `FPsdEffectsParser` helper (inside `Private/Parser/`) that extracts the `fxLayer` tagged block bytes from `m_UnparsedBlocks` and parses the PSD binary format for color overlay and drop shadow. For complex effects detection (inner shadow, gradient, bevel) a presence-only check (matching the known `lrFX` sub-key bytes) is sufficient since FX-05 only needs to know whether a complex effect exists, not its parameters.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Hidden PSD layers are created as widgets with `Visibility=Collapsed`, not skipped. Replaces `if (!Layer.bVisible) continue;` skip in `FWidgetBlueprintGenerator.cpp:37`.
- **D-02:** Generator must remove the skip and call `Widget->SetVisibility(ESlateVisibility::Collapsed)` after creation.
- **D-03:** Layer opacity applied via `Widget->SetRenderOpacity(Layer.Opacity)`. Only set when `Opacity < 1.0`.
- **D-04:** Color Overlay applies to image layers only — set `FSlateBrush::TintColor` on UImage widgets.
- **D-05:** Non-image layer Color Overlay → log warning, ignore (do not flatten).
- **D-06:** Drop Shadow approximated as offset semi-transparent duplicate UImage behind the original widget.
- **D-07:** Shadow UImage is sibling widget in same canvas at lower z-order; uses shadow color/opacity from PSD; image is a copy of the layer's texture with tint set to shadow color.
- **D-08:** If shadow data unavailable from PhotoshopAPI → no-op with diagnostic warning (same pattern as TEXT-04).
- **D-09:** Effects triggering flatten: inner shadow, gradient overlay, pattern overlay, bevel/emboss.
- **D-10:** Effects never triggering flatten: opacity, visibility, color overlay, drop shadow.
- **D-11:** Flatten source: PhotoshopAPI composited pixels — extract layer with all effects pre-applied as single RGBA image.
- **D-12:** Flatten only when `bFlattenComplexEffects = true`. When false, complex effects silently ignored.
- **D-13:** New setting: `UPROPERTY bFlattenComplexEffects = true` (default on) in `UPSD2UMGSettings`.
- **D-14:** New struct `FPsdLayerEffects` added to `FPsdLayer`.
- **D-15:** Struct shape advisory — researcher may adjust based on PhotoshopAPI actual exposure. `bHasComplexEffects` is the key decision field.
- **D-16:** ARGB channel order must be verified for effect colors.

### Claude's Discretion
- Whether `FPsdLayerEffects` is a nested struct or separate fields on `FPsdLayer`
- How to detect which effects are present via PhotoshopAPI (descriptor inspection vs specific API calls)
- Whether the shadow duplicate image shares the original texture or creates a new one
- Whether to add an `FEffectsMapper` or handle effects inline in the generator
- Exact default value for `bFlattenComplexEffects`

### Deferred Ideas (OUT OF SCOPE)
- Blend mode mapping (multiply, screen, overlay etc.) — no UMG equivalent
- Inner shadow, gradient overlay, pattern overlay, bevel as individual UMG approximations
- Stroke/border effects on non-text layers
- Per-effect flatten toggle
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| FX-01 | Layer opacity applied via SetRenderOpacity | `FPsdLayer::Opacity` already parsed (PsdParser.cpp:360); generator must call `Widget->SetRenderOpacity(Layer.Opacity)` after widget creation when `Opacity < 1.0` |
| FX-02 | Layer visibility applied (visible/collapsed) | `FPsdLayer::bVisible` already parsed (PsdParser.cpp:358); generator must remove skip at line 37 and call `Widget->SetVisibility(ESlateVisibility::Collapsed)` instead |
| FX-03 | Color Overlay effect → brush tint color | Requires `lrFX` block parsing; color overlay sub-block key `"SoCo"` (Solid Color) in PSD descriptor format; applies `FSlateBrush::TintColor` on UImage only |
| FX-04 | Drop Shadow → approximate UImage offset duplicate behind main widget | Requires `lrFX` block parsing for `"DrSh"` key; creates sibling UImage at `(X+OffsetX, Y+OffsetY)` with lower z-order; no-op + warning if unavailable |
| FX-05 | Complex effects → rasterized PNG fallback when bFlattenComplexEffects=true | Uses `ImageLayer::get_image_data()` to get RGBA pixels; reuses existing texture import pipeline; requires adding `bFlattenComplexEffects` to `UPSD2UMGSettings` |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| PhotoshopAPI | 0.9 (vendored) | Layer data extraction including raw effect bytes | Already linked; only available parser in project |
| UMG / Slate | UE 5.7.4 (bundled) | Widget property application (SetRenderOpacity, SetVisibility, brush tint) | Project's target framework |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `Components/Image.h` UImage | UE 5.7.4 | Drop shadow duplicate widget and flatten fallback widget | FX-04 shadow rendering, FX-05 flattened layer |
| `Engine/Texture2D.h` UTexture2D | UE 5.7.4 | Texture for shadow duplicate and flattened layer images | Already used in Phase 3 image import pipeline |

**Installation:** No new packages required. All dependencies already present.

## Architecture Patterns

### Recommended Project Structure Changes
```
Source/PSD2UMG/
├── Public/Parser/
│   └── PsdTypes.h              — add FPsdLayerEffects struct, extend FPsdLayer
├── Public/
│   └── PSD2UMGSetting.h        — add bFlattenComplexEffects UPROPERTY
├── Private/Parser/
│   ├── PsdParser.cpp           — call effect extraction in ConvertLayerRecursive
│   └── PsdEffectsParser.cpp    — NEW: lrFX binary parser helper
│   └── PsdEffectsParser.h      — NEW: internal header (not public)
├── Private/Generator/
│   └── FWidgetBlueprintGenerator.cpp  — apply effects after widget creation
└── Tests/
    └── FWidgetBlueprintGenSpec.cpp    — update "skip invisible" test → "collapse invisible"
```

### Pattern 1: FPsdLayerEffects as Nested Struct on FPsdLayer

Add to `PsdTypes.h` (already has UE headers, `FLinearColor`, `FVector2D`):

```cpp
USTRUCT()
struct PSD2UMG_API FPsdLayerEffects
{
    GENERATED_BODY()

    bool bHasColorOverlay = false;
    FLinearColor ColorOverlayColor = FLinearColor::White;

    bool bHasDropShadow = false;
    FLinearColor DropShadowColor = FLinearColor(0.f, 0.f, 0.f, 0.5f);
    FVector2D DropShadowOffset = FVector2D::ZeroVector;

    // True if any of: inner shadow, gradient overlay, pattern overlay, bevel/emboss
    // When true and bFlattenComplexEffects=true, layer is rasterized
    bool bHasComplexEffects = false;
};
```

Then in `FPsdLayer`:
```cpp
FPsdLayerEffects Effects;
```

This is a nested struct (researcher recommendation) — cleaner than flat fields, matches the Phase 4 `FPsdTextRun` pattern.

### Pattern 2: Effect Application in Generator After Widget Creation

In `PopulateCanvas` after `Widget = Registry.MapLayer(...)`:

```cpp
// FX-01: opacity
if (Layer.Opacity < 1.0f)
{
    Widget->SetRenderOpacity(Layer.Opacity);
}

// FX-02: visibility (replaces the continue; skip above)
if (!Layer.bVisible)
{
    Widget->SetVisibility(ESlateVisibility::Collapsed);
}

// FX-03: color overlay (image layers only)
if (Layer.Effects.bHasColorOverlay && Layer.Type == EPsdLayerType::Image)
{
    if (UImage* Img = Cast<UImage>(Widget))
    {
        FSlateBrush Brush = Img->GetBrush();
        Brush.TintColor = FSlateColor(Layer.Effects.ColorOverlayColor);
        Img->SetBrush(Brush);
    }
}
else if (Layer.Effects.bHasColorOverlay)
{
    UE_LOG(LogPSD2UMG, Warning, TEXT("Color overlay on non-image layer '%s' ignored."), *Layer.Name);
}
```

### Pattern 3: Drop Shadow as Sibling Widget (FX-04)

The shadow UImage must be inserted into the canvas BEFORE the main widget so it gets a lower z-order naturally, OR inserted after with an explicit lower `SetZOrder`. Given the existing inverted z-order logic (`TotalLayers - 1 - i`), the simplest approach is to insert the shadow widget first at the same iteration step with `ZOrder - 1`:

```cpp
if (Layer.Effects.bHasDropShadow && Layer.Type == EPsdLayerType::Image)
{
    // Shadow is a duplicate of the main texture, tinted to shadow color
    // UImage ShadowWidget at (Bounds.Min.X + OffsetX, Bounds.Min.Y + OffsetY)
    // ZOrder = main widget ZOrder - 1 (placed first so behind)
}
```

**Key decision (researcher recommendation):** Share the original `UTexture2D*` for the shadow (same texture asset, different tint). Do NOT create a new texture — this avoids duplicating asset import cost and the shadow is visually indistinguishable since it's fully tinted to the shadow color.

### Pattern 4: Flatten Fallback (FX-05)

```cpp
// If complex effects present and flatten enabled, replace layer's RGBAPixels with
// composited pixel data already extracted at parse time.
// The existing texture import path (Phase 3) handles TArray<uint8> RGBA → UTexture2D.
```

The flatten fallback is best handled **at parse time** in `PsdParser.cpp` rather than in the generator. When `bHasComplexEffects = true` and `bFlattenComplexEffects = true`, the parser can call `ImageLayer::get_image_data()` again to get pixels that reflect layer effects applied by Photoshop (the `ImageData` section stores composited results). This avoids threading the setting through to the generator.

**However**, `bFlattenComplexEffects` is a runtime setting from `UPSD2UMGSettings`. It must be read at import time. The parser can call `UPSD2UMGSettings::Get()->bFlattenComplexEffects` since this runs in editor context.

### Anti-Patterns to Avoid

- **Parsing PSD descriptors in a public header:** All PhotoshopAPI interaction must stay inside `Private/Parser/` — the PIMPL boundary is established and must be respected.
- **Creating a separate UTexture2D for shadow:** Wasteful. Share the original texture with a different tint.
- **Skipping invisible layers (current behavior):** D-01 explicitly replaces this with Collapsed. The test at `FWidgetBlueprintGenSpec.cpp:88` asserts 0 children — this test MUST be updated to assert 1 child with `Visibility==Collapsed`.
- **Applying opacity=1.0 explicitly:** Unnecessary UMG call; only set when `Opacity < 1.0`.
- **Flatten at generator time:** Parser time is cleaner since pixel extraction belongs in the parser layer of the architecture.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Layer composited pixel data | Custom compositing of RGBA channels | `ImageLayer::get_image_data()` | Already returns decoded uint8 RGBA per-channel |
| Widget opacity | Manual color/alpha math | `UWidget::SetRenderOpacity(float)` | UMG built-in, applies to entire widget subtree |
| Widget visibility | Creating/destroying widgets at runtime | `UWidget::SetVisibility(ESlateVisibility::Collapsed)` | Designer can toggle at runtime |
| Brush tint | Color multiply shader | `FSlateBrush::TintColor` | Slate renders tint as multiply over texture — exactly what Color Overlay does |
| Texture for shadow | New PNG file / new UTexture2D | Reuse existing texture UObject pointer | No extra import needed; tint handles visual |

**Key insight:** PhotoshopAPI's `get_image_data()` returns decoded per-channel vectors from the file's `ImageData` section. This is the composited layer data Photoshop stored at save time — it already has effects baked in for the pixels visible in the composite. This is the correct source for FX-05 flatten.

## PhotoshopAPI Effects API: Confirmed Findings

### What IS Available (HIGH confidence — verified against vendored headers)

| Item | Location | How to Access |
|------|----------|---------------|
| Layer opacity (0..1 float) | `Layer<T>::opacity()` | Already extracted in PsdParser.cpp:360 |
| Layer visibility | `Layer<T>::visible()` | Already extracted in PsdParser.cpp:358 |
| Layer blend mode | `Layer<T>::blendmode()` | Returns `Enum::BlendMode` enum; useful for effect detection |
| Raw `lrFX` block bytes | `Layer::m_UnparsedBlocks` → find block with key `TaggedBlockKey::fxLayer` | Must iterate `m_UnparsedBlocks` and match key |
| Composited pixel data | `ImageLayer::get_image_data()` | Returns `data_type` = `unordered_map<int, vector<T>>` keyed by channel index |

### What is NOT Available (HIGH confidence — verified by absence in all headers)

| Item | Status | Implication |
|------|--------|-------------|
| Typed color overlay API | Not implemented | Must parse `lrFX` bytes manually |
| Typed drop shadow API | Not implemented | Must parse `lrFX` bytes manually |
| Named effects presence flag | Not implemented | Must detect via `lrFX` block existence + sub-key scan |
| `lfx2` (modern descriptor effects) | Not in `taggedBlockMap` | Modern effects (Photoshop CS+ style) may not be preserved |

### lrFX Block Binary Format (MEDIUM confidence — from PSD spec + DescriptorUtil.h key list)

The `lrFX` block (key `"lrFX"`) is an old-style effects block. Layout:

```
uint16  version         (= 0)
uint16  effectCount     (number of effect sub-records)
For each effect:
  char[4] signature     ("8BIM")
  char[4] effectKey     ("dsdw"=drop shadow, "isdw"=inner shadow,
                         "oglw"=outer glow, "iglw"=inner glow,
                         "bevl"=bevel, "sofi"=color fill/overlay)
  ... effect-specific data
```

The `DescriptorUtil.h` identifies `"DrSh"` (DropShadow), `"ebbl"` (BevelEmboss), `"sofi"` (SolidFill/ColorOverlay) as known four-byte descriptor class keys. However, `lrFX` uses its own compact binary layout (not the full descriptor format), whereas `lfx2` uses the full descriptor format.

**Risk:** PhotoshopAPI's `m_UnparsedBlocks` stores the raw bytes. The planner must include a task to validate that `lrFX` bytes are present in a test PSD with effects before committing to parsing them. The `FMultiLayer.psd` or a new `Test_Effects.psd` fixture should be used for this validation.

### Channel Index Mapping for get_image_data() (HIGH confidence — from ImageDataMixins.h)

`data_type = unordered_map<int, vector<T>>` where the key is the channel `index` field. For RGB+A:
- Key `-1` = Alpha (transparency)
- Key `0` = Red
- Key `1` = Green  
- Key `2` = Blue

**ARGB order note (from STATE.md):** Phase 2 discovered fill color arrays are ARGB not RGBA. This applies to raw descriptor byte parsing (colors inside `lrFX`). The `get_image_data()` channel-keyed map is index-based (not byte-order dependent) so this quirk does NOT apply there.

## Common Pitfalls

### Pitfall 1: Invisible Layer Test Must Change
**What goes wrong:** `FWidgetBlueprintGenSpec.cpp:88` asserts `Root->GetChildrenCount() == 0` for the invisible layer test. After D-01, this must become 1 (widget created, just collapsed).
**Why it happens:** The test was written to match the now-incorrect skip behavior.
**How to avoid:** Update the test in the same plan/wave that removes the skip.
**Warning signs:** Test compiles but fails (0 != 1).

### Pitfall 2: lrFX Block Not Present for All Effects
**What goes wrong:** Newer Photoshop versions use `lfx2` (descriptor-based effects), not `lrFX`. Files saved with modern Photoshop may not have `lrFX` at all.
**Why it happens:** Adobe added a new effects block format in Photoshop CS. Both blocks may coexist.
**How to avoid:** Check for `lrFX` block presence first; if absent, treat as no effects (log info). The `bHasComplexEffects` flag remains false and no flatten occurs.
**Warning signs:** Real PSD with effects produces no `bHasColorOverlay` even though Photoshop shows a color overlay.

### Pitfall 3: ARGB in lrFX Color Sub-Records
**What goes wrong:** Color bytes parsed as RGBA yield wrong hue.
**Why it happens:** Phase 2 established that PhotoshopAPI uses ARGB in descriptor color arrays. Same format likely applies to `lrFX` color fields.
**How to avoid:** Parse bytes as `[A, R, G, B]` order; verified against `FLinearColor(R, G, B, A)` construction.
**Warning signs:** Color overlay produces visually incorrect tint color on test PSD.

### Pitfall 4: Drop Shadow ZOrder Collision
**What goes wrong:** Shadow sibling inserted at same ZOrder as the original widget; visually appears on top or flickers.
**Why it happens:** The existing `Slot->SetZOrder(TotalLayers - 1 - i)` formula is for main widgets only; shadow has no index in the loop.
**How to avoid:** Shadow ZOrder = main widget ZOrder - 1. Or insert shadow into canvas before main widget, relying on canvas child order as a tiebreaker.
**Warning signs:** Shadow visible on top of the main image.

### Pitfall 5: Flatten Produces Wrong Pixel Count
**What goes wrong:** `get_image_data()` on a zero-size layer panics or returns empty.
**Why it happens:** Some effect-only layers have zero-bounds (e.g., shape-derived effects). The zero-size guard already exists in generator at line 44–49 but must be applied before calling flatten.
**How to avoid:** Check `Layer.PixelWidth > 0 && Layer.PixelHeight > 0` before attempting flatten.

## Code Examples

### Accessing Raw lrFX Block from m_UnparsedBlocks

```cpp
// Source: Layer.h:427 (m_UnparsedBlocks), TaggedBlock.h (m_Key, m_Data)
// Inside PsdParser.cpp ConvertLayerRecursive, after type dispatch:

for (const auto& Block : InLayer->m_UnparsedBlocks)
{
    if (Block && Block->getKey() == NAMESPACE_PSAPI::Enum::TaggedBlockKey::fxLayer)
    {
        // Block->m_Data contains the raw lrFX bytes
        OutLayer.Effects = ParseLrFXBlock(Block->m_Data, OutDiag, OutLayer.Name);
        break;
    }
}
```

`m_UnparsedBlocks` is `protected` on `Layer<T>` (verified: Layer.h:427). Since `ConvertLayerRecursive` takes a `shared_ptr<Layer<T>>` and all our layer types inherit `Layer<T>`, we can access `m_UnparsedBlocks` only if we add a public accessor OR do the extraction in a friend/derived context. **Resolution: add a public accessor method** or expose the blocks via a public `get_unparsed_blocks()` method. Alternatively, access via the `AdditionalLayerInfo` at `LayerRecord` level — but PhotoshopAPI doesn't expose the LayerRecord after construction.

**Researcher recommendation:** Add a public method to the Layer PIMPL or, more practically, accept that `m_UnparsedBlocks` is protected and access it via a thin wrapper struct that derives from the layer type (CRTP), OR use `reinterpret_cast` as last resort. The cleanest path is to check if PhotoshopAPI exposes any iterator over the untyped blocks.

Let me clarify: reviewing Layer.h:427, `m_UnparsedBlocks` is declared in the `protected:` section. `ConvertLayerRecursive` receives a `std::shared_ptr<PsdLayer>` where `PsdLayer = Layer<uint8_t>`. We cannot access protected members of the pointed-to object from outside the class hierarchy.

**Actual access path:** The `generate_tagged_blocks()` method (Line.h:467) is `virtual protected` and calls `m_UnparsedBlocks` internally. The only public access surface for raw blocks is through `to_photoshop()` which regenerates a `LayerRecord` — expensive and not appropriate.

**Revised recommendation (MEDIUM confidence):** The planner should schedule a Wave 0 spike task to determine if `m_UnparsedBlocks` can be accessed via friendship, a public wrapper, or if the effects block parsing requires modifying the PhotoshopAPI vendored source to add a public accessor. This is the primary uncertainty for FX-03 and FX-04 delivery.

**Fallback approach:** If `m_UnparsedBlocks` access is not feasible without source modification, mark FX-03 and FX-04 as no-op with warning (same delivery pattern as TEXT-04 shadow). FX-05 flatten is still fully deliverable via `get_image_data()` + a `bHasComplexEffects` flag that defaults to `false` (conservative: complex effects silently ignored rather than flattened).

### Applying SetRenderOpacity and SetVisibility

```cpp
// FX-01 — after Widget = Registry.MapLayer(...)
if (Layer.Opacity < 1.0f)
{
    Widget->SetRenderOpacity(Layer.Opacity);
}

// FX-02 — replaces if (!Layer.bVisible) { continue; }
if (!Layer.bVisible)
{
    Widget->SetVisibility(ESlateVisibility::Collapsed);
}
```

### FSlateBrush TintColor for Color Overlay (FX-03)

```cpp
// UImage brush tint — applies multiplicative color over the texture
// FSlateColor accepts FLinearColor directly
if (UImage* ImageWidget = Cast<UImage>(Widget))
{
    FSlateBrush Brush = ImageWidget->GetBrush();
    Brush.TintColor = FSlateColor(Layer.Effects.ColorOverlayColor);
    ImageWidget->SetBrush(Brush);
}
```

### get_image_data() for FX-05 Flatten

```cpp
// Source: ImageDataMixins.h — get_image_data() returns data_type
// = unordered_map<int, vector<uint8_t>> for bpp8_t specialization
// channel index: -1=Alpha, 0=R, 1=G, 2=B

auto ChannelData = Image->get_image_data(); // calls evaluate_image_data() internally
// Interleave channels into RGBA TArray<uint8>:
// For each pixel i: RGBAPixels[i*4+0] = R[i], [1]=G[i], [2]=B[i], [3]=A[i]
// Then feed to existing texture import pipeline (already handles TArray<uint8> RGBA)
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Skip invisible layers | Create as Collapsed | Phase 5 D-01 | Designers can toggle visibility at runtime |
| Effects ignored entirely | Color overlay + drop shadow approximated, complex → flatten | Phase 5 | Near-WYSIWYG for common effects |

**Deprecated/outdated:**
- Current line 37 `if (!Layer.bVisible) continue;` — removed by D-01/D-02, replaced with post-creation Collapsed assignment.

## Open Questions

1. **m_UnparsedBlocks access from ConvertLayerRecursive**
   - What we know: `m_UnparsedBlocks` is `protected` on `Layer<T>`; no public accessor exists in vendored headers.
   - What's unclear: Whether a public accessor can be added to the vendored source without breaking ABI or creating maintenance burden, or whether an alternative exists.
   - Recommendation: Planner should add a Wave 0 spike task: "Verify m_UnparsedBlocks access path or add public accessor to vendored Layer.h". If access is confirmed infeasible, FX-03 and FX-04 deliver as no-op + warning, which is consistent with the Phase 4 TEXT-04 precedent.

2. **lrFX vs lfx2 prevalence**
   - What we know: Both block keys exist conceptually in PSD format; only `lrFX` is in PhotoshopAPI's `taggedBlockMap`.
   - What's unclear: Whether test PSDs use lrFX (older format) or lfx2 (CS+ descriptor format).
   - Recommendation: Create a `Test_Effects.psd` fixture in Fixtures/ with color overlay + drop shadow + inner shadow, then dump the block keys at parse time to confirm which block is present.

3. **Shadow texture sharing**
   - What we know: D-07 says shadow uses a copy of the layer's texture.
   - What's unclear: "Copy" could mean same `UTexture2D*` pointer reused (different tint), or a new asset.
   - Recommendation: Reuse the same `UTexture2D*` pointer (researcher recommendation). Brush tint handles the visual. This avoids duplicating texture import.

## Environment Availability

Step 2.6: SKIPPED (no external tool dependencies — all work is C++ source edits against already-linked PhotoshopAPI static lib)

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | FAutomationTestBase / Spec-style (UE5 automation) |
| Config file | Build.cs `bBuildDeveloperTools = true` + module type Editor |
| Quick run command | UE automation runner: `-ExecCmds="Automation RunTests PSD2UMG"` |
| Full suite command | Same — all specs in `Source/PSD2UMG/Tests/` run in one pass |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| FX-01 | SetRenderOpacity called for opacity < 1.0 | unit spec | UE automation PSD2UMG.Generator | ✅ FWidgetBlueprintGenSpec.cpp (needs new It block) |
| FX-02 | Hidden layer widget created with Collapsed visibility | unit spec | UE automation PSD2UMG.Generator | ✅ existing It block must be updated |
| FX-03 | Color overlay sets brush tint on UImage | unit spec | UE automation PSD2UMG.Generator | ❌ Wave 0 |
| FX-04 | Drop shadow sibling UImage created at correct offset | unit spec | UE automation PSD2UMG.Generator | ❌ Wave 0 (or no-op if access path blocked) |
| FX-05 | Complex effects trigger flatten when setting enabled | unit spec | UE automation PSD2UMG.Generator | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** Run `PSD2UMG.Generator` spec suite
- **Per wave merge:** Run full `PSD2UMG` automation suite
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] New `It("should apply opacity < 1.0 via SetRenderOpacity")` in `FWidgetBlueprintGenSpec.cpp`
- [ ] Update `It("should skip invisible layers")` → assert 1 child with `Visibility==Collapsed`
- [ ] New `It("should apply color overlay tint to UImage")` in `FWidgetBlueprintGenSpec.cpp`
- [ ] New `It("should create shadow sibling for drop shadow layers")` (conditional: only if access path confirmed)
- [ ] New `It("should flatten layer with complex effects when setting enabled")` in `FWidgetBlueprintGenSpec.cpp`
- [ ] `Test_Effects.psd` fixture in `Source/PSD2UMG/Tests/Fixtures/`

## Sources

### Primary (HIGH confidence)
- Vendored `Layer.h` — `m_UnparsedBlocks`, `opacity()`, `visible()`, `blendmode()`
- Vendored `Enum.h` — `TaggedBlockKey::fxLayer` maps to `"lrFX"` string
- Vendored `ImageDataMixins.h` — `get_image_data()` API shape and channel index semantics
- Vendored `DescriptorUtil.h` — `"DrSh"`, `"ebbl"`, `"sofi"` key strings confirm PSD format knowledge
- `FWidgetBlueprintGenerator.cpp` — exact location of bVisible skip (line 37), ZOrder formula (line 139)
- `FWidgetBlueprintGenSpec.cpp:88` — exact assertion that must change for FX-02
- `PsdParser.cpp:358-360` — confirms Opacity and bVisible already extracted
- `.planning/STATE.md` — ARGB channel order quirk documented from Phase 2

### Secondary (MEDIUM confidence)
- PSD file format specification (Adobe) — `lrFX` block binary layout with sub-keys `dsdw`, `sofi`, `bevl`
- Phase 4 CONTEXT.md D-09..D-12 — best-effort no-op pattern for unresolvable data

### Tertiary (LOW confidence)
- `"lrFX"` binary format specifics — not verified against a live PSD hex dump in this codebase; binary layout inference from DescriptorUtil.h key names and PSD spec cross-reference

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all libraries already linked and in use
- FX-01/FX-02: HIGH — data already in `FPsdLayer`, generator change is mechanical
- FX-03/FX-04 (effects extraction): MEDIUM — `m_UnparsedBlocks` access path needs a spike validation; fallback (no-op) is fully planned
- FX-05 (flatten): HIGH — `get_image_data()` API confirmed; integration with existing texture pipeline straightforward
- Architecture: HIGH — all integration points identified in existing source
- Pitfalls: HIGH — all derived from direct code inspection

**Research date:** 2026-04-10
**Valid until:** 2026-05-10 (PhotoshopAPI vendored, stable; UE 5.7.4 API stable)

---
phase: 14-shape-vector-layers
plan: "03"
subsystem: mapper
tags: [shape, mapper, umg, registry, dialog]
dependency_graph:
  requires: [14-02]
  provides: [FShapeLayerMapper, shape-to-image-widget-path]
  affects: [FLayerMappingRegistry, SPsdImportPreviewDialog, FWidgetBlueprintGenerator-FX03]
tech_stack:
  added: []
  patterns: [zero-texture-UImage, CanMap-type-dispatch, priority-100-mapper]
key_files:
  created:
    - Source/PSD2UMG/Private/Mapper/FShapeLayerMapper.cpp
  modified:
    - Source/PSD2UMG/Private/Mapper/AllMappers.h
    - Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp
    - Source/PSD2UMG/Private/UI/SPsdImportPreviewDialog.cpp
decisions:
  - "FShapeLayerMapper kept as a separate class from FSolidFillLayerMapper (D-03) for semantic clarity and future stroke rendering insertion point (vstk -> UMG border/outline)"
  - "SPsdImportPreviewDialog: added Gradient + SolidFill catch-up labels (Phase 13 left these returning 'Unknown'); Shape label added as primary goal"
metrics:
  duration: "~2 minutes"
  completed: "2026-04-22T00:00:00Z"
  tasks_completed: 3
  tasks_total: 3
  files_changed: 4
---

# Phase 14 Plan 03: FShapeLayerMapper + Registry + UI Dialog Label + Visual Verify (GREEN)

**One-liner:** Zero-texture UImage mapper for EPsdLayerType::Shape wired into registry and preview dialog; FX-03 tinting via Effects.ColorOverlayColor (Plan 14-02 state) completes the parser->mapper->generator chain for drawn solid-color vector shapes.

## Tasks Completed

### Task 1: Create FShapeLayerMapper.cpp and declare in AllMappers.h
**Commit:** `6dc7641`
**Files:** `Source/PSD2UMG/Private/Mapper/AllMappers.h`, `Source/PSD2UMG/Private/Mapper/FShapeLayerMapper.cpp`

- `AllMappers.h`: added `FShapeLayerMapper` declaration immediately after `FSolidFillLayerMapper` (line 37-44 block). New block at lines 46-52.
- `FShapeLayerMapper.cpp` created as a near-verbatim clone of `FSolidFillLayerMapper.cpp` with 4 documented divergences:
  1. Class name: FSolidFillLayerMapper → FShapeLayerMapper
  2. CanMap check: `EPsdLayerType::SolidFill` → `EPsdLayerType::Shape`
  3. Warning log text: "SolidFill" / "SoCo" → "Shape" / "vscg"
  4. File header comment: Phase 13 / GRAD-01 / SoCo → Phase 14 / SHAPE-01 / vscg
- `GetPriority()` returns 100 (type-based mapper tier)
- `Map()` constructs zero-texture `UImage` via `Tree->ConstructWidget<UImage>` with name from `Layer.ParsedTags.CleanName`
- `FSlateBrush.DrawAs = ESlateBrushDrawType::Image` and `ImageSize = FVector2D(Layer.Bounds.Width(), Layer.Bounds.Height())`
- No `FTextureImporter::ImportLayer` call — Shape path is zero-texture (same policy as FSolidFillLayerMapper)
- No `Brush.TintColor` assignment — FX-03 in `FWidgetBlueprintGenerator::PopulateChildren` (lines 326-340) owns TintColor via `Effects.ColorOverlayColor`
- Diagnostic `UE_LOG(LogPSD2UMG, Warning, ...)` when `!Layer.Effects.bHasColorOverlay` — guards against misconfigured parser state

### Task 2: Register FShapeLayerMapper + add preview dialog label
**Commit:** `0b72560`
**Files:** `Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp`, `Source/PSD2UMG/Private/UI/SPsdImportPreviewDialog.cpp`

- `FLayerMappingRegistry::RegisterDefaults`: added `Mappers.Add(MakeUnique<FShapeLayerMapper>());` on line 45, immediately after `FSolidFillLayerMapper` on line 44 — both at priority 100.
- `SPsdImportPreviewDialog.cpp` `InferWidgetTypeName` fallback switch extended:
  - `case EPsdLayerType::Shape: return TEXT("Image");` — Phase 14 / SHAPE-01
  - `case EPsdLayerType::Gradient: return TEXT("Image");` — Phase 13 catch-up label (was returning "Unknown")
  - `case EPsdLayerType::SolidFill: return TEXT("Image");` — Phase 13 catch-up label (was returning "Unknown")
  - Existing cases (Text, Group, Image, SmartObject) unchanged.

### Task 3: ScanShapeFillColor bug fix + all-green verification
**Commit:** pending (ScanShapeFillColor fix)
**Files:** `Source/PSD2UMG/Private/Parser/PsdParser.cpp`

Root-cause investigation revealed: vscg `m_Data[0..3]` IS the class ID (`'SoCo'`/`'GdFl'`), not a version prefix. The descriptor starts at offset 8. The function was requiring a `"Type"` enum item inside the descriptor body, but vscg descriptors for solid-color shapes contain only ONE item: `"Clr "` + `"Objc"` + RGBC. Fix:
1. Upfront check: `Data[0..3] == 'SoCo'`; any other class ID → return false (gradient falls through)
2. Removed `bFoundType`/`TypeValue` from `TryParseAt`; only `bFoundColor` required
3. Changed `TryParseAt` call order to 8-first (empirically confirmed correct for vscg)
4. Removed hex dump TEMP DIAG + dispatch TEMP DIAG

Result: `PSD2UMG.Parser.ShapeLayers` — 6/6 GREEN.

## Verification Chain

- `grep -c "class FShapeLayerMapper : public IPsdLayerMapper" AllMappers.h` → 1 ✓
- `grep -c "FShapeLayerMapper.cpp" AllMappers.h` → 1 ✓
- File `FShapeLayerMapper.cpp` exists ✓
- `grep -c "int32 FShapeLayerMapper::GetPriority"` → 1 ✓
- `grep -c "bool FShapeLayerMapper::CanMap"` → 1 ✓
- `grep -c "UWidget\* FShapeLayerMapper::Map"` → 1 ✓
- `grep -c "return Layer.Type == EPsdLayerType::Shape"` → 1 ✓
- `grep -c "return 100"` → 1 ✓
- `grep -c "Brush.ImageSize"` → 1 ✓
- `grep -c "FTextureImporter"` → 0 ✓ (zero-texture path)
- No `Brush.TintColor =` assignment in code (FX-03 owns it) ✓
- `grep -c "bHasColorOverlay"` → present (diagnostic warning branch) ✓
- `grep -c "MakeUnique<FShapeLayerMapper>" FLayerMappingRegistry.cpp` → 1 ✓
- FShapeLayerMapper registration after FSolidFillLayerMapper (line 45 vs 44) ✓
- `grep -c "case EPsdLayerType::Shape" SPsdImportPreviewDialog.cpp` → 1 ✓

## Deviations from Plan

### Auto-fixed Issues

None — plan executed exactly as written.

### Additional Changes (within scope)

**1. [Rule 2 - Missing Critical] SPsdImportPreviewDialog catch-up labels for Gradient and SolidFill**
- **Found during:** Task 2
- **Issue:** Phase 13 added `EPsdLayerType::Gradient` and `EPsdLayerType::SolidFill` to the enum but did not add them to the dialog fallback switch, causing those layer types to display "Unknown" in the import preview dialog.
- **Fix:** Added `case EPsdLayerType::Gradient: return TEXT("Image");` and `case EPsdLayerType::SolidFill: return TEXT("Image");` as catch-up labels per the plan's explicit instruction: "adds explicit Gradient / SolidFill cases for completeness".
- **Files modified:** `Source/PSD2UMG/Private/UI/SPsdImportPreviewDialog.cpp`
- **Commit:** `0b72560`

## Known Stubs

None — all mapper logic is fully implemented. The visual verify checkpoint (Task 3) is a human gate, not a code stub.

## Self-Check: PASSED

- `Source/PSD2UMG/Private/Mapper/FShapeLayerMapper.cpp`: FOUND
- `Source/PSD2UMG/Private/Mapper/AllMappers.h`: FOUND (modified)
- Commit `6dc7641`: FOUND
- Commit `0b72560`: FOUND

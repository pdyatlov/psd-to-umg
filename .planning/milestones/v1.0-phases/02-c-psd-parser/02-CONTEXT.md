# Phase 2: C++ PSD Parser — Context

**Gathered:** 2026-04-08
**Status:** Ready for planning
**Source:** /gsd:discuss-phase 2

<domain>
## Phase Boundary

Native C++ PSD parsing for UE 5.7. Link PhotoshopAPI (and its 6 transitive deps) into the PSD2UMG plugin, build `FPsdParser` / `FPsdDocument`, extract layers (image RGBA pixels, text runs, group hierarchy) into UE-friendly types, and intercept `.psd` imports via a new `UPsdImportFactory`.

**In scope:**
- PhotoshopAPI vendoring + Build.cs linkage (Win64 only)
- `FPsdDocument` / `FPsdLayer` POD types
- `FPsdParser::ParseFile()` returning the layer tree
- Image layer RGBA extraction
- Single-run text layer extraction (content + font + size + color + alignment)
- Group/folder hierarchy preservation
- `UPsdImportFactory` registered alongside `UTextureFactory`
- `LogPSD2UMG` category + `FPsdParseDiagnostics`
- Automation Spec test against a checked-in fixture PSD

**Out of scope (belongs to later phases):**
- Layer → UMG widget mapping (Phase 3)
- Multi-run / per-character text styling (Phase 4)
- Effects, blend modes, masks (Phase 5)
- Mac platform support
</domain>

<decisions>
## Implementation Decisions

### Platform
- **Win64 only** for Phase 2. Mac support is dropped from the plugin scope. `PlatformAllowList` in `PSD2UMG.uplugin` stays `["Win64"]`. If Mac is ever needed it becomes a future decimal phase (2.1) — not assumed.

### PhotoshopAPI Vendoring
- **Pre-built static libs vendored in the repo** at `Source/ThirdParty/PhotoshopAPI/Win64/{lib,include}/`.
- Vendored libs come from PhotoshopAPI's actual `vcpkg.json` dep set: **`openimageio`, `fmt`, `libdeflate`, `stduuid`, `eigen3`** (plus PhotoshopAPI itself). Other deps the roadmap mentions (`blosc2`, `zlib-ng`, `simdutf`) come in transitively through OpenImageIO and are not declared directly. The exact final `.lib` set is enumerated empirically after the first CMake bootstrap.
- vcpkg triplet: **`x64-windows-static-md`** (static libs, dynamic CRT) — NOT `x64-windows-static`, which uses `/MT` and won't link against UE's `/MD`.
- A one-time CMake bootstrap script `Scripts/build-photoshopapi.bat` is documented for rebuilding from upstream source (driven by CMake outside UBT — does NOT run during normal UE builds).
- A fresh clone builds with zero extra steps for the common case.
- License files for PhotoshopAPI and each transitive dep are committed alongside the libs under `Source/ThirdParty/PhotoshopAPI/LICENSES/`.

### Build.cs Linkage
- PhotoshopAPI is exposed via a separate `PhotoshopAPI` ThirdParty module (`Source/ThirdParty/PhotoshopAPI/PhotoshopAPI.Build.cs`, `Type = ModuleType.External`) that adds the include path and lib paths.
- The `PSD2UMG` module declares `PhotoshopAPI` as a `PrivateDependencyModuleName`. PhotoshopAPI symbols never leak through public headers (PIMPL inside `FPsdParser`).
- PhotoshopAPI requires `dynamic_pointer_cast` for layer-type detection — confirmed in research — so `bUseRTTI = true` is mandatory on the `PSD2UMG` module. `bEnableExceptions = true` is also required. `CppStandard = CppStandardVersion.Cpp20` must be set explicitly (current Build.cs has none of these).
- All PhotoshopAPI calls are wrapped in `try/catch` inside the parser; exceptions are converted into `FPsdParseDiagnostics` entries and never escape to UE code.

### `PSD2UMG.uplugin` Cleanup
- Remove `"Mac"` from `PlatformAllowList` as part of Phase 2 (Phase 1 deviation we missed). Final list: `["Win64"]`.

### `FPsdDocument` Data Shape
Thin POD wrapper, minimal surface. Phase 3 layer-mapping consumes these directly.

```cpp
UENUM()
enum class EPsdLayerType : uint8 { Image, Text, Group, Unknown };

USTRUCT()
struct FPsdTextRun
{
    FString  Content;
    FString  FontName;     // PhotoshopAPI font family name
    float    SizePx;       // converted from pt at 72 DPI
    FLinearColor Color;
    TEnumAsByte<ETextJustify::Type> Alignment;
};

USTRUCT()
struct FPsdLayer
{
    FString          Name;
    EPsdLayerType    Type;
    FIntRect         Bounds;        // canvas-space pixel rect
    float            Opacity;       // 0..1
    bool             bVisible;
    TArray<FPsdLayer> Children;     // groups only
    TArray<uint8>    RGBAPixels;    // image layers only, length = W*H*4
    int32            PixelWidth = 0;
    int32            PixelHeight = 0;
    FPsdTextRun      Text;          // text layers only
};

USTRUCT()
struct FPsdDocument
{
    FString    SourcePath;
    FIntPoint  CanvasSize;
    TArray<FPsdLayer> RootLayers;
};
```

- No subclassing per layer type.
- No blend modes, effects, or masks (deferred to Phase 5).
- No PhotoshopAPI types in headers — full insulation.

### Text Layer Extraction (Phase 2 minimum)
- Single run only: content, font family name, size in px (converted from pt at 72 DPI), color (`FLinearColor` from sRGB), alignment (left/center/right).
- Multi-run / per-character styling deferred to Phase 4 (Text Pipeline).
- If a text layer has multiple runs in the source PSD, Phase 2 takes the first run's style and the concatenated content; a `FPsdParseDiagnostics` warning is emitted noting that styling was flattened.

### `UPsdImportFactory` UX — Wrapper Mode
- New `UPsdImportFactory` is registered with **higher `ImportPriority` than `UTextureFactory`** for `.psd` files. UE dispatches exactly one factory per import (verified during research — there is no native parallel-dispatch / coexist mode).
- Inside `FactoryCreateBinary`, our factory **internally instantiates `UTextureFactory` and forwards the call** to produce the flat `UTexture2D` exactly as before. The flat texture is preserved as a backwards-compatible side effect.
- After the wrapped call returns, our factory runs `FPsdParser::ParseFile()` on the same byte buffer to produce `FPsdDocument` + `FPsdParseDiagnostics`. Phase 2 logs the result via `LogPSD2UMG`. Phase 3 will plug WBP generation into this factory.
- `OnAssetReimport` delegate hook from Phase 1 is removed — the factory replaces it.
- No settings toggle.

### Test Strategy
- A hand-crafted minimal multi-layer PSD lives at `Source/PSD2UMG/Tests/Fixtures/MultiLayer.psd`. Contents (designed to exercise PRSR-02..05):
  - 1 image layer (solid color, known dimensions)
  - 1 text layer (known content, font, size, color)
  - 1 group containing 1 image + 1 hidden image
  - Known canvas size, layer bounds, opacities, visibility
- A UE Automation Spec `FPsdParserSpec` (in `Source/PSD2UMG/Tests/`, marked Editor-only) asserts:
  - Canvas size and root layer count
  - Each layer's name, type, bounds, opacity, visibility
  - Group child count and parent-child hierarchy
  - Image RGBA dimensions match `Bounds`
  - Text run content, font name, size, color, alignment
- Runs via Session Frontend → Automation. No external test runner.

### Logging & Diagnostics
- New log category `LogPSD2UMG` declared in `PSD2UMGLog.h`.
- `FPsdParseDiagnostics` struct returned alongside `FPsdDocument`:
  ```cpp
  enum class EPsdDiagnosticSeverity : uint8 { Info, Warning, Error };
  struct FPsdDiagnostic { EPsdDiagnosticSeverity Severity; FString LayerName; FString Message; };
  struct FPsdParseDiagnostics { TArray<FPsdDiagnostic> Entries; bool HasErrors() const; };
  ```
- `FPsdParser::ParseFile()` signature: `bool ParseFile(const FString& Path, FPsdDocument& OutDoc, FPsdParseDiagnostics& OutDiag)`.
- `UPsdImportFactory` writes a summary line per import to Output Log (counts of layers/warnings/errors). Message Log integration deferred — only Output Log for Phase 2.

### Claude's Discretion
- Exact file/class layout under `Source/PSD2UMG/Private/Parser/` and `Public/Parser/`
- Whether `FPsdParser` is a free function namespace or a `class` with static methods (lean toward namespace `PSD2UMG::Parser` for stateless ops)
- How to chunk PRSR-01 vendoring into atomic plan tasks (CMake script first vs lib drop first)
- Whether `bUseRTTI` requires module isolation tricks
- pt → px conversion math (assume 72 DPI unless PSD header says otherwise)
- Color space conversion details for `FLinearColor` (sRGB → linear)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project
- `CLAUDE.md` — UE 5.7.4 / C++20 / no Python / Editor-only / static-lib linkage / MIT constraints
- `.planning/PROJECT.md` — Core value, requirements, evolution rules
- `.planning/REQUIREMENTS.md` — PRSR-01..07 acceptance criteria
- `.planning/ROADMAP.md` — Phase 2 goal and success criteria

### Phase 1 (foundation)
- `Source/PSD2UMG/PSD2UMG.Build.cs` — current Build.cs (will be extended)
- `PSD2UMG.uplugin` — module + platform list
- `Source/PSD2UMG/Private/PSD2UMG.cpp` — current import-callback hook (to be removed in favor of factory)
- `.planning/phases/01-ue5-port/01-01-SUMMARY.md` and `01-02-SUMMARY.md` — what shipped, deviations

### Upstream
- PhotoshopAPI: https://github.com/EmilDohne/PhotoshopAPI — README + build instructions, license
- UE 5.7 docs: `UFactory`, `UAutomationSpec`, `Source.Init` (UTexture2D persistence pattern relevant for Phase 3 but informs `FPsdLayer.RGBAPixels` shape)

</canonical_refs>

<specifics>
## Specific Ideas

- Static lib linkage MUST be CMake-built outside UBT — see CLAUDE.md constraint, do not try to add PhotoshopAPI as UBT-compiled source
- Preserve `RunPyCmd` stub from Phase 1 (deprecated UE_LOG) — do not remove in Phase 2; Phase 6+ cleanup
- Replace the `OnAssetReimport` delegate in `FPSD2UMGModule` with the factory; remove the delegate cleanup code in `ShutdownModule` once the factory replaces it
- Test PSD fixture should be small (<100 KB) and committed with `git lfs` if needed — but try to keep under git's normal threshold
</specifics>

<deferred>
## Deferred Ideas

- **Mac platform support** — out of plugin scope for the foreseeable future
- **Multi-run text styling** — Phase 4 (Text Pipeline)
- **Blend modes, layer effects, masks** — Phase 5
- **Message Log integration for parse diagnostics** — defer until user-facing UX phase
- **Settings toggle for native vs flat-texture import** — not needed; coexist is the steady state
- **Golden-output JSON diff testing** — Automation Spec assertions are sufficient for Phase 2
- **Replacing `UTextureFactory`** — explicitly rejected; coexist instead
- **`bUseNativeImporter` opt-in flag** — explicitly rejected
</deferred>

---

*Phase: 02-c-psd-parser*
*Context gathered: 2026-04-08 via /gsd:discuss-phase*

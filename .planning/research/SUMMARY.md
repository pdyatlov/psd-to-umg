# Research Summary - PSD2UMG

**Synthesized:** 2026-04-07
**Sources:** STACK.md, FEATURES.md, ARCHITECTURE.md, PITFALLS.md, PROJECT.md

---

## Stack

- **PhotoshopAPI v0.9.0 (MIT)** is the only viable C++20 PSD parser covering all required layer types (pixel, text, groups, smart objects, 8/16/32-bit). Text layer support (issue 126) is confirmed shipped in v0.9.0 with a full TextLayer API; no manual TySh parsing needed.
- **C++20 is mandatory** -- set `CppStandard = CppStandardVersion.Cpp20` in Build.cs. Required by PhotoshopAPI; UE 5.7 supports it cleanly.
- **PhotoshopAPI must be pre-built as a static lib via CMake** -- UBT cannot drive CMake builds. Build with `-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL` to match UE CRT; mismatched CRT causes linker errors or runtime crashes.
- **All transitive static libs must be explicitly listed in Build.cs** -- PhotoshopAPI.lib does not bundle its deps. At minimum: libdeflate, simdutf, fmt, OpenImageIO, OpenImageIO_Util (plus blosc2/zlib-ng revealed during first full build).
- **OpenImageIO is heavyweight (~100 MB+ compiled).** Investigate stripping it if smart object pixel extraction is not needed in Phases 1-4. If needed, it must ship in ThirdParty.
- **UE 5.7 module list**: Core, CoreUObject, Engine, Slate, SlateCore, UMG, UMGEditor, UnrealEd, AssetRegistry, AssetTools, DeveloperSettings, ToolMenus, Kismet, InputCore. Remove PythonScriptPlugin and EditorScriptingUtilities entirely.
- **UE5 API migrations are mechanical**: FEditorStyle -> FAppStyle, WhitelistPlatforms -> PlatformAllowList, AssetRegistryModule.h path updated, SavePackage uses FSavePackageArgs.
- **Distribution model**: ship source + pre-built ThirdParty libs. Plugin module binaries are engine-version-bound; ThirdParty static libs are not. Source distribution is correct for GitHub; per-version binary builds via RunUAT BuildPlugin are needed for Fab.

---

## Table Stakes Features

- **One-click PSD import** -- drag .psd into Content Browser, receive a UWidgetBlueprint asset. This is the entire value proposition.
- **Layer hierarchy preservation** -- PSD groups become nested UCanvasPanel containers; designer-intended structure is retained.
- **Image layer extraction** -- pixel, shape, and smart object layers become UImage widgets backed by imported UTexture2D assets created in memory (no temp PNG files on disk).
- **Text layer extraction** -- content, font family, font size (DPI-corrected), color, alignment, outline, and shadow. Outline/shadow are expected for game UI; outlined text in HUDs is universal.
- **Button widget generation** -- Button_ prefix groups with normal/hovered/pressed/disabled state images become UButton with four FSlateBrush states.
- **Correct positioning** -- layer bounds converted to UCanvasPanelSlot position/size with DPI conversion (read actual DPI from PSD header; do not hardcode 72).
- **Font mapping system** -- PostScript font name to UE font asset mapping table in UDeveloperSettings; without this all text is unstyled.
- **No Python dependency** -- the primary motivation for the C++ rewrite. Editor-only plugin, zero Python at runtime.
- **Unique widget names** -- MakeUniqueObjectName deduplication required; PSD allows duplicate layer names, UMG widget trees do not.
- **Layer visibility respected** -- hidden PSD layers are skipped; they are designer reference/notes, not UI elements.
- **Color overlay effect** -- multiply blend mode tinting on image layers; extremely common in game UI for palette reuse.
- **DPI-correct text sizing** -- Photoshop 72 DPI to UMG 96 DPI scale factor (0.75). Python baseline uses a `size - 2` hack; the C++ version must apply the actual ratio from the PSD header.

---

## Architecture Patterns

- **Three-stage pipeline**: Stage 1 FPsdParser -> FPsdDocument (pure C++, no UE headers, unit-testable), Stage 2 layer mapper -> FWidgetTreeModel (pure C++), Stage 3 FWidgetBlueprintGenerator -> UWidgetBlueprint (UE APIs, integration-testable only).
- **UPsdImportFactory subclasses UFactory + FReimportHandler** -- .psd becomes a first-class import format producing UWidgetBlueprint. ImportPriority = DefaultImportPriority + 1 wins over UTextureFactory for .psd files.
- **Use FactoryCreateFile not FactoryCreateBinary** -- PhotoshopAPI opens the file itself; buffering the entire PSD into memory first wastes resources on large PSB files.
- **WidgetTree->ConstructWidget<T>() is an absolute rule** -- NewObject<T>() creates widgets outside the tree registry: invisible in UMG Designer, GC-eligible, assertion failures in debug builds. No exceptions.
- **Texture creation via Texture->Source.Init()** -- PlatformData is transient derived data; textures built only on PlatformData turn black after editor restart. Source.Init() is what serializes to .uasset.
- **IPsdLayerMapper + FLayerMappingRegistry (priority-ordered, first-match-wins)** -- pluggable mapper pattern; studios register custom mappers at priority > 100 without forking. Default priority order: Button_/Progress_/List_ prefix mappers (100), text type (50), group type (10), image fallback (0).
- **Strict compile-before-save sequence**: build full widget tree -> WidgetBP->Modify() -> FKismetEditorUtilities::CompileBlueprint() -> FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified() -> UPackage::SavePackage(). Skipping compile produces assets that appear correct but crash or show empty in UMG Designer.
- **Stream layers during large PSD import** -- process one layer at a time, release pixel data immediately after texture creation. 500-layer PSDs can exceed 500 MB if all pixel data is buffered simultaneously.

---

## Critical Pitfalls

1. **PhotoshopAPI CRT mismatch crashes the editor** (Phase 1)
   Build with `-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL` and `-DPSAPI_BUILD_PYTHON=OFF`. UE uses /MD; PhotoshopAPI defaults to /MT with Python enabled. Mismatch causes linker errors or silent heap corruption at runtime.

2. **NewObject instead of ConstructWidget silently corrupts Widget Blueprints** (Phase 2)
   Always use `WidgetTree->ConstructWidget<T>(Class, Name)`. Verify with `WidgetBP->WidgetTree->AllWidgets.Num()` after generation.

3. **Widget Blueprint not compiled before save produces corrupt assets** (Phase 2)
   Missing FKismetEditorUtilities::CompileBlueprint() produces .uasset files that open empty or crash on restart. Follow the 7-step sequence from ARCHITECTURE.md. Verify by opening the generated blueprint immediately after import.

4. **PhotoshopAPI bit-depth templates require explicit compile-time branching** (Phase 1)
   No runtime polymorphism: LayeredFile<bpp8_t/bpp16_t/bpp32_t>. Read PSD header for bit depth first, branch to the correct template. Include 16-bit and 32-bit test PSDs in the test suite; many designers work in 16-bit.

5. **Transitive PhotoshopAPI deps must all be listed in Build.cs** (Phase 1)
   PhotoshopAPI.lib does not embed its dependencies. First build will fail with LNK2019 for blosc2, libdeflate, zlib, etc. Use `dumpbin /DEPENDENTS` (Win64) after CMake build to capture the complete set.

6. **PSD layer order is reversed relative to visual order** (Phase 2)
   PSD stores layers bottom-to-top in file order. Iterating in file order and adding to UCanvasPanel produces an inverted stack. Reverse iteration or assign SetZOrder() from visual position. Must verify empirically whether PhotoshopAPI normalizes this.

7. **WhitelistPlatforms in .uplugin is silently ignored in UE5** (Phase 0)
   UE5 renamed this to PlatformAllowList. The old key is discarded without warning, causing the plugin to load on all platforms including those without a ThirdParty binary. Fix in Phase 0 before any distribution.

---

## Open Questions

These require empirical validation during implementation and cannot be resolved through research alone:

- **Does PhotoshopAPI normalize layer iteration order?** Must verify with a test PSD containing distinct layers at known visual positions. Determines whether z-ordering logic requires reversal.
- **What is the complete set of transitive static libs from the PhotoshopAPI CMake/vcpkg build?** The Build.cs lib list will be incomplete until after the first full build. OpenImageIO transitive chain (libtiff, libpng, OpenEXR, zlib-ng) is variable.
- **Can OpenImageIO be stripped for Phases 1-4?** Smart object pixel extraction (Phase 5) requires it; basic layer pixel data may not. A stripped build could significantly reduce binary size.
- **Does UWidgetBlueprint have AssetImportData in UE 5.7?** Present on UBlueprint in UE 4.x/5.x but rarely used for blueprints. If absent, reimport source path tracking needs an alternative (UMetaData or custom UPROPERTY).
- **What is the text edge-case coverage in PhotoshopAPI v0.9.0?** Text support shipped 8 days before this research. Mixed-font style runs, CJK text, vertical text, and paragraph vs point text need empirical testing with actual designer PSDs.
- **What DPI do real game UI PSDs use?** Reading actual PSD header DPI handles both 72 and 96 DPI files correctly, but the scale factor needs validation against real files before Phase 3 text positioning is considered complete.

---

## Confidence Levels

| Area | Confidence | Basis |
|------|------------|-------|
| UE 5.7 module list and API migrations | HIGH | Cross-referenced with UE 5.7 API docs; all deprecations confirmed |
| PhotoshopAPI text layer API | HIGH | Source headers read directly from cloned v0.9.0 repo; all methods verified |
| PhotoshopAPI build integration | MEDIUM | CMakeLists.txt verified; CRT matching and full transitive dep list need build-time confirmation |
| OpenImageIO strippability | LOW | Unknown without attempting a build with it disabled |
| Table stakes feature set | HIGH | All features proven by AutoPSDUI Python baseline (source code analyzed directly) |
| Differentiator feature value | MEDIUM | Competitive analysis via marketplace listings; actual user demand unverified |
| Three-stage pipeline architecture | HIGH | Pattern verified in AutoPSDUILibrary.cpp, UE docs, and community examples |
| Widget Blueprint creation API | HIGH | Existing code uses this pattern successfully; verified against UE 5.7 API docs |
| Reimport via FReimportHandler | MEDIUM | Standard UE pattern; UBlueprint::AssetImportData presence in UE 5.7 needs header verification |
| Phase 0 UE5 API migrations | HIGH | All deprecations confirmed via official docs and community migration reports |
| PhotoshopAPI text edge cases | MEDIUM | Feature is 8 days old at research date; basic cases work, edge cases unknown |
| PSD layer order (PhotoshopAPI) | MEDIUM | PSD spec is bottom-to-top but PhotoshopAPI normalization is unverified empirically |

**Overall: HIGH confidence for the core pipeline (Phases 0-3). MEDIUM confidence for advanced features (Phases 4-7). Main remaining uncertainties are empirical (build outputs, UE 5.7 header details) rather than conceptual.**

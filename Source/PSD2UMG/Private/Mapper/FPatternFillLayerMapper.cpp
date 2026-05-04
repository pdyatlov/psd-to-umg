// Copyright 2018-2021 - John snow wind
// Phase 23 / PTFL-02 -- pattern fill layer mapper.
// Mirrors FFillLayerMapper exactly; the only difference is CanMap dispatches
// on the parser-set EPsdLayerType::PatternFill value (Plan 23-01 ConvertLayerRecursive
// first-pass detection branch on adjPattern / PtFl tagged block).
// CP-04: PtFl carries no Clr key -- composited RGBAPixels are the only data source,
// produced by ExtractImagePixels on the AdjustmentLayer<T> Adj cast in the parser.

#include "Mapper/AllMappers.h"
#include "Generator/FTextureImporter.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGLog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Misc/Paths.h"

// Phase 23 / D-06: priority 101 (consistent with FFillLayerMapper / FSolidFillLayerMapper /
// FShapeLayerMapper per Phase 20 D-01). FLayerTagParser default-type switch maps
// EPsdLayerType::PatternFill -> EPsdTagType::Image (Plan 23-01 Edit 3) so FImageLayerMapper
// (priority 100) also returns true from CanMap on these layers; the priority delta makes
// FPatternFillLayerMapper win deterministically regardless of TArray::Sort stability.
int32 FPatternFillLayerMapper::GetPriority() const { return 101; }

bool FPatternFillLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::PatternFill;
}

UWidget* FPatternFillLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree)
{
    const FString PsdName = FPaths::GetBaseFilename(Doc.SourcePath);
    UTexture2D* Tex = FTextureImporter::ImportLayer(Layer, FTextureImporter::BuildTexturePath(PsdName));
    if (!Tex)
    {
        // D-04: empty RGBAPixels -> ImportLayer returns nullptr -> we return nullptr +
        // Warning; bHasComplexEffects is NOT set. Generator's "No mapper found" path
        // ultimately swallows the skip. Setting bHasComplexEffects without pixels would
        // still produce nothing via FX-05 (which requires both flag AND pixels).
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("FPatternFillLayerMapper: Texture import returned nullptr for pattern fill layer '%s' -- skipping"),
            *Layer.Name);
        return nullptr;
    }

    UImage* Img = Tree->ConstructWidget<UImage>(UImage::StaticClass(), FName(*Layer.ParsedTags.CleanName));
    // bMatchSize=true so Brush.ImageSize is set to the texture dimensions
    // (otherwise the UImage renders as a zero-sized border -- Phase 13 confirmed pitfall).
    Img->SetBrushFromTexture(Tex, /*bMatchSize=*/true);
    FSlateBrush Brush = Img->GetBrush();
    Brush.DrawAs = ESlateBrushDrawType::Image;
    Img->SetBrush(Brush);
    return Img;
}

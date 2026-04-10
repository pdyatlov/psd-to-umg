// Copyright 2018-2021 - John snow wind
// F9SliceImageLayerMapper — maps image layers with _9s or _9slice suffix to
// UImage widgets using ESlateBrushDrawType::Box (9-slice / border draw mode).
// Optional [L,T,R,B] margin syntax sets FSlateBrush.Margin; default 16px uniform.

#include "Mapper/AllMappers.h"
#include "Generator/FTextureImporter.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGLog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Misc/Paths.h"

int32 F9SliceImageLayerMapper::GetPriority() const { return 150; }

bool F9SliceImageLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    if (Layer.Type != EPsdLayerType::Image)
    {
        return false;
    }
    // Check _9slice before _9s (longer suffix takes priority in Contains check)
    return Layer.Name.Contains(TEXT("_9slice"), ESearchCase::CaseSensitive)
        || Layer.Name.Contains(TEXT("_9s"),     ESearchCase::CaseSensitive);
}

UWidget* F9SliceImageLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree)
{
    const FString PsdName = FPaths::GetBaseFilename(Doc.SourcePath);
    UTexture2D* Tex = FTextureImporter::ImportLayer(Layer, FTextureImporter::BuildTexturePath(PsdName));
    if (!Tex)
    {
        UE_LOG(LogPSD2UMG, Warning, TEXT("F9SliceImageLayerMapper: Texture import returned nullptr for layer '%s' — skipping"), *Layer.Name);
        return nullptr;
    }

    UImage* Img = Tree->ConstructWidget<UImage>(UImage::StaticClass(), FName(*Layer.Name));
    Img->SetBrushFromTexture(Tex, /*bMatchSize=*/true);

    FSlateBrush Brush = Img->GetBrush();
    Brush.DrawAs = ESlateBrushDrawType::Box;

    // Parse optional [L,T,R,B] margin from layer name.
    // Values are pixel counts; normalize to 0-1 by texture dimensions.
    const int32 TexW = Tex->GetSizeX();
    const int32 TexH = Tex->GetSizeY();
    const float W = (TexW > 0) ? static_cast<float>(TexW) : 1.f;
    const float H = (TexH > 0) ? static_cast<float>(TexH) : 1.f;

    bool bFoundMargin = false;
    int32 BracketOpen = INDEX_NONE;
    if (Layer.Name.FindLastChar(TEXT('['), BracketOpen))
    {
        int32 BracketClose = INDEX_NONE;
        if (Layer.Name.FindLastChar(TEXT(']'), BracketClose) && BracketClose > BracketOpen)
        {
            const FString Inner = Layer.Name.Mid(BracketOpen + 1, BracketClose - BracketOpen - 1);
            TArray<FString> Parts;
            Inner.ParseIntoArray(Parts, TEXT(","), /*bCullEmpty=*/true);
            if (Parts.Num() == 4)
            {
                const float L = FCString::Atof(*Parts[0]);
                const float T = FCString::Atof(*Parts[1]);
                const float R = FCString::Atof(*Parts[2]);
                const float B = FCString::Atof(*Parts[3]);
                Brush.Margin = FMargin(L / W, T / H, R / W, B / H);
                bFoundMargin = true;
            }
        }
    }

    if (!bFoundMargin)
    {
        // Default: 16 px uniform margin, normalized to texture dimensions.
        const float DefaultPx = 16.f;
        Brush.Margin = FMargin(DefaultPx / W, DefaultPx / H, DefaultPx / W, DefaultPx / H);
    }

    Img->SetBrush(Brush);
    return Img;
}

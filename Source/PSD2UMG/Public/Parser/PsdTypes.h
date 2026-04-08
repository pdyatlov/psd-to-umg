// Copyright 2018-2021 - John snow wind

#pragma once

#include "CoreMinimal.h"
#include "Framework/Text/TextLayout.h"
#include "PsdTypes.generated.h"

/**
 * Layer kinds the parser can produce. Mirrors the third-party parser's
 * high-level categories without leaking any external types into PSD2UMG public headers.
 */
UENUM()
enum class EPsdLayerType : uint8
{
	Image,
	Text,
	Group,
	Unknown
};

/**
 * A single text run extracted from a PSD text layer. Phase 2 only supports
 * one run per text layer (multi-run flattening with a diagnostic warning).
 * Multi-run / per-character styling is deferred to Phase 4.
 */
USTRUCT()
struct PSD2UMG_API FPsdTextRun
{
	GENERATED_BODY()

	FString Content;
	FString FontName;
	float SizePx = 0.f;
	FLinearColor Color = FLinearColor::White;
	TEnumAsByte<ETextJustify::Type> Alignment = ETextJustify::Left;
};

/**
 * A single PSD layer in canvas space. Image layers carry RGBA pixels;
 * text layers carry a single FPsdTextRun; group layers carry Children.
 * Mutually exclusive payloads are simply left default-constructed.
 */
USTRUCT()
struct PSD2UMG_API FPsdLayer
{
	GENERATED_BODY()

	FString Name;
	EPsdLayerType Type = EPsdLayerType::Unknown;
	FIntRect Bounds;
	float Opacity = 1.f;
	bool bVisible = true;

	TArray<FPsdLayer> Children;

	// Image-layer payload
	TArray<uint8> RGBAPixels;
	int32 PixelWidth = 0;
	int32 PixelHeight = 0;

	// Text-layer payload
	FPsdTextRun Text;
};

/**
 * A parsed PSD document. Owned by callers; populated by FPsdParser::ParseFile.
 * Phase 2 produces this; Phase 3 maps it to a Widget Blueprint.
 */
USTRUCT()
struct PSD2UMG_API FPsdDocument
{
	GENERATED_BODY()

	FString SourcePath;
	FIntPoint CanvasSize = FIntPoint::ZeroValue;
	TArray<FPsdLayer> RootLayers;
};

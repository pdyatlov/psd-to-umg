// Copyright 2018-2021 - John snow wind

#pragma once

#include "CoreMinimal.h"
#include "Framework/Text/TextLayout.h"
#include "Parser/FLayerTagParser.h"
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
	SmartObject,
	Gradient,    // Phase 13: gradient fill layer (GdFl tagged block); carries RGBAPixels baked from ShapeLayer composited channels
	SolidFill,   // Phase 13: solid color fill layer (SoCo tagged block); carries Effects.ColorOverlayColor (no texture)
	Shape,       // Phase 14: drawn vector shape (Rectangle/Ellipse/Pen tool) with solid-color fill (vscg tagged block); carries Effects.ColorOverlayColor (no texture); future home for stroke rendering
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

	// Phase 4 additions -- weight / style flags
	bool bBold = false;
	bool bItalic = false;

	// Phase 4 additions -- layout
	bool bHasExplicitWidth = false;
	float BoxWidthPx = 0.f;

	// Phase 4 additions -- outline (best-effort; zero when not present)
	FLinearColor OutlineColor = FLinearColor::Transparent;
	float OutlineSize = 0.f;

	// Phase 4.1 gap closure (TEXT-04) -- drop shadow payload routed from
	// FPsdLayerEffects by PsdParser::RouteTextEffects after ExtractLayerEffects.
	// Units: raw PSD pixels / linear color; DPI conversion applied by FTextLayerMapper (D-09 Option B).
	FVector2D ShadowOffset = FVector2D::ZeroVector;
	FLinearColor ShadowColor = FLinearColor::Transparent;
};

/**
 * Layer effects data extracted from the lrFX tagged block in PSD files.
 * Phase 5: color overlay, drop shadow, and complex-effect presence.
 */
USTRUCT()
struct PSD2UMG_API FPsdLayerEffects
{
	GENERATED_BODY()

	bool bHasColorOverlay = false;
	FLinearColor ColorOverlayColor = FLinearColor::White;

	bool bHasDropShadow = false;
	FLinearColor DropShadowColor = FLinearColor(0.f, 0.f, 0.f, 0.5f);
	FVector2D DropShadowOffset = FVector2D::ZeroVector;

	// True if any of: inner shadow, gradient overlay, pattern overlay, bevel/emboss (per D-09)
	bool bHasComplexEffects = false;

	// Phase 4.1 TEXT-03 -- Layer-Style Stroke (from lfx2/FrFX descriptor).
	// D-02: populated for ALL layer types; rendering on non-text is deferred to a future phase.
	// Units: raw PSD pixels (DPI applied by FTextLayerMapper -- Option B per D-09).
	bool bHasStroke = false;
	FLinearColor StrokeColor = FLinearColor::Transparent;
	float StrokeSize = 0.f;
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

	// Phase 5: layer effects extracted from lrFX tagged block
	FPsdLayerEffects Effects;

	TArray<FPsdLayer> Children;

	// Phase 9: parsed tag grammar (populated once by PsdParser::ParseFile via FLayerTagParser::Parse).
	// All downstream consumers read from this, not Layer.Name.
	FParsedLayerTags ParsedTags;

	// Image-layer payload
	TArray<uint8> RGBAPixels;
	int32 PixelWidth = 0;
	int32 PixelHeight = 0;

	// Text-layer payload
	FPsdTextRun Text;

	// Smart Object payload (Phase 6)
	FString SmartObjectFilePath;  // linked file path (empty for embedded)
	bool bIsSmartObject = false;  // redundant with Type but useful for quick checks
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

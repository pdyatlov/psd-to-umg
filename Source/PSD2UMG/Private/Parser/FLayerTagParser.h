// Copyright 2018-2021 - John snow wind

#pragma once

#include "CoreMinimal.h"
#include "Misc/Optional.h"

// Forward decl to avoid circular include with PsdTypes.h (FPsdLayer holds
// FParsedLayerTags by value; FindChildByState takes const TArray<FPsdLayer>&).
struct FPsdLayer;
enum class EPsdLayerType : uint8;

/**
 * Phase 9 -- Unified layer-naming tag grammar.
 * Layer names are parsed as: `CleanName @tag @tag:value @tag:v1,v2,...`
 *
 * See Docs/LayerTagGrammar.md for the formal EBNF and the full tag inventory.
 *
 * FLayerTagParser is pure -- no UE_LOG side effects; callers pass an
 * OutDiagnostics FString and emit to LogPSD2UMG at integration time.
 */

enum class EPsdTagType : uint8
{
	None,
	Button,
	Image,
	Text,
	Progress,
	HBox,
	VBox,
	Overlay,
	ScrollBox,
	Slider,
	CheckBox,
	Input,
	List,
	Tile,
	SmartObject,
	Canvas,
};

enum class EPsdAnchorTag : uint8
{
	None,
	TL,
	TC,
	TR,
	CL,
	C,
	CR,
	BL,
	BC,
	BR,
	StretchH,
	StretchV,
	Fill,
};

enum class EPsdStateTag : uint8
{
	None,
	Normal,
	Hover,
	Pressed,
	Disabled,
	Fill,
	Bg,
};

enum class EPsdAnimTag : uint8
{
	None,
	Show,
	Hide,
	Hover,
};

/** Nine-slice margin carried by @9s / @9s:L,T,R,B. */
struct FPsdNineSliceMargin
{
	float L = 0.f;
	float T = 0.f;
	float R = 0.f;
	float B = 0.f;

	/** False when @9s shorthand was used (defaults to 16px on all sides, matching F9SliceImageLayerMapper). */
	bool bExplicit = false;
};

/**
 * Parsed result. One per layer. Default-constructible = "no tags found".
 * Populated on FPsdLayer::ParsedTags during PsdParser::ParseFile.
 */
struct FParsedLayerTags
{
	/** D-20: name before first @-token, trimmed, internal spaces -> underscores. */
	FString CleanName;

	/** D-02: default-from-layer-type inference applied inside Parse(). */
	EPsdTagType Type = EPsdTagType::None;

	EPsdAnchorTag Anchor = EPsdAnchorTag::None;
	EPsdStateTag State = EPsdStateTag::None;
	EPsdAnimTag Anim = EPsdAnimTag::None;

	TOptional<FPsdNineSliceMargin> NineSlice;

	/** @ia:IA_Confirm -- case preserved per D-04. */
	TOptional<FString> InputAction;

	/** @smartobject:TypeName -- case preserved per D-04. */
	TOptional<FString> SmartObjectTypeName;

	/** @variants modifier (on groups -> UWidgetSwitcher). */
	bool bIsVariants = false;

	/** @background modifier — marks an Image inside a @state group as the state brush; skipped during child widget recursion. */
	bool bIsBackground = false;

	/** D-18: unknown tags collected here; caller logs. */
	TArray<FString> UnknownTags;

	bool HasType() const { return Type != EPsdTagType::None; }
};

class FLayerTagParser
{
public:
	/**
	 * Parse a single layer name. Pure -- no side effects beyond filling OutDiagnostics.
	 *
	 * @param LayerName         Raw PSD layer name (verbatim).
	 * @param SourceLayerType   Kind reported by the PSD parser -- used for D-02 default-type
	 *                          inference when no @type tag is present.
	 * @param LayerIndex        Layer's index within its parent -- used for D-21 empty-name fallback.
	 * @param OutDiagnostics    Newline-separated warning/error strings; caller emits to LogPSD2UMG.
	 */
	static FParsedLayerTags Parse(
		FStringView LayerName,
		EPsdLayerType SourceLayerType,
		int32 LayerIndex,
		FString& OutDiagnostics);

	/**
	 * Central state-child lookup. D-12: explicit @state match first.
	 * D-13: if DesiredState == Normal and no explicit match, fall back to first
	 * untagged Image child (EPsdLayerType::Image + State == None).
	 *
	 * Returns nullptr when no match.
	 */
	static const FPsdLayer* FindChildByState(
		const TArray<FPsdLayer>& Children,
		EPsdStateTag DesiredState);
};

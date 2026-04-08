// Copyright 2018-2021 - John snow wind

#pragma once

// Private parser header. MAY include PhotoshopAPI symbols -- never
// included from any public header. All PhotoshopAPI types stay behind
// the PIMPL wall per 02-CONTEXT.md D-Linkage.

#include "CoreMinimal.h"
#include "Parser/PsdTypes.h"
#include "Parser/PsdDiagnostics.h"

// Silence third-party compiler warnings; PhotoshopAPI uses exceptions,
// RTTI and C++20 features that UE's default warning set complains about.
THIRD_PARTY_INCLUDES_START
#include <PhotoshopAPI.h>
THIRD_PARTY_INCLUDES_END

namespace PSD2UMG::Parser::Internal
{
	/** v1: PSD2UMG assumes 8-bit color -- matches 02-CONTEXT.md assumption. */
	using PsdPixelType = NAMESPACE_PSAPI::bpp8_t;
	using PsdLayer = NAMESPACE_PSAPI::Layer<PsdPixelType>;

	/**
	 * Recursively convert a PhotoshopAPI layer node into an FPsdLayer.
	 * Populates Name, Type, Bounds, Opacity, bVisible, and type-specific
	 * payloads (Children for Group, RGBAPixels for Image, Text for Text).
	 * Diagnostics append warnings/errors encountered along the way.
	 */
	void ConvertLayerRecursive(
		const std::shared_ptr<PsdLayer>& InLayer,
		FPsdLayer& OutLayer,
		FPsdParseDiagnostics& OutDiag);
}

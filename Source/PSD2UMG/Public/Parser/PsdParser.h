// Copyright 2018-2021 - John snow wind

#pragma once

#include "CoreMinimal.h"
#include "Parser/PsdTypes.h"
#include "Parser/PsdDiagnostics.h"

/**
 * Native PSD parser entry point. Stateless free function in the
 * PSD2UMG::Parser namespace. No third-party types leak through this
 * header (PIMPL: vendor headers are included only from the .cpp).
 *
 * Reads a .psd from disk, walks the layer tree, and populates an
 * FPsdDocument with layer names, types, bounds, opacity, visibility,
 * group hierarchy, image RGBA pixel data, and single-run text
 * extraction. Any vendor exceptions are caught and converted into
 * FPsdDiagnostic error entries -- exceptions never escape.
 *
 * Returns true iff parsing completed without errors. OutDiag is always
 * populated regardless of success.
 */
namespace PSD2UMG::Parser
{
	PSD2UMG_API bool ParseFile(const FString& Path, FPsdDocument& OutDoc, FPsdParseDiagnostics& OutDiag);
}

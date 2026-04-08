// Copyright 2018-2021 - John snow wind

#pragma once

// Private parser header. Intentionally does NOT include any PhotoshopAPI
// headers: that include lives only in PsdParser.cpp inside a
// THIRD_PARTY_INCLUDES_START/END guard. This header exists as the shared
// surface for any future private translation units in Private/Parser/
// and currently only re-exports the UE-side parser types used by the
// implementation file.

#include "CoreMinimal.h"
#include "Parser/PsdTypes.h"
#include "Parser/PsdDiagnostics.h"

namespace PSD2UMG::Parser::Internal
{
	// Helpers live as static functions inside PsdParser.cpp so they can
	// freely reference PhotoshopAPI template types without exposing them
	// here. Declarations are intentionally omitted.
}

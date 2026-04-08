// Copyright 2018-2021 - John snow wind

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "PsdImportFactory.generated.h"

/**
 * UPsdImportFactory -- the .psd import entry point for PSD2UMG.
 *
 * Wrapper-mode factory (see .planning/phases/02-c-psd-parser/02-CONTEXT.md
 * "D-Factory"). Registered with a higher ImportPriority than the engine's
 * UTextureFactory so UE dispatches this factory first for .psd files. Inside
 * FactoryCreateBinary we internally instantiate UTextureFactory and forward
 * the call to preserve the flat UTexture2D side effect, then run the native
 * PSD parser (PSD2UMG::Parser::ParseFile) on the same byte buffer to produce
 * an FPsdDocument + diagnostics. Phase 2 logs a summary line per import;
 * Phase 3 will plug Widget Blueprint generation into this factory.
 */
UCLASS()
class UPsdImportFactory : public UFactory
{
	GENERATED_BODY()

public:
	UPsdImportFactory();

	virtual UObject* FactoryCreateBinary(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		UObject* Context,
		const TCHAR* Type,
		const uint8*& Buffer,
		const uint8* BufferEnd,
		FFeedbackContext* Warn) override;
};

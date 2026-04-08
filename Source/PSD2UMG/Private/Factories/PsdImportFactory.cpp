// Copyright 2018-2021 - John snow wind

#include "Factories/PsdImportFactory.h"

#include "Engine/Texture2D.h"
#include "Factories/TextureFactory.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"

#include "PSD2UMGLog.h"
#include "Parser/PsdDiagnostics.h"
#include "Parser/PsdParser.h"
#include "Parser/PsdTypes.h"

#define LOCTEXT_NAMESPACE "UPsdImportFactory"

UPsdImportFactory::UPsdImportFactory()
{
	// Claim the same output class as UTextureFactory so UE considers us for
	// .psd imports and applies ImportPriority ordering.
	SupportedClass = UTexture2D::StaticClass();

	bCreateNew = false;
	bEditorImport = true;
	bText = false;

	// "psd;Photoshop Document" -- semicolon-delimited extension and
	// human-readable description per UFactory convention.
	Formats.Add(TEXT("psd;Photoshop Document"));

	// Must outrank UTextureFactory for .psd. Per CONTEXT.md D-Factory, UE
	// dispatches exactly one factory; higher priority wins.
	ImportPriority = DefaultImportPriority + 100;
}

UObject* UPsdImportFactory::FactoryCreateBinary(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	const TCHAR* Type,
	const uint8*& Buffer,
	const uint8* BufferEnd,
	FFeedbackContext* Warn)
{
	// Capture buffer bounds BEFORE forwarding -- UTextureFactory may advance
	// the Buffer reference during its read.
	const uint8* const OriginalBegin = Buffer;
	const uint8* const OriginalEnd = BufferEnd;
	const int64 BufferSize = (OriginalEnd && OriginalBegin && OriginalEnd > OriginalBegin)
		? static_cast<int64>(OriginalEnd - OriginalBegin)
		: 0;

	// Step 1: forward to UTextureFactory to preserve the backwards-compatible
	// flat UTexture2D side effect. This is the "wrapper mode" contract from
	// 02-CONTEXT.md D-Factory.
	UTextureFactory* Inner = NewObject<UTextureFactory>(GetTransientPackage(), UTextureFactory::StaticClass());
	Inner->AddToRoot();
	UObject* Result = nullptr;
	{
		const uint8* ForwardBuffer = OriginalBegin;
		Result = Inner->FactoryCreateBinary(
			UTexture2D::StaticClass(),
			InParent,
			InName,
			Flags,
			Context,
			Type,
			ForwardBuffer,
			OriginalEnd,
			Warn);
	}
	Inner->RemoveFromRoot();

	// Step 2: run the native parser on the same bytes. PhotoshopAPI's
	// LayeredFile::read is path-only (no stream/buffer overload -- confirmed
	// in 02-03-SUMMARY.md), so spill to a temp file under ProjectIntermediate.
	if (BufferSize <= 0 || OriginalBegin == nullptr)
	{
		UE_LOG(LogPSD2UMG, Warning, TEXT("PSD2UMG: empty byte buffer for %s -- skipping parser."), *InName.ToString());
		return Result;
	}

	const FString TempDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectIntermediateDir() / TEXT("PSD2UMG"));
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*TempDir))
	{
		PlatformFile.CreateDirectoryTree(*TempDir);
	}
	const FString TempPath = FPaths::CreateTempFilename(*TempDir, TEXT("PSD2UMG_"), TEXT(".psd"));

	TArray<uint8> ByteCopy;
	ByteCopy.Append(OriginalBegin, BufferSize);
	if (!FFileHelper::SaveArrayToFile(ByteCopy, *TempPath))
	{
		UE_LOG(LogPSD2UMG, Error, TEXT("PSD2UMG: failed to spill import buffer to temp file %s"), *TempPath);
		return Result;
	}

	FPsdDocument Doc;
	FPsdParseDiagnostics Diag;
	const bool bOk = PSD2UMG::Parser::ParseFile(TempPath, Doc, Diag);

	// Best-effort cleanup of the spill file.
	PlatformFile.DeleteFile(*TempPath);

	int32 WarningCount = 0;
	int32 ErrorCount = 0;
	for (const FPsdDiagnostic& Entry : Diag.Entries)
	{
		if (Entry.Severity == EPsdDiagnosticSeverity::Warning) { ++WarningCount; }
		else if (Entry.Severity == EPsdDiagnosticSeverity::Error) { ++ErrorCount; }
	}

	if (bOk)
	{
		UE_LOG(LogPSD2UMG, Log,
			TEXT("PSD2UMG: parsed %s -- canvas %dx%d, %d root layers, %d warnings, %d errors"),
			*InName.ToString(),
			Doc.CanvasSize.X, Doc.CanvasSize.Y,
			Doc.RootLayers.Num(),
			WarningCount, ErrorCount);
	}
	else
	{
		UE_LOG(LogPSD2UMG, Error,
			TEXT("PSD2UMG: parse FAILED for %s -- %d warnings, %d errors"),
			*InName.ToString(), WarningCount, ErrorCount);
		for (const FPsdDiagnostic& Entry : Diag.Entries)
		{
			const TCHAR* Sev =
				Entry.Severity == EPsdDiagnosticSeverity::Error ? TEXT("ERROR") :
				Entry.Severity == EPsdDiagnosticSeverity::Warning ? TEXT("WARN") : TEXT("INFO");
			UE_LOG(LogPSD2UMG, Error, TEXT("PSD2UMG:   [%s] %s: %s"),
				Sev, *Entry.LayerName, *Entry.Message);
		}
	}

	// Phase 3 will extend this factory to also produce a Widget Blueprint
	// from Doc. For Phase 2, we return the wrapped UTexture2D as-is.
	return Result;
}

#undef LOCTEXT_NAMESPACE

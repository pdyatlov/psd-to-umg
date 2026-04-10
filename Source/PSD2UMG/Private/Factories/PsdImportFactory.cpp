// Copyright 2018-2021 - John snow wind

#include "Factories/PsdImportFactory.h"

#include "Engine/Texture2D.h"
#include "Factories/TextureFactory.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/MetaData.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Editor.h"

#include "PSD2UMGLog.h"
#include "Generator/FWidgetBlueprintGenerator.h"
#include "Parser/PsdDiagnostics.h"
#include "Parser/PsdParser.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGSetting.h"
#include "UI/SPsdImportPreviewDialog.h"
#include "UI/PsdLayerTreeItem.h"

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

	// Phase 3: Generate Widget Blueprint from parsed document.
	if (bOk)
	{
		const UPSD2UMGSettings* Settings = UPSD2UMGSettings::Get();
		FString PsdName = FPaths::GetBaseFilename(InName.ToString());
		FString WbpDir = Settings->WidgetBlueprintAssetDir.Path;
		if (WbpDir.IsEmpty()) { WbpDir = TEXT("/Game/UI/Widgets"); }
		FString WbpAssetName = FString::Printf(TEXT("WBP_%s"), *PsdName);

		UWidgetBlueprint* WBP = nullptr;

		if (Settings->bShowPreviewDialog && IsInGameThread())
		{
			// Build tree items for the dialog
			TArray<TSharedPtr<FPsdLayerTreeItem>> RootItems =
				SPsdImportPreviewDialog::BuildTreeFromDocument(Doc);

			bool bConfirmed = false;
			FString UserOutputPath = WbpDir;

			// Create the dialog and host window
			TSharedPtr<SWindow> DialogWindow = SNew(SWindow)
				.Title(NSLOCTEXT("PSD2UMG", "ImportDialogTitle", "PSD Import Preview"))
				.SizingRule(ESizingRule::UserSized)
				.ClientSize(FVector2D(600.f, 640.f))
				.MinWidth(480.f)
				.MinHeight(520.f)
				.SupportsMaximize(false)
				.SupportsMinimize(false);

			TSharedRef<SPsdImportPreviewDialog> DialogContent =
				SNew(SPsdImportPreviewDialog)
				.RootItems(RootItems)
				.InitialOutputPath(WbpDir)
				.bIsReimport(false)
				.OnCancelled(FSimpleDelegate::CreateLambda([&bConfirmed]()
				{
					bConfirmed = false;
				}))
				.OnConfirmed(FOnImportConfirmed::CreateLambda(
					[&bConfirmed, &UserOutputPath](const FString& OutPath, const TArray<TSharedPtr<FPsdLayerTreeItem>>& /*Items*/)
					{
						bConfirmed = true;
						UserOutputPath = OutPath;
					}));

			DialogWindow->SetContent(DialogContent);

			GEditor->EditorAddModalWindow(DialogWindow.ToSharedRef());

			if (bConfirmed)
			{
				// Resolve final output dir and asset name from user path
				FString FinalDir = UserOutputPath;
				if (FinalDir.IsEmpty()) { FinalDir = WbpDir; }

				WBP = FWidgetBlueprintGenerator::Generate(Doc, FinalDir, WbpAssetName);
				if (WBP)
				{
					UE_LOG(LogPSD2UMG, Log, TEXT("PSD2UMG: generated Widget Blueprint %s/%s"), *FinalDir, *WbpAssetName);
				}
				else
				{
					UE_LOG(LogPSD2UMG, Warning, TEXT("PSD2UMG: Widget Blueprint generation failed for %s"), *InName.ToString());
				}
			}
			else
			{
				UE_LOG(LogPSD2UMG, Log, TEXT("PSD2UMG: import cancelled by user for %s"), *InName.ToString());
			}
		}
		else
		{
			// bShowPreviewDialog is false or not on game thread — direct generation
			WBP = FWidgetBlueprintGenerator::Generate(Doc, WbpDir, WbpAssetName);
			if (WBP)
			{
				UE_LOG(LogPSD2UMG, Log, TEXT("PSD2UMG: generated Widget Blueprint %s/%s"), *WbpDir, *WbpAssetName);
			}
			else
			{
				UE_LOG(LogPSD2UMG, Warning, TEXT("PSD2UMG: Widget Blueprint generation failed for %s"), *InName.ToString());
			}
		}

		// Store PSD source path as metadata on the WBP package
		if (WBP)
		{
			UMetaData* MetaData = WBP->GetOutermost()->GetMetaData();
			MetaData->SetValue(WBP, TEXT("PSD2UMG.SourcePsdPath"), *TempPath);
			MetaData->SetValue(WBP, TEXT("PSD2UMG.SourcePsdName"), *InName.ToString());
		}
	}

	return Result;
}

#undef LOCTEXT_NAMESPACE

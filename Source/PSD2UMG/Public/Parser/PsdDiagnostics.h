// Copyright 2018-2021 - John snow wind

#pragma once

#include "CoreMinimal.h"

/**
 * Severity for a single FPsdDiagnostic entry. Plain C++ enum (no reflection):
 * diagnostics never need to round-trip through Blueprints.
 */
enum class EPsdDiagnosticSeverity : uint8
{
	Info,
	Warning,
	Error
};

/**
 * One diagnostic entry produced during PSD parsing. LayerName is optional
 * and may be empty for document-level messages.
 */
struct PSD2UMG_API FPsdDiagnostic
{
	EPsdDiagnosticSeverity Severity = EPsdDiagnosticSeverity::Info;
	FString LayerName;
	FString Message;
};

/**
 * Accumulator for diagnostics produced by FPsdParser::ParseFile. Returned
 * alongside FPsdDocument so callers can decide whether to surface warnings
 * to the user. Plain C++ struct -- no USTRUCT, no reflection.
 */
struct PSD2UMG_API FPsdParseDiagnostics
{
	TArray<FPsdDiagnostic> Entries;

	bool HasErrors() const
	{
		for (const FPsdDiagnostic& Entry : Entries)
		{
			if (Entry.Severity == EPsdDiagnosticSeverity::Error)
			{
				return true;
			}
		}
		return false;
	}

	void AddInfo(const FString& LayerName, const FString& Message)
	{
		Entries.Add(FPsdDiagnostic{EPsdDiagnosticSeverity::Info, LayerName, Message});
	}

	void AddWarning(const FString& LayerName, const FString& Message)
	{
		Entries.Add(FPsdDiagnostic{EPsdDiagnosticSeverity::Warning, LayerName, Message});
	}

	void AddError(const FString& LayerName, const FString& Message)
	{
		Entries.Add(FPsdDiagnostic{EPsdDiagnosticSeverity::Error, LayerName, Message});
	}
};

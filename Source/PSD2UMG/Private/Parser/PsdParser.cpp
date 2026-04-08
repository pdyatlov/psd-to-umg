// Copyright 2018-2021 - John snow wind

#include "Parser/PsdParser.h"
#include "Parser/PsdParserInternal.h"
#include "PSD2UMGLog.h"

#include "Math/IntRect.h"
#include "Math/IntPoint.h"
#include "Math/Color.h"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

// PhotoshopAPI must be included inside UE's third-party include guard
// so its fmt/eigen/openimageio headers don't trip the default warning
// set. All vendor symbols are confined to this translation unit per
// the PIMPL rule in 02-CONTEXT.md D-Linkage.
//
// We also have to push and undefine the Windows.h macros that UE leaks
// into every TU on Win64 (`min`, `max`, `ERROR`, `TEXT`, `IN`, `OUT`,
// `check`). PhotoshopAPI uses `std::numeric_limits<T>::max()` and an
// internal `PSAPI_LOG_ERROR` macro whose expansion contains the bare
// token `ERROR`, both of which collide with the Win32 macros.
// NOMINMAX must be defined BEFORE PhotoshopAPI.h pulls in <iostream>/<filesystem>,
// because those std headers transitively include <windows.h> on MSVC. Without
// NOMINMAX, windows.h re-defines `min`/`max` as macros (even if we #undef them
// here first) and PhotoshopAPI's `std::numeric_limits<T>::max()` calls then get
// macro-expanded into syntax errors.
#ifndef NOMINMAX
    #define NOMINMAX
#endif

THIRD_PARTY_INCLUDES_START
#pragma push_macro("max")
#pragma push_macro("min")
#pragma push_macro("ERROR")
#pragma push_macro("TEXT")
#pragma push_macro("IN")
#pragma push_macro("OUT")
#pragma push_macro("check")
#undef max
#undef min
#undef ERROR
#undef TEXT
#undef IN
#undef OUT
#undef check
#include <PhotoshopAPI.h>
#pragma pop_macro("check")
#pragma pop_macro("OUT")
#pragma pop_macro("IN")
#pragma pop_macro("TEXT")
#pragma pop_macro("ERROR")
#pragma pop_macro("min")
#pragma pop_macro("max")
THIRD_PARTY_INCLUDES_END

namespace PSD2UMG::Parser::Internal
{
	using namespace NAMESPACE_PSAPI;

	/** v1 assumes 8-bit color -- matches 02-CONTEXT.md assumption. */
	using PsdPixelType = bpp8_t;
	using PsdLayer = Layer<PsdPixelType>;

	/** Convert an ANSI/UTF-8 std::string to FString. */
	static FString Utf8ToFString(const std::string& In)
	{
		return FString(UTF8_TO_TCHAR(In.c_str()));
	}

	/** Build an FIntRect from a PhotoshopAPI layer's center + size. */
	static FIntRect ComputeBounds(const std::shared_ptr<PsdLayer>& InLayer)
	{
		const float CenterX = InLayer->center_x();
		const float CenterY = InLayer->center_y();
		const int32 Width = static_cast<int32>(InLayer->width());
		const int32 Height = static_cast<int32>(InLayer->height());

		const int32 HalfW = Width / 2;
		const int32 HalfH = Height / 2;
		const int32 Left = static_cast<int32>(CenterX) - HalfW;
		const int32 Top = static_cast<int32>(CenterY) - HalfH;
		return FIntRect(Left, Top, Left + Width, Top + Height);
	}

	/**
	 * Interleave separate R/G/B/A channel vectors into a tightly-packed
	 * RGBA8 byte buffer. Missing alpha defaults to 0xFF.
	 */
	static void InterleaveRGBA(
		const std::vector<PsdPixelType>& R,
		const std::vector<PsdPixelType>& G,
		const std::vector<PsdPixelType>& B,
		const std::vector<PsdPixelType>* A,
		TArray<uint8>& OutPixels)
	{
		const size_t Count = R.size();
		OutPixels.SetNumUninitialized(static_cast<int32>(Count * 4));
		for (size_t i = 0; i < Count; ++i)
		{
			const int32 Dst = static_cast<int32>(i * 4);
			OutPixels[Dst + 0] = R[i];
			OutPixels[Dst + 1] = G[i];
			OutPixels[Dst + 2] = B[i];
			OutPixels[Dst + 3] = A ? (*A)[i] : static_cast<uint8>(0xFF);
		}
	}

	/** Extract RGBA pixels from an ImageLayer into OutLayer. */
	static void ExtractImagePixels(
		const std::shared_ptr<ImageLayer<PsdPixelType>>& Image,
		FPsdLayer& OutLayer,
		FPsdParseDiagnostics& OutDiag)
	{
		try
		{
			OutLayer.PixelWidth = static_cast<int32>(Image->width());
			OutLayer.PixelHeight = static_cast<int32>(Image->height());

			if (OutLayer.PixelWidth <= 0 || OutLayer.PixelHeight <= 0)
			{
				OutDiag.AddWarning(OutLayer.Name,
					TEXT("Image layer has zero-sized bounds; skipping pixel extraction."));
				return;
			}

			// Pull individual channels. get_channel(ChannelID::Alpha) will
			// throw if alpha isn't present, so probe via has_alpha indirectly
			// by catching the exception on Alpha only.
			std::vector<PsdPixelType> R = Image->get_channel(Enum::ChannelID::Red);
			std::vector<PsdPixelType> G = Image->get_channel(Enum::ChannelID::Green);
			std::vector<PsdPixelType> B = Image->get_channel(Enum::ChannelID::Blue);

			std::vector<PsdPixelType> A;
			bool bHasAlpha = false;
			try
			{
				A = Image->get_channel(Enum::ChannelID::Alpha);
				bHasAlpha = (A.size() == R.size());
			}
			catch (const std::exception&)
			{
				bHasAlpha = false;
			}

			if (R.size() != G.size() || G.size() != B.size())
			{
				OutDiag.AddWarning(OutLayer.Name,
					TEXT("Image layer R/G/B channel sizes mismatch; pixel data skipped."));
				return;
			}

			InterleaveRGBA(R, G, B, bHasAlpha ? &A : nullptr, OutLayer.RGBAPixels);
		}
		catch (const std::exception& e)
		{
			OutDiag.AddError(OutLayer.Name,
				FString::Printf(TEXT("Failed to extract image pixels: %s"), UTF8_TO_TCHAR(e.what())));
		}
	}

	/** Map PhotoshopAPI paragraph Justification to UE ETextJustify. */
	static ETextJustify::Type MapJustification(TextLayerEnum::Justification J)
	{
		switch (J)
		{
			case TextLayerEnum::Justification::Right:
			case TextLayerEnum::Justification::JustifyLastRight:
				return ETextJustify::Right;
			case TextLayerEnum::Justification::Center:
			case TextLayerEnum::Justification::JustifyLastCenter:
				return ETextJustify::Center;
			default:
				return ETextJustify::Left;
		}
	}

	/** Read a single-run style summary from a TextLayer. */
	static void ExtractSingleRunText(
		const std::shared_ptr<TextLayer<PsdPixelType>>& Text,
		FPsdLayer& OutLayer,
		FPsdParseDiagnostics& OutDiag)
	{
		try
		{
			// Content.
			if (auto Content = Text->text(); Content.has_value())
			{
				OutLayer.Text.Content = Utf8ToFString(*Content);
			}

			// Multi-run detection: if there's more than one style run, flatten
			// to the first run's style and emit a warning per 02-CONTEXT.md.
			if (auto RunLengths = Text->style_run_lengths(); RunLengths.has_value() && RunLengths->size() > 1)
			{
				OutDiag.AddWarning(OutLayer.Name,
					TEXT("Multi-run text flattened to first run's style. Multi-run support arrives in Phase 4."));
			}

			// Font (first run).
			if (auto Font = Text->font_postscript_name(0); Font.has_value())
			{
				OutLayer.Text.FontName = Utf8ToFString(*Font);
			}

			// Size in points -> pixels. At 72 DPI one pt equals one px, the
			// standard PSD assumption. The UMG-DPI conversion (x0.75) is
			// deliberately NOT applied here; it belongs to Phase 4 TEXT-01.
			if (auto FontSize = Text->style_run_font_size(0); FontSize.has_value())
			{
				OutLayer.Text.SizePx = static_cast<float>(*FontSize);
			}

			// Fill colour. PhotoshopAPI reports 0..1 doubles (RGB[A]) in sRGB space.
			// Single-color text layers store the color at the "normal style" level
			// (layer default); only text with mixed styling has a per-run override.
			// Try the run override first, then fall back to the normal style.
			std::optional<std::vector<double>> Fill = Text->style_run_fill_color(0);
			if (!Fill.has_value() || Fill->size() < 3)
			{
				Fill = Text->style_normal_fill_color();
			}
			if (Fill.has_value() && Fill->size() >= 3)
			{
				const uint8 R = static_cast<uint8>(FMath::Clamp((*Fill)[0], 0.0, 1.0) * 255.0);
				const uint8 G = static_cast<uint8>(FMath::Clamp((*Fill)[1], 0.0, 1.0) * 255.0);
				const uint8 B = static_cast<uint8>(FMath::Clamp((*Fill)[2], 0.0, 1.0) * 255.0);
				OutLayer.Text.Color = FLinearColor::FromSRGBColor(FColor(R, G, B));
			}
			else
			{
				OutDiag.AddWarning(OutLayer.Name,
					TEXT("Text layer fill color not found in run or normal style; leaving default."));
			}

			// Justification (first paragraph run).
			if (auto J = Text->paragraph_run_justification(0); J.has_value())
			{
				OutLayer.Text.Alignment = MapJustification(*J);
			}
		}
		catch (const std::exception& e)
		{
			OutDiag.AddWarning(OutLayer.Name,
				FString::Printf(TEXT("Text layer style extraction failed: %s"), UTF8_TO_TCHAR(e.what())));
		}
	}

	void ConvertLayerRecursive(
		const std::shared_ptr<PsdLayer>& InLayer,
		FPsdLayer& OutLayer,
		FPsdParseDiagnostics& OutDiag)
	{
		if (!InLayer)
		{
			OutDiag.AddWarning(TEXT(""), TEXT("Null layer encountered during walk; skipped."));
			OutLayer.Type = EPsdLayerType::Unknown;
			return;
		}

		OutLayer.Name = Utf8ToFString(InLayer->name());
		OutLayer.bVisible = InLayer->visible();
		// PhotoshopAPI Layer<T>::opacity() already returns a normalised 0..1 float.
		OutLayer.Opacity = FMath::Clamp(InLayer->opacity(), 0.0f, 1.0f);
		OutLayer.Bounds = ComputeBounds(InLayer);

		// Layer-type dispatch via RTTI. bUseRTTI=true is set in PSD2UMG.Build.cs.
		if (auto Group = std::dynamic_pointer_cast<GroupLayer<PsdPixelType>>(InLayer))
		{
			OutLayer.Type = EPsdLayerType::Group;
			const auto& Children = Group->layers();
			OutLayer.Children.Reserve(static_cast<int32>(Children.size()));
			for (const auto& Child : Children)
			{
				FPsdLayer& ChildOut = OutLayer.Children.AddDefaulted_GetRef();
				ConvertLayerRecursive(Child, ChildOut, OutDiag);
			}
			return;
		}

		if (auto Image = std::dynamic_pointer_cast<ImageLayer<PsdPixelType>>(InLayer))
		{
			OutLayer.Type = EPsdLayerType::Image;
			ExtractImagePixels(Image, OutLayer, OutDiag);
			return;
		}

		if (auto Text = std::dynamic_pointer_cast<TextLayer<PsdPixelType>>(InLayer))
		{
			OutLayer.Type = EPsdLayerType::Text;
			ExtractSingleRunText(Text, OutLayer, OutDiag);
			return;
		}

		OutLayer.Type = EPsdLayerType::Unknown;
		OutDiag.AddWarning(OutLayer.Name,
			TEXT("Unknown layer kind; skipped (no image/text/group dispatch)."));
	}
} // namespace PSD2UMG::Parser::Internal

namespace PSD2UMG::Parser
{
	bool ParseFile(const FString& Path, FPsdDocument& OutDoc, FPsdParseDiagnostics& OutDiag)
	{
		OutDoc = FPsdDocument();
		OutDoc.SourcePath = Path;

		try
		{
			const std::string PathUtf8(TCHAR_TO_UTF8(*Path));
			std::filesystem::path FsPath(PathUtf8);

			// Read + build the layered representation in one call. PhotoshopAPI
			// only supports reading from disk paths -- no in-memory stream
			// overload exists on LayeredFile<T>::read. Plan 02-04 will spill the
			// factory byte buffer to a temp file to bridge the gap.
			auto File = NAMESPACE_PSAPI::LayeredFile<Internal::PsdPixelType>::read(FsPath);

			OutDoc.CanvasSize = FIntPoint(
				static_cast<int32>(File.width()),
				static_cast<int32>(File.height()));

			auto& RootLayers = File.layers();
			OutDoc.RootLayers.Reserve(static_cast<int32>(RootLayers.size()));
			for (const auto& Child : RootLayers)
			{
				FPsdLayer& ChildOut = OutDoc.RootLayers.AddDefaulted_GetRef();
				Internal::ConvertLayerRecursive(Child, ChildOut, OutDiag);
			}
		}
		catch (const std::exception& e)
		{
			OutDiag.AddError(TEXT(""),
				FString::Printf(TEXT("PhotoshopAPI threw while parsing '%s': %s"),
					*Path, UTF8_TO_TCHAR(e.what())));
			UE_LOG(LogPSD2UMG, Error, TEXT("ParseFile failed: %s"),
				*(OutDiag.Entries.Num() > 0 ? OutDiag.Entries.Last().Message : FString(TEXT("<no message>"))));
			return false;
		}
		catch (...)
		{
			OutDiag.AddError(TEXT(""),
				FString::Printf(TEXT("PhotoshopAPI threw an unknown exception while parsing '%s'."), *Path));
			UE_LOG(LogPSD2UMG, Error, TEXT("ParseFile failed with unknown exception"));
			return false;
		}

		const bool bOk = !OutDiag.HasErrors();
		int32 Warnings = 0;
		int32 Errors = 0;
		for (const FPsdDiagnostic& D : OutDiag.Entries)
		{
			if (D.Severity == EPsdDiagnosticSeverity::Warning) ++Warnings;
			else if (D.Severity == EPsdDiagnosticSeverity::Error) ++Errors;
		}
		UE_LOG(LogPSD2UMG, Log,
			TEXT("ParseFile('%s') -> %s: RootLayers=%d Canvas=%dx%d Warnings=%d Errors=%d"),
			*Path, bOk ? TEXT("ok") : TEXT("failed"),
			OutDoc.RootLayers.Num(), OutDoc.CanvasSize.X, OutDoc.CanvasSize.Y,
			Warnings, Errors);

		return bOk;
	}
}

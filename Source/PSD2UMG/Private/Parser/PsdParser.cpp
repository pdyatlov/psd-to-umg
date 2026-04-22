// Copyright 2018-2021 - John snow wind

#include "Parser/PsdParser.h"
#include "Parser/PsdParserInternal.h"
#include "PSD2UMGLog.h"

#include "Math/IntRect.h"
#include "Math/IntPoint.h"
#include "Math/Color.h"
#include "Misc/FileHelper.h"

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
#include <PhotoshopAPI/LayeredFile/LayerTypes/AdjustmentLayer.h>
#include <PhotoshopAPI/LayeredFile/LayerTypes/ArtboardLayer.h>
#include <PhotoshopAPI/LayeredFile/LayerTypes/ShapeLayer.h>
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
	template<typename TLayer>
	static void ExtractImagePixels(
		const std::shared_ptr<TLayer>& Image,
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
			// Vector smart objects (.ai, unsupported LayerType) and layers with
			// broken linked files throw here. These are not fatal — the layer
			// simply has no pixel data and downstream code falls back to bounds-
			// only placement or rasterized preview. Keeping this at Warning
			// allows the import to proceed and the WBP to be generated.
			OutDiag.AddWarning(OutLayer.Name,
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

			// Multi-run detection: find dominant run by character length. When runs differ
			// in color, the dominant run's color wins (best approximates designer intent for
			// a single UTextBlock). All other run colors are logged as warnings.
			size_t DominantRunIdx = 0;
			{
				auto RunLengths = Text->style_run_lengths();
				if (RunLengths.has_value() && RunLengths->size() > 1)
				{
					size_t MaxLen = 0;
					for (size_t ri = 0; ri < RunLengths->size(); ++ri)
					{
						if ((*RunLengths)[ri] > MaxLen)
						{
							MaxLen = (*RunLengths)[ri];
							DominantRunIdx = ri;
						}
					}
					if (DominantRunIdx != 0)
					{
						UE_LOG(LogPSD2UMG, Log,
							TEXT("Text layer '%s': %d runs, dominant run[%zu] (%zu chars) chosen for color/style."),
							*OutLayer.Name, (int32)RunLengths->size(), DominantRunIdx, MaxLen);
					}
				}
			}

			// Font (dominant run): style_run_font() returns the FontSet index the run
			// uses; font_postscript_name() takes a FontSet index — these are NOT the
			// same value. Passing 0 to font_postscript_name always reads FontSet[0]
			// regardless of which font the run actually references.
			{
				int32_t RunFontIdx = 0;
				if (auto Idx = Text->style_run_font(DominantRunIdx); Idx.has_value() && *Idx >= 0)
					RunFontIdx = *Idx;
				if (auto Font = Text->font_postscript_name(static_cast<size_t>(RunFontIdx)); Font.has_value())
				{
					OutLayer.Text.FontName = Utf8ToFString(*Font);
					UE_LOG(LogPSD2UMG, Log,
						TEXT("Text layer '%s' font: FontSet[%d] = '%s'"),
						*OutLayer.Name, RunFontIdx, *OutLayer.Text.FontName);
				}
			}

			// Size in points -> pixels. At 72 DPI one pt equals one px, the
			// standard PSD assumption. The UMG-DPI conversion (x0.75) is
			// deliberately NOT applied here; it belongs to Phase 4 TEXT-01.
			// Fallback mirrors the fill-color strategy: dominant run first, then normal style.
			{
				std::optional<double> FontSize = Text->style_run_font_size(DominantRunIdx);
				const TCHAR* SizeSource = TEXT("run[dominant]");
				if (!FontSize.has_value())
				{
					FontSize = Text->style_normal_font_size();
					SizeSource = TEXT("normal");
				}
				if (FontSize.has_value())
				{
					// Apply TySh transform scale: effective size = base * scale_y.
					// If designer used Free Transform to resize text, FontSize stays at
					// the base value but scale_y captures the visual scaling. Identity=1.0.
					double EffectiveSize = *FontSize;
					if (const auto ScaleY = Text->scale_y(); ScaleY.has_value() && *ScaleY > 0.01)
					{
						EffectiveSize *= *ScaleY;
					}
					OutLayer.Text.SizePx = static_cast<float>(EffectiveSize);
					UE_LOG(LogPSD2UMG, Log,
						TEXT("Text layer '%s' font size source=%s raw=%.4f scale_y=%.4f effective=%.4f"),
						*OutLayer.Name, SizeSource, *FontSize,
						Text->scale_y().value_or(1.0), EffectiveSize);
				}
				else
				{
					UE_LOG(LogPSD2UMG, Warning,
						TEXT("Text layer '%s' font size not found in dominant run or normal style; leaving default."),
						*OutLayer.Name);
				}
			}

			// Fill colour. Use dominant run first, then normal style fallback.
			const TCHAR* FillSource = TEXT("none");
			std::optional<std::vector<double>> Fill = Text->style_run_fill_color(DominantRunIdx);
			if (Fill.has_value() && Fill->size() >= 3)
			{
				FillSource = DominantRunIdx == 0 ? TEXT("run[0]") : TEXT("run[dominant]");
			}
			else
			{
				Fill = Text->style_normal_fill_color();
				if (Fill.has_value() && Fill->size() >= 3)
				{
					FillSource = TEXT("normal");
				}
			}

			// TEXT-F-03 diagnostic (Phase 12): log raw fill-color values so future
			// fill-channel regressions can be diagnosed without code edits.
			UE_LOG(LogPSD2UMG, Verbose,
				TEXT("Text layer '%s' fill source=%s size=%zu values=[%.4f,%.4f,%.4f,%.4f]"),
				*OutLayer.Name, FillSource,
				Fill.has_value() ? Fill->size() : static_cast<size_t>(0u),
				(Fill.has_value() && Fill->size() > 0) ? (*Fill)[0] : 0.0,
				(Fill.has_value() && Fill->size() > 1) ? (*Fill)[1] : 0.0,
				(Fill.has_value() && Fill->size() > 2) ? (*Fill)[2] : 0.0,
				(Fill.has_value() && Fill->size() > 3) ? (*Fill)[3] : 0.0);

			if (Fill.has_value() && Fill->size() >= 4)
			{
				// PhotoshopAPI returns text fill color as ARGB doubles in [0..1],
				// verified empirically against a pure-red (#FF0000) fixture which
				// came back as [1.0, 1.0, 0.0, 0.0] = (A=1, R=1, G=0, B=0).
				const double A = (*Fill)[0];
				const double Rd = (*Fill)[1];
				const double Gd = (*Fill)[2];
				const double Bd = (*Fill)[3];
				const uint8 R = static_cast<uint8>(FMath::Clamp(Rd, 0.0, 1.0) * 255.0);
				const uint8 G = static_cast<uint8>(FMath::Clamp(Gd, 0.0, 1.0) * 255.0);
				const uint8 B = static_cast<uint8>(FMath::Clamp(Bd, 0.0, 1.0) * 255.0);
				const uint8 Alpha = static_cast<uint8>(FMath::Clamp(A, 0.0, 1.0) * 255.0);
				OutLayer.Text.Color = FLinearColor::FromSRGBColor(FColor(R, G, B, Alpha));
			}
			else if (Fill.has_value() && Fill->size() == 3)
			{
				// Fallback for 3-channel RGB (no alpha). Treat as pure RGB in [0..1].
				const uint8 R = static_cast<uint8>(FMath::Clamp((*Fill)[0], 0.0, 1.0) * 255.0);
				const uint8 G = static_cast<uint8>(FMath::Clamp((*Fill)[1], 0.0, 1.0) * 255.0);
				const uint8 B = static_cast<uint8>(FMath::Clamp((*Fill)[2], 0.0, 1.0) * 255.0);
				OutLayer.Text.Color = FLinearColor::FromSRGBColor(FColor(R, G, B));
			}
			else
			{
				UE_LOG(LogPSD2UMG, Warning,
					TEXT("Text layer '%s' fill color not found in run[0] or normal style; leaving default."),
					*OutLayer.Name);
				OutDiag.AddWarning(OutLayer.Name,
					TEXT("Text layer fill color not found in run or normal style; leaving default."));
			}

			// Justification: prefer the first paragraph run; fall back to the default
			// paragraph sheet for layers whose run dict omits the Justification key
			// (common for single-line center-aligned point text).
			// TEXT-F-02 (Phase 12) — see 12-RESEARCH §TEXT-F-02.
			const TCHAR* JustSource = TEXT("none");
			if (auto J = Text->paragraph_run_justification(0); J.has_value())
			{
				OutLayer.Text.Alignment = MapJustification(*J);
				JustSource = TEXT("run[0]");
			}
			else if (auto normalJustification = Text->paragraph_normal_justification(); normalJustification.has_value())
			{
				OutLayer.Text.Alignment = MapJustification(*normalJustification);
				JustSource = TEXT("normal");
			}
			UE_LOG(LogPSD2UMG, Verbose,
				TEXT("Text layer '%s' justification source=%s value=%d"),
				*OutLayer.Name, JustSource, (int32)OutLayer.Text.Alignment.GetValue());

			// Phase 4 -- weight / style flags (dominant run).
			// Check Faux Bold/Italic flags from Photoshop character panel.
			if (auto Bold = Text->style_run_faux_bold(DominantRunIdx); Bold.has_value())
			{
				OutLayer.Text.bBold = *Bold;
			}
			if (auto Italic = Text->style_run_faux_italic(DominantRunIdx); Italic.has_value())
			{
				OutLayer.Text.bItalic = *Italic;
			}
			// Also detect bold/italic from the PostScript font name itself
			// (e.g. "Arial-BoldMT", "Arial-ItalicMT") — covers real bold/italic
			// fonts where Faux flags are not set.
			if (!OutLayer.Text.bBold || !OutLayer.Text.bItalic)
			{
				const FString& FN = OutLayer.Text.FontName;
				if (!OutLayer.Text.bBold)
				{
					OutLayer.Text.bBold = FN.Contains(TEXT("Bold"), ESearchCase::IgnoreCase);
				}
				if (!OutLayer.Text.bItalic)
				{
					OutLayer.Text.bItalic = FN.Contains(TEXT("Italic"), ESearchCase::IgnoreCase)
						|| FN.Contains(TEXT("Oblique"), ESearchCase::IgnoreCase);
				}
			}

			// Phase 4 -- box / point distinction.
			OutLayer.Text.bHasExplicitWidth = Text->is_box_text();
			if (OutLayer.Text.bHasExplicitWidth)
			{
				if (auto BW = Text->box_width(); BW.has_value())
				{
					OutLayer.Text.BoxWidthPx = static_cast<float>(*BW);
				}
			}

			// Phase 4 -- outline (stroke).
			// TEXT-03 partial delivery: PhotoshopAPI's style_run_stroke_flag reads
			// character-level stroke attributes from the TySh descriptor. Photoshop
			// Layer Style strokes (fx → Stroke) are stored in the lfx2 descriptor
			// which PhotoshopAPI v0.9 does not expose. The wiring below is correct
			// and will activate if the PSD has character-level strokes, but standard
			// Layer Style strokes will not be detected. Full layer-effects parsing
			// belongs to Phase 5 scope.
			bool bStroke = false;
			if (auto SF = Text->style_run_stroke_flag(0); SF.has_value())
			{
				bStroke = *SF;
			}
			if (bStroke)
			{
				if (auto OW = Text->style_run_outline_width(0); OW.has_value())
				{
					OutLayer.Text.OutlineSize = static_cast<float>(*OW);
				}
				if (auto SC = Text->style_run_stroke_color(0); SC.has_value() && SC->size() >= 3)
				{
					// Interpret ARGB to match fill-color quirk (Phase 2-03).
					double A = 1.0, Rd = 0.0, Gd = 0.0, Bd = 0.0;
					if (SC->size() >= 4)
					{
						A  = (*SC)[0];
						Rd = (*SC)[1];
						Gd = (*SC)[2];
						Bd = (*SC)[3];
					}
					else
					{
						Rd = (*SC)[0];
						Gd = (*SC)[1];
						Bd = (*SC)[2];
					}
					const uint8 R = static_cast<uint8>(FMath::Clamp(Rd, 0.0, 1.0) * 255.0);
					const uint8 G = static_cast<uint8>(FMath::Clamp(Gd, 0.0, 1.0) * 255.0);
					const uint8 B = static_cast<uint8>(FMath::Clamp(Bd, 0.0, 1.0) * 255.0);
					const uint8 Alpha = static_cast<uint8>(FMath::Clamp(A, 0.0, 1.0) * 255.0);
					OutLayer.Text.OutlineColor = FLinearColor::FromSRGBColor(FColor(R, G, B, Alpha));
				}
			}
		}
		catch (const std::exception& e)
		{
			OutDiag.AddWarning(OutLayer.Name,
				FString::Printf(TEXT("Text layer style extraction failed: %s"), UTF8_TO_TCHAR(e.what())));
		}
	}

	/**
	 * Parse the lrFX tagged block for a layer and populate OutLayer.Effects.
	 * Handles color overlay (sofi), drop shadow (dsdw), and complex effects (isdw, bevl).
	 * Gracefully skips missing or malformed blocks.
	 */
	static void ExtractLayerEffects(
		const std::shared_ptr<PsdLayer>& InLayer,
		FPsdLayer& OutLayer,
		FPsdParseDiagnostics& OutDiag)
	{
		try
		{
			for (const auto& Block : InLayer->unparsed_tagged_blocks())
			{
				if (!Block || Block->getKey() != NAMESPACE_PSAPI::Enum::TaggedBlockKey::fxLayer)
					continue;

				const auto& Data = Block->m_Data;
				if (Data.size() < 4) break;

				size_t Pos = 0;
				auto ReadU16 = [&]() -> uint16 {
					if (Pos + 2 > Data.size()) return 0;
					uint16 Val = (std::to_integer<uint16>(Data[Pos]) << 8) | std::to_integer<uint16>(Data[Pos + 1]);
					Pos += 2;
					return Val;
				};
				auto ReadU32 = [&]() -> uint32 {
					if (Pos + 4 > Data.size()) return 0;
					uint32 Val = (std::to_integer<uint32>(Data[Pos]) << 24)
					           | (std::to_integer<uint32>(Data[Pos + 1]) << 16)
					           | (std::to_integer<uint32>(Data[Pos + 2]) << 8)
					           | std::to_integer<uint32>(Data[Pos + 3]);
					Pos += 4;
					return Val;
				};
				auto ReadU8 = [&]() -> uint8 {
					if (Pos + 1 > Data.size()) return 0;
					return std::to_integer<uint8>(Data[Pos++]);
				};

				uint16 Version = ReadU16();
				uint16 EffectCount = ReadU16();

				for (uint16 e = 0; e < EffectCount; ++e)
				{
					if (Pos + 8 > Data.size()) break;

					Pos += 4; // skip signature ("8BIM")

					char Key[5] = {};
					for (int k = 0; k < 4; ++k)
						Key[k] = std::to_integer<char>(Data[Pos++]);

					uint32 EffectSize = ReadU32();
					size_t EffectEnd = Pos + EffectSize;
					if (EffectEnd > Data.size()) EffectEnd = Data.size();

					// TEXT-F-03 / TEXT-04 byte-layout diagnostic: dump up to 40 bytes of the
					// effect payload so the actual sofi/dsdw structure can be verified against
					// the PSD spec without code edits. Position-zero is the first byte of the
					// effect-specific payload (after sig/key/size header).
					{
						const size_t DumpLen = FMath::Min<size_t>(40, EffectEnd > Pos ? EffectEnd - Pos : 0);
						FString HexDump;
						for (size_t i = 0; i < DumpLen; ++i)
						{
							HexDump += FString::Printf(TEXT("%02X "), std::to_integer<uint32>(Data[Pos + i]));
						}
						UE_LOG(LogPSD2UMG, Verbose,
							TEXT("Layer '%s' lrFX effect '%c%c%c%c' size=%u payload[0..%zu]= %s"),
							*OutLayer.Name, Key[0], Key[1], Key[2], Key[3],
							EffectSize, DumpLen, *HexDump);
					}

					if (FCStringAnsi::Strcmp(Key, "sofi") == 0)
					{
						// Color overlay / solid fill -- per Adobe PSD spec lrFX v0:
						//   4 bytes Version
						//   4 bytes Blend signature ("8BIM")   <-- previously skipped via
						//   4 bytes Blend key                       a single ReadU32(BlendKey)
						//   2 bytes Color space + 8 bytes color (R,G,B,_)
						//   1 byte Opacity (0..255)
						//   1 byte Enabled
						uint32 SofiVer = ReadU32();
						ReadU32(); // Blend signature ("8BIM") -- was missing, caused 4-byte slip
						uint32 BlendKey = ReadU32();
						uint16 ColorSpace = ReadU16();
						float C0 = static_cast<float>(ReadU16()) / 65535.f;
						float C1 = static_cast<float>(ReadU16()) / 65535.f;
						float C2 = static_cast<float>(ReadU16()) / 65535.f;
						ReadU16(); // unused 4th channel
						uint8 OpacityByte = ReadU8();
						uint8 Enabled = ReadU8();
						float A = static_cast<float>(OpacityByte) / 255.f;

						UE_LOG(LogPSD2UMG, Verbose,
							TEXT("Layer '%s' sofi parsed: ver=%u blendKey=0x%08X colorSpace=%u "
							     "C0=%.3f C1=%.3f C2=%.3f opacity=%u enabled=%u"),
							*OutLayer.Name, SofiVer, BlendKey, ColorSpace,
							C0, C1, C2, OpacityByte, Enabled);

						OutLayer.Effects.bHasColorOverlay = (Enabled != 0);
						OutLayer.Effects.ColorOverlayColor = Enabled ? FLinearColor(C0, C1, C2, A) : FLinearColor::White;
					}
					else if (FCStringAnsi::Strcmp(Key, "dsdw") == 0)
					{
						// Drop shadow -- per Adobe PSD spec lrFX v0:
						//   4  Version (=2)
						//   4  Blur (fixed 16.16)
						//   4  Intensity
						//   4  Angle (fixed 16.16, degrees)
						//   4  Distance (fixed 16.16, pixels)
						//   10 Color (2 colorSpace + 8 channel data)
						//   4  Blend signature ("8BIM")     <-- previously read as a "Sign" byte
						//   4  Blend key                         which corrupted the rest
						//   1  Enabled
						//   1  Use angle in all effects
						//   1  Opacity (0..255)
						//   10 Native color
						uint32 DsdwVer = ReadU32();
						uint32 BlurFixed = ReadU32();
						uint32 IntensityFixed = ReadU32();
						uint32 AngleFixed = ReadU32();
						uint32 DistanceFixed = ReadU32();
						float DistancePx = static_cast<float>(DistanceFixed) / 65536.f;
						float AngleDeg = static_cast<float>(AngleFixed) / 65536.f;
						float AngleRad = FMath::DegreesToRadians(AngleDeg);
						// Photoshop convention: shadow drops in the OPPOSITE direction of the
						// global light angle. Negate to get the visible offset on screen.
						float OffsetX = -DistancePx * FMath::Cos(AngleRad);
						float OffsetY =  DistancePx * FMath::Sin(AngleRad);

						uint16 ColorSpace = ReadU16();
						float C0 = static_cast<float>(ReadU16()) / 65535.f;
						float C1 = static_cast<float>(ReadU16()) / 65535.f;
						float C2 = static_cast<float>(ReadU16()) / 65535.f;
						ReadU16(); // unused 4th channel

						ReadU32(); // Blend signature "8BIM" -- was misread as Sign byte
						uint32 BlendKey = ReadU32();

						uint8 ShadowEnabled = (Pos < EffectEnd) ? ReadU8() : 1;
						uint8 UseAngle      = (Pos < EffectEnd) ? ReadU8() : 1;
						uint8 ShadowOpacity = (Pos < EffectEnd) ? ReadU8() : 128;

						float ShadowA = static_cast<float>(ShadowOpacity) / 255.f;

						UE_LOG(LogPSD2UMG, Verbose,
							TEXT("Layer '%s' dsdw parsed: ver=%u blendKey=0x%08X colorSpace=%u "
							     "C0=%.3f C1=%.3f C2=%.3f angle=%.1f dist=%.2fpx offset=(%.2f,%.2f) "
							     "opacity=%u enabled=%u useAngle=%u"),
							*OutLayer.Name, DsdwVer, BlendKey, ColorSpace,
							C0, C1, C2, AngleDeg, DistancePx, OffsetX, OffsetY,
							ShadowOpacity, ShadowEnabled, UseAngle);

						OutLayer.Effects.DropShadowOffset = FVector2D(OffsetX, OffsetY);
						OutLayer.Effects.bHasDropShadow   = (ShadowEnabled != 0);
						OutLayer.Effects.DropShadowColor  = ShadowEnabled
							? FLinearColor(C0, C1, C2, ShadowA)
							: FLinearColor(0, 0, 0, 0);
					}
					else if (FCStringAnsi::Strcmp(Key, "isdw") == 0
					      || FCStringAnsi::Strcmp(Key, "bevl") == 0)
					{
						// Inner shadow or bevel/emboss — complex effects (per D-09)
						OutLayer.Effects.bHasComplexEffects = true;
					}

					Pos = EffectEnd; // advance to next effect
				}
				break; // only one lrFX block expected
			}
		}
		catch (...)
		{
			OutDiag.AddWarning(OutLayer.Name, TEXT("Failed to parse lrFX effects block; effects ignored."));
		}

		if (OutLayer.Effects.bHasColorOverlay || OutLayer.Effects.bHasDropShadow || OutLayer.Effects.bHasComplexEffects)
		{
			UE_LOG(LogPSD2UMG, Log, TEXT("Layer '%s' effects: ColorOverlay=%d DropShadow=%d Complex=%d"),
				*OutLayer.Name,
				OutLayer.Effects.bHasColorOverlay ? 1 : 0,
				OutLayer.Effects.bHasDropShadow ? 1 : 0,
				OutLayer.Effects.bHasComplexEffects ? 1 : 0);
		}
	}

	// ---------------------------------------------------------------------------
	// Phase 4.1 TEXT-03: lfx2 / FrFX descriptor walker
	//
	// Layer-Style Stroke in Photoshop CS+ is stored in the 'lfx2' (object-based
	// effects) tagged block under descriptor key 'FrFX' (FrameFX).
	// PhotoshopAPI v0.9 silently drops 'lfx2' blocks -- they never appear in
	// unparsed_tagged_blocks(). We therefore scan the raw PSD file bytes directly
	// for 8BIM+lfx2 signatures and build a TMap<LayerName, FPsdStrokeInfo> in
	// ParseFile before ConvertLayerRecursive is called.
	// ---------------------------------------------------------------------------

	/** Stroke info extracted from a raw lfx2/FrFX descriptor. */
	struct FPsdStrokeInfo
	{
		bool         bEnabled  = false;
		float        SizePx    = 0.f;
		FLinearColor Color     = FLinearColor::White;
	};

	/**
	 * Parse a Photoshop FrFX descriptor from raw bytes.
	 * Data must start at the flags+version prefix (8 bytes) that precedes the
	 * top-level Descriptor (i.e. raw lfx2 content, offset 0).
	 * Returns true if a stroke was found and enabled.
	 */
	static bool ParseFrFXDescriptor(
		const TArrayView<const uint8>& Data,
		FPsdStrokeInfo& Out)
	{
		if (Data.Num() < 8) return false;

		// Discriminator: bytes [4..7] must be 0x00000010 (descriptor version 16).
		const uint32 DescVersion =
			(static_cast<uint32>(Data[4]) << 24) |
			(static_cast<uint32>(Data[5]) << 16) |
			(static_cast<uint32>(Data[6]) << 8)  |
			 static_cast<uint32>(Data[7]);
		if (DescVersion != 0x00000010u) return false;

		// The descriptor starts at byte offset 8 (after the 8-byte prefix).
		size_t Pos = 8;

			// ---- Low-level reader helpers ----
		auto CheckRemaining = [&](size_t Need) -> bool
		{
			return (Pos + Need) <= static_cast<size_t>(Data.Num());
		};
		auto ReadU8 = [&]() -> uint8
		{
			if (!CheckRemaining(1)) return 0;
			return static_cast<uint8>(Data[Pos++]);
		};
		auto ReadU32BE = [&]() -> uint32
		{
			if (!CheckRemaining(4)) return 0;
			uint32 V = (static_cast<uint32>(Data[Pos])   << 24)
			         | (static_cast<uint32>(Data[Pos+1]) << 16)
			         | (static_cast<uint32>(Data[Pos+2]) << 8)
			         |  static_cast<uint32>(Data[Pos+3]);
			Pos += 4;
			return V;
		};
		auto ReadDoubleBE = [&]() -> double
		{
			if (!CheckRemaining(8)) return 0.0;
			uint8 Buf[8];
			for (int i = 0; i < 8; ++i) Buf[i] = Data[Pos + i];
			Pos += 8;
			// Reverse for little-endian host
			uint8 Rev[8] = { Buf[7], Buf[6], Buf[5], Buf[4], Buf[3], Buf[2], Buf[1], Buf[0] };
			double V;
			FMemory::Memcpy(&V, Rev, 8);
			return V;
		};
		// ps_string: uint32 len; if len==0 read 4-byte ASCII tag; else read len bytes.
		auto ReadPsString = [&]() -> std::string
		{
			uint32 Len = ReadU32BE();
			if (Len == 0)
			{
				if (!CheckRemaining(4)) return {};
				char Tag[5] = {};
				for (int i = 0; i < 4; ++i)
					Tag[i] = static_cast<char>(Data[Pos + i]);
				Pos += 4;
				return std::string(Tag);
			}
			if (!CheckRemaining(Len)) return {};
			std::string S;
			S.resize(Len);
			for (uint32 i = 0; i < Len; ++i)
				S[i] = static_cast<char>(Data[Pos + i]);
			Pos += Len;
			return S;
		};
		// unicode_string: uint32 num_chars, then num_chars*2 bytes UTF-16 BE (skip content).
		auto SkipUnicodeString = [&]()
		{
			uint32 Len = ReadU32BE();
			Pos += Len * 2; // skip UTF-16 chars
		};

		// Recursive skip helper: ostype already consumed (4 bytes), skip the payload.
		// Declared as std::function to allow self-recursion for Objc/VlLs.
		std::function<void(const char*)> SkipValueAfterOsType;
		SkipValueAfterOsType = [&](const char* OsType)
		{
			if (FCStringAnsi::Strcmp(OsType, "bool") == 0) { Pos += 1; }
			else if (FCStringAnsi::Strcmp(OsType, "long") == 0) { Pos += 4; }
			else if (FCStringAnsi::Strcmp(OsType, "doub") == 0) { Pos += 8; }
			else if (FCStringAnsi::Strcmp(OsType, "UntF") == 0) { Pos += 12; } // 4 unit + 8 double
			else if (FCStringAnsi::Strcmp(OsType, "enum") == 0) { ReadPsString(); ReadPsString(); }
			else if (FCStringAnsi::Strcmp(OsType, "TEXT") == 0)
			{
				uint32 Len = ReadU32BE();
				Pos += Len * 2; // UTF-16 chars
			}
			else if (FCStringAnsi::Strcmp(OsType, "tdta") == 0)
			{
				uint32 Len = ReadU32BE();
				Pos += Len;
			}
			else if (FCStringAnsi::Strcmp(OsType, "Objc") == 0)
			{
				SkipUnicodeString();
				ReadPsString(); // classID
				uint32 Count = ReadU32BE();
				for (uint32 i = 0; i < Count && CheckRemaining(8); ++i)
				{
					ReadPsString();
					char OT2[5] = {};
					for (int k = 0; k < 4; ++k)
						OT2[k] = static_cast<char>(Data[Pos + k]);
					Pos += 4;
					SkipValueAfterOsType(OT2);
				}
			}
			else if (FCStringAnsi::Strcmp(OsType, "VlLs") == 0)
			{
				uint32 N = ReadU32BE();
				for (uint32 i = 0; i < N && CheckRemaining(4); ++i)
				{
					char OT2[5] = {};
					for (int k = 0; k < 4; ++k)
						OT2[k] = static_cast<char>(Data[Pos + k]);
					Pos += 4;
					SkipValueAfterOsType(OT2);
				}
			}
			else
			{
				// Unknown ostype -- log verbose and stop walking (safe abort)
				UE_LOG(LogPSD2UMG, Verbose,
					TEXT("ParseFrFXDescriptor: unknown ostype '%.4hs' at pos %zu; aborting"),
					OsType, Pos);
				Pos = static_cast<size_t>(Data.Num()); // force loop exit
			}
		};

		// ---- Walk the top-level descriptor ----
		// Descriptor header: unicode_class_name, ps_string classID, uint32 item_count
		SkipUnicodeString(); // class name (usually empty)
		ReadPsString();       // classID ('null')
		uint32 TopCount = ReadU32BE();

		bool bFoundStroke = false;
		for (uint32 i = 0; i < TopCount && CheckRemaining(8) && !bFoundStroke; ++i)
		{
			std::string ItemKey = ReadPsString();

			if (!CheckRemaining(4)) break;
			char OsType[5] = {};
			for (int k = 0; k < 4; ++k)
				OsType[k] = static_cast<char>(Data[Pos + k]);
			Pos += 4;

			// We are looking for 'FrFX' with ostype 'Objc' (the stroke sub-descriptor)
			if (ItemKey == "FrFX" && FCStringAnsi::Strcmp(OsType, "Objc") == 0)
			{
				// ---- Parse the FrFX Objc descriptor ----
				SkipUnicodeString(); // class name
				ReadPsString();       // classID ('FrFX')
				uint32 FrFXCount = ReadU32BE();

				bool   bEnab    = false;
				double SzPx     = 0.0;
				double OpctPct  = 100.0;
				double Rd       = 0.0, Grn = 0.0, Bl = 0.0;

				for (uint32 j = 0; j < FrFXCount && CheckRemaining(8); ++j)
				{
					std::string FKey = ReadPsString();

					if (!CheckRemaining(4)) break;
					char FOsType[5] = {};
					for (int k = 0; k < 4; ++k)
						FOsType[k] = static_cast<char>(Data[Pos + k]);
					Pos += 4;

					if (FKey == "enab" && FCStringAnsi::Strcmp(FOsType, "bool") == 0)
					{
						bEnab = (ReadU8() != 0);
					}
					else if (FKey == "Sz  " && FCStringAnsi::Strcmp(FOsType, "UntF") == 0)
					{
						Pos += 4; // skip unit tag (#Pxl)
						SzPx = ReadDoubleBE();
					}
					else if (FKey == "Opct" && FCStringAnsi::Strcmp(FOsType, "UntF") == 0)
					{
						Pos += 4; // skip unit tag (#Prc)
						OpctPct = ReadDoubleBE();
					}
					else if (FKey == "Clr " && FCStringAnsi::Strcmp(FOsType, "Objc") == 0)
					{
						// RGBC sub-descriptor: keys "Rd  ", "Grn ", "Bl  " -- doubles in 0..255
						SkipUnicodeString();
						ReadPsString(); // classID ('RGBC')
						uint32 ClrCount = ReadU32BE();
						for (uint32 c = 0; c < ClrCount && CheckRemaining(8); ++c)
						{
							std::string CKey = ReadPsString();
							if (!CheckRemaining(4)) break;
							char COsType[5] = {};
							for (int k = 0; k < 4; ++k)
								COsType[k] = static_cast<char>(Data[Pos + k]);
							Pos += 4;
							if (FCStringAnsi::Strcmp(COsType, "doub") == 0)
							{
								double V = ReadDoubleBE();
								if      (CKey == "Rd  ") Rd  = V;
								else if (CKey == "Grn ") Grn = V;
								else if (CKey == "Bl  ") Bl  = V;
							}
							else
							{
								SkipValueAfterOsType(COsType);
							}
						}
					}
					else
					{
						SkipValueAfterOsType(FOsType);
					}
				}

				if (bEnab)
				{
					Out.bEnabled = true;
					Out.SizePx   = static_cast<float>(SzPx);
					const float A = FMath::Clamp(static_cast<float>(OpctPct / 100.0), 0.f, 1.f);
					Out.Color = FLinearColor::FromSRGBColor(
						FColor(
							static_cast<uint8>(FMath::Clamp(Rd  / 255.0, 0.0, 1.0) * 255.0),
							static_cast<uint8>(FMath::Clamp(Grn / 255.0, 0.0, 1.0) * 255.0),
							static_cast<uint8>(FMath::Clamp(Bl  / 255.0, 0.0, 1.0) * 255.0),
							static_cast<uint8>(A * 255.0)));
				}

				bFoundStroke = true; // stop searching this descriptor
			}
			else
			{
				SkipValueAfterOsType(OsType);
			}
		}

		return bFoundStroke && Out.bEnabled;
	}

	/**
	 * Phase 13 / GRAD-01: Scan a SoCo (solid color fill) tagged block payload
	 * and populate OutLayer.Effects.ColorOverlayColor + bHasColorOverlay.
	 *
	 * The SoCo payload is a Photoshop object descriptor with an "Objc" item
	 * keyed "Clr " containing a "RGBC" sub-descriptor with "Rd  "/"Grn "/"Bl  "
	 * doubles in [0..255]. Walker mirrors ParseFrFXDescriptor's lambda pattern
	 * (see that function above) with three differences:
	 *   - No 8-byte prefix expected (unlike lfx2/FrFX). We try Pos=0 first;
	 *     if TopCount is absurd we retry at Pos=8 as a defensive fallback.
	 *   - No enabled/opacity/size fields -- solid fill is always enabled by
	 *     Photoshop spec.
	 *   - Output lands in Effects.ColorOverlayColor with bHasColorOverlay=true
	 *     so FWidgetBlueprintGenerator.cpp FX-03 (lines 326-340) tints the
	 *     UImage built by Phase 13 Plan 03 FSolidFillLayerMapper.
	 *
	 * Research refs: 13-RESEARCH Pattern 4, Open Question 1.
	 */
	static void ScanSolidFillColor(
		const std::shared_ptr<PsdLayer>& InLayer,
		FPsdLayer& OutLayer,
		FPsdParseDiagnostics& OutDiag)
	{
		for (const auto& Block : InLayer->unparsed_tagged_blocks())
		{
			if (!Block || Block->getKey() != NAMESPACE_PSAPI::Enum::TaggedBlockKey::adjSolidColor)
				continue;

			const auto& Data = Block->m_Data;
			if (Data.size() < 16)
			{
				OutDiag.AddWarning(OutLayer.Name,
					TEXT("SoCo block too small to contain a descriptor; skipping."));
				return;
			}

			// Diagnostic hex dump -- first 40 bytes so planner/implementer can
			// verify descriptor offset alignment from a single import run.
			{
				const size_t DumpLen = FMath::Min<size_t>(40, Data.size());
				FString HexDump;
				for (size_t i = 0; i < DumpLen; ++i)
				{
					HexDump += FString::Printf(TEXT("%02X "), std::to_integer<uint32>(Data[i]));
				}
				UE_LOG(LogPSD2UMG, Verbose,
					TEXT("Layer '%s' SoCo payload[0..%zu]= %s"),
					*OutLayer.Name, DumpLen, *HexDump);
			}

			auto TryParseAt = [&](size_t StartPos) -> bool
			{
				size_t Pos = StartPos;

				auto CheckRemaining = [&](size_t Need) -> bool {
					return (Pos + Need) <= Data.size();
				};
				auto ReadU32BE = [&]() -> uint32 {
					if (!CheckRemaining(4)) return 0;
					uint32 V = (std::to_integer<uint32>(Data[Pos])   << 24)
					         | (std::to_integer<uint32>(Data[Pos+1]) << 16)
					         | (std::to_integer<uint32>(Data[Pos+2]) << 8)
					         |  std::to_integer<uint32>(Data[Pos+3]);
					Pos += 4;
					return V;
				};
				auto ReadDoubleBE = [&]() -> double {
					if (!CheckRemaining(8)) return 0.0;
					uint8 Buf[8];
					for (int i = 0; i < 8; ++i) Buf[i] = std::to_integer<uint8>(Data[Pos + i]);
					Pos += 8;
					uint8 Rev[8] = { Buf[7], Buf[6], Buf[5], Buf[4], Buf[3], Buf[2], Buf[1], Buf[0] };
					double V;
					FMemory::Memcpy(&V, Rev, 8);
					return V;
				};
				auto ReadPsString = [&]() -> std::string {
					uint32 Len = ReadU32BE();
					if (Len == 0)
					{
						if (!CheckRemaining(4)) return {};
						char Tag[5] = {};
						for (int i = 0; i < 4; ++i) Tag[i] = std::to_integer<char>(Data[Pos + i]);
						Pos += 4;
						return std::string(Tag);
					}
					if (!CheckRemaining(Len)) return {};
					std::string S;
					S.resize(Len);
					for (uint32 i = 0; i < Len; ++i) S[i] = std::to_integer<char>(Data[Pos + i]);
					Pos += Len;
					return S;
				};
				auto SkipUnicodeString = [&]() {
					uint32 Len = ReadU32BE();
					Pos += Len * 2;
				};

				std::function<void(const char*)> SkipValueAfterOsType;
				SkipValueAfterOsType = [&](const char* OsType) {
					if (FCStringAnsi::Strcmp(OsType, "bool") == 0) { Pos += 1; }
					else if (FCStringAnsi::Strcmp(OsType, "long") == 0) { Pos += 4; }
					else if (FCStringAnsi::Strcmp(OsType, "doub") == 0) { Pos += 8; }
					else if (FCStringAnsi::Strcmp(OsType, "UntF") == 0) { Pos += 12; }
					else if (FCStringAnsi::Strcmp(OsType, "enum") == 0) { ReadPsString(); ReadPsString(); }
					else if (FCStringAnsi::Strcmp(OsType, "TEXT") == 0) { uint32 Len = ReadU32BE(); Pos += Len * 2; }
					else if (FCStringAnsi::Strcmp(OsType, "tdta") == 0) { uint32 Len = ReadU32BE(); Pos += Len; }
					else if (FCStringAnsi::Strcmp(OsType, "Objc") == 0)
					{
						SkipUnicodeString();
						ReadPsString();
						uint32 Count = ReadU32BE();
						for (uint32 i = 0; i < Count && CheckRemaining(8); ++i)
						{
							ReadPsString();
							char OT2[5] = {};
							for (int k = 0; k < 4; ++k) OT2[k] = std::to_integer<char>(Data[Pos + k]);
							Pos += 4;
							SkipValueAfterOsType(OT2);
						}
					}
					else if (FCStringAnsi::Strcmp(OsType, "VlLs") == 0)
					{
						uint32 N = ReadU32BE();
						for (uint32 i = 0; i < N && CheckRemaining(4); ++i)
						{
							char OT2[5] = {};
							for (int k = 0; k < 4; ++k) OT2[k] = std::to_integer<char>(Data[Pos + k]);
							Pos += 4;
							SkipValueAfterOsType(OT2);
						}
					}
					else
					{
						UE_LOG(LogPSD2UMG, Verbose,
							TEXT("ScanSolidFillColor: unknown ostype '%.4hs' at pos %zu; aborting"),
							OsType, Pos);
						Pos = Data.size();
					}
				};

				// Top-level descriptor header: unicode_class_name, ps_string classID, uint32 item_count.
				SkipUnicodeString();
				ReadPsString();
				uint32 TopCount = ReadU32BE();

				// Sanity: reject this offset when TopCount is nonsensical.
				if (TopCount == 0 || TopCount > 256) return false;

				bool bFoundColor = false;
				double Rd = 0.0, Grn = 0.0, Bl = 0.0;

				for (uint32 i = 0; i < TopCount && CheckRemaining(8) && !bFoundColor; ++i)
				{
					std::string ItemKey = ReadPsString();
					if (!CheckRemaining(4)) break;
					char OsType[5] = {};
					for (int k = 0; k < 4; ++k) OsType[k] = std::to_integer<char>(Data[Pos + k]);
					Pos += 4;

					if (ItemKey == "Clr " && FCStringAnsi::Strcmp(OsType, "Objc") == 0)
					{
						SkipUnicodeString();
						ReadPsString(); // "RGBC" classID
						uint32 ClrCount = ReadU32BE();
						for (uint32 j = 0; j < ClrCount && CheckRemaining(8); ++j)
						{
							std::string CKey = ReadPsString();
							if (!CheckRemaining(4)) break;
							char COT[5] = {};
							for (int k = 0; k < 4; ++k) COT[k] = std::to_integer<char>(Data[Pos + k]);
							Pos += 4;

							if (FCStringAnsi::Strcmp(COT, "doub") == 0)
							{
								double V = ReadDoubleBE();
								if (CKey == "Rd  ") Rd = V;
								else if (CKey == "Grn ") Grn = V;
								else if (CKey == "Bl  ") Bl = V;
							}
							else
							{
								SkipValueAfterOsType(COT);
							}
						}
						bFoundColor = true;
					}
					else
					{
						SkipValueAfterOsType(OsType);
					}
				}

				if (!bFoundColor) return false;

				const uint8 R8 = static_cast<uint8>(FMath::Clamp(Rd,  0.0, 255.0));
				const uint8 G8 = static_cast<uint8>(FMath::Clamp(Grn, 0.0, 255.0));
				const uint8 B8 = static_cast<uint8>(FMath::Clamp(Bl,  0.0, 255.0));
				OutLayer.Effects.ColorOverlayColor = FLinearColor::FromSRGBColor(FColor(R8, G8, B8, 255));
				OutLayer.Effects.bHasColorOverlay = true;

				UE_LOG(LogPSD2UMG, Verbose,
					TEXT("Layer '%s' SoCo parsed at startPos=%zu: R=%.1f G=%.1f B=%.1f => linear(%.4f,%.4f,%.4f)"),
					*OutLayer.Name, StartPos, Rd, Grn, Bl,
					OutLayer.Effects.ColorOverlayColor.R,
					OutLayer.Effects.ColorOverlayColor.G,
					OutLayer.Effects.ColorOverlayColor.B);

				return true;
			};

			// PSD spec: SoCo additional-layer-info data starts with a 4-byte version (= 16)
			// followed by the descriptor. Try offset 4 first, then 0 and 8 as fallbacks.
			if (!TryParseAt(4) && !TryParseAt(0) && !TryParseAt(8))
			{
				OutDiag.AddWarning(OutLayer.Name,
					TEXT("SoCo descriptor parse failed at offsets 4, 0, and 8; solid fill colour will default to white."));
			}
			return; // Only one SoCo block per layer.
		}
	}

	/**
	 * Phase 14 / SHAPE-01: Scan a vscg (vecStrokeContentData) tagged block
	 * payload and, WHEN the fill-type enum is 'solidColorLayer', populate
	 * OutLayer.Effects.ColorOverlayColor + bHasColorOverlay.
	 *
	 * The vscg payload is a Photoshop object descriptor (same format family
	 * as SoCo). It carries both a Type enum discriminator and the fill color
	 * in a single descriptor:
	 *   - Item key "Type" with ostype "enum" -> enum value string, expected
	 *     to be "solidColorLayer" for Phase 14's target path. "gradientFill"
	 *     and any other value send the layer back to the Phase 13 Gradient
	 *     fallthrough via a `return false`.
	 *   - Item key "Clr " OR "FlCl" (research Pitfall 3: variants exist by
	 *     Photoshop version) with ostype "Objc" -> RGBC sub-descriptor with
	 *     "Rd  "/"Grn "/"Bl  " doubles in [0..255].
	 *
	 * Walker mirrors ScanSolidFillColor's lambda pattern verbatim; three
	 * divergences annotated inline below.
	 *
	 * Returns:
	 *   - true  iff vscg found AND Type == "solidColorLayer" AND Clr/FlCl
	 *           successfully parsed. OutLayer.Effects updated.
	 *   - false if vscg absent, Type != "solidColorLayer" (e.g. gradientFill),
	 *           or descriptor parse failure. OutLayer.Effects unchanged.
	 *
	 * Caller (ConvertLayerRecursive ShapeLayer branch): uses the bool to
	 * decide between EPsdLayerType::Shape (true) and Gradient fallthrough
	 * (false). SoCo check is performed FIRST by the caller so SolidFill
	 * layers with BOTH SoCo and vscg (rare) still dispatch as SolidFill.
	 *
	 * Research refs: 14-RESEARCH Pattern 1, Pattern 3, Common Pitfalls 1/2/3/4,
	 * Open Questions 1 (offset), 2 (exact Type string), 3 (FlCl vs Clr).
	 */
	static bool ScanShapeFillColor(
		const std::shared_ptr<PsdLayer>& InLayer,
		FPsdLayer& OutLayer,
		FPsdParseDiagnostics& OutDiag)
	{
		for (const auto& Block : InLayer->unparsed_tagged_blocks())
		{
			if (!Block || Block->getKey() != NAMESPACE_PSAPI::Enum::TaggedBlockKey::vecStrokeContentData)
				continue;

			const auto& Data = Block->m_Data;
			if (Data.size() < 16)
			{
				OutDiag.AddWarning(OutLayer.Name,
					TEXT("vscg block too small to contain a descriptor; treating as non-solid."));
				return false;
			}

			// Bytes [0..3] of vscg m_Data are the class ID: 'SoCo' = solid fill,
			// 'GdFl' = gradient fill. This is the type discriminator; only solid
			// shapes should produce EPsdLayerType::Shape.
			if (Data.size() < 4
				|| std::to_integer<char>(Data[0]) != 'S'
				|| std::to_integer<char>(Data[1]) != 'o'
				|| std::to_integer<char>(Data[2]) != 'C'
				|| std::to_integer<char>(Data[3]) != 'o')
			{
				UE_LOG(LogPSD2UMG, Verbose,
					TEXT("Layer '%s' vscg classID is not 'SoCo'; treating as non-solid shape."),
					*OutLayer.Name);
				return false;
			}

			auto TryParseAt = [&](size_t StartPos) -> bool
			{
				size_t Pos = StartPos;

				auto CheckRemaining = [&](size_t Need) -> bool {
					return (Pos + Need) <= Data.size();
				};
				auto ReadU32BE = [&]() -> uint32 {
					if (!CheckRemaining(4)) return 0;
					uint32 V = (std::to_integer<uint32>(Data[Pos])   << 24)
					         | (std::to_integer<uint32>(Data[Pos+1]) << 16)
					         | (std::to_integer<uint32>(Data[Pos+2]) << 8)
					         |  std::to_integer<uint32>(Data[Pos+3]);
					Pos += 4;
					return V;
				};
				auto ReadDoubleBE = [&]() -> double {
					if (!CheckRemaining(8)) return 0.0;
					uint8 Buf[8];
					for (int i = 0; i < 8; ++i) Buf[i] = std::to_integer<uint8>(Data[Pos + i]);
					Pos += 8;
					uint8 Rev[8] = { Buf[7], Buf[6], Buf[5], Buf[4], Buf[3], Buf[2], Buf[1], Buf[0] };
					double V;
					FMemory::Memcpy(&V, Rev, 8);
					return V;
				};
				auto ReadPsString = [&]() -> std::string {
					uint32 Len = ReadU32BE();
					if (Len == 0)
					{
						if (!CheckRemaining(4)) return {};
						char Tag[5] = {};
						for (int i = 0; i < 4; ++i) Tag[i] = std::to_integer<char>(Data[Pos + i]);
						Pos += 4;
						return std::string(Tag);
					}
					if (!CheckRemaining(Len)) return {};
					std::string S;
					S.resize(Len);
					for (uint32 i = 0; i < Len; ++i) S[i] = std::to_integer<char>(Data[Pos + i]);
					Pos += Len;
					return S;
				};
				auto SkipUnicodeString = [&]() {
					uint32 Len = ReadU32BE();
					Pos += Len * 2;
				};

				std::function<void(const char*)> SkipValueAfterOsType;
				SkipValueAfterOsType = [&](const char* OsType) {
					if (FCStringAnsi::Strcmp(OsType, "bool") == 0) { Pos += 1; }
					else if (FCStringAnsi::Strcmp(OsType, "long") == 0) { Pos += 4; }
					else if (FCStringAnsi::Strcmp(OsType, "doub") == 0) { Pos += 8; }
					else if (FCStringAnsi::Strcmp(OsType, "UntF") == 0) { Pos += 12; }
					else if (FCStringAnsi::Strcmp(OsType, "enum") == 0) { ReadPsString(); ReadPsString(); }
					else if (FCStringAnsi::Strcmp(OsType, "TEXT") == 0) { uint32 Len = ReadU32BE(); Pos += Len * 2; }
					else if (FCStringAnsi::Strcmp(OsType, "tdta") == 0) { uint32 Len = ReadU32BE(); Pos += Len; }
					else if (FCStringAnsi::Strcmp(OsType, "Objc") == 0)
					{
						SkipUnicodeString();
						ReadPsString();
						uint32 Count = ReadU32BE();
						for (uint32 i = 0; i < Count && CheckRemaining(8); ++i)
						{
							ReadPsString();
							char OT2[5] = {};
							for (int k = 0; k < 4; ++k) OT2[k] = std::to_integer<char>(Data[Pos + k]);
							Pos += 4;
							SkipValueAfterOsType(OT2);
						}
					}
					else if (FCStringAnsi::Strcmp(OsType, "VlLs") == 0)
					{
						uint32 N = ReadU32BE();
						for (uint32 i = 0; i < N && CheckRemaining(4); ++i)
						{
							char OT2[5] = {};
							for (int k = 0; k < 4; ++k) OT2[k] = std::to_integer<char>(Data[Pos + k]);
							Pos += 4;
							SkipValueAfterOsType(OT2);
						}
					}
					else
					{
						UE_LOG(LogPSD2UMG, Verbose,
							TEXT("ScanShapeFillColor: unknown ostype '%.4hs' at pos %zu; aborting"),
							OsType, Pos);
						Pos = Data.size();
					}
				};

				// Top-level descriptor header: unicode_class_name, ps_string classID, uint32 item_count.
				SkipUnicodeString();
				ReadPsString();
				uint32 TopCount = ReadU32BE();

				if (TopCount == 0 || TopCount > 256) return false;

				bool bFoundColor = false;
				double Rd = 0.0, Grn = 0.0, Bl = 0.0;

				for (uint32 i = 0; i < TopCount && CheckRemaining(8); ++i)
				{
					std::string ItemKey = ReadPsString();
					if (!CheckRemaining(4)) break;
					char OsType[5] = {};
					for (int k = 0; k < 4; ++k) OsType[k] = std::to_integer<char>(Data[Pos + k]);
					Pos += 4;

					// Accept "Clr " OR "FlCl" for the color object:
					if ((ItemKey == "Clr " || ItemKey == "FlCl")
					      && FCStringAnsi::Strcmp(OsType, "Objc") == 0)
					{
						SkipUnicodeString();
						ReadPsString(); // "RGBC" classID
						uint32 ClrCount = ReadU32BE();
						for (uint32 j = 0; j < ClrCount && CheckRemaining(8); ++j)
						{
							std::string CKey = ReadPsString();
							if (!CheckRemaining(4)) break;
							char COT[5] = {};
							for (int k = 0; k < 4; ++k) COT[k] = std::to_integer<char>(Data[Pos + k]);
							Pos += 4;

							if (FCStringAnsi::Strcmp(COT, "doub") == 0)
							{
								double V = ReadDoubleBE();
								// Named-key assignment ONLY -- no ARGB swizzle
								// (research Pitfall 4).
								if (CKey == "Rd  ") Rd = V;
								else if (CKey == "Grn ") Grn = V;
								else if (CKey == "Bl  ") Bl = V;
							}
							else
							{
								SkipValueAfterOsType(COT);
							}
						}
						bFoundColor = true;
					}
					else
					{
						SkipValueAfterOsType(OsType);
					}
				}

				if (!bFoundColor)
				{
					UE_LOG(LogPSD2UMG, Verbose,
						TEXT("ScanShapeFillColor: vscg at startPos=%zu missing Color; reject"),
						StartPos);
					return false;
				}

				const uint8 R8 = static_cast<uint8>(FMath::Clamp(Rd,  0.0, 255.0));
				const uint8 G8 = static_cast<uint8>(FMath::Clamp(Grn, 0.0, 255.0));
				const uint8 B8 = static_cast<uint8>(FMath::Clamp(Bl,  0.0, 255.0));
				OutLayer.Effects.ColorOverlayColor = FLinearColor::FromSRGBColor(FColor(R8, G8, B8, 255));
				OutLayer.Effects.bHasColorOverlay = true;

				UE_LOG(LogPSD2UMG, Verbose,
					TEXT("Layer '%s' vscg parsed at startPos=%zu: R=%.1f G=%.1f B=%.1f => linear(%.4f,%.4f,%.4f)"),
					*OutLayer.Name, StartPos, Rd, Grn, Bl,
					OutLayer.Effects.ColorOverlayColor.R,
					OutLayer.Effects.ColorOverlayColor.G,
					OutLayer.Effects.ColorOverlayColor.B);

				return true;
			};

			// vscg layout: bytes[0..3]=classID('SoCo'), bytes[4..7]=version(16),
			// descriptor at offset 8. Try 8 first (empirically confirmed); fall
			// back to 4 and 0 defensively.
			if (TryParseAt(8)) return true;
			if (TryParseAt(4)) return true;
			if (TryParseAt(0)) return true;

			OutDiag.AddWarning(OutLayer.Name,
				TEXT("vscg descriptor parse failed at offsets 4, 0, and 8; treating as non-solid (Gradient fallthrough)."));
			return false; // only one vscg block per layer
		}

		return false; // vscg block not present in this layer
	}

	// ---------------------------------------------------------------------------
	// Raw PSD lfx2 scanner
	//
	// Scans the raw PSD file bytes for 8BIM+lfx2 signatures. For each hit,
	// scans backwards for the nearest 8BIM+luni (unicode layer name) to
	// associate the stroke with a layer name. Builds Lfx2StrokeMap passed
	// into ConvertLayerRecursive.
	// ---------------------------------------------------------------------------

	/**
	 * Parse an 8BIMluni block at FileBytes[LuniOffset] and return the layer name.
	 * LuniOffset points at the start of the 8BIM signature.
	 * Returns empty string on parse failure (caller logs warning).
	 */
	static FString ParseLuniLayerName(const TArray<uint8>& FileBytes, int64 LuniOffset)
	{
		// Format: "8BIM" (4) + "luni" (4) + uint32BE length (4) + uint32BE char_count (4) + UTF-16BE chars
		const int64 Total = FileBytes.Num();
		const int64 LenOffset = LuniOffset + 8;
		if (LenOffset + 8 > Total) return FString();

		const uint32 BlockLen =
			(static_cast<uint32>(FileBytes[LenOffset])     << 24) |
			(static_cast<uint32>(FileBytes[LenOffset + 1]) << 16) |
			(static_cast<uint32>(FileBytes[LenOffset + 2]) << 8)  |
			 static_cast<uint32>(FileBytes[LenOffset + 3]);

		const int64 CharCountOffset = LenOffset + 4;
		if (CharCountOffset + 4 > Total) return FString();

		const uint32 CharCount =
			(static_cast<uint32>(FileBytes[CharCountOffset])     << 24) |
			(static_cast<uint32>(FileBytes[CharCountOffset + 1]) << 16) |
			(static_cast<uint32>(FileBytes[CharCountOffset + 2]) << 8)  |
			 static_cast<uint32>(FileBytes[CharCountOffset + 3]);

		const int64 CharsOffset = CharCountOffset + 4;
		if (CharCount == 0 || CharsOffset + static_cast<int64>(CharCount) * 2 > Total)
			return FString();

		// Convert UTF-16 BE to FString via manual byte swap to LE
		TArray<TCHAR> Chars;
		Chars.Reserve(static_cast<int32>(CharCount) + 1);
		for (uint32 c = 0; c < CharCount; ++c)
		{
			const uint8 Hi = FileBytes[CharsOffset + c * 2];
			const uint8 Lo = FileBytes[CharsOffset + c * 2 + 1];
			const uint16 CP = (static_cast<uint16>(Hi) << 8) | Lo;
			Chars.Add(static_cast<TCHAR>(CP));
		}
		Chars.Add(0);
		return FString(Chars.GetData());
	}

	/**
	 * Scan raw PSD file bytes for all 8BIM+lfx2 blocks. For each, scan backwards
	 * for the nearest 8BIM+luni to get the layer name. Parse the lfx2 content via
	 * ParseFrFXDescriptor. Populate OutMap with enabled stroke info per layer name.
	 */
	static void ScanRawLfx2Blocks(
		const TArray<uint8>& FileBytes,
		TMap<FString, FPsdStrokeInfo>& OutMap,
		FPsdParseDiagnostics& OutDiag)
	{
		const int64 N = FileBytes.Num();
		if (N < 12) return;

		// Signatures as byte arrays for memcmp scanning
		// 8BIM+lfx2 = { 0x38,0x42,0x49,0x4D, 0x6C,0x66,0x78,0x32 }
		// 8BIM+luni = { 0x38,0x42,0x49,0x4D, 0x6C,0x75,0x6E,0x69 }
		static const uint8 Lfx2Sig[8] = { 0x38,0x42,0x49,0x4D, 0x6C,0x66,0x78,0x32 };
		static const uint8 LuniSig[8] = { 0x38,0x42,0x49,0x4D, 0x6C,0x75,0x6E,0x69 };

		int32 HitCount = 0;
		for (int64 i = 0; i <= N - 12; ++i)
		{
			// Scan for 8BIMlfx2
			if (FileBytes[i] != 0x38) continue;
			if (FMemory::Memcmp(&FileBytes[i], Lfx2Sig, 8) != 0) continue;

			// Read uint32BE block length immediately after the 8-byte sig
			const int64 LenAt = i + 8;
			if (LenAt + 4 > N) continue;
			const uint32 BlockLen =
				(static_cast<uint32>(FileBytes[LenAt])     << 24) |
				(static_cast<uint32>(FileBytes[LenAt + 1]) << 16) |
				(static_cast<uint32>(FileBytes[LenAt + 2]) << 8)  |
				 static_cast<uint32>(FileBytes[LenAt + 3]);

			const int64 ContentStart = LenAt + 4;
			if (ContentStart + static_cast<int64>(BlockLen) > N) continue;

			// Scan FORWARD for nearest 8BIM+luni after this offset.
			// PSD layer records write lfx2 near the START of the additional-info
			// section and luni near the END, so the owning layer's luni is the
			// next luni in the file, not the previous one.
			FString LayerName;
			for (int64 j = ContentStart + BlockLen; j <= N - 8; ++j)
			{
				if (FileBytes[j] != 0x38) continue;
				if (FMemory::Memcmp(&FileBytes[j], LuniSig, 8) != 0) continue;
				LayerName = ParseLuniLayerName(FileBytes, j);
				break;
			}

			if (LayerName.IsEmpty())
			{
				OutDiag.AddWarning(TEXT(""),
					FString::Printf(TEXT("lfx2 at offset %lld: could not find following luni block; skipping."),
						static_cast<long long>(i)));
				continue;
			}

			// Build a TArrayView over the content bytes and parse
			TArrayView<const uint8> ContentView(FileBytes.GetData() + ContentStart, static_cast<int32>(BlockLen));
			FPsdStrokeInfo StrokeInfo;
			if (ParseFrFXDescriptor(ContentView, StrokeInfo))
			{
				OutMap.Add(LayerName, StrokeInfo);
				UE_LOG(LogPSD2UMG, Log,
					TEXT("lfx2 raw scan: layer '%s' stroke enab=1 size=%.2fpx color=(%.2f,%.2f,%.2f,%.2f)"),
					*LayerName,
					StrokeInfo.SizePx,
					StrokeInfo.Color.R,
					StrokeInfo.Color.G,
					StrokeInfo.Color.B,
					StrokeInfo.Color.A);
			}
			++HitCount;
		}

		UE_LOG(LogPSD2UMG, Verbose,
			TEXT("ScanRawLfx2Blocks: scanned %lld bytes, found %d lfx2 blocks, %d with enabled stroke"),
			static_cast<long long>(N), HitCount, OutMap.Num());
	}

	/**
	 * Apply pre-scanned lfx2 stroke info (from ScanRawLfx2Blocks) to the layer.
	 * Replaces the dead PhotoshopAPI-block-based walker.
	 */
	static void ExtractLfx2Stroke(
		const FString& LayerName,
		const TMap<FString, FPsdStrokeInfo>& Lfx2Map,
		FPsdLayer& OutLayer)
	{
		const FPsdStrokeInfo* Info = Lfx2Map.Find(LayerName);
		if (!Info || !Info->bEnabled) return;

		OutLayer.Effects.bHasStroke  = true;
		OutLayer.Effects.StrokeSize  = Info->SizePx;
		OutLayer.Effects.StrokeColor = Info->Color;
	}

	// Phase 4.1 D-05/D-06: route text-parented layer effects onto the text payload
	// and clear the Effects flags so downstream generator paths do not double-render
	// (D-13). Called from ConvertLayerRecursive after both ExtractLayerEffects and
	// ExtractLfx2Stroke have populated their respective fields.
	static void RouteTextEffects(FPsdLayer& Layer)
	{
		if (Layer.Type != EPsdLayerType::Text) return;

		if (Layer.Effects.bHasDropShadow)
		{
			Layer.Text.ShadowOffset = Layer.Effects.DropShadowOffset;  // raw PSD pixels
			Layer.Text.ShadowColor  = Layer.Effects.DropShadowColor;   // opacity baked into A
			Layer.Effects.bHasDropShadow = false;                      // D-13 double-render guard
		}

		// TEXT-03 -- D-03: Layer-Style stroke wins over TySh stroke (D-04: silent override
		// with optional Verbose log if TySh had already set OutlineSize).
		if (Layer.Effects.bHasStroke)
		{
			if (Layer.Text.OutlineSize > 0.f)
			{
				UE_LOG(LogPSD2UMG, Verbose,
					TEXT("Layer-Style stroke overriding TySh stroke on layer '%s'"), *Layer.Name);
			}
			Layer.Text.OutlineColor  = Layer.Effects.StrokeColor;
			Layer.Text.OutlineSize   = Layer.Effects.StrokeSize; // raw PSD px; mapper applies x0.75
			Layer.Effects.bHasStroke = false;                    // D-13-style guard
		}

		// TEXT-F-03 (Phase 12): Color Overlay on text layers wins over the run/normal
		// fill (matches Photoshop's render order — overlay paints on top of fill).
		// Mirrors drop-shadow and stroke routing above; clears the flag so the
		// generator's image-only FX-03 block does not emit the "non-image layer
		// ignored" warning (D-13 double-render guard).
		// See 12-RESEARCH §TEXT-F-03 CORRECTION.
		if (Layer.Effects.bHasColorOverlay)
		{
			Layer.Text.Color = Layer.Effects.ColorOverlayColor;
			Layer.Effects.bHasColorOverlay = false; // D-13 double-render guard
		}
	}

	void ConvertLayerRecursive(
		const std::shared_ptr<PsdLayer>& InLayer,
		FPsdLayer& OutLayer,
		FPsdParseDiagnostics& OutDiag,
		const TMap<FString, FPsdStrokeInfo>& Lfx2Map)
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

		// Phase 5: extract layer effects from lrFX tagged block
		ExtractLayerEffects(InLayer, OutLayer, OutDiag);
		// Phase 4.1 TEXT-03: extract Layer-Style Stroke from raw-scanned lfx2 map
		// (PhotoshopAPI v0.9 silently drops lfx2 blocks -- scanned from raw bytes in ParseFile)
		ExtractLfx2Stroke(OutLayer.Name, Lfx2Map, OutLayer);

		// Phase 13 / GRAD-01: fill-layer detection via tagged blocks.
		// PhotoshopAPI classifies gradient and solid-color fill layers as AdjustmentLayer
		// (not ShapeLayer), so dynamic_pointer_cast<ShapeLayer> fails for them. Detect by
		// tagged-block presence instead — type-agnostic and robust across PhotoshopAPI versions.
		// adjSolidColor = "SoCo", adjGradient = "GdFl" per Enum.h TaggedBlockKey.
		{
			bool bIsSolidFill = false, bIsGradientFill = false;
			for (const auto& Block : InLayer->unparsed_tagged_blocks())
			{
				if (!Block) continue;
				const auto Key = Block->getKey();
				if (Key == NAMESPACE_PSAPI::Enum::TaggedBlockKey::adjSolidColor) bIsSolidFill = true;
				if (Key == NAMESPACE_PSAPI::Enum::TaggedBlockKey::adjGradient)   bIsGradientFill = true;
			}
			if (bIsSolidFill)
			{
				OutLayer.Type = EPsdLayerType::SolidFill;
				ScanSolidFillColor(InLayer, OutLayer, OutDiag);
				UE_LOG(LogPSD2UMG, Log,
					TEXT("Layer '%s' dispatched as SolidFill (fill tag branch)"), *OutLayer.Name);
				return;
			}
			if (bIsGradientFill)
			{
				OutLayer.Type = EPsdLayerType::Gradient;
				// Must cast to AdjustmentLayer (the concrete type PhotoshopAPI uses for
				// gradient fill layers) so ExtractImagePixels resolves get_channel
				// on AdjustmentLayer, not the ambiguous Layer<T>/ImageDataMixin base.
				if (auto Adj = std::dynamic_pointer_cast<AdjustmentLayer<PsdPixelType>>(InLayer))
					ExtractImagePixels(Adj, OutLayer, OutDiag);
				else if (auto Shape = std::dynamic_pointer_cast<ShapeLayer<PsdPixelType>>(InLayer))
					ExtractImagePixels(Shape, OutLayer, OutDiag);
				else
					OutDiag.AddWarning(OutLayer.Name, TEXT("Gradient fill: unknown concrete type, pixel extraction skipped."));
				UE_LOG(LogPSD2UMG, Log,
					TEXT("Layer '%s' dispatched as Gradient (fill tag branch)"), *OutLayer.Name);
				return;
			}
		}

		// Layer-type dispatch via RTTI. bUseRTTI=true is set in PSD2UMG.Build.cs.
		//
		// ArtboardLayer inherits from Layer<T> directly (NOT GroupLayer), so
		// dynamic_pointer_cast<GroupLayer> fails for artboards. In PhotoshopAPI,
		// artboard children are siblings of the ArtboardLayer in the parent's
		// layers() vector — the artboard is a section-divider marker only.
		// We detect it early, log it, and treat it as an empty group container
		// (ParsedTags will default it to Canvas so the generator places a CanvasPanel).
		if (auto Artboard = std::dynamic_pointer_cast<ArtboardLayer<PsdPixelType>>(InLayer))
		{
			OutLayer.Type = EPsdLayerType::Group; // treated as group container
			UE_LOG(LogPSD2UMG, Log, TEXT("Layer '%s' is an ArtboardLayer — children appear as siblings in parent"), *OutLayer.Name);
			// ArtboardLayer has no layers() — children are siblings; no recursion here.
			return;
		}

		if (auto Group = std::dynamic_pointer_cast<GroupLayer<PsdPixelType>>(InLayer))
		{
			OutLayer.Type = EPsdLayerType::Group;
			const auto& Children = Group->layers();
			OutLayer.Children.Reserve(static_cast<int32>(Children.size()));
			for (const auto& Child : Children)
			{
				FPsdLayer& ChildOut = OutLayer.Children.AddDefaulted_GetRef();
				ConvertLayerRecursive(Child, ChildOut, OutDiag, Lfx2Map);
			}
			return;
		}

		// SmartObjectLayer MUST be checked before ImageLayer because SmartObjectLayer
		// inherits from Layer<T> directly and dynamic_pointer_cast<ImageLayer> would fail,
		// but we want the explicit SmartObject type to win when the layer is detected.
		if (auto SmartObj = std::dynamic_pointer_cast<SmartObjectLayer<PsdPixelType>>(InLayer))
		{
			OutLayer.Type = EPsdLayerType::SmartObject;
			OutLayer.bIsSmartObject = true;
			// Extract composited preview pixels as fallback image data
			ExtractImagePixels(SmartObj, OutLayer, OutDiag);
			// Extract linked file path if available
			const std::filesystem::path LinkedPath = SmartObj->filepath();
			if (!LinkedPath.empty())
			{
				OutLayer.SmartObjectFilePath = UTF8_TO_TCHAR(LinkedPath.string().c_str());
			}
			UE_LOG(LogPSD2UMG, Log, TEXT("Layer '%s' detected as SmartObject; LinkedFile='%s'"),
				*OutLayer.Name, *OutLayer.SmartObjectFilePath);
			return;
		}

		if (auto Image = std::dynamic_pointer_cast<ImageLayer<PsdPixelType>>(InLayer))
		{
			OutLayer.Type = EPsdLayerType::Image;
			ExtractImagePixels(Image, OutLayer, OutDiag);
			return;
		}

		// Phase 13 / GRAD-01: ShapeLayer dispatch (gradient fill AND solid color fill).
		// Must come AFTER ImageLayer/SmartObject/Group (they are distinct types; the
		// dynamic_pointer_cast<ImageLayer> fails on ShapeLayer so this branch is
		// reached only for actual ShapeLayer instances). Safe to come before
		// TextLayer: fill layers are never text.
		//
		// Detection: scan unparsed_tagged_blocks for SoCo (TaggedBlockKey::adjSolidColor).
		// Present -> solid color fill: call ScanSolidFillColor to populate Effects.
		//   The generator's FX-03 block will tint the UImage built by
		//   FSolidFillLayerMapper (Plan 13-03). No pixel extraction.
		// Absent -> gradient fill (or any other fill subtype -- pattern fill is OOS
		//   per 13-CONTEXT Deferred, but baked pixels are still correct because
		//   PhotoshopAPI composites the layer before we see it).
		if (auto Shape = std::dynamic_pointer_cast<ShapeLayer<PsdPixelType>>(InLayer))
		{
			bool bIsSolidFill = false;
			for (const auto& Block : InLayer->unparsed_tagged_blocks())
			{
				if (Block && Block->getKey() == NAMESPACE_PSAPI::Enum::TaggedBlockKey::adjSolidColor)
				{
					bIsSolidFill = true;
					break;
				}
			}

			// 3-way dispatch:
			//   SoCo present    -> SolidFill (Phase 13, unchanged)
			//   vscg Type=solid -> Shape     (Phase 14, NEW)
			//   else            -> Gradient  (Phase 13 fallthrough, unchanged)
			// SoCo check MUST be first so layers carrying BOTH SoCo and vscg
			// (rare but possible) still dispatch as SolidFill (Phase 13
			// regression guard via FPsdParserGradientSpec::solid_gray).
			const TCHAR* DispatchTag = TEXT("Gradient");
			if (bIsSolidFill)
			{
				OutLayer.Type = EPsdLayerType::SolidFill;
				ScanSolidFillColor(InLayer, OutLayer, OutDiag);
				DispatchTag = TEXT("SolidFill");
				// No pixel extraction: FSolidFillLayerMapper produces a zero-texture
				// UImage and FX-03 tints it.
			}
			else if (ScanShapeFillColor(InLayer, OutLayer, OutDiag))
			{
				// Phase 14: drawn vector shape with solid fill (vscg Type==solidColorLayer).
				// ScanShapeFillColor already populated Effects.ColorOverlayColor +
				// bHasColorOverlay. No pixel extraction: FShapeLayerMapper (Plan
				// 14-03) produces a zero-texture UImage and FX-03 tints it.
				OutLayer.Type = EPsdLayerType::Shape;
				DispatchTag = TEXT("Shape");
			}
			else
			{
				OutLayer.Type = EPsdLayerType::Gradient;
				ExtractImagePixels(Shape, OutLayer, OutDiag);
			}
			UE_LOG(LogPSD2UMG, Log,
				TEXT("Layer '%s' dispatched as %s (ShapeLayer branch)"),
				*OutLayer.Name, DispatchTag);
			return;
		}

		if (auto Text = std::dynamic_pointer_cast<TextLayer<PsdPixelType>>(InLayer))
		{
			OutLayer.Type = EPsdLayerType::Text;
			ExtractSingleRunText(Text, OutLayer, OutDiag);
			RouteTextEffects(OutLayer);
			return;
		}

		OutLayer.Type = EPsdLayerType::Unknown;
		OutDiag.AddWarning(OutLayer.Name,
			TEXT("Unknown layer kind; skipped (no image/text/group dispatch)."));
	}
	/**
	 * Phase 9: walk the parsed tree and populate FPsdLayer::ParsedTags via
	 * FLayerTagParser. Done as a post-pass so that the Type-dependent default
	 * inference (D-02) sees the final EPsdLayerType set by ConvertLayerRecursive.
	 *
	 * LayerIndex is the child's index within its parent (used for D-21 fallback).
	 */
	void PopulateParsedTagsRecursive(
		FPsdLayer& Layer,
		int32 LayerIndex,
		const FString& ParentPath,
		FPsdParseDiagnostics& OutDiag)
	{
		FString Diag;
		Layer.ParsedTags = FLayerTagParser::Parse(Layer.Name, Layer.Type, LayerIndex, Diag);

		const FString FullPath = ParentPath.IsEmpty()
			? Layer.Name
			: FString::Printf(TEXT("%s/%s"), *ParentPath, *Layer.Name);

		if (!Diag.IsEmpty())
		{
			UE_LOG(LogPSD2UMG, Warning, TEXT("Layer '%s': %s"), *FullPath, *Diag);
			OutDiag.AddWarning(Layer.Name, Diag);
		}

		for (int32 i = 0; i < Layer.Children.Num(); ++i)
		{
			PopulateParsedTagsRecursive(Layer.Children[i], i, FullPath, OutDiag);
		}
	}
} // namespace PSD2UMG::Parser::Internal

namespace PSD2UMG::Parser
{
	// Post-pass: PhotoshopAPI always returns (0,0)-(0,0) for GroupLayer bounds.
	// Walk bottom-up and compute each group's bounds as the union of its children.
	static void ComputeGroupBoundsFromChildren(FPsdLayer& Layer)
	{
		for (FPsdLayer& Child : Layer.Children)
			ComputeGroupBoundsFromChildren(Child);

		if (Layer.Type != EPsdLayerType::Group)
			return;
		if (!Layer.Bounds.IsEmpty())
			return;

		FIntRect Union(INT_MAX, INT_MAX, INT_MIN, INT_MIN);
		bool bAny = false;
		for (const FPsdLayer& Child : Layer.Children)
		{
			if (!Child.Bounds.IsEmpty())
			{
				if (!bAny) { Union = Child.Bounds; bAny = true; }
				else
				{
					Union.Min.X = FMath::Min(Union.Min.X, Child.Bounds.Min.X);
					Union.Min.Y = FMath::Min(Union.Min.Y, Child.Bounds.Min.Y);
					Union.Max.X = FMath::Max(Union.Max.X, Child.Bounds.Max.X);
					Union.Max.Y = FMath::Max(Union.Max.Y, Child.Bounds.Max.Y);
				}
			}
		}
		if (bAny)
		{
			UE_LOG(LogPSD2UMG, Log, TEXT("Group '%s' bounds from children: (%d,%d)-(%d,%d)"),
				*Layer.Name, Union.Min.X, Union.Min.Y, Union.Max.X, Union.Max.Y);
			Layer.Bounds = Union;
		}
	}

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

			// Pre-scan raw PSD bytes for lfx2 stroke data (PhotoshopAPI v0.9 drops lfx2 silently)
			TMap<FString, Internal::FPsdStrokeInfo> Lfx2Map;
			{
				TArray<uint8> RawBytes;
				if (FFileHelper::LoadFileToArray(RawBytes, *Path))
				{
					Internal::ScanRawLfx2Blocks(RawBytes, Lfx2Map, OutDiag);
				}
				else
				{
					OutDiag.AddWarning(TEXT(""),
						FString::Printf(TEXT("Could not load raw bytes for lfx2 scan: '%s'; stroke effects will be missing."), *Path));
				}
			}

			auto& RootLayers = File.layers();
			OutDoc.RootLayers.Reserve(static_cast<int32>(RootLayers.size()));
			for (const auto& Child : RootLayers)
			{
				FPsdLayer& ChildOut = OutDoc.RootLayers.AddDefaulted_GetRef();
				Internal::ConvertLayerRecursive(Child, ChildOut, OutDiag, Lfx2Map);
			}

			// Phase 9: populate FPsdLayer::ParsedTags after Type is known (post-pass).
			for (int32 i = 0; i < OutDoc.RootLayers.Num(); ++i)
			{
				Internal::PopulateParsedTagsRecursive(OutDoc.RootLayers[i], i, FString(), OutDiag);
			}

			// Post-pass: compute group bounds bottom-up from children.
			// PhotoshopAPI always returns (0,0)-(0,0) for GroupLayer bounds.
			for (FPsdLayer& Layer : OutDoc.RootLayers)
				ComputeGroupBoundsFromChildren(Layer);
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

// Copyright 2018-2021 - John snow wind

#include "Parser/FLayerTagParser.h"

#include "Parser/PsdTypes.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "Misc/CString.h"

namespace
{
	// ---------- Lookup tables (initialised once) ----------

	const TMap<FString, EPsdTagType>& TypeTagMap()
	{
		static const TMap<FString, EPsdTagType> Map = {
			{ TEXT("button"),      EPsdTagType::Button      },
			{ TEXT("image"),       EPsdTagType::Image       },
			{ TEXT("text"),        EPsdTagType::Text        },
			{ TEXT("progress"),    EPsdTagType::Progress    },
			{ TEXT("hbox"),        EPsdTagType::HBox        },
			{ TEXT("vbox"),        EPsdTagType::VBox        },
			{ TEXT("overlay"),     EPsdTagType::Overlay     },
			{ TEXT("scrollbox"),   EPsdTagType::ScrollBox   },
			{ TEXT("slider"),      EPsdTagType::Slider      },
			{ TEXT("checkbox"),    EPsdTagType::CheckBox    },
			{ TEXT("input"),       EPsdTagType::Input       },
			{ TEXT("list"),        EPsdTagType::List        },
			{ TEXT("tile"),        EPsdTagType::Tile        },
			{ TEXT("smartobject"), EPsdTagType::SmartObject },
			{ TEXT("canvas"),      EPsdTagType::Canvas      },
		};
		return Map;
	}

	const TMap<FString, EPsdAnchorTag>& AnchorTagMap()
	{
		static const TMap<FString, EPsdAnchorTag> Map = {
			{ TEXT("tl"),        EPsdAnchorTag::TL       },
			{ TEXT("tc"),        EPsdAnchorTag::TC       },
			{ TEXT("tr"),        EPsdAnchorTag::TR       },
			{ TEXT("cl"),        EPsdAnchorTag::CL       },
			{ TEXT("c"),         EPsdAnchorTag::C        },
			{ TEXT("cr"),        EPsdAnchorTag::CR       },
			{ TEXT("bl"),        EPsdAnchorTag::BL       },
			{ TEXT("bc"),        EPsdAnchorTag::BC       },
			{ TEXT("br"),        EPsdAnchorTag::BR       },
			{ TEXT("stretch-h"), EPsdAnchorTag::StretchH },
			{ TEXT("stretch-v"), EPsdAnchorTag::StretchV },
			{ TEXT("fill"),      EPsdAnchorTag::Fill     },
		};
		return Map;
	}

	const TMap<FString, EPsdStateTag>& StateTagMap()
	{
		static const TMap<FString, EPsdStateTag> Map = {
			{ TEXT("normal"),   EPsdStateTag::Normal   },
			{ TEXT("hover"),    EPsdStateTag::Hover    },
			{ TEXT("pressed"),  EPsdStateTag::Pressed  },
			{ TEXT("disabled"), EPsdStateTag::Disabled },
			{ TEXT("fill"),     EPsdStateTag::Fill     },
			{ TEXT("bg"),       EPsdStateTag::Bg       },
		};
		return Map;
	}

	const TMap<FString, EPsdAnimTag>& AnimTagMap()
	{
		static const TMap<FString, EPsdAnimTag> Map = {
			{ TEXT("show"),  EPsdAnimTag::Show  },
			{ TEXT("hide"),  EPsdAnimTag::Hide  },
			{ TEXT("hover"), EPsdAnimTag::Hover },
		};
		return Map;
	}

	FString TypeTagToString(EPsdTagType Type)
	{
		switch (Type)
		{
		case EPsdTagType::Button:      return TEXT("Button");
		case EPsdTagType::Image:       return TEXT("Image");
		case EPsdTagType::Text:        return TEXT("Text");
		case EPsdTagType::Progress:    return TEXT("Progress");
		case EPsdTagType::HBox:        return TEXT("HBox");
		case EPsdTagType::VBox:        return TEXT("VBox");
		case EPsdTagType::Overlay:     return TEXT("Overlay");
		case EPsdTagType::ScrollBox:   return TEXT("ScrollBox");
		case EPsdTagType::Slider:      return TEXT("Slider");
		case EPsdTagType::CheckBox:    return TEXT("CheckBox");
		case EPsdTagType::Input:       return TEXT("Input");
		case EPsdTagType::List:        return TEXT("List");
		case EPsdTagType::Tile:        return TEXT("Tile");
		case EPsdTagType::SmartObject: return TEXT("SmartObject");
		case EPsdTagType::Canvas:      return TEXT("Canvas");
		default:                       return TEXT("Layer");
		}
	}

	void AppendDiag(FString& Diag, const FString& Line)
	{
		if (!Diag.IsEmpty())
		{
			Diag += TEXT("\n");
		}
		Diag += Line;
	}

	/**
	 * Apply a single token (already verified to start with '@') to Out.
	 * Captures conflicts / unknown / multiple-type events into Diag.
	 */
	void ApplyToken(const FString& RawToken, FParsedLayerTags& Out, FString& Diag)
	{
		// Strip leading '@'
		FString Body = RawToken.Mid(1);
		if (Body.IsEmpty())
		{
			AppendDiag(Diag, TEXT("Malformed tag '@' (empty name); ignored."));
			return;
		}

		FString Name;
		FString Value;
		if (!Body.Split(TEXT(":"), &Name, &Value))
		{
			Name = Body;
			Value = FString();
		}

		if (Name.IsEmpty())
		{
			AppendDiag(Diag, FString::Printf(TEXT("Malformed tag '%s' (empty name); ignored."), *RawToken));
			return;
		}

		const FString NameLower = Name.ToLower();
		const FString ValueLower = Value.ToLower();

		// ---- Type ----
		if (const EPsdTagType* TypeFound = TypeTagMap().Find(NameLower))
		{
			if (Out.Type != EPsdTagType::None && Out.Type != *TypeFound)
			{
				AppendDiag(Diag, FString::Printf(
					TEXT("error: multiple type tags ('%s' after '%s'); last wins."),
					*TypeTagToString(*TypeFound), *TypeTagToString(Out.Type)));
			}
			Out.Type = *TypeFound;
			return;
		}

		// ---- Anchor ----
		if (NameLower == TEXT("anchor"))
		{
			if (Value.IsEmpty())
			{
				AppendDiag(Diag, TEXT("Malformed @anchor (no value); ignored."));
				Out.UnknownTags.Add(Name);
				return;
			}
			if (const EPsdAnchorTag* Found = AnchorTagMap().Find(ValueLower))
			{
				if (Out.Anchor != EPsdAnchorTag::None && Out.Anchor != *Found)
				{
					AppendDiag(Diag, FString::Printf(
						TEXT("conflict: @anchor specified twice ('%s' overrides previous); last wins."),
						*Value));
				}
				Out.Anchor = *Found;
			}
			else
			{
				AppendDiag(Diag, FString::Printf(TEXT("Unknown @anchor value '%s'; ignored."), *Value));
				Out.UnknownTags.Add(FString::Printf(TEXT("anchor:%s"), *Value));
			}
			return;
		}

		// ---- State ----
		if (NameLower == TEXT("state"))
		{
			if (const EPsdStateTag* Found = StateTagMap().Find(ValueLower))
			{
				if (Out.State != EPsdStateTag::None && Out.State != *Found)
				{
					AppendDiag(Diag, FString::Printf(
						TEXT("conflict: @state specified twice ('%s' overrides previous); last wins."),
						*Value));
				}
				Out.State = *Found;
			}
			else
			{
				AppendDiag(Diag, FString::Printf(TEXT("Unknown @state value '%s'; ignored."), *Value));
				Out.UnknownTags.Add(FString::Printf(TEXT("state:%s"), *Value));
			}
			return;
		}

		// ---- Anim ----
		if (NameLower == TEXT("anim"))
		{
			if (const EPsdAnimTag* Found = AnimTagMap().Find(ValueLower))
			{
				if (Out.Anim != EPsdAnimTag::None && Out.Anim != *Found)
				{
					AppendDiag(Diag, FString::Printf(
						TEXT("conflict: @anim specified twice ('%s' overrides previous); last wins."),
						*Value));
				}
				Out.Anim = *Found;
			}
			else
			{
				AppendDiag(Diag, FString::Printf(TEXT("Unknown @anim value '%s'; ignored."), *Value));
				Out.UnknownTags.Add(FString::Printf(TEXT("anim:%s"), *Value));
			}
			return;
		}

		// ---- 9-slice ----
		if (NameLower == TEXT("9s") || NameLower == TEXT("9slice"))
		{
			FPsdNineSliceMargin Margin;
			if (Value.IsEmpty())
			{
				// Shorthand -- planner default 16px on all sides (matches F9SliceImageLayerMapper).
				Margin.L = Margin.T = Margin.R = Margin.B = 16.f;
				Margin.bExplicit = false;
			}
			else
			{
				TArray<FString> Parts;
				Value.ParseIntoArray(Parts, TEXT(","), /*CullEmpty=*/true);
				if (Parts.Num() != 4)
				{
					AppendDiag(Diag, FString::Printf(
						TEXT("Malformed @9s value '%s' (need L,T,R,B); ignored."), *Value));
					Out.UnknownTags.Add(FString::Printf(TEXT("9s:%s"), *Value));
					return;
				}
				Margin.L = FCString::Atof(*Parts[0]);
				Margin.T = FCString::Atof(*Parts[1]);
				Margin.R = FCString::Atof(*Parts[2]);
				Margin.B = FCString::Atof(*Parts[3]);
				Margin.bExplicit = true;
			}
			if (Out.NineSlice.IsSet())
			{
				AppendDiag(Diag, TEXT("conflict: @9s specified twice; last wins."));
			}
			Out.NineSlice = Margin;
			return;
		}

		// ---- @variants (modifier, no value) ----
		if (NameLower == TEXT("variants"))
		{
			Out.bIsVariants = true;
			return;
		}

		// ---- @background (modifier, no value) ----
		if (NameLower == TEXT("background"))
		{
			Out.bIsBackground = true;
			return;
		}

		// ---- @ia:IA_Name (case preserved) ----
		if (NameLower == TEXT("ia"))
		{
			if (Value.IsEmpty())
			{
				AppendDiag(Diag, TEXT("Malformed @ia (no value); ignored."));
				Out.UnknownTags.Add(Name);
				return;
			}
			if (Out.InputAction.IsSet() && Out.InputAction.GetValue() != Value)
			{
				AppendDiag(Diag, FString::Printf(
					TEXT("conflict: @ia specified twice ('%s' overrides previous); last wins."),
					*Value));
			}
			Out.InputAction = Value; // case preserved per D-04
			return;
		}

		// ---- @smartobject:TypeName (case preserved) ----
		if (NameLower == TEXT("smartobject"))
		{
			// Type is set as well so SmartObject mapper dispatch works uniformly.
			if (Out.Type != EPsdTagType::None && Out.Type != EPsdTagType::SmartObject)
			{
				AppendDiag(Diag, FString::Printf(
					TEXT("error: multiple type tags ('SmartObject' after '%s'); last wins."),
					*TypeTagToString(Out.Type)));
			}
			Out.Type = EPsdTagType::SmartObject;
			if (!Value.IsEmpty())
			{
				if (Out.SmartObjectTypeName.IsSet() && Out.SmartObjectTypeName.GetValue() != Value)
				{
					AppendDiag(Diag, FString::Printf(
						TEXT("conflict: @smartobject specified twice ('%s' overrides previous); last wins."),
						*Value));
				}
				Out.SmartObjectTypeName = Value; // case preserved
			}
			return;
		}

		// ---- Unknown ----
		AppendDiag(Diag, FString::Printf(TEXT("Unknown tag '@%s'; ignored."), *Name));
		Out.UnknownTags.Add(Name);
	}
} // namespace

FParsedLayerTags FLayerTagParser::Parse(
	FStringView LayerName,
	EPsdLayerType SourceLayerType,
	int32 LayerIndex,
	FString& OutDiagnostics)
{
	FParsedLayerTags Out;
	OutDiagnostics.Reset();

	const FString Source(LayerName);

	// 1. Split clean-name from tag stream on first '@'.
	int32 FirstAt = INDEX_NONE;
	Source.FindChar(TEXT('@'), FirstAt);

	const FString CleanRaw = (FirstAt == INDEX_NONE) ? Source : Source.Left(FirstAt);
	Out.CleanName = CleanRaw.TrimStartAndEnd().Replace(TEXT(" "), TEXT("_"));

	// 2. Tokenise the tag stream.
	if (FirstAt != INDEX_NONE)
	{
		const FString Rest = Source.Mid(FirstAt);
		TArray<FString> Tokens;
		Rest.ParseIntoArrayWS(Tokens);
		for (const FString& Token : Tokens)
		{
			if (!Token.StartsWith(TEXT("@")))
			{
				// Stray content between tags -- treat as malformed, ignore.
				continue;
			}
			ApplyToken(Token, Out, OutDiagnostics);
		}
	}

	// 3. D-02 default-type inference.
	if (Out.Type == EPsdTagType::None)
	{
		switch (SourceLayerType)
		{
		case EPsdLayerType::Group:       Out.Type = EPsdTagType::Canvas;      break;
		case EPsdLayerType::Image:       Out.Type = EPsdTagType::Image;       break;
		case EPsdLayerType::Text:        Out.Type = EPsdTagType::Text;        break;
		case EPsdLayerType::SmartObject: Out.Type = EPsdTagType::SmartObject; break;
		case EPsdLayerType::Gradient:    Out.Type = EPsdTagType::Image;       break;
		case EPsdLayerType::SolidFill:   Out.Type = EPsdTagType::Image;       break;
		case EPsdLayerType::Shape:       Out.Type = EPsdTagType::Image;       break;
		default: break;
		}
	}

	// 4. D-21 empty-name fallback.
	if (Out.CleanName.IsEmpty())
	{
		Out.CleanName = FString::Printf(TEXT("%s_%02d"), *TypeTagToString(Out.Type), LayerIndex);
		AppendDiag(OutDiagnostics,
			FString::Printf(TEXT("Layer with empty name -> '%s'"), *Out.CleanName));
	}

	return Out;
}

const FPsdLayer* FLayerTagParser::FindChildByState(
	const TArray<FPsdLayer>& Children,
	EPsdStateTag DesiredState)
{
	if (DesiredState == EPsdStateTag::None)
	{
		return nullptr;
	}

	// D-12: explicit @state match first.
	for (const FPsdLayer& Child : Children)
	{
		if (Child.ParsedTags.State == DesiredState)
		{
			return &Child;
		}
	}

	// D-13: fall back to the first untagged Image child for Normal only.
	if (DesiredState == EPsdStateTag::Normal)
	{
		for (const FPsdLayer& Child : Children)
		{
			if (Child.ParsedTags.State == EPsdStateTag::None && Child.Type == EPsdLayerType::Image)
			{
				return &Child;
			}
		}
	}

	return nullptr;
}

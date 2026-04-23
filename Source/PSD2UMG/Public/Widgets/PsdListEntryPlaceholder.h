#pragma once

#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "PsdListEntryPlaceholder.generated.h"

/**
 * Placeholder entry widget assigned by PSD2UMG to UListView/UTileView on import.
 * Implements IUserObjectListEntry so blueprint compilation succeeds.
 * Replace EntryWidgetClass with your real entry widget after import.
 */
UCLASS(NotBlueprintable, HideDropdown)
class PSD2UMG_API UPsdListEntryPlaceholder : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
};

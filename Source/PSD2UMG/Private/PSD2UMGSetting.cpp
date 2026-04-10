// Copyright 2018-2021 - John snow wind

#include "PSD2UMGSetting.h"

UPSD2UMGSettings::UPSD2UMGSettings()
{
    bEnabled = true;
    bFlattenComplexEffects = true;
    TextureSrcDir.Path = FPaths::ProjectDir() / TEXT("Art/UI/Texture");
    TextureAssetDir.Path = TEXT("/Game/Widgets/Texture");
    WidgetBlueprintAssetDir.Path = TEXT("/Game/UI/Widgets");
}

UPSD2UMGSettings* UPSD2UMGSettings::Get()
{
    return Cast<UPSD2UMGSettings>(StaticClass()->GetDefaultObject());
}

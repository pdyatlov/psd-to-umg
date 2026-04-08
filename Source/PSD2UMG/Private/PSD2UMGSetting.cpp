// Copyright 2018-2021 - John snow wind

#include "PSD2UMGSetting.h"

UPSD2UMGSettings::UPSD2UMGSettings()
{
    bEnabled = true;
    TextureSrcDir.Path = FPaths::ProjectDir() / TEXT("Art/UI/Texture");
    TextureAssetDir.Path = TEXT("/Game/Widgets/Texture");
}

UPSD2UMGSettings* UPSD2UMGSettings::Get()
{
    return Cast<UPSD2UMGSettings>(StaticClass()->GetDefaultObject());
}

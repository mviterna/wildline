#include "WildlineUserSettings.h"
#include "Engine/Engine.h"
UWildlineUserSettings::UWildlineUserSettings() { bInvertMouseY=true; MouseSensitivity=.8f; bBloodEnabled=true; HillConfig.SquirrelGoal=10;for(auto& Bot:FarmConfig.Defenders)Bot.Weapon=1; }
UWildlineUserSettings* UWildlineUserSettings::Get() { return GEngine ? Cast<UWildlineUserSettings>(GEngine->GetGameUserSettings()) : nullptr; }
void UWildlineUserSettings::EnforceHighQuality() {
    // Match the former High Quality preset, with native-resolution rendering.
    SetOverallScalabilityLevel(3);
    SetResolutionScaleValueEx(100.f);
    SetDynamicResolutionEnabled(false);
}
void UWildlineUserSettings::LoadSettings(bool bForceReload) {
    Super::LoadSettings(bForceReload);
    EnforceHighQuality();
    ClassicConfig.Clamp();FarmConfig.Clamp();HillConfig.Clamp();
}
void UWildlineUserSettings::ValidateSettings() {
    Super::ValidateSettings();
    EnforceHighQuality();
    ClassicConfig.Clamp();FarmConfig.Clamp();HillConfig.Clamp();
}
void UWildlineUserSettings::Persist() { MouseSensitivity=FMath::Clamp(MouseSensitivity,.15f,2.f); HunterDifficulty=FMath::Clamp(HunterDifficulty,0,2); ExpeditionMode=FMath::Clamp(ExpeditionMode,0,2);SelectedWeapon=FMath::Clamp(SelectedWeapon,0,1);ScopeSensitivity=FMath::Clamp(ScopeSensitivity,.05f,2.f);SoundVolume=FMath::Clamp(SoundVolume,0.f,1.f);MusicVolume=FMath::Clamp(MusicVolume,0.f,1.f);ClassicConfig.Clamp();FarmConfig.Clamp();HillConfig.Clamp();HunterProfile.Clamp(); SaveSettings(); }

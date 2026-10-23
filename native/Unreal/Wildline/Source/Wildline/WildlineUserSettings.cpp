#include "WildlineUserSettings.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
FString UWildlineUserSettings::CleanPlayerName(const FString& Name){FString Out;for(TCHAR C:Name.TrimStartAndEnd())if(!FChar::IsControl(C)&&C!=TEXT('?')&&C!=TEXT('&')&&C!=TEXT('|'))Out.AppendChar(C);return Out.Left(24).TrimStartAndEnd();}
UWildlineUserSettings::UWildlineUserSettings() { bInvertMouseY=true; MouseSensitivity=.8f; bBloodEnabled=true; HillConfig.SquirrelGoal=10;for(auto& Bot:FarmConfig.Defenders)Bot.Weapon=1; }
UWildlineUserSettings* UWildlineUserSettings::Get() { return GEngine ? Cast<UWildlineUserSettings>(GEngine->GetGameUserSettings()) : nullptr; }
void UWildlineUserSettings::InitializeGraphics() {
    if(GraphicsVersion>0)return;
    SetOverallScalabilityLevel(3);
    SetResolutionScaleValueEx(100.f);
    SetDynamicResolutionEnabled(false);
    GraphicsVersion=1;
}
void UWildlineUserSettings::ApplyNonResolutionSettings(){Super::ApplyNonResolutionSettings();
    if(AntiAliasingMethod!=0&&AntiAliasingMethod!=1&&AntiAliasingMethod!=2&&AntiAliasingMethod!=4)AntiAliasingMethod=4;
    auto Set=[](const TCHAR* Name,float Value){if(auto* C=IConsoleManager::Get().FindConsoleVariable(Name))C->Set(Value,ECVF_SetByCode);};
    Set(TEXT("r.AntiAliasingMethod"),AntiAliasingMethod);Set(TEXT("r.DefaultFeature.MotionBlur"),bMotionBlur?1:0);Set(TEXT("r.MotionBlur.Amount"),bMotionBlur?.35f:0.f);Set(TEXT("r.Tonemapper.Sharpen"),FMath::Clamp(Sharpening,0.f,2.f));
}
void UWildlineUserSettings::LoadSettings(bool bForceReload) {
    Super::LoadSettings(bForceReload);
    LoadLocalProfile();
    InitializeGraphics();
    TreeLeafDensity=FMath::Clamp(TreeLeafDensity,0.f,100.f);WinterTreeLeafDensity=FMath::Clamp(WinterTreeLeafDensity,0.f,100.f);
    ClassicConfig.Clamp();FarmConfig.Clamp();HillConfig.Clamp();LakeConfig.Clamp();
}
void UWildlineUserSettings::ValidateSettings() {
    Super::ValidateSettings();
    InitializeGraphics();
    TreeLeafDensity=FMath::Clamp(TreeLeafDensity,0.f,100.f);WinterTreeLeafDensity=FMath::Clamp(WinterTreeLeafDensity,0.f,100.f);
    ClassicConfig.Clamp();FarmConfig.Clamp();HillConfig.Clamp();LakeConfig.Clamp();
}
void UWildlineUserSettings::Persist() { MouseSensitivity=FMath::Clamp(MouseSensitivity,.15f,2.f); HunterDifficulty=FMath::Clamp(HunterDifficulty,0,2); ExpeditionMode=FMath::Clamp(ExpeditionMode,0,3);SelectedWeapon=FMath::Clamp(SelectedWeapon,0,1);ScopeSensitivity=FMath::Clamp(ScopeSensitivity,.05f,2.f);SoundVolume=FMath::Clamp(SoundVolume,0.f,1.f);MusicVolume=FMath::Clamp(MusicVolume,0.f,1.f);ClassicConfig.Clamp();FarmConfig.Clamp();HillConfig.Clamp();LakeConfig.Clamp();HunterProfile.Clamp(); SaveSettings(); bLastProfileSaveSucceeded=SaveLocalProfile(); }

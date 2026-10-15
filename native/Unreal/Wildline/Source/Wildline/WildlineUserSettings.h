#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "WildlineHunterProfile.h"
#include "WildlineMatchConfig.h"
#include "WildlineUserSettings.generated.h"
UCLASS(Config=GameUserSettings)
class WILDLINE_API UWildlineUserSettings : public UGameUserSettings {
    GENERATED_BODY()
public:
    UWildlineUserSettings();
    void LoadSettings(bool bForceReload = false) override;
    void ValidateSettings() override;
    void EnforceHighQuality();
    UPROPERTY(Config, BlueprintReadWrite) bool bInvertMouseY = true;
    UPROPERTY(Config, BlueprintReadWrite) float MouseSensitivity = .8f;
    UPROPERTY(Config, BlueprintReadWrite) float ScopeSensitivity = .256f;
    UPROPERTY(Config) float SoundVolume=.8f;
    UPROPERTY(Config) float MusicVolume=.65f;
    UPROPERTY(Config, BlueprintReadWrite) float TreeLeafDensity=72.f;
    UPROPERTY(Config, BlueprintReadWrite) float WinterTreeLeafDensity=0.f;
    UPROPERTY(Config) FWildlineMatchConfig ClassicConfig;
    UPROPERTY(Config) FWildlineMatchConfig FarmConfig;
    UPROPERTY(Config) FWildlineMatchConfig HillConfig;
    FWildlineMatchConfig& ConfigForMode(int32 Mode){return Mode==2?HillConfig:Mode==1?FarmConfig:ClassicConfig;}
    UPROPERTY(Config, BlueprintReadWrite) bool bBloodEnabled = true;
    UPROPERTY(Config, BlueprintReadWrite) int32 HunterDifficulty = 1;
    UPROPERTY(Config) int32 ExpeditionMode=0;
    UPROPERTY(Config) int32 SelectedWeapon=0;
    UPROPERTY(Config) FWildlineHunterProfile HunterProfile;
    static UWildlineUserSettings* Get();
    void Persist();
};

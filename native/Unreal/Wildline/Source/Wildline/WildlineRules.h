#pragma once
#include "CoreMinimal.h"
namespace WildlineRules {
constexpr float ThermalDuration = 20.f;
constexpr float ThermalCooldown = 120.f;
constexpr float RoundDuration = 360.f;
constexpr float ReloadDuration = 2.2f;
constexpr float ShotInterval = .85f;
constexpr int32 MagazineSize = 5;
constexpr int32 KillsToWin = 8;
constexpr int32 AcornsToWin = 12;
constexpr float MapRadius = 14500.f;
constexpr int32 ClassicMode=0,FarmMode=1,HillMode=2,LakeMode=3,RifleWeapon=0,ShotgunWeapon=1;
inline int32 WeaponCapacity(int32 Weapon){return Weapon==ShotgunWeapon?2:MagazineSize;}
inline float WeaponInterval(int32 Weapon){return Weapon==ShotgunWeapon?1.05f:ShotInterval;}
inline float WeaponReload(int32 Weapon){return Weapon==ShotgunWeapon?2.8f:ReloadDuration;}
constexpr int32 ShotgunPellets=12;
constexpr float ShotgunRange=4500.f,ShotgunSpreadDegrees=4.f;
inline float PelletDamage(float Distance){return FMath::Lerp(24.f,7.f,FMath::Clamp(Distance/ShotgunRange,0.f,1.f));}
inline const TCHAR* ModeMap(int32 Mode){return Mode==LakeMode?TEXT("TaylorsvilleLake"):Mode==HillMode?TEXT("Neighborhood"):Mode==FarmMode?TEXT("Farmstead"):TEXT("Blackwood");}
inline const TCHAR* FoodName(int32 Mode){return Mode==FarmMode?TEXT("PEANUT"):TEXT("ACORN");}
inline int32 TimeoutWinner(int32 Mode){return Mode==ClassicMode||Mode==LakeMode?2:1;}
// Centimetres and seconds; all hunter AI uses the host's replicated difficulty.
struct FHunterDifficulty { float SightRange,ReactionTime,ShotDelay,AimError,WalkSpeed,StopDistance,MemoryTime; };
inline FHunterDifficulty HunterDifficulty(int32 Level) {
    if(Level<=0)return {2400.f,2.5f,6.5f,100.f,235.f,950.f,2.f};
    if(Level>=2)return {6500.f,.65f,1.65f,12.f,420.f,550.f,6.f};
    return {4200.f,1.4f,3.8f,42.f,320.f,800.f,4.f};
}
inline const TCHAR* DifficultyName(int32 Level){return Level<=0?TEXT("EASY"):Level>=2?TEXT("HARD"):TEXT("MEDIUM");}
inline bool RevealAcorn(int32 Level,float Distance){return Level<=0||Distance<=(Level>=2?2200.f:4500.f);}
inline bool RevealHunter(int32 Level,float Distance,bool Visible){return Level<=0||(Level==1&&Distance<=2500.f&&Visible);}
constexpr float AcornPickupRadius=195.f;
inline bool CanPickAcorn(const FVector& Player,const FVector& Acorn){return FVector::Dist2D(Player,Acorn)<AcornPickupRadius&&FMath::Abs(Player.Z-Acorn.Z)<65.f;}
inline bool CanThermal(bool bHunter, bool bAlive, float Now, float ReadyAt) { return bHunter && bAlive && Now >= ReadyAt; }
inline float PitchInput(float Axis, bool bInverted, float Sensitivity) { return Axis * Sensitivity * (bInverted ? -1.f : 1.f); }
inline float Ground(float X, float Y) {
    const float Radius=FMath::Sqrt(X*X+Y*Y);
    const float Rim=FMath::Clamp((Radius-MapRadius)/6000.f,0.f,1.f);
    const float Ridge=Rim*Rim*(3-2*Rim)*(2600.f+550.f*FMath::Sin(FMath::Atan2(Y,X)*5));
    return 280.f*FMath::Sin(X*.00034f)+210.f*FMath::Cos(Y*.00041f)+80.f*FMath::Sin((X+Y)*.00066f)+Ridge;
}
inline float FarmGround(float X,float Y){return FMath::Lerp(210.f,Ground(X,Y),FMath::SmoothStep(5000.f,9000.f,static_cast<float>(FVector2D(X,Y).Size())));}
}

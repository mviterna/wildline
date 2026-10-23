#pragma once
#include "CoreMinimal.h"
#include "WildlineMatchConfig.generated.h"

USTRUCT()
struct FWildlineBotConfig {
    GENERATED_BODY()
    UPROPERTY() int32 Weapon=0;
    UPROPERTY() int32 Difficulty=1;
    void Clamp(){Weapon=FMath::Clamp(Weapon,0,1);Difficulty=FMath::Clamp(Difficulty,0,2);}
    bool operator==(const FWildlineBotConfig& Other)const{return Weapon==Other.Weapon&&Difficulty==Other.Difficulty;}
};

// Saved independently for each mode, and replicated as one host-owned ruleset.
USTRUCT()
struct FWildlineMatchConfig {
    GENERATED_BODY()
    static constexpr int32 MaxBotsPerTeam=8;
    UPROPERTY() int32 MaxPlayers=8;
    UPROPERTY() float DurationSeconds=360.f;
    UPROPERTY() int32 SquirrelGoal=12;
    UPROPERTY() int32 DefenderGoal=8;
    UPROPERTY() int32 DefenderBots=1;
    UPROPERTY() int32 SquirrelBots=2;
    // Normalized skill: UI 0-100, saved/replicated 0-1. Keep property names so
    // existing saves retain their displayed values; former values above 100 clamp.
    UPROPERTY() float BotSightScale=.5f;
    UPROPERTY() float BotReactionScale=.5f;
    UPROPERTY() float BotAccuracyScale=.5f;
    UPROPERTY() float HunterAggression=.5f;
    UPROPERTY() float SquirrelCaution=.5f;
    UPROPERTY() float BotSpacing=180.f;
    UPROPERTY() float BotTreeSeconds=6.f;
    UPROPERTY() bool bBotClimbing=true;
    UPROPERTY() TArray<FWildlineBotConfig> Defenders;
    UPROPERTY() TArray<FWildlineBotConfig> Squirrels;
    UPROPERTY() bool bAllowMinimap=true;
    UPROPERTY() int32 MapIntel=1;
    UPROPERTY() int32 NeighborhoodLayout=0;
    UPROPERTY() int32 ClassicLayout=0;
    UPROPERTY() float SquirrelHealth=40.f;
    UPROPERTY() int32 SquirrelHitbox=50;
    UPROPERTY() float RespawnSeconds=7.f;
    UPROPERTY() float SpawnProtectionSeconds=0.f;
    UPROPERTY() int32 CarryCapacity=4;
    UPROPERTY() float MovementScale=1.f;
    UPROPERTY() float ReloadScale=1.f;
    UPROPERTY() bool bUnlimitedStamina=false;
    UPROPERTY() bool bInfiniteAmmo=false;
    UPROPERTY() bool bAllowThermal=true;
    UPROPERTY() float ThermalSeconds=20.f;
    UPROPERTY() float ThermalCooldownSeconds=120.f;
    UPROPERTY() bool bLivestockDamage=true;
    // 0 follows the mode; 1 defenders; 2 squirrels.
    UPROPERTY() int32 TimeWinner=0;

    FWildlineMatchConfig(){Defenders.SetNum(MaxBotsPerTeam);Squirrels.SetNum(MaxBotsPerTeam);}
    static float AIStrength(float Skill,float Low,float Normal,float Maximum){
        Skill=FMath::Clamp(Skill,0.f,1.f);
        if(Skill<=0)return Low;if(Skill>=1)return Maximum;if(Skill==.5f)return Normal;
        return Skill<=.5f?FMath::Lerp(Low,Normal,Skill*2):FMath::Lerp(Normal,Maximum,(Skill-.5f)*2);
    }
    float SightMultiplier()const{return AIStrength(BotSightScale,.15f,1.f,3.f);}
    float ReactionDelay(float Normal)const{return Normal<=.02f?Normal:AIStrength(BotReactionScale,Normal*3,Normal,.02f);}
    float ShotDelay(float Normal,float WeaponInterval)const{return AIStrength(BotReactionScale,Normal*2,Normal,WeaponInterval);}
    float AimError(float Normal)const{return Normal*4*FMath::Square(1-FMath::Clamp(BotAccuracyScale,0.f,1.f));}
    float PursuitSpeed(float Normal)const{return AIStrength(HunterAggression,Normal*.6f,Normal,580.f);}
    float StopDistance(float Normal)const{return AIStrength(HunterAggression,Normal*1.5f,Normal,110.f);}
    float SearchMemory(float Normal)const{return AIStrength(HunterAggression,1.f,Normal,20.f);}
    float CautionMultiplier()const{return AIStrength(SquirrelCaution,.25f,1.f,2.5f);}
    // 50 preserves the original 7 cm radius / 12 cm half-height target.
    float SquirrelHitboxScale()const{
        const int32 Value=FMath::Clamp(SquirrelHitbox,1,100);
        return Value<=50?FMath::Lerp(.5f,1.f,(Value-1)/49.f):FMath::Lerp(1.f,2.f,(Value-50)/50.f);
    }
    void Clamp(){
        MaxPlayers=FMath::Clamp(MaxPlayers,1,8);DurationSeconds=FMath::Clamp(DurationSeconds,60.f,1800.f);
        SquirrelGoal=FMath::Clamp(SquirrelGoal,1,30);DefenderGoal=FMath::Clamp(DefenderGoal,1,30);
        DefenderBots=FMath::Clamp(DefenderBots,0,MaxBotsPerTeam);SquirrelBots=FMath::Clamp(SquirrelBots,0,MaxBotsPerTeam);
        BotSightScale=FMath::Clamp(BotSightScale,0.f,1.f);BotReactionScale=FMath::Clamp(BotReactionScale,0.f,1.f);
        BotAccuracyScale=FMath::Clamp(BotAccuracyScale,0.f,1.f);HunterAggression=FMath::Clamp(HunterAggression,0.f,1.f);
        SquirrelCaution=FMath::Clamp(SquirrelCaution,0.f,1.f);BotSpacing=FMath::Clamp(BotSpacing,60.f,400.f);BotTreeSeconds=FMath::Clamp(BotTreeSeconds,2.f,12.f);
        Defenders.SetNum(MaxBotsPerTeam);Squirrels.SetNum(MaxBotsPerTeam);
        for(auto& Bot:Defenders)Bot.Clamp();for(auto& Bot:Squirrels)Bot.Clamp();
        MapIntel=FMath::Clamp(MapIntel,0,2);TimeWinner=FMath::Clamp(TimeWinner,0,2);NeighborhoodLayout=FMath::Clamp(NeighborhoodLayout,0,1);ClassicLayout=FMath::Clamp(ClassicLayout,0,2);
        SquirrelHealth=FMath::Clamp(SquirrelHealth,10.f,200.f);RespawnSeconds=FMath::Clamp(RespawnSeconds,1.f,30.f);
        SquirrelHitbox=FMath::Clamp(SquirrelHitbox,1,100);
        SpawnProtectionSeconds=FMath::Clamp(SpawnProtectionSeconds,0.f,10.f);CarryCapacity=FMath::Clamp(CarryCapacity,1,12);
        MovementScale=FMath::Clamp(MovementScale,.5f,1.5f);ReloadScale=FMath::Clamp(ReloadScale,.25f,2.f);
        ThermalSeconds=FMath::Clamp(ThermalSeconds,5.f,60.f);ThermalCooldownSeconds=FMath::Clamp(ThermalCooldownSeconds,0.f,300.f);
    }
    static FWildlineMatchConfig Preset(int32 Mode,int32 Scheme){
        FWildlineMatchConfig C;if(Mode==2)C.SquirrelGoal=10;
        if(Mode==1)for(auto& Bot:C.Defenders)Bot.Weapon=1;
        if(Scheme==1){
            C.DurationSeconds=600;C.SquirrelHealth=80;C.RespawnSeconds=3;C.SpawnProtectionSeconds=3;C.MapIntel=0;
            C.CarryCapacity=6;C.ThermalCooldownSeconds=60;C.BotAccuracyScale=.3f;C.BotReactionScale=.35f;C.HunterAggression=.35f;for(auto& Bot:C.Defenders)Bot.Difficulty=0;
        }else if(Scheme==2){
            C.bAllowMinimap=false;C.bAllowThermal=false;C.RespawnSeconds=12;C.CarryCapacity=2;
            C.DefenderBots=2;C.MapIntel=2;C.BotSightScale=.85f;C.BotReactionScale=.9f;C.BotAccuracyScale=.95f;C.HunterAggression=.9f;C.SquirrelCaution=.85f;for(auto& Bot:C.Defenders)Bot.Difficulty=2;for(auto& Bot:C.Squirrels)Bot.Difficulty=2;
        }else if(Scheme==3){
            C.DurationSeconds=1800;C.SquirrelGoal=30;C.DefenderGoal=30;C.SquirrelHealth=100;C.RespawnSeconds=1;
            C.SpawnProtectionSeconds=5;C.CarryCapacity=12;C.bUnlimitedStamina=true;C.bInfiniteAmmo=true;
            C.MapIntel=0;C.ReloadScale=.5f;C.ThermalCooldownSeconds=0;C.bLivestockDamage=false;
        }
        return C;
    }
    bool operator==(const FWildlineMatchConfig& O)const{
        return MaxPlayers==O.MaxPlayers&&DurationSeconds==O.DurationSeconds&&SquirrelGoal==O.SquirrelGoal&&DefenderGoal==O.DefenderGoal
            &&DefenderBots==O.DefenderBots&&SquirrelBots==O.SquirrelBots&&Defenders==O.Defenders&&Squirrels==O.Squirrels
            &&BotSightScale==O.BotSightScale&&BotReactionScale==O.BotReactionScale&&BotAccuracyScale==O.BotAccuracyScale
            &&HunterAggression==O.HunterAggression&&SquirrelCaution==O.SquirrelCaution&&BotSpacing==O.BotSpacing&&BotTreeSeconds==O.BotTreeSeconds&&bBotClimbing==O.bBotClimbing
            &&bAllowMinimap==O.bAllowMinimap&&MapIntel==O.MapIntel&&ClassicLayout==O.ClassicLayout&&NeighborhoodLayout==O.NeighborhoodLayout&&SquirrelHealth==O.SquirrelHealth&&RespawnSeconds==O.RespawnSeconds
            &&SquirrelHitbox==O.SquirrelHitbox&&SpawnProtectionSeconds==O.SpawnProtectionSeconds&&CarryCapacity==O.CarryCapacity&&MovementScale==O.MovementScale&&ReloadScale==O.ReloadScale
            &&bUnlimitedStamina==O.bUnlimitedStamina&&bInfiniteAmmo==O.bInfiniteAmmo&&bAllowThermal==O.bAllowThermal
            &&ThermalSeconds==O.ThermalSeconds&&ThermalCooldownSeconds==O.ThermalCooldownSeconds&&bLivestockDamage==O.bLivestockDamage&&TimeWinner==O.TimeWinner;
    }
    bool operator!=(const FWildlineMatchConfig& Other)const{return !(*this==Other);}
};

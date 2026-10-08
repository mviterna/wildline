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
    UPROPERTY() TArray<FWildlineBotConfig> Defenders;
    UPROPERTY() TArray<FWildlineBotConfig> Squirrels;
    UPROPERTY() bool bAllowMinimap=true;
    UPROPERTY() int32 MapIntel=1;
    UPROPERTY() float SquirrelHealth=40.f;
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
    void Clamp(){
        MaxPlayers=FMath::Clamp(MaxPlayers,1,8);DurationSeconds=FMath::Clamp(DurationSeconds,60.f,1800.f);
        SquirrelGoal=FMath::Clamp(SquirrelGoal,1,30);DefenderGoal=FMath::Clamp(DefenderGoal,1,30);
        DefenderBots=FMath::Clamp(DefenderBots,0,MaxBotsPerTeam);SquirrelBots=FMath::Clamp(SquirrelBots,0,MaxBotsPerTeam);
        Defenders.SetNum(MaxBotsPerTeam);Squirrels.SetNum(MaxBotsPerTeam);
        for(auto& Bot:Defenders)Bot.Clamp();for(auto& Bot:Squirrels)Bot.Clamp();
        MapIntel=FMath::Clamp(MapIntel,0,2);TimeWinner=FMath::Clamp(TimeWinner,0,2);
        SquirrelHealth=FMath::Clamp(SquirrelHealth,10.f,200.f);RespawnSeconds=FMath::Clamp(RespawnSeconds,1.f,30.f);
        SpawnProtectionSeconds=FMath::Clamp(SpawnProtectionSeconds,0.f,10.f);CarryCapacity=FMath::Clamp(CarryCapacity,1,12);
        MovementScale=FMath::Clamp(MovementScale,.5f,1.5f);ReloadScale=FMath::Clamp(ReloadScale,.25f,2.f);
        ThermalSeconds=FMath::Clamp(ThermalSeconds,5.f,60.f);ThermalCooldownSeconds=FMath::Clamp(ThermalCooldownSeconds,0.f,300.f);
    }
    static FWildlineMatchConfig Preset(int32 Mode,int32 Scheme){
        FWildlineMatchConfig C;if(Mode==2)C.SquirrelGoal=10;
        if(Mode==1)for(auto& Bot:C.Defenders)Bot.Weapon=1;
        if(Scheme==1){
            C.DurationSeconds=600;C.SquirrelHealth=80;C.RespawnSeconds=3;C.SpawnProtectionSeconds=3;C.MapIntel=0;
            C.CarryCapacity=6;C.ThermalCooldownSeconds=60;for(auto& Bot:C.Defenders)Bot.Difficulty=0;
        }else if(Scheme==2){
            C.bAllowMinimap=false;C.bAllowThermal=false;C.RespawnSeconds=12;C.CarryCapacity=2;
            C.DefenderBots=2;C.MapIntel=2;for(auto& Bot:C.Defenders)Bot.Difficulty=2;for(auto& Bot:C.Squirrels)Bot.Difficulty=2;
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
            &&bAllowMinimap==O.bAllowMinimap&&MapIntel==O.MapIntel&&SquirrelHealth==O.SquirrelHealth&&RespawnSeconds==O.RespawnSeconds
            &&SpawnProtectionSeconds==O.SpawnProtectionSeconds&&CarryCapacity==O.CarryCapacity&&MovementScale==O.MovementScale&&ReloadScale==O.ReloadScale
            &&bUnlimitedStamina==O.bUnlimitedStamina&&bInfiniteAmmo==O.bInfiniteAmmo&&bAllowThermal==O.bAllowThermal
            &&ThermalSeconds==O.ThermalSeconds&&ThermalCooldownSeconds==O.ThermalCooldownSeconds&&bLivestockDamage==O.bLivestockDamage&&TimeWinner==O.TimeWinner;
    }
    bool operator!=(const FWildlineMatchConfig& Other)const{return !(*this==Other);}
};

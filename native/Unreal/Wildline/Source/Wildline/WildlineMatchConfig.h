#pragma once
#include "CoreMinimal.h"
#include "WildlineMatchConfig.generated.h"

USTRUCT()
struct FWildlineMatchConfig {
    GENERATED_BODY()
    UPROPERTY() int32 MaxPlayers=8;
    UPROPERTY() float DurationSeconds=360.f;
    UPROPERTY() int32 SquirrelGoal=12;
    UPROPERTY() int32 DefenderGoal=8;
    void Clamp(){MaxPlayers=FMath::Clamp(MaxPlayers,1,8);DurationSeconds=FMath::Clamp(DurationSeconds,60.f,1800.f);SquirrelGoal=FMath::Clamp(SquirrelGoal,1,30);DefenderGoal=FMath::Clamp(DefenderGoal,1,30);}
    bool operator==(const FWildlineMatchConfig& Other)const{return MaxPlayers==Other.MaxPlayers&&DurationSeconds==Other.DurationSeconds&&SquirrelGoal==Other.SquirrelGoal&&DefenderGoal==Other.DefenderGoal;}
    bool operator!=(const FWildlineMatchConfig& Other)const{return !(*this==Other);}
};

#pragma once
#include "CoreMinimal.h"

// Bark cross sections measured from the CC0 source geometry, in centimetres.
// X/Y are the stem centre, Z is height, W is a conservative bark radius.
// Stop at continuous load-bearing branches, before disconnected crown twigs.
inline const TArray<FVector4>& WildlineTrunkProfile(int32 Species){
    static const TArray<FVector4> Profiles[]={
    {FVector4(3,26,0,66),FVector4(3,16,100,64),FVector4(3,3,200,61),FVector4(18,-24,300,77),FVector4(123,-75,400,81),FVector4(221,-124,500,45),FVector4(280,-149,600,38),FVector4(315,-210,800,47),FVector4(332,-219,1000,32)},
    {FVector4(0,0,0,59),FVector4(1,2,100,58),FVector4(1,3,200,57),FVector4(6,0,300,56),FVector4(20,-14,400,50),FVector4(24,-26,500,45),FVector4(11,-37,600,41),FVector4(36,37,800,42),FVector4(64,102,1000,34),FVector4(59,87,1200,29),FVector4(-3,125,1400,28)},
    {FVector4(0,1,0,57),FVector4(-2,2,100,52),FVector4(-4,3,200,48),FVector4(-5,5,300,47),FVector4(-16,-35,400,41),FVector4(-25,-69,500,39),FVector4(-36,-97,600,39),FVector4(-17,-59,800,31),FVector4(-9,-83,1000,24),FVector4(-7,-147,1200,20)},
    {FVector4(-4,3,460,36),FVector4(-4,3,500,35),FVector4(1,-2,600,30),FVector4(0,0,800,26),FVector4(-1,0,1000,22),FVector4(0,0,1200,18),FVector4(0,-1,1400,14),FVector4(0,0,1600,12)},
    };
    return Profiles[FMath::Clamp(Species,0,3)];
}

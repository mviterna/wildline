#pragma once
#include "CoreMinimal.h"
#include "Math/RotationMatrix.h"

namespace WildlineHunterGrip {
    // Weapon coordinates: +X muzzle, +Y right, +Z up. The mannequin wrist
    // origin is behind the palm; placing it at the trigger puts fingers on the optic.
    inline FQuat Rotation(int32 Side){
        return Side==0?FRotationMatrix::MakeFromXY(FVector(-.85f,0,.53f),FVector(0,-1,0)).ToQuat()
                      :FRotationMatrix::MakeFromXY(FVector(0,1,0),FVector(0,0,-1)).ToQuat();
    }
    inline FVector Wrist(int32 Side,bool Shotgun,float HandScale){
        const FVector Base=Side==0?FVector(-20.5f,4.6f,Shotgun?-3.5f:-1.5f):FVector(Shotgun?18.5f:17.f,-8.f,-4.2f);
        const FVector Contact=Side==0?FVector(-7.f,2.5f,1.f):FVector(8.f,0,0);
        return Base+Rotation(Side).RotateVector(Contact)*(1-HandScale);
    }
}

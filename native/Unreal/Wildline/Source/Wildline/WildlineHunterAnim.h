#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "WildlineHunterAnim.generated.h"
class UAnimSequence;

UCLASS(Transient)
class WILDLINE_API UWildlineHunterAnim : public UAnimInstance {
    GENERATED_BODY()
public:
    void NativeInitializeAnimation() override;
    void NativePostEvaluateAnimation() override;
    UPROPERTY() UAnimSequence* Idle;
    UPROPERTY() UAnimSequence* Walk;
    UPROPERTY() UAnimSequence* Run;
    UPROPERTY() TArray<UAnimSequence*> WalkDirections;
    UPROPERTY() TArray<UAnimSequence*> RunDirections;
    UPROPERTY() UAnimSequence* JumpPose;
    UPROPERTY() UAnimSequence* RifleReady;
    UPROPERTY() UAnimSequence* CrouchIdle;
    UPROPERTY() UAnimSequence* JumpStart;
    UPROPERTY() UAnimSequence* Land;
    UPROPERTY() UAnimSequence* TurnLeft;
    UPROPERTY() UAnimSequence* TurnRight;
    UPROPERTY() TArray<UAnimSequence*> CrouchDirections;
    UPROPERTY() TArray<UAnimSequence*> WalkStarts;
    UPROPERTY() TArray<UAnimSequence*> WalkStops;
    UPROPERTY() TArray<UAnimSequence*> RunStarts;
    UPROPERTY() TArray<UAnimSequence*> RunStops;
    float GaitPhase=0,FootPlantWeight[2]={0,0};
    float CrouchBlend=0,MovementBlend=0,GroundOffset=0,AimBlend=0;
    int32 FootPlantSerial[2]={0,0};
    bool bCapturedMotion=false;
protected:
    FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};

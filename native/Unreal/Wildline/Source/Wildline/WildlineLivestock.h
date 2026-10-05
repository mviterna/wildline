#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Animation/AnimInstance.h"
#include "WildlineLivestock.generated.h"
class UCapsuleComponent;
class USkeletalMeshComponent;
class AWildlineCharacter;
UCLASS()
class WILDLINE_API AWildlineLivestock : public AActor {
    GENERATED_BODY()
public:
    AWildlineLivestock();
    void BeginPlay() override;
    void Tick(float Delta) override;
    void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    UPROPERTY() UCapsuleComponent* Body;
    UPROPERTY() USkeletalMeshComponent* Mesh;
    UPROPERTY(ReplicatedUsing=OnRepSpecies) int32 Species=0;
    UPROPERTY(Replicated) FVector Home=FVector::ZeroVector;
    UPROPERTY(Replicated) float Phase=0;
    UPROPERTY(Replicated) float WalkSpeed=0;
    UPROPERTY(Replicated) float StrikeAt=0;
    UFUNCTION() void OnRepSpecies();
    UFUNCTION(NetMulticast,Unreliable) void MulticastWarning();
    float ServerTime() const;
private:
    TWeakObjectPtr<AWildlineCharacter> Threat;
    float ReadyAt=0;
    bool bStrikeDone=true;
};
UCLASS(Transient)
class WILDLINE_API UWildlineLivestockAnim : public UAnimInstance {
    GENERATED_BODY()
protected:
    FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};

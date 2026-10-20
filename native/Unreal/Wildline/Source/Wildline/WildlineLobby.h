#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WildlineLobby.generated.h"
class UStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;
class USceneComponent;
class UPointLightComponent;

// Both interiors are built deterministically on every peer, above the reserve.
// Only readiness/player state replicates; decorative components need no network traffic.
UCLASS()
class WILDLINE_API AWildlineLobby : public AActor {
    GENERATED_BODY()
public:
    AWildlineLobby();
    void BeginPlay() override;
    void Tick(float DeltaSeconds) override;
    static AWildlineLobby* Find(UWorld* World);
    static FVector Centre(bool Squirrel);
    static FVector Spawn(bool Squirrel,int32 Index=0);
    static FVector Portal(bool Squirrel);
    static bool OnPortal(bool Squirrel,FVector Position);
    static int32 WeaponAt(FVector Position);
    static FVector Rack(int32 Weapon);
    UPROPERTY() TArray<UStaticMeshComponent*> Props;
    UPROPERTY() TArray<USceneComponent*> LodgeScenery;
    UPROPERTY() UPointLightComponent* HearthLight=nullptr;
    FString ExteriorTheme;
private:
    bool bLodgeSceneryVisible=false;
    UStaticMeshComponent* Piece(const TCHAR* Name,const TCHAR* Shape,FVector Position,FVector Size,UMaterialInterface* Material,FRotator Rotation=FRotator::ZeroRotator,bool Collision=false);
    UStaticMeshComponent* Model(const TCHAR* Asset,FVector Position,FVector DesiredSize=FVector::ZeroVector,FRotator Rotation=FRotator::ZeroRotator,bool Collision=false);
    void Sign(const FString& Text,FVector Position,float Size,FRotator Rotation=FRotator(0,180,0));
    UPointLightComponent* Light(FVector Position,FLinearColor Color,float Intensity,float Radius,bool Shadows=false);
    void BuildLodge();
    void BuildLodgeDetails();
    void BuildLodgeExterior();
    void BuildTree();
    void BuildPortal(bool Squirrel);
};

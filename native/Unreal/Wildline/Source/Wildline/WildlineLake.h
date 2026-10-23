#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WildlineLake.generated.h"
class UStaticMeshComponent;
class UProceduralMeshComponent;
class UMaterialInterface;

// Deterministic scenery on every peer. Water and fishing queries are also used
// by the authoritative character, rather than relying on client overlap RPCs.
UCLASS()
class WILDLINE_API AWildlineLake : public AActor {
    GENERATED_BODY()
public:
    AWildlineLake();
    void BeginPlay() override;
    void Tick(float Delta) override;
    static AWildlineLake* Find(UWorld* World);
    static FVector HuntingCentre(){return FVector(1800,1200,100);}
    static FVector2D Extent(bool Lodge=false){return Lodge?FVector2D(9000,6500):FVector2D(10000,7600);}
    static float ShoreShape(float Angle,bool Lodge=false);
    static FVector ShorePoint(float Angle,float Radius=1.f,bool Lodge=false);
    static int32 FishCount(bool Lodge=false){return Lodge?32:64;}
    static FVector LodgeCentre();
    static float RadiusAt(FVector Point,bool Lodge=false);
    static float BedHeight(float X,float Y);
    static FVector TrailPoint(float Angle);
    static bool WaterAt(UWorld* World,FVector Point,float& Surface);
    static bool InLodgeGrounds(FVector Point);
    static FVector FishingSpot(int32 Index);
    static int32 FishingSpotAt(FVector Point);
    static FVector RouteAroundLake(FVector From,FVector Target);
    UPROPERTY() TArray<UStaticMeshComponent*> Props;
    UPROPERTY() TArray<UStaticMeshComponent*> Fish;
    UPROPERTY() TArray<UProceduralMeshComponent*> Surfaces;
private:
    UStaticMeshComponent* Prop(const TCHAR* Name,const TCHAR* Mesh,FVector Point,FVector Scale,UMaterialInterface* Material=nullptr,FRotator Rotation=FRotator::ZeroRotator,bool Solid=false);
    void BuildWater(bool Lodge);
    void BuildDock(FVector Shore,bool Lodge);
};

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "WildlineWeaponFX.generated.h"
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UDecalComponent;

UCLASS()
class WILDLINE_API UWildlineFXMesh : public UStaticMeshComponent {
    GENERATED_BODY()
public:
    // A 90 ms flash must render on the first shot, even while a PSO is warming.
    bool UsePSOPrecacheRenderProxyDelay() const override { return false; }
};

USTRUCT()
struct FWildlineImpact {
    GENERATED_BODY()
    UPROPERTY() FVector_NetQuantize Position;
    UPROPERTY() FVector_NetQuantizeNormal Normal;
    // Dirt, wood, stone, metal. Cosmetic only; damage stays server-owned.
    UPROPERTY() uint8 Surface=0;
};

USTRUCT()
struct FWildlineFxParticle {
    GENERATED_BODY()
    UPROPERTY() UStaticMeshComponent* Mesh=nullptr;
    UPROPERTY() UMaterialInstanceDynamic* Material=nullptr;
    FVector Velocity=FVector::ZeroVector;
    float Life=1,Size=10,Growth=0,Gravity=0,Opacity=1;
    bool bBillboard=true;
};

UCLASS()
class WILDLINE_API AWildlineWeaponFX : public AActor {
    GENERATED_BODY()
public:
    AWildlineWeaponFX();
    void Tick(float Delta) override;
    void Muzzle(FVector Direction,bool bShotgun);
    void Shell(FVector Right);
    void Impact(FVector Normal,uint8 Surface,bool bDebris);
    static FWildlineImpact DescribeImpact(const FHitResult& Hit);
    static void Warmup(UWorld* World);
    UPROPERTY() TArray<FWildlineFxParticle> Particles;
    UPROPERTY() UPointLightComponent* FlashLight=nullptr;
private:
    float Age=0;
    void Particle(const TCHAR* Material,FVector Offset,FVector Velocity,FLinearColor Tint,float Size,float Life,float Growth,float Opacity=1,float Gravity=0,bool Billboard=true);
};

UCLASS()
class WILDLINE_API AWildlineImpactMark : public AActor {
    GENERATED_BODY()
public:
    AWildlineImpactMark();
    void BeginPlay() override;
    void Tick(float Delta) override;
    UPROPERTY() UDecalComponent* Mark;
    UPROPERTY() UMaterialInstanceDynamic* Material=nullptr;
private:
    float Age=0;
};

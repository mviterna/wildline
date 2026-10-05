#include "WildlineWeaponFX.h"
#include "WildlineAudio.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

AWildlineWeaponFX::AWildlineWeaponFX(){
    PrimaryActorTick.bCanEverTick=true;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Weapon effect"));
    SetActorEnableCollision(false);SetLifeSpan(2.2f);
}
void AWildlineWeaponFX::Particle(const TCHAR* Name,FVector Offset,FVector Velocity,FLinearColor Tint,float Size,float Life,float Growth,float Opacity,float Gravity,bool Billboard){
    FWildlineFxParticle P;P.Mesh=NewObject<UWildlineFXMesh>(this);P.Mesh->SetupAttachment(RootComponent);P.Mesh->SetMobility(EComponentMobility::Movable);P.Mesh->SetForceDisableNanite(true);
    P.Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Billboard?TEXT("/Engine/BasicShapes/Plane.Plane"):TEXT("/Engine/BasicShapes/Cube.Cube")));
    auto* Base=LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Wildline/FX/%s.%s"),Name,Name));
    P.Material=UMaterialInstanceDynamic::Create(Base,this);if(!P.Material)return;
    P.Material->SetVectorParameterValue(TEXT("Tint"),Tint);P.Material->SetScalarParameterValue(TEXT("Opacity"),Opacity);P.Material->SetScalarParameterValue(TEXT("Seed"),FMath::FRand()*40);
    P.Mesh->SetMaterial(0,P.Material);P.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);P.Mesh->SetCastShadow(false);P.Mesh->bReceivesDecals=false;
    P.Mesh->SetRelativeLocation(Offset);P.Mesh->SetRelativeScale3D(Billboard?FVector(Size/100):FVector(Size/100,Size/180,Size/300));P.Mesh->RegisterComponent();
    P.Velocity=Velocity;P.Size=Size;P.Life=Life;P.Growth=Growth;P.Opacity=Opacity;P.Gravity=Gravity;P.bBillboard=Billboard;Particles.Add(P);
}
void AWildlineWeaponFX::Warmup(UWorld* World){
    static TWeakObjectPtr<UWorld> WarmWorld;if(!World||World->GetNetMode()==NM_DedicatedServer||WarmWorld==World)return;WarmWorld=World;
    if(auto* FX=World->SpawnActor<AWildlineWeaponFX>(FVector(0,0,-100000),FRotator::ZeroRotator)){
        for(const TCHAR* Name:{TEXT("M_Flash"),TEXT("M_Smoke"),TEXT("M_Spark"),TEXT("M_Chip")})FX->Particle(Name,FVector::ZeroVector,FVector::ZeroVector,FLinearColor::White,1,.2f,0,0,0,FString(Name)!=TEXT("M_Chip"));
        LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Wildline/FX/M_Impact.M_Impact"));
    }
}
void AWildlineWeaponFX::Muzzle(FVector Direction,bool Shotgun){
    const float Power=Shotgun?1.f:.6f;
    // Tiny luminous core and irregular flame; no persistent beam/tracer.
    Particle(TEXT("M_Flash"),FVector::ZeroVector,Direction*40,FLinearColor(4200,1300,130),17*Power,.09f,120,.9f);
    Particle(TEXT("M_Flash"),Direction*12,Direction*140,FLinearColor(2400,620,50),24*Power,.065f,150,.8f);
    for(int32 I=0;I<(Shotgun?7:4);I++){
        const FVector Drift=Direction*FMath::FRandRange(45.f,110.f)+FMath::VRand()*17+FVector(0,0,24);
        Particle(TEXT("M_Smoke"),Direction*(I*2.5f)+FVector(0,0,2),Drift,FLinearColor(.43f,.45f,.47f),FMath::FRandRange(10.f,17.f)*Power,FMath::FRandRange(.9f,1.7f),FMath::FRandRange(28.f,42.f),.42f);
    }
    FlashLight=NewObject<UPointLightComponent>(this);FlashLight->SetupAttachment(RootComponent);FlashLight->SetIntensity(Shotgun?24000:15000);FlashLight->SetAttenuationRadius(170);FlashLight->SetLightColor(FLinearColor(1,.46f,.12f));FlashLight->SetCastShadows(false);FlashLight->RegisterComponent();
}
void AWildlineWeaponFX::Shell(FVector Right){
    Particle(TEXT("M_Chip"),FVector::ZeroVector,Right*115+FVector(0,0,90),FLinearColor(.55f,.045f,.012f),3,1.8f,0,1,980,false);
    auto* Case=Particles.Last().Mesh;Case->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));Case->SetRelativeScale3D(FVector(.02f,.02f,.065f));
    auto* Cap=NewObject<UWildlineFXMesh>(this);Cap->SetupAttachment(Case);Cap->SetMobility(EComponentMobility::Movable);Cap->SetForceDisableNanite(true);Cap->SetStaticMesh(Case->GetStaticMesh());Cap->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Wildline/Farm/Materials/F_Metal.F_Metal")));Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);Cap->SetCastShadow(false);Cap->SetRelativeLocation(FVector(0,0,-44));Cap->SetRelativeScale3D(FVector(1.03f,1.03f,.15f));Cap->RegisterComponent();
}
FWildlineImpact AWildlineWeaponFX::DescribeImpact(const FHitResult& Hit){
    FWildlineImpact I;I.Position=Hit.ImpactPoint;I.Normal=Hit.ImpactNormal;
    const auto* C=Cast<UStaticMeshComponent>(Hit.GetComponent());
    const FString Name=C&&C->GetStaticMesh()?C->GetStaticMesh()->GetName():Hit.GetComponent()?Hit.GetComponent()->GetName():FString();
    if(Name.Contains(TEXT("Silo"))||Name.Contains(TEXT("Steel"))||Name.Contains(TEXT("Metal")))I.Surface=3;
    else if(Name.Contains(TEXT("Rock"))||Name.Contains(TEXT("Stone"))||Name.Contains(TEXT("Brick")))I.Surface=2;
    else if(Name.Contains(TEXT("Farmhouse"))||Name.Contains(TEXT("Barn"))||Name.Contains(TEXT("Fence"))||Name.Contains(TEXT("Crate"))||Name.Contains(TEXT("Log"))||Name.Contains(TEXT("Oak"))||Name.Contains(TEXT("Pine"))||Name.Contains(TEXT("Tree")))I.Surface=1;
    return I;
}
void AWildlineWeaponFX::Impact(FVector Normal,uint8 Surface,bool Debris){
    if(!Debris)return;WildlineAudio::Play(this,WildlineAudio::Hit,GetActorLocation());
    const FLinearColor Colors[]={FLinearColor(.31f,.23f,.13f),FLinearColor(.37f,.22f,.10f),FLinearColor(.42f,.42f,.40f),FLinearColor(.52f,.53f,.55f)};
    Surface=FMath::Min<uint8>(Surface,3);
    if(Surface!=3)for(int32 I=0;I<3;I++)Particle(TEXT("M_Smoke"),Normal*2,Normal*FMath::FRandRange(20.f,50.f)+FMath::VRand()*22+FVector(0,0,12),Colors[Surface],FMath::FRandRange(9.f,14.f),FMath::FRandRange(.6f,1.1f),25,.35f);
    for(int32 I=0;I<(Surface==3?6:5);I++)Particle(Surface==3?TEXT("M_Spark"):TEXT("M_Chip"),Normal*3,Normal*FMath::FRandRange(70.f,130.f)+FMath::VRand()*65,Surface==3?FLinearColor(18,7,.8f):Colors[Surface],Surface==1?FMath::FRandRange(.8f,2.f):.7f,Surface==3?.25f:.8f,0,1,650,false);
}
void AWildlineWeaponFX::Tick(float Delta){
    Super::Tick(Delta);Age+=Delta;if(FlashLight&&Age>.09f){FlashLight->DestroyComponent();FlashLight=nullptr;}
    const auto* Camera=UGameplayStatics::GetPlayerCameraManager(this,0);
    for(auto& P:Particles){
        if(Age>=P.Life){P.Mesh->SetVisibility(false,true);continue;}
        const float T=Age/P.Life;P.Velocity.Z-=P.Gravity*Delta;P.Mesh->AddWorldOffset(P.Velocity*Delta);
        if(P.bBillboard){if(Camera)P.Mesh->SetWorldRotation(FRotationMatrix::MakeFromZ(Camera->GetCameraLocation()-P.Mesh->GetComponentLocation()).Rotator());P.Mesh->SetWorldScale3D(FVector((P.Size+P.Growth*Age)/100));}
        else P.Mesh->AddLocalRotation(FRotator(Delta*250,Delta*170,0));
        P.Material->SetScalarParameterValue(TEXT("Opacity"),P.Opacity*FMath::Square(1-T));
    }
}

AWildlineImpactMark::AWildlineImpactMark(){
    PrimaryActorTick.bCanEverTick=true;Mark=CreateDefaultSubobject<UDecalComponent>(TEXT("Pellet mark"));RootComponent=Mark;
    Mark->DecalSize=FVector(3,3.2f,3.2f);Mark->SetFadeScreenSize(.00005f);SetActorEnableCollision(false);SetLifeSpan(25);
}
void AWildlineImpactMark::BeginPlay(){
    Super::BeginPlay();Material=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Wildline/FX/M_Impact.M_Impact")),this);Mark->SetDecalMaterial(Material);
    // Bound long-lived marks even when an entire co-op team fires continuously.
    int32 Count=0;AWildlineImpactMark* Oldest=nullptr;float OldestAge=-1;
    for(TActorIterator<AWildlineImpactMark> It(GetWorld());It;++It){Count++;if(It->Age>OldestAge){Oldest=*It;OldestAge=It->Age;}}
    if(Count>96&&Oldest&&Oldest!=this)Oldest->Destroy();
}
void AWildlineImpactMark::Tick(float Delta){Super::Tick(Delta);Age+=Delta;if(Material)Material->SetScalarParameterValue(TEXT("Opacity"),FMath::Clamp((25-Age)/5,0.f,1.f));}

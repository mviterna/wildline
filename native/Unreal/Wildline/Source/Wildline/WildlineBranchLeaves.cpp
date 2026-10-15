#include "WildlineCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"

AWildlineBranchLeaves::AWildlineBranchLeaves(){
    bReplicates=true;SetNetCullDistanceSquared(FMath::Square(12000.f));
    PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Branch disturbance"));
    SetReplicateMovement(true);
}
void AWildlineBranchLeaves::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AWildlineBranchLeaves,bSnow);
}
void AWildlineBranchLeaves::BeginPlay(){
    Super::BeginPlay();SetLifeSpan(3.2f);
    if(GetNetMode()==NM_DedicatedServer)return;
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,bSnow?TEXT("/Engine/BasicShapes/Sphere.Sphere"):TEXT("/Game/Wildline/Forest/Meshes/SM_FallenLeaf.SM_FallenLeaf"));
    if(!Mesh)return;
    const float Size=FMath::Max(1.f,static_cast<float>(Mesh->GetBoundingBox().GetSize().GetMax()));
    for(int32 I=0;I<12;I++){
        auto* Leaf=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(Leaf);Leaf->SetupAttachment(RootComponent);
        Leaf->SetMobility(EComponentMobility::Movable);Leaf->SetStaticMesh(Mesh);
        Leaf->SetCollisionEnabled(ECollisionEnabled::NoCollision);Leaf->SetCastShadow(false);
        Leaf->SetRelativeScale3D(bSnow?FVector(.02f,.02f,.004f):FVector((9.f+I%4*2.f)/Size));
        if(bSnow)Leaf->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Wildline/Forest/Materials/M_Snow.M_Snow")));
        Leaf->RegisterComponent();Leaves.Add(Leaf);
    }
    Tick(0);
}
void AWildlineBranchLeaves::Tick(float Delta){
    Super::Tick(Delta);Age+=Delta;
    for(int32 I=0;I<Leaves.Num();I++){
        const float Phase=I*2.399f;
        // A brief shake around the occupied limb becomes a fluttering fall.
        const float Fall=FMath::Max(0.f,Age-.45f-I*.025f);
        const float Radius=25.f+(I%4)*13.f+Fall*22.f;
        const FVector Offset(FMath::Cos(Phase)*Radius+FMath::Sin(Age*16+Phase)*9,
            FMath::Sin(Phase)*Radius+FMath::Cos(Age*13+Phase)*8,
            45.f+(I%3)*18.f+FMath::Sin(Age*18+Phase)*5-Fall*65-Fall*Fall*28);
        Leaves[I]->SetRelativeLocation(Offset);
        Leaves[I]->SetRelativeRotation(FRotator(FMath::Sin(Age*9+Phase)*65,I*47+Age*110,FMath::Cos(Age*7+Phase)*40));
        Leaves[I]->SetVisibility(Age<2.5f||I%3>0);
    }
}

#include "WildlineWorld.h"
#include "WildlineRules.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

void AWildlineWorld::GenerateFarm(){
    Generate();
    auto Mesh=[](const TCHAR* Name){return LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Wildline/Farm/Meshes/%s.%s"),Name,Name));};
    Farmhouse->SetStaticMesh(Mesh(TEXT("SM_Farmhouse")));Farmhouse->AddInstance(FTransform(FarmhouseLocation));
    const TCHAR* Names[]={TEXT("SM_Barn"),TEXT("SM_Silo"),TEXT("SM_HayBale"),TEXT("SM_Crate"),TEXT("SM_Fence"),TEXT("SM_Trough"),TEXT("SM_Corn"),TEXT("SM_Field")};
    for(int32 I=0;I<8;I++){FarmProps[I]->SetStaticMesh(Mesh(Names[I]));FarmProps[I]->SetCullDistances(0,I==6?9000:40000);}
    FarmProps[6]->SetCollisionEnabled(ECollisionEnabled::NoCollision);FarmProps[7]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto Add=[&](int32 I,float X,float Y,float Yaw=0.f,float S=1.f,float Lift=0.f){FarmProps[I]->AddInstance(FTransform(FRotator(0,Yaw,0),FVector(X,Y,GroundHeight(X,Y)+Lift),FVector(S)));};
    Add(0,3500,2550,180);Add(1,4400,1550,0,.7f);
    for(int32 I=0;I<10;I++)Add(2,2800+(I%4)*150,3450+(I/4)*95,0,1, I>=8?95:0);
    for(int32 I=0;I<5;I++)Add(3,1600+I*75,-1350,12);
    Add(5,-250,2600);Add(5,2100,-2550);
    // Fence gaps provide legible routes through both pasture and crop rows.
    for(float X=-2700;X<=3000;X+=220){if(FMath::Abs(X)<320)continue;Add(4,X,2700);Add(4,X,-2700);}
    for(float Y=-2450;Y<2600;Y+=220){if(FMath::Abs(Y)<350)continue;Add(4,-2700,Y,90);}
    FRandomStream Random(9324);
    for(int32 Side:{-1,1}){
        Add(7,-850,Side*1550);
        for(float X=-2350;X<660;X+=92)for(float Y=920;Y<2230;Y+=105)Add(6,X+Random.FRandRange(-10,10),Side*Y+Random.FRandRange(-9,9),Random.FRandRange(0,360),Random.FRandRange(.82f,1.08f));
    }
    // Short lawn, wild edges and an orchard connect the homestead to the forest.
    for(int32 I=0;I<14500;I++){
        const float X=Random.FRandRange(-5200,5000),Y=Random.FRandRange(-4700,4700);
        if((X>1080&&X<2950&&Y>-1490&&Y<730)||(X>2800&&X<4300&&Y>1950&&Y<3100)||InCropCover(FVector(X,Y,GroundHeight(X,Y)+10)))continue;
        if(FMath::Abs(Y)<220&&X>-4400&&X<2950)continue;
        ForestLayers[4+I%3]->AddInstance(FTransform(FRotator(0,Random.FRandRange(0,360),0),FVector(X,Y,GroundHeight(X,Y)-1),FVector(Random.FRandRange(.20f,.42f))));
    }
    Acorns->ClearInstances();AcornCaps->ClearInstances();AcornLocations.Empty();
    Acorns->SetStaticMesh(Mesh(TEXT("SM_Peanut")));Acorns->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Wildline/Farm/Materials/F_Peanut.F_Peanut")));
    // Floor-level pantry spills are reachable without jumping onto furniture.
    for(int32 I=0;I<30;I++){
        FVector P=FarmhouseLocation+FVector(160+(I%6)*37,-170+(I/6)*70,FarmFloor+.2f);
        AcornLocations.Add(P);Acorns->AddInstance(FTransform(FRotator(0,Random.FRandRange(0,360),0),P,FVector(1.5f)));
    }
    DenLocation=FVector(-4300,0,GroundHeight(-4300,0));DenMesh->SetRelativeLocation(DenLocation+FVector(0,0,26));DenMesh->SetRelativeRotation(FRotator(0,90,0));
}

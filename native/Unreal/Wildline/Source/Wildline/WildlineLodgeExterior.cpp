#include "WildlineLobby.h"
#include "WildlineWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void AWildlineLobby::BuildLodgeExterior(){
    const FVector O=Centre(false);auto* World=AWildlineWorld::Find(GetWorld());
    if(World&&World->bLake){ExteriorTheme=TEXT("TaylorsvilleLake");return;}
    const bool Winter=World?World->bWinter:GetWorld()->GetMapName().Contains(TEXT("Frostwood"));
    const bool Farm=World&&World->bFarm,Residential=World&&World->bResidential;
    ExteriorTheme=World&&World->bKnobs?TEXT("KnobsStateForest"):Winter?TEXT("Frostwood"):Farm?TEXT("Farmstead"):Residential?(GetWorld()->GetMapName().Contains(TEXT("Cedar"))?TEXT("NeighborhoodCedar"):TEXT("Neighborhood")):TEXT("Blackwood");
    auto Forest=[](const TCHAR* Name){return LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Wildline/Forest/Materials/%s.%s"),Name,Name));};
    auto* Ground=World&&World->Terrain?World->Terrain->GetMaterial(0):Forest(Winter?TEXT("M_Snow"):TEXT("M_LeafLitter"));
    LodgeScenery.Add(Piece(TEXT("Map ground outside lodge"),TEXT("Cube"),O+FVector(0,0,-55),FVector(28000,28000,80),Ground));
    for(int I=0;I<32;I++){
        const float A=I*2*PI/32;
        LodgeScenery.Add(Piece(TEXT("Map horizon beyond lodge"),TEXT("Sphere"),O+FVector(FMath::Cos(A)*9500,FMath::Sin(A)*9500,-1450),FVector(6500,6500,4200),Ground));
    }
    FRandomStream Random(68123);const TCHAR* Species[]={TEXT("SM_OakA"),TEXT("SM_OakB"),TEXT("SM_Pine"),TEXT("SM_OakC")};
    for(int S=0;S<4;S++){
        const FString Name=FString(Species[S])+(Winter?TEXT("_Bare"):TEXT("_Thin"));
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Wildline/Forest/Meshes/%s.%s"),*Name,*Name));if(!Mesh)continue;
        auto* C=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);AddInstanceComponent(C);C->SetupAttachment(RootComponent);C->SetMobility(EComponentMobility::Static);C->SetStaticMesh(Mesh);C->SetCollisionProfileName(TEXT("NoCollision"));C->SetCastShadow(false);
        if(Winter)for(int Slot=0;Slot<Mesh->GetStaticMaterials().Num();Slot++)if(auto* M=Mesh->GetMaterial(Slot);M&&(M->GetName()==TEXT("M_TreeBark")||M->GetName()==TEXT("M_FrostBark")))C->SetMaterial(Slot,Forest(TEXT("M_FrostBark")));
        C->RegisterComponent();LodgeScenery.Add(C);
        const int Count=Residential?10:Farm?16:36;
        for(int I=0;I<Count;I++){
            const float Angle=(I*4+S)*2*PI/(Count*4)+Random.FRandRange(-.02f,.02f);
            const float Radius=(Farm||Residential)?Random.FRandRange(7200,10000):(I<12?Random.FRandRange(2700,4200):Random.FRandRange(4300,8800));
            const float Scale=Random.FRandRange(.55f,1.10f),Collar=S==2?460.f:0.f;
            const float Height=Radius>6400?FMath::Square((Radius-6400)/3100)*400:-15;
            C->AddInstance(FTransform(FRotator(0,Random.FRandRange(0,360),0),O+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,Height-(Mesh->GetBoundingBox().Min.Z+Collar)*Scale),FVector(Scale)));
        }
    }
    // Reuse the selected map's actual art and placements. These decorative
    // copies have no collision or replication and only render from the lodge.
    if(World&&(Farm||Residential)&&!FParse::Param(FCommandLine::Get(),TEXT("WildlineNoLodgeMapProps"))){
        const FVector Offset=O-FVector(0,0,World->GroundHeight(0,0)+15);
        const FBox RoomBounds(FVector(-850,-850,-500),FVector(850,850,800));
        auto Allowed=[&](UStaticMesh* Mesh,const FTransform& T){const FBox B=Mesh->GetBoundingBox().TransformBy(T);return !B.Intersect(RoomBounds)&&T.GetLocation().SizeSquared2D()<FMath::Square(13000.f);};
        auto CopyMaterials=[](UStaticMeshComponent* Target,UStaticMeshComponent* Source){for(int I=0;I<Source->GetNumMaterials();I++)Target->SetMaterial(I,Source->GetMaterial(I));};
        if(Farm){TArray<UHierarchicalInstancedStaticMeshComponent*> Sources=World->FarmProps;Sources.Add(World->Farmhouse);
            for(auto* Source:Sources){if(!Source||!Source->GetStaticMesh())continue;UStaticMesh* Mesh=Source->GetStaticMesh();auto* C=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);AddInstanceComponent(C);C->SetupAttachment(RootComponent);C->SetMobility(EComponentMobility::Static);C->SetStaticMesh(Mesh);CopyMaterials(C,Source);C->SetCollisionProfileName(TEXT("NoCollision"));C->SetCastShadow(false);C->bAutoRebuildTreeOnInstanceChanges=false;
                for(int I=0;I<Source->GetInstanceCount();I++){FTransform T;Source->GetInstanceTransform(I,T,true);if(!Allowed(Mesh,T))continue;T.AddToTranslation(Offset);C->AddInstance(T);}
                if(C->GetInstanceCount()==0){C->DestroyComponent();continue;}
                // These distant window-view copies use conventional instancing.
                // The playable map keeps its original Nanite render components.
                C->SetForceDisableNanite(true);
                // Publish the finished instance buffer once, before the first
                // rendered frame, rather than resizing a registered Nanite HISM.
                C->BuildTreeIfOutdated(false,true);C->SetVisibility(false);C->RegisterComponent();C->bAutoRebuildTreeOnInstanceChanges=true;LodgeScenery.Add(C);
            }
        }else for(auto* Source:World->ResidentialProps){
            if(!Source||!Source->GetStaticMesh())continue;UStaticMesh* Mesh=Source->GetStaticMesh();FTransform T=Source->GetComponentTransform();if(!Allowed(Mesh,T))continue;T.AddToTranslation(Offset);
            auto* C=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(C);C->SetupAttachment(RootComponent);C->SetMobility(EComponentMobility::Static);C->SetStaticMesh(Mesh);CopyMaterials(C,Source);C->SetRelativeTransform(T);C->SetCollisionProfileName(TEXT("NoCollision"));C->SetCastShadow(false);C->RegisterComponent();LodgeScenery.Add(C);
        }
    }
    if(!Winter&&!Farm&&!Residential)for(float Side:{-1.f,1.f}){
        if(auto* C=Model(TEXT("SM_dead_tree_trunk"),O+FVector(-180,Side*1190,-15),FVector(280,55,45),FRotator(0,32,0)))LodgeScenery.Add(C);
        if(auto* C=Model(TEXT("SM_moss_01"),O+FVector(-170,Side*1180,18),FVector(140,70,12)))LodgeScenery.Add(C);
    }
    if(Winter)for(float Side:{-1.f,1.f})for(float X:{-405.f,455.f})LodgeScenery.Add(Piece(TEXT("Snow on outside window ledge"),TEXT("Cube"),O+FVector(X,Side*720,150),FVector(247,38,5),Forest(TEXT("M_Snow"))));
    for(auto* C:LodgeScenery)if(C)C->SetVisibility(false,true);
    UE_LOG(LogTemp,Display,TEXT("LODGE EXTERIOR %s: %d scenery components"),*ExteriorTheme,LodgeScenery.Num());
}

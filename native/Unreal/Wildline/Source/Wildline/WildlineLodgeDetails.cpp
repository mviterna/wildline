#include "WildlineLobby.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace {
UMaterialInterface* LodgeSurface(const TCHAR* Name){return LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Wildline/Lobby/Photoreal/Materials/%s.%s"),Name,Name));}
UMaterialInterface* ForestSurface(const TCHAR* Name){return LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Wildline/Forest/Materials/%s.%s"),Name,Name));}
}

void AWildlineLobby::BuildLodgeDetails(){
    const FVector O=Centre(false);
    for(float Side:{-1.f,1.f})for(float X:{-405.f,455.f}){
        Model(TEXT("SM_LodgeWindowDressings"),O+FVector(X,Side*687,155),FVector::ZeroVector,FRotator(0,Side>0?-90:90,0));
        Light(O+FVector(X,Side*615,260),FLinearColor(.69f,.83f,1),1200,510);
        Model(TEXT("SM_wooden_bowl_01"),O+FVector(X+65,Side*668,151),FVector(0,0,10));
    }
    Model(TEXT("SM_LodgeWindowDressings"),O+FVector(687,125,155),FVector::ZeroVector,FRotator(0,180,0));
    Light(O+FVector(600,125,260),FLinearColor(.69f,.83f,1),1200,510);
    Model(TEXT("SM_LodgeClock"),O+FVector(95,683,350),FVector::ZeroVector,FRotator(0,-90,0));
    Model(TEXT("SM_LodgePegRail"),O+FVector(-681,100,163));
    // A narrow display shelf adds useful objects below the specimen collection.
    Piece(TEXT("Trophy wall oak shelf"),TEXT("Cube"),O+FVector(-660,-245,122),FVector(49,390,7),LodgeSurface(TEXT("M_WindowOak")));
    for(float Y:{-390.f,-100.f})Piece(TEXT("Shelf brass bracket"),TEXT("Cube"),O+FVector(-673,Y,105),FVector(22,5,30),LodgeSurface(TEXT("M_LodgeDetailBrass")));
    Model(TEXT("SM_book_encyclopedia_set_01"),O+FVector(-655,-350,126),FVector(0,0,25),FRotator(0,90,0));
    Model(TEXT("SM_Lantern_01"),O+FVector(-653,-155,126),FVector(0,0,34));
    Model(TEXT("SM_brass_goblets"),O+FVector(-654,-250,126),FVector(0,0,17));
    Light(O+FVector(-620,-155,154),FLinearColor(1,.73f,.43f),450,260);
    // Noticeboard beside the racks: framed paper, a trail sketch and field notes.
    Piece(TEXT("Lodge field noticeboard"),TEXT("Cube"),O+FVector(-85,-683,322),FVector(142,9,83),LodgeSurface(TEXT("M_WindowOak")));
    auto* Paper=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Wildline/Lobby/M_Paper.M_Paper"));
    Piece(TEXT("Pinned field map"),TEXT("Cube"),O+FVector(-119,-677,324),FVector(66,1,64),Paper,FRotator(0,0,-3));
    Piece(TEXT("Pinned lodge notes"),TEXT("Cube"),O+FVector(-46,-677,325),FVector(49,1,61),Paper,FRotator(0,0,5));
    Sign(TEXT("BLACKWOOD\nTRAIL NOTES"),O+FVector(-46,-675,340),6,FRotator(0,90,0));
    Sign(TEXT("CHECK THE WIND\nPACK LIGHT\nLEAVE NO TRACE"),O+FVector(-46,-675,319),4.4f,FRotator(0,90,0));
    for(int32 I=0;I<9;I++){
        const float X=-143+I*6;
        Piece(TEXT("Sketch map trail"),TEXT("Cube"),O+FVector(X,-675,309+FMath::Sin(I*.85f)*14),FVector(8,.5f,1.2f),LodgeSurface(TEXT("M_ClockInk")),FRotator(0,0,FMath::Cos(I*.85f)*20));
    }
    for(float X:{-143.f,-69.f})Piece(TEXT("Brass map pin"),TEXT("Sphere"),O+FVector(X,-674,350),FVector(2.5f,1.5f,2.5f),LodgeSurface(TEXT("M_LodgeDetailBrass")));
    // The lobby is above the play maps. A shared woodland clearing around the
    // lodge supplies actual parallax through every window in all three modes.
    // Instanced trees keep the detailed source forest inexpensive to draw.
    LodgeScenery.Add(Piece(TEXT("Woodland clearing outside lodge"),TEXT("Cube"),O+FVector(0,0,-55),FVector(28000,28000,80),ForestSurface(TEXT("M_LeafLitter"))));
    // Rising wooded ground closes the horizon from the elevated waiting room.
    // The near clearing remains level with the sills, and has real parallax.
    for(int32 I=0;I<32;I++){
        const float A=I*2*PI/32;
        LodgeScenery.Add(Piece(TEXT("Wooded ridge beyond lodge"),TEXT("Sphere"),O+FVector(FMath::Cos(A)*9500,FMath::Sin(A)*9500,-1450),FVector(6500,6500,4200),ForestSurface(TEXT("M_LeafLitter"))));
    }
    FRandomStream Random(68123);
    const TCHAR* Trees[]={TEXT("SM_OakA_Thin"),TEXT("SM_OakB_Thin"),TEXT("SM_Pine_Thin"),TEXT("SM_OakC_Thin")};
    for(int32 Species=0;Species<4;Species++){
        const FString Path=FString::Printf(TEXT("/Game/Wildline/Forest/Meshes/%s.%s"),Trees[Species],Trees[Species]);
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);if(!Mesh)continue;
        auto* Instances=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
        AddInstanceComponent(Instances);Instances->SetupAttachment(RootComponent);Instances->SetMobility(EComponentMobility::Static);
        // Distant scenery retains its forest materials without spending virtual
        // shadow-map pages on foliage surrounding a sealed waiting room.
        Instances->SetStaticMesh(Mesh);Instances->SetCollisionProfileName(TEXT("NoCollision"));Instances->SetCastShadow(false);Instances->RegisterComponent();
        LodgeScenery.Add(Instances);
        for(int32 I=0;I<36;I++){
            const float Angle=(I*4+Species)*2*PI/144+Random.FRandRange(-.02f,.02f);
            const float Radius=I<12?Random.FRandRange(1250,2800):Random.FRandRange(3000,7800);
            const float Scale=Random.FRandRange(.55f,1.10f);
            const float Collar=Species==2?460.f:0.f;
            const float Ground=Radius>6400?FMath::Square((Radius-6400)/3100)*400:-15;
            Instances->AddInstance(FTransform(FRotator(0,Random.FRandRange(0,360),0),O+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,Ground-(Mesh->GetBoundingBox().Min.Z+Collar)*Scale),FVector(Scale)));
        }
    }
    // Nearby deadwood and ferns give the sills a foreground beyond distant trees.
    for(float Side:{-1.f,1.f}){
        if(auto* C=Model(TEXT("SM_dead_tree_trunk"),O+FVector(-180,Side*1190,-15),FVector(280,55,45),FRotator(0,32,0)))LodgeScenery.Add(C);
        if(auto* C=Model(TEXT("SM_moss_01"),O+FVector(-170,Side*1180,18),FVector(140,70,12)))LodgeScenery.Add(C);
    }
    for(auto* C:LodgeScenery)if(C)C->SetVisibility(false,true);
}

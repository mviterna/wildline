#include "WildlineLake.h"
#include "WildlineWorld.h"
#include "WildlineLobby.h"
#include "WildlineRules.h"
#include "WildlineCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Camera/PlayerCameraManager.h"

namespace {
UMaterialInterface* LakeMat(const TCHAR* Name){return LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Wildline/Lake/Materials/%s.%s"),Name,Name));}
const TCHAR* Cube=TEXT("/Engine/BasicShapes/Cube.Cube");
const TCHAR* Cylinder=TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
}
AWildlineLake::AWildlineLake(){PrimaryActorTick.bCanEverTick=true;SetNetAddressable();RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Lakeshore"));RootComponent->SetNetAddressable();}
AWildlineLake* AWildlineLake::Find(UWorld* W){if(!W)return nullptr;for(TActorIterator<AWildlineLake> It(W);It;++It)return *It;return nullptr;}
FVector AWildlineLake::LodgeCentre(){return AWildlineLobby::Centre(false)+FVector(10100,125,-90);}
float AWildlineLake::ShoreShape(float A,bool Lodge){return Lodge?1.f:1.f+.055f*FMath::Sin(A*3)+.035f*FMath::Sin(A*5+.7f);}
FVector AWildlineLake::ShorePoint(float A,float Q,bool Lodge){const auto E=Extent(Lodge);return (Lodge?LodgeCentre():HuntingCentre())+FVector(FMath::Cos(A)*E.X,FMath::Sin(A)*E.Y,0)*Q*ShoreShape(A,Lodge);}
float AWildlineLake::RadiusAt(FVector P,bool Lodge){const FVector D=P-(Lodge?LodgeCentre():HuntingCentre());const auto E=Extent(Lodge);const FVector2D Q(D.X/E.X,D.Y/E.Y);return Q.Size()/ShoreShape(FMath::Atan2(Q.Y,Q.X),Lodge);}
float AWildlineLake::BedHeight(float X,float Y){
    const float R=RadiusAt(FVector(X,Y,0));
    if(R<=1)return HuntingCentre().Z-1800*(1-R*R);
    const float Shore=HuntingCentre().Z+FMath::SmoothStep(1.f,1.13f,R)*190;
    return FMath::Lerp(Shore,WildlineRules::Ground(X,Y),FMath::SmoothStep(1.24f,1.6f,R));
}
FVector AWildlineLake::TrailPoint(float A){FVector P=ShorePoint(A,1.15f);P.Z=BedHeight(P.X,P.Y)+3;return P;}
bool AWildlineLake::WaterAt(UWorld* W,FVector P,float& Surface){
    const auto* F=AWildlineWorld::Find(W);if(!F||!F->bLake)return false;
    const bool Lodge=P.Z>2500;const FVector C=Lodge?LodgeCentre():HuntingCentre();Surface=C.Z;
    return RadiusAt(P,Lodge)<1&&P.Z<Surface+250&&P.Z>Surface-2100;
}
bool AWildlineLake::InLodgeGrounds(FVector P){const FVector D=P-AWildlineLobby::Centre(false);return D.X>-950&&D.X<25000&&FMath::Abs(D.Y)<11500&&D.Z>-2100&&D.Z<2500;}
FVector AWildlineLake::FishingSpot(int32 I){return AWildlineLobby::Centre(false)+FVector(2330,(I%4)*140-85,105);}
int32 AWildlineLake::FishingSpotAt(FVector P){int32 Best=INDEX_NONE;float Distance=155;for(int32 I=0;I<4;I++){const float D=FVector::Dist2D(P,FishingSpot(I));if(D<Distance&&FMath::Abs(P.Z-FishingSpot(I).Z)<65){Best=I;Distance=D;}}return Best;}
FVector AWildlineLake::RouteAroundLake(FVector From,FVector Target){
    // Follow the nearest shore arc if the straight route would cross water.
    bool Crosses=false;for(int32 I=1;I<20;I++)if(RadiusAt(FMath::Lerp(From,Target,I/20.f))<1.1f){Crosses=true;break;}
    if(!Crosses)return Target;
    const FVector C=HuntingCentre(),D=From-C,E=Target-C;
    const auto Size=Extent();const float A=FMath::Atan2(D.Y/Size.Y,D.X/Size.X),B=FMath::Atan2(E.Y/Size.Y,E.X/Size.X);
    const float Arc=FMath::FindDeltaAngleRadians(A,B);
    if(FMath::Abs(Arc)<.12f)return Target;
    return TrailPoint(A+FMath::Clamp(Arc,-.16f,.16f));
}
UStaticMeshComponent* AWildlineLake::Prop(const TCHAR* Name,const TCHAR* Mesh,FVector P,FVector Scale,UMaterialInterface* M,FRotator R,bool Solid){
    auto* C=NewObject<UStaticMeshComponent>(this,FName(*FString::Printf(TEXT("%s_%d"),Name,Props.Num())));AddInstanceComponent(C);C->SetNetAddressable();C->SetupAttachment(RootComponent);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Mesh));C->SetRelativeLocation(P);C->SetRelativeScale3D(Scale);C->SetRelativeRotation(R);if(M)C->SetMaterial(0,M);C->SetCollisionProfileName(Solid?TEXT("BlockAll"):TEXT("NoCollision"));C->SetCastShadow(Solid);C->RegisterComponent();Props.Add(C);return C;
}
void AWildlineLake::BuildWater(bool Lodge){
    const FVector Centre=Lodge?LodgeCentre():HuntingCentre();
    auto Surface=[&](bool Bed){
        const FName Name=Lodge?(Bed?TEXT("LodgeLakeBed"):TEXT("LodgeLakeSurface")):(Bed?TEXT("HuntingLakeBed"):TEXT("HuntingLakeSurface"));
        auto* C=NewObject<UProceduralMeshComponent>(this,Name);AddInstanceComponent(C);C->SetNetAddressable();C->SetupAttachment(RootComponent);C->RegisterComponent();Surfaces.Add(C);
        TArray<FVector> V,N;TArray<int32> T;TArray<FVector2D> UV;TArray<FLinearColor> Colors;TArray<FProcMeshTangent> Tangents;
        constexpr int32 Rings=64,Slices=192;
        for(int32 R=0;R<=Rings;R++)for(int32 S=0;S<=Slices;S++){
            const float A=S*2*PI/Slices,Q=static_cast<float>(R)/Rings*(Bed&&Lodge?1.7f:1.f);FVector P=ShorePoint(A,Q,Lodge);
            if(Bed){if(!Lodge)P.Z=BedHeight(P.X,P.Y)+2;else{const float Depth=Q<1?-1050*(1-Q*Q):FMath::SmoothStep(1.f,1.08f,Q)*90;P.Z+=Depth;}}
            V.Add(P);UV.Add(FVector2D(P.X/300,P.Y/300));N.Add(FVector::UpVector);Colors.Add(FLinearColor::White);Tangents.Add(FProcMeshTangent(1,0,0));
            if(R<Rings&&S<Slices){const int32 I=R*(Slices+1)+S;T.Append({I,I+1,I+Slices+1,I+1,I+Slices+2,I+Slices+1});}
        }
        C->CreateMeshSection_LinearColor(0,V,T,N,UV,Colors,Tangents,Bed&&Lodge);C->SetMaterial(0,LakeMat(Bed?TEXT("M_LakeBed"):TEXT("M_LakeWater")));C->SetCollisionEnabled(Bed&&Lodge?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);C->SetCastShadow(false);
    };
    Surface(false);Surface(true);
    FRandomStream Random(Lodge?2056:1985);
    // Submerged rock ledges and branching deadwood remain visible when diving.
    for(int32 I=0;I<(Lodge?70:180);I++){
        const float A=Random.FRandRange(0,2*PI),Q=Random.FRandRange(.2f,.96f);FVector P=ShorePoint(A,Q,Lodge);P.Z=Lodge?Centre.Z-1050*(1-Q*Q):BedHeight(P.X,P.Y);
        const float S=Random.FRandRange(.5f,2.1f);Prop(TEXT("Submerged limestone"),TEXT("/Game/Wildline/Forest/Meshes/SM_Stone.SM_Stone"),P,FVector(S,S*.7f,S*.5f),nullptr,FRotator(0,A*180/PI,0),true);
        if(I%4==0)Prop(TEXT("Submerged fallen limb"),TEXT("/Game/Wildline/Forest/Meshes/SM_Log.SM_Log"),P+FVector(0,0,28),FVector(1.3f),nullptr,FRotator(0,A*180/PI,0),true);
    }
    for(int32 I=0;I<FishCount(Lodge);I++){
        const FVector P=Centre+FVector(100+I*12,0,-150);auto* F=Prop(TEXT("Largemouth bass"),TEXT("/Game/Wildline/Lake/Meshes/SM_PhotorealBass.SM_PhotorealBass"),P,FVector(Random.FRandRange(.6f,1.25f)));F->SetCullDistance(6500);Fish.Add(F);
    }
    // Native shoreline grasses use the existing textured fern asset.
    for(int32 I=0;I<300;I++){
        const float A=Random.FRandRange(0,2*PI),Q=Random.FRandRange(1.005f,1.065f);FVector P=ShorePoint(A,Q,Lodge);P.Z=Lodge?Centre.Z+FMath::SmoothStep(1.f,1.08f,Q)*90:BedHeight(P.X,P.Y);
        Prop(TEXT("Shoreline fern"),TEXT("/Game/Wildline/Forest/Meshes/SM_Fern2.SM_Fern2"),P,FVector(Random.FRandRange(.7f,1.4f)),nullptr,FRotator(0,I*71,0));
    }
    if(Lodge)for(int32 I=0;I<100;I++){
        const float A=I*2*PI/100,Q=Random.FRandRange(1.18f,1.58f);FVector P=ShorePoint(A,Q,Lodge);P.Z+=90;
        const FVector D=P-AWildlineLobby::Centre(false);if(FMath::Abs(D.Y)<1000&&D.X<2800)continue;
        const float Scale=Random.FRandRange(.65f,1.1f);auto* Tree=Prop(TEXT("Lodge shoreline oak"),TEXT("/Game/Wildline/Forest/Meshes/SM_OakA_Thin.SM_OakA_Thin"),P,FVector(Scale),nullptr,FRotator(0,I*79,0));
        if(Tree->GetStaticMesh())Tree->AddLocalOffset(FVector(0,0,-Tree->GetStaticMesh()->GetBoundingBox().Min.Z*Scale));
    }
}
void AWildlineLake::BuildDock(FVector Shore,bool Lodge){
    auto* Wood=LakeMat(TEXT("M_DockWood"));auto* Metal=LakeMat(TEXT("M_LakeMetal"));
    for(int32 I=0;I<74;I++)Prop(TEXT("Dock board"),Cube,Shore+FVector(I*24,0,-6),FVector(.225f,4.9f,.12f),Wood,FRotator::ZeroRotator,true);
    const FVector End=Shore+FVector(1700,0,0);
    for(int32 I=0;I<30;I++)Prop(TEXT("Fishing platform board"),Cube,End+FVector(0,(I-14.5f)*24,-6),FVector(5,.225f,.12f),Wood,FRotator::ZeroRotator,true);
    for(int32 I=0;I<7;I++)for(float Side:{-1.f,1.f}){
        FVector P=Shore+FVector(50+I*260,Side*210,0);const float Q=RadiusAt(P,true);
        const float Bed=Lodge?LodgeCentre().Z+(Q<1?-1050*(1-Q*Q):90):BedHeight(P.X,P.Y);
        const float Bottom=Bed-40,Top=Shore.Z+100;P.Z=(Bottom+Top)*.5f;
        Prop(TEXT("Dock piling"),Cylinder,P,FVector(.2f,.2f,(Top-Bottom)/100),Wood,FRotator::ZeroRotator,true);
        if(I<5)Prop(TEXT("Dock rope rail"),Cube,Shore+FVector(180+I*260,Side*210,85),FVector(2.6f,.035f,.035f),Wood);
    }
    for(float Side:{-1.f,1.f}){
        const FVector P=End+FVector(-420,Side*430,Lodge?-115:-190);
        Prop(TEXT("Moored fishing boat"),TEXT("/Game/Wildline/Lake/Meshes/SM_FishingBoat.SM_FishingBoat"),P,FVector(1),nullptr,FRotator(0,Side*5,0),true);
        Prop(TEXT("Mooring cleat"),Cube,End+FVector(-350,Side*260,8),FVector(.4f,.1f,.1f),Metal);
        Prop(TEXT("Mooring rope"),Cube,End+FVector(-350,Side*330,-15),FVector(.025f,1.4f,.025f),Wood,FRotator(12,0,0));
    }
    for(float Side:{-1.f,1.f})Prop(TEXT("Rod rack support"),Cube,End+FVector(-95,Side*270,28),FVector(.12f,.12f,.56f),Wood);
    Prop(TEXT("Rod rack rest"),Cube,End+FVector(-95,0,58),FVector(.1f,6.4f,.08f),Wood);
    for(int32 I=0;I<4;I++){
        const FVector P=End+FVector(-30,(I-1.5f)*140,0);
        Prop(TEXT("Fishing rod rack"),TEXT("/Game/Wildline/Lake/Meshes/SM_FishingRod.SM_FishingRod"),P+FVector(-65,0,65),FVector(1),nullptr,FRotator(65,0,0));
        Prop(TEXT("Bait bucket"),TEXT("/Game/Wildline/Lobby/Photoreal/Meshes/SM_wooden_bucket_01.SM_wooden_bucket_01"),P+FVector(-120,-40,0),FVector(.45f));
        Prop(TEXT("Tackle box"),Cube,P+FVector(-130,35,12),FVector(.4f,.28f,.24f),LakeMat(TEXT("M_BoatGreen")));
    }
    if(Lodge){
        auto* Sign=NewObject<UTextRenderComponent>(this);AddInstanceComponent(Sign);Sign->SetupAttachment(RootComponent);Sign->SetRelativeLocation(End+FVector(-160,0,170));Sign->SetRelativeRotation(FRotator(0,180,0));Sign->SetWorldSize(15);Sign->SetHorizontalAlignment(EHTA_Center);Sign->SetText(FText::FromString(TEXT("TAYLORSVILLE LAKE\nLODGE FISHING DOCK\nCATCH & RELEASE")));Sign->RegisterComponent();
    }
}
void AWildlineLake::BeginPlay(){
    Super::BeginPlay();BuildWater(false);BuildWater(true);
    FVector MainDock=ShorePoint(PI)+FVector(-200,0,0);MainDock.Z=260;
    BuildDock(MainDock,false);BuildDock(AWildlineLobby::Centre(false)+FVector(700,125,0),true);
    auto* Wood=LakeMat(TEXT("M_DockWood"));
    const FVector RampStart=TrailPoint(PI),RampEnd=MainDock+FVector(50,0,0),RampDirection=RampEnd-RampStart;
    Prop(TEXT("Shore boardwalk ramp"),Cube,(RampStart+RampEnd)*.5f-FVector(0,0,8),FVector(RampDirection.Size()/100,4.9f,.16f),Wood,RampDirection.Rotation(),true);
    // Continuous veranda meets the lodge doorway and the dock at floor height.
    Prop(TEXT("Lodge veranda"),Cube,AWildlineLobby::Centre(false)+FVector(820,125,-12),FVector(3.2f,7.5f,.24f),Wood,FRotator::ZeroRotator,true);
    const FVector Lodge=AWildlineLobby::Centre(false);auto* RoofMaterial=LakeMat(TEXT("M_BoatGreen"));
    for(float Side:{-1.f,1.f})Prop(TEXT("Lake lodge pitched roof"),Cube,Lodge+FVector(0,Side*370,600),FVector(15.5f,8.2f,.16f),RoofMaterial,FRotator(0,0,-Side*18),true);
    Prop(TEXT("Lake lodge ridge cap"),Cube,Lodge+FVector(0,0,730),FVector(16,.30f,.18f),RoofMaterial);
    auto* Gables=NewObject<UProceduralMeshComponent>(this);AddInstanceComponent(Gables);Gables->SetupAttachment(RootComponent);Gables->RegisterComponent();Surfaces.Add(Gables);
    TArray<FVector> GableVertices,GableNormals;TArray<int32> GableTriangles;TArray<FVector2D> GableUV;TArray<FLinearColor> GableColors;TArray<FProcMeshTangent> GableTangents;
    for(float Side:{-1.f,1.f}){const int32 J=GableVertices.Num();for(const FVector P:{FVector(Side*710,-715,490),FVector(Side*710,715,490),FVector(Side*710,0,725)}){GableVertices.Add(Lodge+P);GableNormals.Add(FVector(Side,0,0));GableUV.Add(FVector2D(P.Y/180,P.Z/180));GableColors.Add(FLinearColor::White);GableTangents.Add(FProcMeshTangent(0,1,0));}if(Side>0)GableTriangles.Append({J,J+2,J+1});else GableTriangles.Append({J,J+1,J+2});}
    Gables->CreateMeshSection_LinearColor(0,GableVertices,GableTriangles,GableNormals,GableUV,GableColors,GableTangents,false);Gables->SetMaterial(0,Wood);Gables->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto* F=AWildlineWorld::Find(GetWorld());
    // A continuous, terrain-conforming loop leaves room for both teams to pass.
    auto* Trail=NewObject<UProceduralMeshComponent>(this);AddInstanceComponent(Trail);Trail->SetupAttachment(RootComponent);Trail->RegisterComponent();Surfaces.Add(Trail);
    TArray<FVector> V,N;TArray<int32> T;TArray<FVector2D> UV;TArray<FLinearColor> C;TArray<FProcMeshTangent> Tangents;
    for(int32 I=0;I<=256;I++)for(float Side:{-1.f,1.f}){const float A=I*2*PI/256;FVector P=TrailPoint(A)+FVector(FMath::Cos(A),FMath::Sin(A),0)*Side*125;P.Z=F->GroundHeight(P.X,P.Y)+4;V.Add(P);N.Add(FVector::UpVector);UV.Add(FVector2D(I*.5f,Side));C.Add(FLinearColor::White);Tangents.Add(FProcMeshTangent(1,0,0));if(I<256&&Side<0){const int32 J=I*2;T.Append({J,J+2,J+1,J+1,J+2,J+3});}}
    Trail->CreateMeshSection_LinearColor(0,V,T,N,UV,C,Tangents,false);Trail->SetMaterial(0,LakeMat(TEXT("M_LakeTrail")));Trail->SetCollisionEnabled(ECollisionEnabled::NoCollision);Trail->SetCastShadow(false);
    UE_LOG(LogTemp,Display,TEXT("TAYLORSVILLE LAKE READY: %d fish, %d props, shore loop and four fishing stations"),Fish.Num(),Props.Num());
}
void AWildlineLake::Tick(float Delta){
    Super::Tick(Delta);const float Time=GetWorld()->GetTimeSeconds();
    for(int32 I=0;I<Fish.Num();I++){
        const bool Lodge=I>=FishCount();const float A=Time*(.006f+(I%5)*.001f)+I*2.39996f,Q=.60f+(I%7)*.048f;
        const float Depth=FMath::Min((Lodge?1050.f:1800.f)*(1-Q*Q)-55,100+(I%4)*60-FMath::Sin(Time*.8f+I)*12);
        const FVector P=ShorePoint(A,Q,Lodge)-FVector(0,0,Depth);
        const FVector Ahead=ShorePoint(A+.001f,Q,Lodge);
        Fish[I]->SetRelativeLocation(P);Fish[I]->SetRelativeRotation(FRotator(0,(Ahead-P).Rotation().Yaw,FMath::Sin(Time*4+I)*2));
    }
    auto* PC=GetWorld()->GetFirstPlayerController();const bool LodgeVisible=PC&&PC->PlayerCameraManager&&InLodgeGrounds(PC->PlayerCameraManager->GetCameraLocation());
    for(auto* P:Props)if(P->GetRelativeLocation().Z>2500)P->SetVisibility(LodgeVisible);
    for(auto* Rod:Props)if(Rod->GetName().Contains(TEXT("Fishing rod rack"))&&Rod->GetRelativeLocation().Z>2500){
        bool Occupied=false;for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(It->FishingStage&&It->ServerTime()-It->FishingEquippedAt>.35f&&FVector::Dist2D(It->FishingAnchor,Rod->GetComponentLocation())<100){Occupied=true;break;}
        Rod->SetVisibility(LodgeVisible&&!Occupied);
    }
    for(auto* P:Surfaces)if(P->Bounds.Origin.Z>2500)P->SetVisibility(LodgeVisible);
    TArray<UTextRenderComponent*> Signs;GetComponents(Signs);for(auto* Sign:Signs)if(Sign->GetComponentLocation().Z>2500)Sign->SetVisibility(LodgeVisible);
}

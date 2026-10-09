#include "WildlineWorld.h"
#include "WildlineRules.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "WildlineNeighborhoodConfig.h"

FVector AWildlineWorld::CourseTarget(int32 Lane,int32 Checkpoint)const{
    if(Checkpoint<=0)return NeighborhoodConfig&&NeighborhoodConfig->Routes.IsValidIndex(Lane)?NeighborhoodConfig->Routes[Lane].Start:CourseStart+FVector(0,Lane==1?170:-170,0);
    if(Checkpoint>=5)return FeederLocation;
    const int32 Index=FMath::Clamp(Lane,0,1)*4+Checkpoint-1;
    return CoursePlatforms.IsValidIndex(Index)?CoursePlatforms[Index]:CourseStart;
}
bool AWildlineWorld::OnCourseTarget(FVector P,int32 Lane,int32 Checkpoint)const{
    const FVector T=CourseTarget(Lane,Checkpoint);
    const float Radius=Checkpoint==0?95.f:Checkpoint==5?65.f:70.f;
    return FVector::Dist2D(P,T)<Radius&&FMath::Abs(P.Z-T.Z-13.f)<9.f;
}

void AWildlineWorld::GenerateResidential(){
    if(GenerateConfiguredNeighborhood())return;
    // Keep the wooded skyline, while clearing the playable neighborhood of trees,
    // rocks and undergrowth that could bypass the final feeder jump.
    RootComponent->SetMobility(EComponentMobility::Static);Generate();Terrain->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Wildline/Residential/M_Lawn.M_Lawn")));Acorns->ClearInstances();AcornCaps->ClearInstances();AcornLocations.Empty();DenMesh->SetVisibility(false);
    auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto* Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    auto* Cone=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cone.Cone"));
    auto Mat=[](const TCHAR* N){return LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Wildline/Farm/Materials/%s.%s"),N,N));};
    auto Add=[&](UStaticMesh* Mesh,FVector P,FVector Size,const TCHAR* Material,FRotator R=FRotator::ZeroRotator,bool Solid=true){
        auto* C=NewObject<UStaticMeshComponent>(this,*FString::Printf(TEXT("Neighborhood_%04d"),ResidentialProps.Num()));
        AddInstanceComponent(C);C->SetupAttachment(RootComponent);C->SetStaticMesh(Mesh);C->SetMaterial(0,Mat(Material));
        C->SetRelativeLocation(P);C->SetRelativeRotation(R);C->SetRelativeScale3D(Size/100.f);
        C->SetCollisionProfileName(Solid?TEXT("BlockAll"):TEXT("NoCollision"));C->SetMobility(EComponentMobility::Static);C->RegisterComponent();ResidentialProps.Add(C);return C;
    };
    auto Box=[&](FVector P,FVector S,const TCHAR* M,FRotator R=FRotator::ZeroRotator,bool Solid=true){return Add(Cube,P,S,M,R,Solid);};
    auto Beam=[&](FVector A,FVector B,float Width,const TCHAR* M,bool Solid=true){return Add(Cylinder,(A+B)*.5f,FVector(Width,Width,(B-A).Size()),M,(B-A).Rotation()+FRotator(90,0,0),Solid);};
    // Quiet residential street, curb and sidewalks behind six individually fenced yards.
    Box(FVector(0,-4300,213),FVector(11000,1100,6),TEXT("F_Steel"));
    for(int32 Side:{-1,1}){
        Box(FVector(0,-4300+Side*560,220),FVector(11000,25,20),TEXT("F_Trim"));
        Box(FVector(0,-4300+Side*700,218),FVector(11000,250,16),TEXT("F_Trim"));
    }
    for(int32 I=-9;I<=9;I++)Box(FVector(I*550,-4300,217),FVector(220,10,1),TEXT("F_Trim"),FRotator::ZeroRotator,false);
    for(int32 Row=0;Row<2;Row++)for(int32 Lot=-1;Lot<=1;Lot++){
        const float X=Lot*3000,Y=Row==0?-2550:3000;
        const TCHAR* Siding=Lot==0?TEXT("F_Trim"):Lot==1?TEXT("F_Brick"):TEXT("F_Wood");
        Box(FVector(X,Y,420),FVector(1600,950,420),Siding);
        for(int32 Side:{-1,1})Box(FVector(X,Y+Side*230,802),FVector(1700,600,35),TEXT("F_Roof"),FRotator(0,0,Side*35));
        // Close each roof end with a textured triangular gable, preserving a
        // solid residential silhouette instead of an open gap above the walls.
        for(int32 End:{-1,1}){
            auto* G=NewObject<UProceduralMeshComponent>(this,*FString::Printf(TEXT("HouseGable_%02d"),ResidentialGables.Num()));AddInstanceComponent(G);G->SetupAttachment(RootComponent);G->SetMobility(EComponentMobility::Static);G->RegisterComponent();ResidentialGables.Add(G);
            const float GX=X+End*800;TArray<FVector> V={FVector(GX,Y-475,630),FVector(GX,Y+475,630),FVector(GX,Y,963)};
            TArray<int32> Indices={0,1,2,2,1,0};TArray<FVector> Normals={FVector(End,0,0),FVector(End,0,0),FVector(End,0,0)};TArray<FVector2D> UV={FVector2D(0,0),FVector2D(4,0),FVector2D(2,2)};TArray<FLinearColor> Colors={FLinearColor::White,FLinearColor::White,FLinearColor::White};TArray<FProcMeshTangent> Tangents={FProcMeshTangent(0,1,0),FProcMeshTangent(0,1,0),FProcMeshTangent(0,1,0)};
            G->CreateMeshSection_LinearColor(0,V,Indices,Normals,UV,Colors,Tangents,true);G->SetMaterial(0,Mat(Siding));G->SetCollisionProfileName(TEXT("BlockAll"));
        }
        // Gutters and siding courses keep the houses readable across the yards.
        for(int32 Side:{-1,1})Box(FVector(X,Y+Side*478,632),FVector(1690,12,14),TEXT("F_Trim"));
        if(Lot!=1)for(int32 Board=0;Board<12;Board++)for(int32 Side:{-1,1})Box(FVector(X,Y+Side*476,230+Board*33),FVector(1590,3,3),TEXT("F_Wood"),FRotator::ZeroRotator,false);
        Box(FVector(X+500,Y,870),FVector(95,95,230),TEXT("F_Brick"));
        const float Face=Row==0?Y+480:Y-480;
        Box(FVector(X,Face,328),FVector(160,12,230),TEXT("F_Wood"));
        for(int32 Side:{-1,1}){
            Box(FVector(X+Side*450,Face,465),FVector(255,15,170),TEXT("F_Trim"));
            Box(FVector(X+Side*450,Face+(Row==0?9:-9),465),FVector(220,4,140),TEXT("F_Water"),FRotator::ZeroRotator,false);
            Box(FVector(X+Side*450,Face+(Row==0?12:-12),465),FVector(8,6,140),TEXT("F_Trim"),FRotator::ZeroRotator,false);
        }
        Box(FVector(X,Face+(Row==0?150:-150),230),FVector(380,280,40),TEXT("F_Trim"));
        Box(FVector(X+10-150,Y-280,215),FVector(340,1800,10),TEXT("F_Trim"));
        // Fence gaps give defenders and squirrels access to the shared central yard.
        for(int32 I=-6;I<=6;I++){
            const float FX=X+I*200;
            if(FMath::Abs(FX)<1500&&Row==0)continue;
            Box(FVector(FX,Row==0?-1200:1800,280),FVector(170,12,140),TEXT("F_Wood"));
            Box(FVector(FX+95,Row==0?-1200:1800,292),FVector(12,16,170),TEXT("F_Trim"));
        }
    }
    // Power lines hang above the street, outside the course and climbing system.
    for(int32 I=-2;I<=2;I++){
        const FVector Pole(I*2300,-3450,210);
        Beam(Pole,Pole+FVector(0,0,1050),28,TEXT("F_Wood"));
        Box(Pole+FVector(0,0,985),FVector(240,24,24),TEXT("F_Wood"));
        if(I<2)for(int32 Wire=-1;Wire<=1;Wire++)for(int32 S=0;S<12;S++){
            auto Point=[&](float T){return Pole+FVector(T*2300,Wire*85,1010-100*FMath::Sin(T*PI));};
            Beam(Point(S/12.f),Point((S+1)/12.f),3,TEXT("F_Steel"),false);
        }
    }
    // Lawn clumps: short enough to read from a squirrel's eye height.
    FRandomStream R(417);
    for(int32 I=0;I<18000;I++){
        const float X=R.FRandRange(-4700,4700),Y=R.FRandRange(-1000,1700);
        if(X>-1300&&X<1300&&FMath::Abs(Y)<650)continue;
        ForestLayers[4+I%3]->AddInstance(FTransform(FRotator(0,R.FRandRange(0,360),0),FVector(X,Y,210),FVector(.15f)));
    }
    // Two mirrored routes: bin -> garden bench -> fence ledge -> pergola.
    // Each step rises 40-50 cm, within the squirrel's 55 cm jump apex.
    CourseStart=FVector(-950,0,210);FeederLocation=FVector(350,0,450);DenLocation=CourseStart;
    for(int32 Lane=0;Lane<2;Lane++){
        const float Y=Lane==0?-170.f:170.f;
        Box(CourseStart+FVector(0,Y,1),FVector(190,190,2),Lane==0?TEXT("F_Water"):TEXT("F_Hay"),FRotator::ZeroRotator,false);
        const FVector Targets[]={FVector(-650,Y,250),FVector(-400,Y,300),FVector(-150,Y,350),FVector(100,Y,400)};
        for(const FVector& T:Targets)CoursePlatforms.Add(T);
        Add(Cylinder,FVector(-650,Y,230),FVector(130,130,40),TEXT("F_Metal"));
        Box(FVector(-650,Y,247),FVector(150,150,6),TEXT("F_Steel"));
        Box(FVector(-400,Y,295),FVector(170,160,10),TEXT("F_Wood"));
        for(int32 Side:{-1,1})Box(FVector(-400+Side*62,Y,250),FVector(12,145,80),TEXT("F_Wood"));
        Box(FVector(-150,Y,345),FVector(160,160,10),TEXT("F_Wood"));
        for(int32 Side:{-1,1})Box(FVector(-150,Y+Side*64,275),FVector(12,12,130),TEXT("F_Trim"));
        Box(FVector(100,Y,395),FVector(190,180,10),TEXT("F_Wood"));
        for(int32 A:{-1,1})for(int32 B:{-1,1})Box(FVector(100+A*75,Y+B*70,300),FVector(12,12,180),TEXT("F_Wood"));
        // Colored plaques identify the next checkpoint without painting the whole prop.
        for(int32 Step=0;Step<4;Step++)Box(Targets[Step]+FVector(-78,0,-12),FVector(3,55,18),Lane==0?TEXT("F_Water"):TEXT("F_Hay"),FRotator::ZeroRotator,false);
    }
    // Shared 2.4 m feeder: the final diagonal jump is about 1.3 m edge to edge.
    // The smooth metal pole is deliberately absent from Trees/NearestTree.
    Beam(FVector(350,0,210),FVector(350,0,446),14,TEXT("F_Steel"));
    Add(Cone,FVector(350,0,340),FVector(120,120,70),TEXT("F_Metal"),FRotator(180,0,0));
    Box(FVector(350,0,444),FVector(150,150,12),TEXT("F_Wood"));
    for(int32 A:{-1,1})for(int32 B:{-1,1})Box(FVector(350+A*60,B*60,490),FVector(7,7,80),TEXT("F_Wood"));
    for(int32 Side:{-1,1})Box(FVector(350+Side*35,0,545),FVector(95,180,10),TEXT("F_Roof"),FRotator(Side*30,0,0));
    for(int32 I=0;I<18;I++)Add(Cylinder,FVector(320+I%6*12,-20+I/6*15,452),FVector(3,5,2),TEXT("F_Peanut"),FRotator::ZeroRotator,false);
    // Yard furniture away from the final approach; nothing can serve as a feeder shortcut.
    for(float X:{-2400.f,2600.f}){
        Box(FVector(X,1000,288),FVector(280,160,16),TEXT("F_Wood"));
        for(int32 Side:{-1,1})Box(FVector(X+Side*95,1000,250),FVector(15,140,80),TEXT("F_Wood"));
        Add(Cylinder,FVector(X+280,1000,240),FVector(90,90,60),TEXT("F_Brick"));
    }
}

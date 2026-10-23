#include "WildlineWorld.h"
#include "WildlineKnobs.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"

void AWildlineWorld::GenerateKnobsScenery(){
    using namespace WildlineKnobs;
    auto Material=[](const TCHAR* Name){return LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Wildline/Knobs/Materials/%s.%s"),Name,Name));};
    auto* Dirt=Material(TEXT("M_KnobsDirt"));auto* Gravel=Material(TEXT("M_KnobsGravel"));auto* MeadowMat=Material(TEXT("M_KnobsMeadow"));
    auto* Wood=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Wildline/Lake/Materials/M_DockWood.M_DockWood"));
    auto* Dark=Material(TEXT("M_KnobsSign"));auto* Red=Material(TEXT("M_KnobsRed"));
    const FColor Colors[]={FColor(55,145,240),FColor(245,137,32),FColor(248,213,65)};
    // Densely sampled ribbons follow the same surface as pawn collision, without
    // adding a raised collision lip at trail edges or creek crossings.
    int32 Section=1;
    auto Ribbon=[&](const TArray<FVector2D>& Points,float HalfWidth,UMaterialInterface* Mat,float Lift,bool Taper=false){
        TArray<FVector> V,N;TArray<int32> Tris;TArray<FVector2D> UV;TArray<FLinearColor> C;TArray<FProcMeshTangent> T;
        for(int32 I=0;I<Points.Num();I++){
            const auto Direction=(Points[FMath::Min(I+1,Points.Num()-1)]-Points[FMath::Max(0,I-1)]).GetSafeNormal();
            const FVector2D Side(-Direction.Y,Direction.X);
            for(float Sign:{-1.f,1.f}){const auto P=Points[I]+Side*HalfWidth*Sign*(Taper?FMath::Sin(PI*I/(Points.Num()-1)):1);V.Add(FVector(P.X,P.Y,GroundHeight(P.X,P.Y)+Lift));N.Add(FVector::UpVector);UV.Add(FVector2D(Sign>0?1:0,I*.4f));C.Add(FLinearColor::White);T.Add(FProcMeshTangent(1,0,0));}
            if(I){const int32 A=(I-1)*2;Tris.Append({A,A+1,A+2,A+1,A+3,A+2});}
        }
        Terrain->CreateMeshSection_LinearColor(Section,V,Tris,N,UV,C,T,false);Terrain->SetMaterial(Section++,Mat);
    };
    for(int32 R=0;R<3;R++)Ribbon(Trail(R),80,Dirt,3);
    TArray<FVector2D> Creek,Road;
    for(float X=-13200;X<=12200;X+=80){Creek.Add(FVector2D(X,CreekY(X)));Road.Add(FVector2D(X,RoadY(X)));}
    Ribbon(Creek,135,Gravel,4);Ribbon(Road,145,Gravel,4);
    TArray<FVector2D> Turnaround;for(int32 I=0;I<=48;I++){const float A=I*2*PI/48;Turnaround.Add(FVector2D(11900+FMath::Cos(A)*270,RoadY(11900)+FMath::Sin(A)*270));}Ribbon(Turnaround,145,Gravel,4);
    // Short disconnected shallows leave visible dry gravel between them, as in
    // the forest's intermittent stream. They never activate lake mechanics.
    for(int32 Pool=0;Pool<5;Pool++){TArray<FVector2D> Water;for(float X=-10300+Pool*4300;X<-9500+Pool*4300;X+=40)Water.Add(FVector2D(X,CreekY(X)));Ribbon(Water,60,Material(TEXT("M_KnobsWater")),8,true);}
    auto Piece=[&](const TCHAR* Name,FVector P,FVector Size,UMaterialInterface* Mat,FRotator Rotation=FRotator::ZeroRotator){
        auto* C=NewObject<UStaticMeshComponent>(this,MakeUniqueObjectName(this,UStaticMeshComponent::StaticClass(),FName(Name)));AddInstanceComponent(C);C->SetupAttachment(RootComponent);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));C->SetMaterial(0,Mat);C->SetRelativeLocation(P);C->SetRelativeRotation(Rotation);C->SetRelativeScale3D(Size/100);C->SetCollisionProfileName(TEXT("BlockAll"));C->RegisterComponent();ResidentialProps.Add(C);return C;
    };
    auto Label=[&](const FString& Text,FVector P,float Size,FColor Color){
        auto* C=NewObject<UTextRenderComponent>(this);AddInstanceComponent(C);C->SetupAttachment(RootComponent);C->SetText(FText::FromString(Text));C->SetWorldSize(Size);C->SetMaterial(0,Material(TEXT("M_KnobsLetters")));C->SetTextRenderColor(Color);C->SetHorizontalAlignment(EHTA_Center);C->SetVerticalAlignment(EVRTA_TextCenter);C->SetRelativeLocation(P);C->SetRelativeRotation(FRotator(0,-90,0));C->RegisterComponent();KnobsLabels.Add(C);
    };
    auto Sign=[&](const FString& Text,FVector2D XY,FColor Color,float Width=240){
        const float Z=GroundHeight(XY.X,XY.Y);
        for(float Side:{-1.f,1.f})Piece(TEXT("Knobs signpost"),FVector(XY.X+Side*(Width*.5f-15),XY.Y,Z+108),FVector(12,12,216),Wood);
        Piece(TEXT("Knobs trail sign"),FVector(XY.X,XY.Y,Z+184),FVector(Width,12,76),Dark);
        Label(Text,FVector(XY.X,XY.Y-7,Z+184),17,Color);
    };
    // Meadow floor is a soft-edged ecological clearing, with tall grass rather
    // than buildings or recreational lake facilities.
    for(int32 Field=0;Field<2;Field++){
        const float CX=Field?2800:-4400,CY=-6000,RX=Field?1700:2000,RY=Field?900:1150;
        for(float Y=CY-RY;Y<CY+RY;Y+=100){const float W=RX*FMath::Sqrt(FMath::Max(0.f,1-FMath::Square((Y-CY)/RY)));TArray<FVector2D> Row;for(float X=CX-W;X<=CX+W;X+=70)Row.Add(FVector2D(X,Y));if(Row.Num()>1)Ribbon(Row,51,MeadowMat,2);}
    }
    for(int32 I=0;I<3;I++){
        const auto P=Parking(I);for(float Y=-350;Y<=350;Y+=70){TArray<FVector2D> Row;for(float X=-600;X<=600;X+=60)Row.Add(P+FVector2D(X,Y));Ribbon(Row,36,Gravel,5);}
        Sign(I==2?TEXT("KY 733\nSOUTH TRAILHEAD"):I==1?TEXT("CROOKED CREEK\nEAST TRAILHEAD"):TEXT("KNOBS STATE FOREST\nCROOKED CREEK"),P+FVector2D(0,400),FColor(240,224,178),350);
        for(int32 J=0;J<5;J++){const FVector2D B=P+FVector2D(-480+J*240,340);Piece(TEXT("Parking timber stop"),FVector(B.X,B.Y,GroundHeight(B.X,B.Y)+12),FVector(150,20,22),Wood);}
    }
    const auto K=Parking(0)+FVector2D(-430,410);const float KZ=GroundHeight(K.X,K.Y);
    for(float Side:{-1.f,1.f})Piece(TEXT("Information kiosk post"),FVector(K.X+Side*140,K.Y,KZ+155),FVector(18,18,310),Wood);
    Piece(TEXT("Information kiosk board"),FVector(K.X,K.Y,KZ+205),FVector(310,18,180),Dark);
    for(float Side:{-1.f,1.f})Piece(TEXT("Kiosk pitched roof"),FVector(K.X+Side*90,K.Y,KZ+320),FVector(200,150,12),Wood,FRotator(0,0,Side*18));
    Label(TEXT("KNOBS STATE FOREST\nTRAIL GUIDE\nBLUE  /  CREEK HOLLOW\nORANGE  /  HAY RIDGE\nYELLOW  /  SOUTH WOODS\nFOOT TRAFFIC ONLY"),FVector(K.X,K.Y-11,KZ+208),16,FColor(235,226,188));
    for(int32 R=0;R<3;R++){
        const auto& Path=Trail(R);for(int32 I=0;I<Path.Num()-1;I+=4){const auto P=Path[I]+FVector2D(125,0);const float Z=GroundHeight(P.X,P.Y);Piece(TEXT("Trail blaze stake"),FVector(P.X,P.Y,Z+65),FVector(10,10,130),Wood);auto* Blaze=Piece(TEXT("Colored trail blaze"),FVector(P.X,P.Y-6,Z+109),FVector(12,3,25),Material(R==0?TEXT("M_KnobsBlue"):R==1?TEXT("M_KnobsOrange"):TEXT("M_KnobsYellow")));Blaze->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
        Sign(R==0?TEXT("BLUE LOOP\nCREEK HOLLOW"):R==1?TEXT("ORANGE LOOP\nHAY RIDGE"):TEXT("YELLOW LOOP\nSOUTH WOODS"),Path[4]+FVector2D(170,170),Colors[R]);
    }
    Sign(TEXT("HAY RIDGE\nOAK - HICKORY WOODLAND"),FVector2D(4300,2400),FColor(240,215,165),360);
    // Red boundary blazes follow the game reserve edge; avoid an invented park
    // fence across the forest, while retaining the game's finite play area.
    for(int32 I=0;I<64;I++){const float A=I*2*PI/64;const float X=FMath::Cos(A)*14350,Y=FMath::Sin(A)*14350,Z=GroundHeight(X,Y);Piece(TEXT("State forest boundary marker"),FVector(X,Y,Z+95),FVector(11,11,190),Wood);Piece(TEXT("Red boundary blaze"),FVector(X,Y-7,Z+158),FVector(15,3,30),Red);}
    FRandomStream Random(850733);
    for(int32 I=0;I<300;I++){const float X=Random.FRandRange(-12500,12000),Y=CreekY(X)+Random.FRandRange(-125,125);Rocks->AddInstance(FTransform(FRotator(0,Random.FRandRange(0,360),0),FVector(X,Y,GroundHeight(X,Y)+2),FVector(Random.FRandRange(.035f,.10f))));}
}

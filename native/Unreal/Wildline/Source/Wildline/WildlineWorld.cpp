#include "WildlineWorld.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "WildlineLobby.h"
#include "WildlineLake.h"
#include "WildlineRules.h"
#include "WildlineUserSettings.h"
#include "WildlineTrunkProfiles.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

AWildlineWorld::AWildlineWorld() {
    bReplicates=true; bAlwaysRelevant=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Terrain=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Forest floor"));Terrain->SetupAttachment(RootComponent);Terrain->bUseAsyncCooking=true;
    Trunks=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Trunks"));Trunks->SetupAttachment(RootComponent);
    Foliage=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Needles"));Foliage->SetupAttachment(RootComponent);Foliage->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Branches=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Branches"));Branches->SetupAttachment(RootComponent);Branches->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Rocks=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Rocks"));Rocks->SetupAttachment(RootComponent);Rocks->SetCollisionProfileName(TEXT("BlockAll"));Rocks->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Grass=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Ground cover"));Grass->SetupAttachment(RootComponent);Grass->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Acorns=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Forage markers"));Acorns->SetupAttachment(RootComponent);Acorns->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Trunks->SetStaticMesh(Cylinder.Object);Branches->SetStaticMesh(Cylinder.Object);Foliage->SetStaticMesh(Plane.Object);Grass->SetStaticMesh(Plane.Object);Rocks->SetStaticMesh(Sphere.Object);
    Acorns->SetStaticMesh(Sphere.Object);
    AcornCaps=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Acorn caps"));AcornCaps->SetupAttachment(RootComponent);AcornCaps->SetStaticMesh(Sphere.Object);AcornCaps->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ReserveFence=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Reserve split rail boundary"));ReserveFence->SetupAttachment(RootComponent);
    ReserveFence->SetStaticMesh(Cylinder.Object);ReserveFence->SetCollisionProfileName(TEXT("BlockAll"));
    Trunks->SetCollisionProfileName(TEXT("BlockAll"));
    Trunks->SetCullDistances(0,26000);Foliage->SetCullDistances(16000,23000);Branches->SetCullDistances(12000,18000);Grass->SetCullDistances(2600,5000);
    // Separate species retain shared geometry and clustered instance culling.
    for(int32 I=0;I<19;I++){
        auto* Layer=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(*FString::Printf(TEXT("Woodland species %02d"),I));
        Layer->SetupAttachment(RootComponent);Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Layer->SetCullDistances(I<4?22000:4800,I<4?30000:6500);ForestLayers.Add(Layer);
    }
    for(int32 I=0;I<4;I++){
        auto* Wood=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(*FString::Printf(TEXT("Climbable wood %d"),I));
        Wood->SetupAttachment(RootComponent);Wood->SetVisibility(false);Wood->SetCastShadow(false);
        Wood->SetCollisionProfileName(TEXT("BlockAll"));ClimbWood.Add(Wood);
    }
    Trunks->SetVisibility(false);Trunks->SetCastShadow(false);
    Foliage->SetVisibility(false);Branches->SetVisibility(false);Grass->SetVisibility(false);
    Farmhouse=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Farmhouse"));Farmhouse->SetupAttachment(RootComponent);Farmhouse->SetCollisionProfileName(TEXT("BlockAll"));
    for(int32 I=0;I<8;I++){auto* Layer=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(*FString::Printf(TEXT("Farm prop %02d"),I));Layer->SetupAttachment(RootComponent);Layer->SetCollisionProfileName(TEXT("BlockAll"));FarmProps.Add(Layer);}
    DenMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hollow den trunk"));DenMesh->SetupAttachment(RootComponent);DenMesh->SetCollisionProfileName(TEXT("BlockAll"));
}
AWildlineWorld* AWildlineWorld::Find(UWorld* W){for(TActorIterator<AWildlineWorld> It(W);It;++It)return *It;return nullptr;}
FVector AWildlineWorld::TrunkCentre(int32 Index,float WorldZ,float& Radius) const {
    const auto& T=Trees[Index];if(T.bStraightTrunk){Radius=T.Radius;return FVector(T.Base.X,T.Base.Y,WorldZ);}const auto& Profile=WildlineTrunkProfile(T.Species);
    const float LocalZ=(WorldZ-T.MeshTransform.GetLocation().Z)/T.MeshTransform.GetScale3D().Z;
    FVector4 Ring=Profile[0];
    for(int32 I=1;I<Profile.Num();I++){
        const auto A=Profile[I-1],B=Profile[I];
        Ring=FMath::Lerp(A,B,FMath::Clamp((LocalZ-A.Z)/(B.Z-A.Z),0.f,1.f));
        if(LocalZ<=B.Z)break;
    }
    Radius=Ring.W*T.MeshTransform.GetScale3D().X;
    FVector Centre=T.MeshTransform.TransformPosition(FVector(Ring.X,Ring.Y,Ring.Z));Centre.Z=WorldZ;return Centre;
}
int32 AWildlineWorld::NearestTree(FVector P,float Max) const {int32 Result=INDEX_NONE;for(int32 I=0;I<Trees.Num();I++){float Radius;const auto Centre=TrunkCentre(I,P.Z,Radius);const float D=FVector::Dist2D(P,Centre)-Radius;if(D<Max&&P.Z<Trees[I].ClimbTop){Max=D;Result=I;}}return Result;}
void AWildlineWorld::BeginPlay(){Super::BeginPlay();float ReviewYaw;if(Trees.Num()==0||(bResidential&&FParse::Value(FCommandLine::Get(),TEXT("WildlineCourseOakYaw="),ReviewYaw)))Rebuild();bool WoodReady=ClimbWood.Num()==4;for(auto* Wood:ClimbWood)WoodReady&=Wood&&Wood->GetStaticMesh()&&Wood->GetInstanceCount()>0;Trunks->SetCollisionEnabled(WoodReady?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryAndPhysics);if(!AWildlineLobby::Find(GetWorld()))GetWorld()->SpawnActor<AWildlineLobby>();if(bLake&&!AWildlineLake::Find(GetWorld())){FActorSpawnParameters LakeSpawn;LakeSpawn.Name=TEXT("TaylorsvilleLakeScenery");GetWorld()->SpawnActor<AWildlineLake>(LakeSpawn);}if(auto* S=UWildlineUserSettings::Get())ApplyTreeLeafDensity(bWinter?S->WinterTreeLeafDensity:S->TreeLeafDensity);}
void AWildlineWorld::ApplyTreeLeafDensity(float Density){
    Density=FMath::Clamp(Density,0.f,100.f);
    // Visual meshes only: wood collision, climb profiles and instance placement
    // stay fixed. Each connected leaf card carries a stable rank in vertex G.
    for(TActorIterator<AActor> It(GetWorld());It;++It){
        TArray<UStaticMeshComponent*> Components;It->GetComponents(Components);
        for(auto* C:Components){
            if(!C->GetStaticMesh())continue;const FString Name=C->GetStaticMesh()->GetName();
            for(const TCHAR* Base:{TEXT("SM_OakA"),TEXT("SM_OakB"),TEXT("SM_OakC"),TEXT("SM_Pine")}){
                if(Name!=Base&&Name!=FString(Base)+TEXT("_Thin")&&Name!=FString(Base)+TEXT("_Bare")&&Name!=FString(Base)+TEXT("_Density"))continue;
                const FString Target=FString(Base)+(Density<=0?TEXT("_Bare"):TEXT("_Density"));
                if(Name!=Target){
                    const FString Path=FString::Printf(TEXT("/Game/Wildline/Forest/Meshes/%s.%s"),*Target,*Target);
                    if(auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Path)){
                        // Yard props and lodge scenery are registered as static.
                        // Temporarily allow the visual mesh change, then restore
                        // their authored mobility and lighting behavior.
                        const auto Mobility=C->Mobility;C->SetMobility(EComponentMobility::Movable);
                        C->EmptyOverrideMaterials();C->SetStaticMesh(Mesh);C->SetMobility(Mobility);
                    }
                }
                for(int32 Slot=0;Slot<C->GetNumMaterials();Slot++){
                    auto* Material=C->GetMaterial(Slot);if(!Material)continue;
                    auto* Source=C->GetStaticMesh()->GetMaterial(Slot);const FString MaterialName=Source?Source->GetName():Material->GetName();
                    if(MaterialName.Contains(TEXT("M_Leaves"))||MaterialName.Contains(TEXT("M_LivingLeaf"))||MaterialName.Contains(TEXT("M_Needles"))){
                        auto* MID=Cast<UMaterialInstanceDynamic>(Material);if(!MID)MID=C->CreateDynamicMaterialInstance(Slot);
                        if(MID)MID->SetScalarParameterValue(TEXT("LeafDensity"),Density*.01f);
                    }else if(MaterialName==TEXT("M_TreeBark")||MaterialName==TEXT("M_FrostBark"))C->SetMaterial(Slot,LoadObject<UMaterialInterface>(nullptr,bWinter?TEXT("/Game/Wildline/Forest/Materials/M_FrostBark.M_FrostBark"):TEXT("/Game/Wildline/Forest/Materials/M_TreeBark.M_TreeBark")));
                }
                break;
            }
        }
    }
}
void AWildlineWorld::Rebuild(){for(auto* Wood:ClimbWood)Wood->ClearInstances();Trunks->ClearInstances();Foliage->ClearInstances();Branches->ClearInstances();Rocks->ClearInstances();Grass->ClearInstances();Acorns->ClearInstances();AcornCaps->ClearInstances();ReserveFence->ClearInstances();for(auto* Layer:ForestLayers)Layer->ClearInstances();Terrain->ClearAllMeshSections();Trees.Empty();AcornLocations.Empty();Farmhouse->ClearInstances();for(auto* Prop:FarmProps)Prop->ClearInstances();for(auto* Prop:ResidentialProps)if(Prop)Prop->DestroyComponent();ResidentialProps.Empty();for(auto* Gable:ResidentialGables)if(Gable)Gable->DestroyComponent();ResidentialGables.Empty();CoursePlatforms.Empty();DenMesh->SetVisibility(true);if(bResidential)GenerateResidential();else if(bFarm)GenerateFarm();else Generate();}
void AWildlineWorld::ShowLocalAcorns(const TArray<int32>& Collected){
    for(int32 I=0;I<AcornLocations.Num();I++){
        FTransform T;Acorns->GetInstanceTransform(I,T);
        const FVector Scale=Collected.Contains(I)?FVector::ZeroVector:(bFarm?FVector(1.5f):FVector(.065f,.065f,.085f));
        if(!T.GetScale3D().Equals(Scale)){T.SetScale3D(Scale);Acorns->UpdateInstanceTransform(I,T,false,true);}
        FTransform Cap;if(AcornCaps->GetInstanceTransform(I,Cap)){const FVector CapScale=Collected.Contains(I)?FVector::ZeroVector:FVector(.075f,.075f,.03f);if(!Cap.GetScale3D().Equals(CapScale)){Cap.SetScale3D(CapScale);AcornCaps->UpdateInstanceTransform(I,Cap,false,true);}}
    }
}
void AWildlineWorld::Generate(){
    using namespace WildlineRules;
    auto Mat=[](const TCHAR* Name){return LoadObject<UMaterialInterface>(nullptr,Name);};
    Terrain->SetMaterial(0,Mat(bWinter?TEXT("/Game/Wildline/Forest/Materials/M_Snow.M_Snow"):TEXT("/Game/Wildline/Forest/Materials/M_LeafLitter.M_LeafLitter")));
    Trunks->SetVisibility(false);Foliage->SetVisibility(false);Branches->SetVisibility(false);Grass->SetVisibility(false);
    const TCHAR* MeshNames[]={TEXT("SM_OakA"),TEXT("SM_OakB"),TEXT("SM_OakC"),TEXT("SM_Pine"),TEXT("SM_GrassA"),TEXT("SM_GrassB"),TEXT("SM_GrassC"),TEXT("SM_DryGrass"),TEXT("SM_Clover"),TEXT("SM_CloverWide"),TEXT("SM_Weed"),TEXT("SM_WeedB"),TEXT("SM_Fern1"),TEXT("SM_Fern2"),TEXT("SM_Fern3"),TEXT("SM_Fern4"),TEXT("SM_FallenLeaf"),TEXT("SM_OakA"),TEXT("SM_Log")};
    for(int32 I=0;I<ForestLayers.Num();I++){
        const FString Path=FString::Printf(TEXT("/Game/Wildline/Forest/Meshes/%s.%s"),MeshNames[I],MeshNames[I]);
        ForestLayers[I]->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Path));
        if(I<4||I==17){
            const FString Seasonal=FString::Printf(TEXT("/Game/Wildline/Forest/Meshes/%s_%s.%s_%s"),MeshNames[I],bWinter?TEXT("Bare"):TEXT("Thin"),MeshNames[I],bWinter?TEXT("Bare"):TEXT("Thin"));
            if(auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Seasonal))ForestLayers[I]->SetStaticMesh(Mesh);
        }
        if(I<4){
            const FString Wood=FString::Printf(TEXT("/Game/Wildline/Forest/Meshes/%s_Wood.%s_Wood"),MeshNames[I],MeshNames[I]);
            ClimbWood[I]->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Wood));
        }
        ForestLayers[I]->SetVisibility(true);
        if(I==18)ForestLayers[I]->SetCollisionProfileName(TEXT("BlockAll"));
        ForestLayers[I]->bAutoRebuildTreeOnInstanceChanges=false;
        if(I>=4)ForestLayers[I]->SetCastShadow(I>=10&&I!=16);
    }
    Rocks->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Wildline/Forest/Meshes/SM_Stone.SM_Stone")));
    Rocks->SetMaterial(0,Mat(bWinter?TEXT("/Game/Wildline/Forest/Materials/M_FrostStone.M_FrostStone"):TEXT("/Game/Wildline/Forest/Materials/M_Stone.M_Stone")));
    // Serialized reserve actors also need the repaired component collision settings.
    Rocks->SetCollisionProfileName(TEXT("BlockAll"));Rocks->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ReserveFence->SetMaterial(0,Mat(TEXT("/Game/Wildline/Forest/Materials/M_TreeBark.M_TreeBark")));
    constexpr int32 N=420;constexpr float Extent=22000.f;const float Step=Extent*2/N;
    TArray<float> GridX,GridY;
    for(int32 I=0;I<=N;I++){GridX.Add(-Extent+I*Step);GridY.Add(-Extent+I*Step);}
    // Exact basin edges keep lawn triangles out of recessed pools. The normal
    // terrain grid is too coarse to follow a pool's narrow concrete coping.
    if(bResidential)for(const FBox& Basin:ResidentialPoolBasins){
        for(float X:{float(Basin.Min.X),float(Basin.Min.X+35),float(Basin.Max.X-35),float(Basin.Max.X)})GridX.AddUnique(X);
        for(float Y:{float(Basin.Min.Y),float(Basin.Min.Y+35),float(Basin.Max.Y-35),float(Basin.Max.Y)})GridY.AddUnique(Y);
    }
    GridX.Sort();GridY.Sort();const int32 Width=GridX.Num();
    TArray<FVector> Vertices,Normals;TArray<int32> Indices;TArray<FVector2D> UV;TArray<FLinearColor> Colors;TArray<FProcMeshTangent> Tangents;
    for(int32 Y=0;Y<GridY.Num();Y++)for(int32 X=0;X<Width;X++){
        const float PX=GridX[X],PY=GridY[Y];
        Vertices.Add(FVector(PX,PY,GroundHeight(PX,PY)));UV.Add(FVector2D(PX/220.f,PY/220.f));
        Normals.Add(FVector(GroundHeight(PX-10,PY)-GroundHeight(PX+10,PY),GroundHeight(PX,PY-10)-GroundHeight(PX,PY+10),20).GetSafeNormal());
        Colors.Add(FLinearColor::White);Tangents.Add(FProcMeshTangent(1,0,0));
        if(X<Width-1&&Y<GridY.Num()-1){const int32 A=Y*Width+X;Indices.Append({A,A+Width,A+1,A+1,A+Width,A+Width+1});}
    }
    Terrain->CreateMeshSection_LinearColor(0,Vertices,Indices,Normals,UV,Colors,Tangents,true);Terrain->SetCollisionProfileName(TEXT("BlockAll"));
    FRandomStream R(84672);
    auto AddTree=[&](float X,float Y){if(bLake&&AWildlineLake::RadiusAt(FVector(X,Y,0))<1.26f)return;FWildlineTree T;T.Base=FVector(X,Y,GroundHeight(X,Y));T.Radius=R.FRandRange(38,67);T.Height=R.FRandRange(1850,3100);Trees.Add(T);};
    if(bResidential){}else if(bLake){AddTree(-11900,200);AddTree(-11650,1350);AddTree(-11300,-1200);AddTree(-12400,-800);AddTree(-11200,-2900);}else if(bFarm){AddTree(-4800,750);AddTree(-4500,-800);AddTree(-3800,1400);AddTree(-3600,-1500);AddTree(-5500,0);}else{AddTree(600,200);AddTree(-650,350);AddTree(-1100,-1000);AddTree(1250,-800);AddTree(100,-2100);}
    for(int32 I=0;I<340;I++){const float X=R.FRandRange(-14500,14500),Y=R.FRandRange(-14500,14500);if(FMath::Sqrt(X*X+Y*Y)>14200||FMath::Sqrt(X*X+Y*Y)<1800)continue;if((bFarm||bResidential)&&FMath::Sqrt(X*X+Y*Y)<5200)continue;bool Near=false;for(const auto& T:Trees)if(FVector::Dist2D(FVector(X,Y,0),T.Base)<650){Near=true;break;}if(!Near)AddTree(X,Y);}
    for(int32 I=0;I<Trees.Num();I++){
        auto& T=Trees[I];const int32 Species=I<5?I%3:R.RandRange(0,3);T.Species=Species;
        const float NativeRadius[]={71.07f,58.23f,56.85f,35.f};
        const float XY=T.Radius/NativeRadius[Species];
        // Bury the root collar below the lowest ground under its footprint.
        float RootGround=T.Base.Z;
        for(int32 J=0;J<8;J++){const float A=J*PI/4;RootGround=FMath::Min(RootGround,GroundHeight(T.Base.X+FMath::Cos(A)*T.Radius*1.6f,T.Base.Y+FMath::Sin(A)*T.Radius*1.6f));}
        const float ZScale=T.Height/2400.f;
        const FBox Bounds=ForestLayers[Species]->GetStaticMesh()->GetBoundingBox();
        const float Collar=Species==3?460.f:0.f;
        const FVector Planted(T.Base.X,T.Base.Y,RootGround-8.f-(Bounds.Min.Z+Collar)*ZScale);
        T.MeshTransform=FTransform(FRotator(0,R.FRandRange(0,360),0),Planted,FVector(XY,XY,ZScale));
        ForestLayers[Species]->AddInstance(T.MeshTransform);AddClimbWood(I);
        const auto& Profile=WildlineTrunkProfile(Species);T.ClimbTop=Planted.Z+Profile.Last().Z*ZScale;
        // Segmented collision follows the actual curved/tapered stem.
        const float Bottom=T.Base.Z-15;
        for(float Z=Bottom;Z<T.ClimbTop;Z+=80){
            const float EndZ=FMath::Min(Z+80,T.ClimbTop);float RA,RB;
            const FVector A=TrunkCentre(I,Z,RA),B=TrunkCentre(I,EndZ,RB),D=B-A;
            // A tilted cylinder has a wider horizontal section. Compensate so
            // the climb path and camera remain outside its collision surface.
            const float Radius=FMath::Min(RA,RB)*D.Z/D.Size();
            Trunks->AddInstance(FTransform(FQuat::FindBetweenNormals(FVector::UpVector,D.GetSafeNormal()),(A+B)*.5f,FVector(Radius/50,Radius/50,D.Size()/100)));
        }
    }
    // A dozen accessible acorns around the camp trees, plus scattered reserve forage.
    // Independent seed keeps the shared pickup layout stable on server and clients.
    FRandomStream ForageRandom(29571);
    for(int32 I=0;I<30;I++){
        const auto& T=Trees[I<12?I%5:(I*7+2)%Trees.Num()];const float A=I<12?(I/5)*2*PI/3+.3f:ForageRandom.FRandRange(0,2*PI);
        FVector P=T.Base+FVector(FMath::Cos(A)*310,FMath::Sin(A)*310,0);P.Z=GroundHeight(P.X,P.Y)+4.5f;AcornLocations.Add(P);
    }
    DenLocation=bLake?FVector(-11700,2200,GroundHeight(-11700,2200)):FVector(0,1200,GroundHeight(0,1200));
    auto NearForage=[&](float X,float Y,float Radius){const FVector P(X,Y,0);for(const auto& A:AcornLocations)if(FVector::DistSquared2D(P,A)<FMath::Square(Radius))return true;return FVector::DistSquared2D(P,DenLocation)<FMath::Square(Radius+100);};
    // Visible rails backed by wooded slopes give the reserve a finite skyline.
    constexpr int32 FenceSections=240;
    const float FenceRadius=MapRadius-60.f;
    auto Rail=[&](FVector A,FVector B,float Radius){const FVector D=B-A;ReserveFence->AddInstance(FTransform(FQuat::FindBetweenNormals(FVector::UpVector,D.GetSafeNormal()),(A+B)*.5f,FVector(Radius/50,Radius/50,D.Size()/100)));};
    for(int32 I=0;I<FenceSections;I++){
        const float A=I*2*PI/FenceSections,B=(I+1)*2*PI/FenceSections;
        FVector P(FMath::Cos(A)*FenceRadius,FMath::Sin(A)*FenceRadius,0),Q(FMath::Cos(B)*FenceRadius,FMath::Sin(B)*FenceRadius,0);
        P.Z=GroundHeight(P.X,P.Y);Q.Z=GroundHeight(Q.X,Q.Y);
        Rail(P-FVector(0,0,12),P+FVector(0,0,132),7);
        Rail(P+FVector(0,0,52),Q+FVector(0,0,52),5);
        Rail(P+FVector(0,0,106),Q+FVector(0,0,106),5);
    }
    FRandomStream RimRandom(11903); // Preserve gameplay tree and forage seeds.
    for(int32 I=0;I<560;I++){
        const float A=RimRandom.FRandRange(0,2*PI),D=RimRandom.FRandRange(MapRadius+250,20700);
        const float X=FMath::Cos(A)*D,Y=FMath::Sin(A)*D;const int32 Species=RimRandom.RandRange(0,3);
        const float S=RimRandom.FRandRange(.65f,1.2f);
        const auto Bounds=ForestLayers[Species]->GetStaticMesh()->GetBoundingBox();
        ForestLayers[Species]->SetCullDistances(0,65000);
        const float Collar=Species==3?460.f:0.f;
        ForestLayers[Species]->AddInstance(FTransform(FRotator(0,RimRandom.FRandRange(0,360),0),FVector(X,Y,GroundHeight(X,Y)-45-(Bounds.Min.Z+Collar)*S),FVector(S)));
    }
    auto GroundInstance=[&](int32 Species,float X,float Y,float S,float Sink=0.f){
        if(bLake&&AWildlineLake::RadiusAt(FVector(X,Y,0))<1.23f)return;
        const FVector Normal=FVector(GroundHeight(X-10,Y)-GroundHeight(X+10,Y),GroundHeight(X,Y-10)-GroundHeight(X,Y+10),20).GetSafeNormal();
        const FQuat Align=FQuat::FindBetweenNormals(FVector::UpVector,Normal)*FQuat(FVector::UpVector,FMath::DegreesToRadians(R.FRandRange(0,360)));
        const float RootSink=Species==17?8.f:Sink;
        ForestLayers[Species]->AddInstance(FTransform(Align,FVector(X,Y,GroundHeight(X,Y)-RootSink),FVector(S)));
    };
    // Broad ecological patches keep trails open and avoid a uniformly planted lawn.
    for(int32 I=0;I<(bWinter?0:112000);I++){
        const bool Camp=I<18000;
        float X,Y;
        if(Camp){X=R.FRandRange(-4400,4400);Y=R.FRandRange(-4400,4400);}
        else{const auto& T=Trees[R.RandRange(0,Trees.Num()-1)];X=T.Base.X+R.FRandRange(-1100,1100);Y=T.Base.Y+R.FRandRange(-1100,1100);}
        if((bFarm&&FMath::Sqrt(X*X+Y*Y)<4700)||(bResidential&&FMath::Abs(X)<6400&&FMath::Abs(Y)<5600))continue;if(NearForage(X,Y,110))continue;
        const float Trail=FMath::Abs(Y-700.f-FMath::Sin(X*.0007f)*210.f);
        if(Trail<100.f||(FMath::Abs(X)<130&&Y>500&&Y<1350))continue;
        const float Patch=FMath::Sin(X*.0018f)*FMath::Cos(Y*.0014f)+FMath::Sin((X+Y)*.0033f)*.3f;
        if(Patch<-.4f&&R.FRand()>.12f)continue;
        int32 Species=4+R.RandRange(0,2);
        if(Patch>.4f&&R.FRand()<.4f)Species=8+R.RandRange(0,1);
        else if(R.FRand()<.16f)Species=7;
        GroundInstance(Species,X,Y,R.FRandRange(.65f,1.25f),.8f);
    }
    for(int32 I=0;I<(bWinter?0:4800);I++){
        const auto& T=Trees[R.RandRange(0,Trees.Num()-1)];const float A=R.FRandRange(0,2*PI),D=R.FRandRange(120,650);
        const float X=T.Base.X+FMath::Cos(A)*D,Y=T.Base.Y+FMath::Sin(A)*D;
        if(NearForage(X,Y,200))continue;
        const int32 Species=R.FRand()<.76f?12+R.RandRange(0,3):10+R.RandRange(0,1);
        GroundInstance(Species,X,Y,R.FRandRange(.6f,1.25f),1.3f);
    }
    for(int32 I=0;I<(bWinter?0:19000);I++){
        float X=R.FRandRange(-15800,15800),Y=R.FRandRange(-15800,15800);
        if(I<6500){X=R.FRandRange(-2400,2400);Y=R.FRandRange(-2400,2400);}
        if((bFarm&&FVector2D(X,Y).Size()<4700)||(bResidential&&FMath::Abs(X)<6400&&FMath::Abs(Y)<5600))continue;GroundInstance(16,X,Y,R.FRandRange(.7f,1.2f),-.35f);
    }
    for(int32 I=0;I<1400;I++){
        const float X=R.FRandRange(-16000,16000),Y=R.FRandRange(-16000,16000),S=R.FRandRange(.12f,1.6f);
        if((bFarm&&FVector2D(X,Y).Size()<5100)||(bResidential&&FMath::Abs(X)<6400&&FMath::Abs(Y)<5600))continue;if(NearForage(X,Y,280*S+195))continue;
        Rocks->AddInstance(FTransform(FRotator(R.FRandRange(-15,15),R.FRandRange(0,360),R.FRandRange(-10,10)),FVector(X,Y,GroundHeight(X,Y)-5*S),FVector(S,S*R.FRandRange(.8f,1.3f),S)));
    }
    for(int32 I=0;I<150;I++){
        const auto& T=Trees[R.RandRange(0,Trees.Num()-1)];
        const float X=T.Base.X+R.FRandRange(-900,900),Y=T.Base.Y+R.FRandRange(-900,900);
        if(!NearForage(X,Y,260))GroundInstance(17,X,Y,R.FRandRange(.045f,.09f));
    }
    for(int32 I=0;I<110;I++){
        const auto& T=Trees[(I*13+5)%Trees.Num()];
        const float X=T.Base.X+R.FRandRange(-650,650),Y=T.Base.Y+R.FRandRange(-650,650);
        if(NearForage(X,Y,500))continue;
        ForestLayers[18]->AddInstance(FTransform(FRotator(R.FRandRange(-4,4),R.FRandRange(0,360),0),FVector(X,Y,GroundHeight(X,Y)+20),FVector(R.FRandRange(.6f,1.25f))));
    }
    for(auto* Layer:ForestLayers){Layer->BuildTreeIfOutdated(false,true);Layer->bAutoRebuildTreeOnInstanceChanges=true;}
    Acorns->SetMaterial(0,Mat(TEXT("/Game/Wildline/Materials/M_Belly.M_Belly")));
    AcornCaps->SetMaterial(0,Mat(TEXT("/Game/Wildline/Forest/Materials/M_TreeBark.M_TreeBark")));
    for(const FVector& P:AcornLocations){Acorns->AddInstance(FTransform(FRotator::ZeroRotator,P,FVector(.065f,.065f,.085f)));AcornCaps->AddInstance(FTransform(FRotator::ZeroRotator,P+FVector(0,0,3.5f),FVector(.075f,.075f,.03f)));}
    DenMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Wildline/Forest/Meshes/SM_Log.SM_Log")));
    DenMesh->SetCollisionProfileName(TEXT("BlockAll"));
    DenMesh->SetRelativeLocation(DenLocation+FVector(0,0,26));DenMesh->SetRelativeRotation(FRotator(0,75,0));
}

float AWildlineWorld::GroundHeight(float X,float Y) const{
    if(bLake)return AWildlineLake::BedHeight(X,Y);
    float Height=(bFarm||bResidential)?WildlineRules::FarmGround(X,Y):WildlineRules::Ground(X,Y);
    if(bResidential)for(const FBox& Basin:ResidentialPoolBasins){
        const float Edge=FMath::Min(FMath::Min(X-Basin.Min.X,Basin.Max.X-X),FMath::Min(Y-Basin.Min.Y,Basin.Max.Y-Y));
        if(Edge>0)Height=FMath::Lerp(Height,float(Basin.Min.Z-2),FMath::SmoothStep(0.f,35.f,Edge));
    }
    return Height;
}
bool AWildlineWorld::InsideFarmhouse(FVector P) const{const FVector D=P-FarmhouseLocation;return bFarm&&FMath::Abs(D.X)<585&&FMath::Abs(D.Y)<470&&D.Z>FarmFloor-1&&D.Z<FarmFloor+218;}
bool AWildlineWorld::InCropCover(FVector P) const{return bFarm&&P.X>-2400&&P.X<700&&FMath::Abs(P.Y)>850&&FMath::Abs(P.Y)<2250&&P.Z-GroundHeight(P.X,P.Y)<100;}
FVector AWildlineWorld::FarmRoute(FVector From,FVector Target) const{
    if(bLake)return AWildlineLake::RouteAroundLake(From,Target);
    if(!bFarm)return Target;
    // Use the open porch doorway before crossing an exterior wall.
    const FVector Door=FarmhouseLocation+FVector(-720,0,FarmFloor+5);
    if(InsideFarmhouse(From)&&!InsideFarmhouse(Target))return Door;
    if(!InsideFarmhouse(From)&&InsideFarmhouse(Target)){
        if(From.X<Door.X-60||FMath::Abs(From.Y)>65)return Door;
    }
    return Target;
}

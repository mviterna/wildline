#include "WildlineWorld.h"
#include "WildlineNeighborhoodConfig.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

bool AWildlineWorld::GenerateConfiguredNeighborhood(){
    if(!NeighborhoodConfig)NeighborhoodConfig=LoadObject<UWildlineNeighborhoodConfig>(nullptr,TEXT("/Game/Wildline/Residential/Configs/DA_MapleCrescent.DA_MapleCrescent"));
    if(!NeighborhoodConfig||NeighborhoodConfig->Routes.Num()!=2)return false;
    for(const auto& Route:NeighborhoodConfig->Routes)if(Route.Targets.Num()!=4||Route.Labels.Num()!=4)return false;
    ResidentialPoolBasins.Empty();
    for(const auto& Prop:NeighborhoodConfig->Props)if(Prop.Mesh.Contains(TEXT("SM_KitPool"))){
        if(auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Prop.Mesh))ResidentialPoolBasins.Add(Mesh->GetBoundingBox().TransformBy(Prop.Transform));
    }
    RootComponent->SetMobility(EComponentMobility::Static);Generate();
    Terrain->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Wildline/Residential/Materials/M_NeighborhoodLawn.M_NeighborhoodLawn")));
    Acorns->ClearInstances();AcornCaps->ClearInstances();AcornLocations.Empty();DenMesh->SetVisibility(false);
    CoursePlatforms.Empty();CourseEntryTrees.Empty();FeederLocation=NeighborhoodConfig->Feeder;
    CourseStart=NeighborhoodConfig->Routes[0].Start;DenLocation=CourseStart;
    for(const auto& Prop:NeighborhoodConfig->Props){
        FTransform Placement=Prop.Transform;float ReviewYaw;
        if(!Prop.bCollision&&Prop.Mesh.Contains(TEXT("SM_Oak"))&&FParse::Value(FCommandLine::Get(),TEXT("WildlineCourseOakYaw="),ReviewYaw))Placement.SetRotation(FRotator(0,ReviewYaw,0).Quaternion());
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Prop.Mesh);
        if(!Mesh){UE_LOG(LogTemp,Error,TEXT("NEIGHBORHOOD missing mesh %s"),*Prop.Mesh);continue;}
        auto* C=NewObject<UStaticMeshComponent>(this,*FString::Printf(TEXT("Neighborhood_%04d"),ResidentialProps.Num()));
        AddInstanceComponent(C);C->SetupAttachment(RootComponent);C->SetMobility(EComponentMobility::Static);C->SetStaticMesh(Mesh);
        C->SetRelativeTransform(Placement);C->SetCollisionProfileName(Prop.bCollision?TEXT("BlockAll"):TEXT("NoCollision"));C->RegisterComponent();ResidentialProps.Add(C);
        // Yard oaks share the same climbable bark and limbs as the reserve.
        for(int32 Species=0;Species<3;Species++)if(Prop.Mesh.Contains(FString::Printf(TEXT("SM_Oak%c"),TCHAR('A'+Species)))){
            FWildlineTree T;T.Species=Species;T.MeshTransform=Placement;T.Base=Placement.GetLocation();
            T.Height=Mesh->GetBoundingBox().GetSize().Z*Prop.Transform.GetScale3D().Z;T.ClimbTop=T.Base.Z+T.Height;T.Radius=55*Prop.Transform.GetScale3D().X;
            AddClimbWood(Trees.Add(T));C->SetCollisionProfileName(TEXT("NoCollision"));break;
        }
    }
    // The kit's example pool has open ends. Complete its recessed concrete
    // liner so the terrain depression never presents as exposed grass walls.
    for(const FBox& Basin:ResidentialPoolBasins){
        const FVector Centre=Basin.GetCenter(),Size=Basin.GetSize();const float Floor=Basin.Min.Z+12,Top=Basin.Max.Z-2;
        auto Liner=[&](FVector Position,FVector Dimensions){
            auto* C=NewObject<UStaticMeshComponent>(this,*FString::Printf(TEXT("PoolLiner_%04d"),ResidentialProps.Num()));AddInstanceComponent(C);C->SetupAttachment(RootComponent);C->SetMobility(EComponentMobility::Static);
            C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));C->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Wildline/Farm/Materials/F_Trim.F_Trim")));
            C->SetRelativeLocation(Position);C->SetRelativeScale3D(Dimensions/100.f);C->SetCollisionProfileName(TEXT("BlockAll"));C->RegisterComponent();ResidentialProps.Add(C);
        };
        Liner(FVector(Centre.X,Centre.Y,Floor-2),FVector(Size.X-50,Size.Y-50,4));
        for(float Side:{-1.f,1.f}){
            Liner(FVector(Centre.X+Side*(Size.X*.5f-25),Centre.Y,(Floor+Top)*.5f),FVector(20,Size.Y-30,Top-Floor));
            Liner(FVector(Centre.X,Centre.Y+Side*(Size.Y*.5f-25),(Floor+Top)*.5f),FVector(Size.X-30,20,Top-Floor));
        }
    }
    for(const auto& Route:NeighborhoodConfig->Routes){
        CoursePlatforms.Append(Route.Targets);int32 Entry=INDEX_NONE;
        if(Route.EntryClimbTop>0){FWildlineTree T;T.Base=Route.EntryTrunk;T.Radius=Route.EntryRadius;T.Height=Route.EntryClimbTop-T.Base.Z;T.ClimbTop=Route.EntryClimbTop;T.bStraightTrunk=true;Entry=Trees.Add(T);}
        CourseEntryTrees.Add(Entry);
    }
    FRandomStream R(417);
    for(int32 I=0;I<14000;I++){
        const FVector P(R.FRandRange(-4700,4700),R.FRandRange(-1500,1700),210);
        bool Near=false;for(const auto& Route:NeighborhoodConfig->Routes){if(FVector::Dist2D(P,Route.Start)<180)Near=true;for(const auto& T:Route.Targets)if(FVector::Dist2D(P,T)<150)Near=true;}
        if(Near||FVector::Dist2D(P,FeederLocation)<170)continue;
        ForestLayers[4+I%3]->AddInstance(FTransform(FRotator(0,R.FRandRange(0,360),0),P,FVector(.11f)));
    }
    UE_LOG(LogTemp,Display,TEXT("NEIGHBORHOOD CONFIG %s: %d props, two organic routes"),*NeighborhoodConfig->DisplayName,ResidentialProps.Num());return true;
}
FString AWildlineWorld::CourseLabel(int32 Lane,int32 Step)const{
    if(Step==0)return NeighborhoodConfig?TEXT("YARD ENTRANCE"):TEXT("START PAD");
    if(Step>=5)return TEXT("BIRD FEEDER");
    if(NeighborhoodConfig&&NeighborhoodConfig->Routes.IsValidIndex(Lane)&&NeighborhoodConfig->Routes[Lane].Labels.IsValidIndex(Step-1))return NeighborhoodConfig->Routes[Lane].Labels[Step-1];
    const TCHAR* Labels[]={TEXT("BIN"),TEXT("GARDEN BENCH"),TEXT("FENCE LEDGE"),TEXT("PERGOLA")};return Labels[FMath::Clamp(Step-1,0,3)];
}
float AWildlineWorld::CourseLaunchZ(FVector Position)const{
    if(NeighborhoodConfig)for(int32 Lane=0;Lane<2;Lane++){const FVector T=CourseTarget(Lane,4);if(FVector::Dist2D(Position,T)<112&&FMath::Abs(Position.Z-T.Z-13)<10)return NeighborhoodConfig->Routes[Lane].TrampolineLaunchZ;}
    return 360;
}
int32 AWildlineWorld::CourseEntryTree(int32 Lane)const{return CourseEntryTrees.IsValidIndex(Lane)?CourseEntryTrees[Lane]:INDEX_NONE;}

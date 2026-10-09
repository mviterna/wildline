#include "WildlineWorld.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"

bool AWildlineWorld::IsLimbSurface(int32 Tree,FVector Point,FVector Normal)const{
    if(!Trees.IsValidIndex(Tree)||Trees[Tree].bStraightTrunk)return false;
    float Radius;const FVector Stem=TrunkCentre(Tree,Point.Z,Radius);
    return FMath::Abs(Normal.Z)>.65f||FVector::Dist2D(Point,Stem)>Radius+35;
}

void AWildlineWorld::AddClimbWood(int32 Index){
    if(!Trees.IsValidIndex(Index))return;
    const auto& T=Trees[Index];
    if(!T.bStraightTrunk&&ClimbWood.IsValidIndex(T.Species)&&ClimbWood[T.Species]->GetStaticMesh())ClimbWood[T.Species]->AddInstance(T.MeshTransform);
}

bool AWildlineWorld::TraceWood(FVector Start,FVector End,FHitResult& Hit,int32& Index,float SweepRadius)const{
    bool Found=false;float Distance=FVector::Dist(Start,End)+1;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(SquirrelWood),true);
    for(auto* Wood:ClimbWood){
        FHitResult Candidate;
        if(!Wood)continue;
        const bool Contact=SweepRadius>0?Wood->SweepComponent(Candidate,Start,End,FQuat::Identity,FCollisionShape::MakeSphere(SweepRadius),false):Wood->LineTraceComponent(Candidate,Start,End,Query);
        if(!Contact||Candidate.bStartPenetrating||Candidate.Distance>=Distance)continue;
        FTransform Transform;
        if(!Wood->GetInstanceTransform(Candidate.Item,Transform,true))continue;
        for(int32 I=0;I<Trees.Num();I++)if(!Trees[I].bStraightTrunk&&Trees[I].MeshTransform.Equals(Transform,.1f)){
            Hit=Candidate;Index=I;Found=true;Distance=Candidate.Distance;break;
        }
    }
    return Found;
}

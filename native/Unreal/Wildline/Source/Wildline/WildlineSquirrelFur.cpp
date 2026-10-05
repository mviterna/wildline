#include "WildlineCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "AnimationRuntime.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"

void AWildlineCharacter::BuildFur(){
    for(auto* Section:FurSections)if(Section)Section->DestroyComponent();FurSections.Empty();
    if(!bSquirrel||!SquirrelMeshAsset||GetNetMode()==NM_DedicatedServer)return;
    const auto* Render=SquirrelMeshAsset->GetResourceForRendering();if(!Render||Render->LODRenderData.IsEmpty())return;
    const auto& LOD=Render->LODRenderData[0];const auto& Vertices=LOD.StaticVertexBuffers;
    if(!SquirrelMeshAsset->GetLODInfo(0)->bAllowCPUAccess)return;
    const auto* Weights=GetMesh()->GetSkinWeightBuffer(0);if(!Weights)return;
    const auto* Indices=LOD.MultiSizeIndexContainer.GetIndexBuffer();
    TArray<FTransform> Rest;const auto& Ref=SquirrelMeshAsset->GetRefSkeleton();
    FAnimationRuntime::FillUpComponentSpaceTransforms(Ref,Ref.GetRefBonePose(),Rest);
    struct FTriangle{int32 A,B,C,Section;float End;};TArray<FTriangle> Triangles;float Area=0;
    for(int32 S=0;S<LOD.RenderSections.Num();S++){
        const auto& Section=LOD.RenderSections[S];if(Section.MaterialIndex!=0)continue;
        for(uint32 I=Section.BaseIndex;I<Section.BaseIndex+Section.NumTriangles*3;I+=3){
            const int32 A=Indices->Get(I),B=Indices->Get(I+1),C=Indices->Get(I+2);
            const FVector PA(Vertices.PositionVertexBuffer.VertexPosition(A)),PB(Vertices.PositionVertexBuffer.VertexPosition(B)),PC(Vertices.PositionVertexBuffer.VertexPosition(C));
            const float FaceArea=FVector::CrossProduct(PB-PA,PC-PA).Size()*.5f;if(FaceArea<.0001f)continue;
            Area+=FaceArea;Triangles.Add({A,B,C,S,Area});
        }
    }
    if(Triangles.IsEmpty())return;
    struct FPatch{TArray<FVector> P,N;TArray<int32> I;TArray<FVector2D> UV;TArray<FLinearColor> C;TArray<FProcMeshTangent> T;};
    TMap<int32,FPatch> Patches;FRandomStream Random(17429);
    for(int32 Hair=0;Hair<18000;Hair++){
        const float Pick=Random.FRand()*Area;int32 Low=0,High=Triangles.Num()-1;while(Low<High){const int32 Mid=(Low+High)/2;if(Triangles[Mid].End<Pick)Low=Mid+1;else High=Mid;}
        const auto& Tri=Triangles[Low];const auto& Section=LOD.RenderSections[Tri.Section];
        const float U=FMath::Sqrt(Random.FRand()),V=Random.FRand();const float W[3]={1-U,U*(1-V),U*V};const int32 Id[3]={Tri.A,Tri.B,Tri.C};
        FVector Position=FVector::ZeroVector,Normal=FVector::ZeroVector;FVector2D UV=FVector2D::ZeroVector;
        TMap<int32,float> Influence;
        for(int32 J=0;J<3;J++){
            Position+=FVector(Vertices.PositionVertexBuffer.VertexPosition(Id[J]))*W[J];Normal+=FVector(Vertices.StaticMeshVertexBuffer.VertexTangentZ(Id[J]))*W[J];UV+=FVector2D(Vertices.StaticMeshVertexBuffer.GetVertexUV(Id[J],0))*W[J];
            for(uint32 K=0;K<Weights->GetMaxBoneInfluences();K++){const uint32 Local=Weights->GetBoneIndex(Id[J],K);if(Section.BoneMap.IsValidIndex(Local))Influence.FindOrAdd(Section.BoneMap[Local])+=Weights->GetBoneWeight(Id[J],K)/65535.f*W[J];}
        }
        int32 Bone=0;float Max=0;for(const auto& Pair:Influence)if(Pair.Value>Max){Max=Pair.Value;Bone=Pair.Key;}
        const FString Name=Ref.GetBoneName(Bone).ToString();
        if(Name.Contains(TEXT("Eye"))||Name.Contains(TEXT("Finger"))||Name.Contains(TEXT("Toe"))||Name.Contains(TEXT("Hand"))||Name.Contains(TEXT("Foot"))||Name.Contains(TEXT("Nose"))||Position.Z<1.4f)continue;
        const bool Tail=Name.Contains(TEXT("Tail")),Head=Name.Contains(TEXT("Head"))||Name.Contains(TEXT("Ear"))||Name.Contains(TEXT("FACIAL"));
        Normal.Normalize();const float Length=Random.FRandRange(.7f,1.2f)*(Tail?1.45f:Head?.12f:.33f);
        const FVector Grain=Tail?FVector(0,-1,.08f):FVector(0,-.8f,-.1f);
        const FVector Growth=(Normal*.9f+Grain*.45f).GetSafeNormal();
        FVector Width=FVector::CrossProduct(Growth,Random.VRand()).GetSafeNormal()*(Tail?.025f:.012f);
        Position-=Normal*.035f;const FVector Middle=Position+Growth*Length*.55f,Tip=Position+Growth*Length+Normal*Length*.1f;
        auto& Patch=Patches.FindOrAdd(Bone);const int32 Base=Patch.P.Num();
        const FVector Points[6]={Position-Width,Position+Width,Middle-Width*.6f,Middle+Width*.6f,Tip-Width*.07f,Tip+Width*.07f};
        for(const auto& P:Points){Patch.P.Add(Rest[Bone].InverseTransformPosition(P));Patch.N.Add(Rest[Bone].InverseTransformVectorNoScale(Normal));Patch.UV.Add(UV);Patch.C.Add(FLinearColor::White);Patch.T.Add(FProcMeshTangent(Rest[Bone].InverseTransformVectorNoScale(Width).GetSafeNormal(),false));}
        Patch.I.Append({Base,Base+2,Base+1,Base+1,Base+2,Base+3,Base+2,Base+4,Base+3,Base+3,Base+4,Base+5});
    }
    auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Wildline/Squirrel/M_SquirrelStrands.M_SquirrelStrands"));
    for(auto& Pair:Patches){
        auto* Fur=NewObject<UProceduralMeshComponent>(this);Fur->SetupAttachment(GetMesh(),Ref.GetBoneName(Pair.Key));Fur->SetRelativeTransform(FTransform::Identity);Fur->SetCollisionEnabled(ECollisionEnabled::NoCollision);Fur->CastShadow=false;Fur->SetOwnerNoSee(!bThirdPerson);Fur->SetMaterial(0,Material);Fur->RegisterComponent();
        auto& P=Pair.Value;Fur->CreateMeshSection_LinearColor(0,P.P,P.I,P.N,P.UV,P.C,P.T,false);FurSections.Add(Fur);
    }
}

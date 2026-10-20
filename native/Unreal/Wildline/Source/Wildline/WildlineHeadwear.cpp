#include "WildlineHeadwear.h"
#include "WildlineCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Math/RotationMatrix.h"

void AWildlineCharacter::UpdateHeadwear(){
    const int32 Id=WildlineHeadwear::Clamp(HunterProfile.Headwear);
    HunterHat->SetStaticMesh(Id>0?LoadObject<UStaticMesh>(nullptr,*WildlineHeadwear::Asset(Id)):nullptr);
    HunterHat->SetVisibility(!bSquirrel&&Id>0);HunterHat->SetLightingChannels(!bPreviewHunter,bPreviewHunter,false);
    HunterHat->AttachToComponent(GetMesh(),FAttachmentTransformRules::KeepRelativeTransform,TEXT("head"));
    if(auto* Asset=GetMesh()->GetSkeletalMeshAsset()){
        const auto& Ref=Asset->GetRefSkeleton();int32 Bone=Ref.FindBoneIndex(TEXT("head"));FTransform Head=FTransform::Identity;
        while(Bone>=0){Head=Head*Ref.GetRefBonePose()[Bone];Bone=Ref.GetParentIndex(Bone);}
        const FTransform Band(FRotationMatrix::MakeFromXZ(FVector::YAxisVector,FVector::UpVector).ToQuat(),Head.GetLocation()+FVector(0,-1,5));
        HunterHat->SetRelativeTransform(Band.GetRelativeTransform(Head));
    }
}

#include "WildlineSquirrelAnim.h"
#include "WildlineCharacter.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNodeBase.h"
#include "AnimationRuntime.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TwoBoneIK.h"
#include "Engine/World.h"

struct FWildlineSquirrelProxy : public FAnimInstanceProxy {
    explicit FWildlineSquirrelProxy(UAnimInstance* Instance):FAnimInstanceProxy(Instance){}
    float Phase=0,Time=0,Move=0,Run=0,Vertical=0,HeadYaw=0,Landing=0;
    bool Climbing=false,Falling=false,Dead=false,Initialized=false,WasFalling=false;
    FVector Previous=FVector::ZeroVector;
    TArray<FTransform> RestComponent;
    TArray<FName> Names;
    FVector RootLocalUp=FVector::UpVector;
    float PawGround[4]={0,0,0,0};
    void PreUpdate(UAnimInstance* Instance,float Delta) override {
        FAnimInstanceProxy::PreUpdate(Instance,Delta);
        auto* Pawn=Cast<AWildlineCharacter>(Instance->GetOwningActor());
        auto* Mesh=Instance->GetSkelMeshComponent();if(!Pawn||!Mesh)return;
        if(Names.IsEmpty()&&Mesh->GetSkeletalMeshAsset()){
            const auto& Ref=Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();
            FAnimationRuntime::FillUpComponentSpaceTransforms(Ref,Ref.GetRefBonePose(),RestComponent);
            for(int32 I=0;I<Ref.GetNum();I++)Names.Add(Ref.GetBoneName(I));
            const int32 Spine=Names.IndexOfByKey(FName("SKEL_Spine_Root"));
            if(Spine!=INDEX_NONE){const int32 Parent=Ref.GetParentIndex(Spine);if(Parent!=INDEX_NONE)RootLocalUp=RestComponent[Parent].InverseTransformVector(FVector::UpVector);}
        }
        const FVector Position=Pawn->GetActorLocation();
        const float Distance=Initialized?FVector::Dist(Position,Previous):0;
        Previous=Position;Initialized=true;Time=Pawn->ServerTime();
        const float Speed=FMath::Max(Pawn->GetVelocity().Size2D(),Delta>0?FMath::Min(Distance/Delta,750.f):0.f);
        Climbing=Pawn->TreeIndex!=INDEX_NONE&&(!Pawn->bWoodClimbing||!Pawn->bBranchClimbing);Falling=Pawn->GetCharacterMovement()->IsFalling();Dead=Pawn->bDead;
        Vertical=Pawn->GetVelocity().Z;
        Move=FMath::FInterpTo(Move,FMath::Clamp(Speed/140.f,0.f,1.f),Delta,12);
        Run=FMath::FInterpTo(Run,FMath::Clamp((Speed-170.f)/250.f,0.f,1.f),Delta,8);
        // Travel-driven footfall frequency: no treadmill motion while stationary.
        if(!Dead&&!Falling)Phase+=FMath::Min(Distance,60.f)/(Climbing?32.f:FMath::Lerp(26.f,105.f,Run))*2*PI;
        if(WasFalling&&!Falling)Landing=1.f;
        Landing=FMath::Max(0.f,Landing-Delta*7.f);WasFalling=Falling;
        HeadYaw=FMath::FInterpTo(HeadYaw,FMath::Clamp(FMath::FindDeltaAngleDegrees(Pawn->GetActorRotation().Yaw,Pawn->GetControlRotation().Yaw),-35.f,35.f),Delta,6);
        if(!Dead&&!Climbing&&!Falling){
            const TCHAR* Feet[4]={TEXT("SKEL_L_Hand"),TEXT("SKEL_R_Hand"),TEXT("SKEL_L_Foot"),TEXT("SKEL_R_Foot")};
            for(int32 I=0;I<4;I++){
                const int32 Bone=Names.IndexOfByKey(FName(Feet[I]));if(Bone==INDEX_NONE)continue;
                FVector Local=RestComponent[Bone].GetTranslation();Local.Z=0;
                const FVector World=Mesh->GetComponentTransform().TransformPosition(Local);
                FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(SquirrelPaw),false,Pawn);
                if(Pawn->GetWorld()->LineTraceSingleByChannel(Hit,World+FVector(0,0,12),World-FVector(0,0,25),ECC_Visibility,Query))
                    PawGround[I]=FMath::FInterpTo(PawGround[I],FMath::Clamp(static_cast<float>(Mesh->GetComponentTransform().InverseTransformPosition(Hit.ImpactPoint).Z),-4.f,4.f),Delta,16);
            }
        }
    }
    bool Evaluate(FPoseContext& Output) override {
        Output.Pose.ResetToRefPose();
        if(Names.IsEmpty())return true;
        const auto& Bones=Output.Pose.GetBoneContainer();
        auto Rotate=[&](FName Name,FVector Axis,float Degrees){
            const int32 Index=Names.IndexOfByKey(Name);if(Index==INDEX_NONE)return;
            const FCompactPoseBoneIndex Compact=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Index));
            if(Compact.GetInt()==INDEX_NONE)return;
            auto& T=Output.Pose[Compact];
            const FVector LocalAxis=RestComponent[Index].GetRotation().Inverse().RotateVector(Axis);
            T.SetRotation((T.GetRotation()*FQuat(LocalAxis,FMath::DegreesToRadians(Degrees))).GetNormalized());
        };
        auto Pitch=[&](FName Bone,float Degrees){Rotate(Bone,FVector::ForwardVector,Degrees);}; // source forward is +Y
        auto Yaw=[&](FName Bone,float Degrees){Rotate(Bone,FVector::UpVector,Degrees);};
        const float Gait=FMath::Sin(Phase),Bounce=(1-FMath::Cos(Phase*2))*.5f;
        const float Activity=Dead?0.f:Move;
        const float Idle=1-Activity;
        const float Flight=Falling&&!Dead?1.f:0.f;
        const float Tuck=Flight*FMath::Clamp(Vertical/350.f,0.f,1.f);
        // Pelvis-driven bound and lumbar flex, measured in centimetres.
        const int32 Root=Names.IndexOfByKey(FName("SKEL_Spine_Root"));
        if(Root!=INDEX_NONE){const auto I=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Root));if(I.GetInt()!=INDEX_NONE)Output.Pose[I].AddToTranslation(RootLocalUp*((Bounce*1.15f*Run-Landing*1.6f)*Activity));}
        Pitch("SKEL_Pelvis",Dead?12:Gait*3*Activity*Run);
        Pitch("SKEL_Spine0",Dead?8:(Gait*6*Run*Activity+Landing*8));
        Pitch("SKEL_Spine1",Gait*-4*Run*Activity+FMath::Sin(Time*3.4f)*.6f*Idle);
        Pitch("SKEL_Spine2",Gait*-3*Run*Activity);
        Pitch("SKEL_Neck0",Dead?12:-Gait*2*Activity+FMath::Sin(Time*1.8f)*2*Idle);
        Pitch("SKEL_Head",Dead?18:Flight*-8+FMath::Sin(Time*1.1f)*3*Idle);
        Yaw("SKEL_Head",Dead?8:HeadYaw*.65f+FMath::Sin(Time*.71f)*7*Idle);
        for(int32 Side=0;Side<2;Side++){
            const FString Prefix=Side==0?TEXT("SKEL_L_"):TEXT("SKEL_R_");
            const float Offset=Side==0?0.f:PI;
            const float Walk=FMath::Sin(Phase+Offset);
            const float Hind=FMath::Lerp(Walk,FMath::Sin(Phase+(Side?.22f:0.f)),Run)*Activity;
            const float Front=FMath::Lerp(-Walk,FMath::Sin(Phase+PI*.9f+(Side?.25f:0.f)),Run)*Activity;
            const float HindLift=FMath::Max(0.f,Hind),FrontLift=FMath::Max(0.f,Front);
            Pitch(FName(*(Prefix+TEXT("Thigh"))),Dead?28:Hind*29-25*Tuck);
            Pitch(FName(*(Prefix+TEXT("Calf"))),Dead?-45:-HindLift*42-45*Tuck+Landing*16);
            Pitch(FName(*(Prefix+TEXT("Foot"))),Dead?18:-Hind*15+HindLift*24+20*Tuck);
            Pitch(FName(*(Prefix+TEXT("Toe0"))),-HindLift*12);
            Pitch(FName(*(Prefix+TEXT("UpperArm"))),Dead?35:Front*26+35*Tuck);
            Pitch(FName(*(Prefix+TEXT("Forearm"))),Dead?-40:-FrontLift*40-50*Tuck-Landing*10);
            Pitch(FName(*(Prefix+TEXT("Hand"))),Dead?12:-Front*20+FrontLift*25+15*Tuck);
            Pitch(FName(*(Prefix+TEXT("Finger00"))),-FrontLift*14);
        }
        // A lifted resting tail uncurls for acceleration and balances a leap.
        for(int32 I=0;I<4;I++){
            const float IdleCurl[4]={-48.f,-22.f,23.f,30.f};
            const float RunCurl[4]={-12.f,-6.f,8.f,12.f};
            const float Curl=FMath::Lerp(IdleCurl[I],RunCurl[I],FMath::Max(Run,Flight));
            Pitch(FName(*FString::Printf(TEXT("SKEL_Tail%d"),I)),Dead?6:Curl+FMath::Sin(Time*3- I*.65f)*(Idle*2+Activity*4)+Gait*Run*(I+1)*1.5f);
            Yaw(FName(*FString::Printf(TEXT("SKEL_Tail%d"),I)),Dead?4:FMath::Sin(Time*2.1f-I*.7f)*3+HeadYaw*-.08f);
        }
        if(!Dead){
            Pitch("FACIAL_L_Ear0",FMath::Sin(Time*2.3f)*3);
            Pitch("FACIAL_R_Ear0",FMath::Sin(Time*2.3f+.7f)*3);
            const float Blink=FMath::Max(0.f,1-FMath::Abs(FMath::Fmod(Time,4.8f)-.12f)/.10f);
            Pitch("FACIAL_L_EyelidUpper",Blink*28);Pitch("FACIAL_R_EyelidUpper",Blink*28);
            Pitch("FACIAL_Nose",FMath::Sin(Time*7.f)*.7f);
        }
        if(!Falling&&!Dead){
            FCSPose<FCompactPose> Space;Space.InitPose(Output.Pose);
            // Alternating walking contacts become paired bounds at running speed.
            // Each paw follows a short swing arc and a planted support phase.
            for(int32 Leg=0;Leg<4;Leg++){
                const bool Hind=Leg>=2,Right=(Leg%2)==1;
                const FString Prefix=Right?TEXT("SKEL_R_"):TEXT("SKEL_L_");
                const FName A(*(Prefix+(Hind?TEXT("Thigh"):TEXT("UpperArm"))));
                const FName B(*(Prefix+(Hind?TEXT("Calf"):TEXT("Forearm"))));
                const FName C(*(Prefix+(Hind?TEXT("Foot"):TEXT("Hand"))));
                const int32 AI=Names.IndexOfByKey(A),BI=Names.IndexOfByKey(B),CI=Names.IndexOfByKey(C);
                if(AI<0||BI<0||CI<0)continue;
                const auto AC=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(AI)),BC=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(BI)),CC=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(CI));
                if(AC.GetInt()<0||BC.GetInt()<0||CC.GetInt()<0)continue;
                const float WalkOffset=(Right?PI:0)+(Hind?PI:0);
                const float RunOffset=(Hind?0:PI*.9f)+(Right?.22f:0);
                const float Cycle=Phase+FMath::Lerp(WalkOffset,RunOffset,Run);
                const float Swing=FMath::Sin(Cycle),Lift=FMath::Max(0.f,FMath::Cos(Cycle));
                FVector Target=RestComponent[CI].GetTranslation();
                Target.Y+=Swing*(Hind?3.1f:2.2f)*Activity;
                Target.Z+=(Climbing?0:PawGround[Leg])+Lift*FMath::Lerp(1.25f,3.f,Run)*Activity+Bounce*.6f*Run*Activity;
                FTransform Upper=Space.GetComponentSpaceTransform(AC),Lower=Space.GetComponentSpaceTransform(BC),End=Space.GetComponentSpaceTransform(CC);
                const FVector Pole=RestComponent[BI].GetTranslation()+FVector(0,Hind?8:-8,0);
                AnimationCore::SolveTwoBoneIK(Upper,Lower,End,Pole,Target,false,1,1);
                End.SetRotation(RestComponent[CI].GetRotation());
                TArray<FBoneTransform> Solved;Solved.Emplace(AC,Upper);Solved.Emplace(BC,Lower);Solved.Emplace(CC,End);Space.SafeSetCSBoneTransforms(Solved);
            }
            FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(Space,Output.Pose);
        }
        return true;
    }
};
FAnimInstanceProxy* UWildlineSquirrelAnim::CreateAnimInstanceProxy(){return new FWildlineSquirrelProxy(this);}
void UWildlineSquirrelAnim::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy){delete Proxy;}
void UWildlineSquirrelAnim::PrepareMesh(USkeletalMesh* Mesh){
#if WITH_EDITOR
    if(Mesh){Mesh->Modify();auto* LOD=Mesh->GetLODInfo(0);LOD->bAllowCPUAccess=true;LOD->BuildSettings.bRecomputeNormals=false;LOD->BuildSettings.bRecomputeTangents=true;LOD->BuildSettings.bComputeWeightedNormals=false;Mesh->PostEditChange();Mesh->MarkPackageDirty();}
#endif
}

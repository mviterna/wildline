#include "WildlineHunterAnim.h"
#include "WildlineCharacter.h"
#include "WildlineHunterMotionData.h"
#include "WildlineAudio.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimationPoseData.h"
#include "AnimationRuntime.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TwoBoneIK.h"
#include "Engine/World.h"

// Evaluate the body first, place the stock in its animated shoulder pocket,
// then solve both hands against that same weapon transform. Publish the weapon
// on the game thread after evaluation, avoiding a frame of attachment/IK lag.
struct FWildlineHunterProxy : public FAnimInstanceProxy {
    explicit FWildlineHunterProxy(UAnimInstance* Instance):FAnimInstanceProxy(Instance){}
    UAnimSequence* Idle=nullptr;UAnimSequence* Walk=nullptr;UAnimSequence* Run=nullptr;
    UAnimSequence* Jump=nullptr;UAnimSequence* JumpStart=nullptr;UAnimSequence* Land=nullptr;
    UAnimSequence* RifleReady=nullptr;UAnimSequence* CrouchIdle=nullptr;UAnimSequence* Transition=nullptr;UAnimSequence* Turn=nullptr;
    float Cycle=0,Move=0,Fast=0,Air=0,Elapsed=0,AirTime=0,AimPitch=-6,Crouch=0;
    float LastSpeed=0,VerticalSpeed=0,TransitionTime=10,TransitionStart=0,TransitionDuration=.4f,LandTime=10;
    float LastYaw=0,RootYaw=0,TurnTime=10,TurnStart=0,PelvisAdjust=0;
    bool bWasFalling=false,bYawInitialized=false,bGrounded=false,bStep=false;
    int32 StepSide=0,LastDirection=0;
    float DirectionWeights[8]={1,0,0,0,0,0,0,0};
    UAnimSequence* Walks[8]={};UAnimSequence* Runs[8]={};UAnimSequence* Crouches[8]={};
    FVector Hands[2],Poles[2];
    FVector Forward=FVector::YAxisVector,Right=FVector::XAxisVector,Butt;
    FQuat WeaponRotation=FQuat::Identity;
    FTransform WeaponPose=FTransform::Identity,SupportAction=FTransform::Identity;
    bool Shotgun=false,bWeaponPoseValid=false;
    FVector BarrelAxis=FVector::ForwardVector;
    TArray<FName> Names;
    TArray<bool> UpperBody;
    FVector FootTargets[2],FootNormals[2],LockedFeet[2];
    float FootAlpha[2]={0,0},FootFloorZ[2]={0,0},AnkleHeight[2]={13.5f,13.5f};
    bool FootLocked[2]={false,false},FootHit[2]={false,false};
    int32 FootSerial[2]={0,0};
    float HandScale=1,FootScale=1;
    bool bPreview=false;
    void ApplyProportions(FCSPose<FCompactPose>& Space,const FBoneContainer& Bones){
        const FName Parts[]={TEXT("hand_l"),TEXT("hand_r"),TEXT("foot_l"),TEXT("foot_r")};
        for(int32 Part=0;Part<4;Part++){
            const float Scale=Part<2?HandScale:FootScale;if(FMath::IsNearlyEqual(Scale,1.f))continue;
            const int32 Index=Names.IndexOfByKey(Parts[Part]);if(Index==INDEX_NONE)continue;
            const auto Compact=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Index));if(Compact.GetInt()==INDEX_NONE)continue;
            FTransform Bone=Space.GetComponentSpaceTransform(Compact);Bone.SetScale3D(Bone.GetScale3D()*Scale);
            // Ground IK already lowers the ankle for the smaller sole. Apply
            // the same offset in the studio and in airborne animation poses.
            if(Part>=2&&(!FootHit[Part-2]||Air>=.1f))Bone.AddToTranslation(FVector::UpVector*AnkleHeight[Part-2]*(Scale-1.f));
            TArray<FBoneTransform> Sized;Sized.Emplace(Compact,Bone);Space.SafeSetCSBoneTransforms(Sized);
        }
    }
    void PreUpdate(UAnimInstance* Instance,float Delta) override {
        FAnimInstanceProxy::PreUpdate(Instance,Delta);
        auto* Anim=Cast<UWildlineHunterAnim>(Instance);
        auto* Pawn=Cast<AWildlineCharacter>(Instance->GetOwningActor());
        auto* Mesh=Instance->GetSkelMeshComponent();if(!Anim||!Pawn||!Mesh)return;
        const float BodyScale=FMath::Max(static_cast<float>(Mesh->GetComponentScale().Z),.1f);
        bPreview=Pawn->bPreviewHunter;HandScale=Pawn->HunterProfile.HandScale();FootScale=Pawn->HunterProfile.FootScale();
        Idle=Anim->Idle;Walk=Anim->Walk;Run=Anim->Run;RifleReady=bPreview?nullptr:Anim->RifleReady;CrouchIdle=Anim->CrouchIdle;
        JumpStart=Anim->JumpStart;Land=Anim->Land;
        const auto* Movement=Pawn->GetCharacterMovement();
        const FVector LocalVelocity=Pawn->GetActorTransform().InverseTransformVectorNoScale(Pawn->GetVelocity());
        // Blend neighboring headings and ease weight changes, including across
        // the backward/forward wrap. Never swap a live clip at a sector edge.
        const float Heading=FMath::Fmod(FMath::Atan2(LocalVelocity.Y,LocalVelocity.X)/ (PI/4.f)+8.f,8.f);
        // A double just below eight can round to 8.0 when stored as float.
        // Wrap the index as well as the angle so forward never loses all eight
        // weights after a turn (and silently falls back to the reference pose).
        const int32 Sector=FMath::FloorToInt(Heading),A=Sector%8,B=(A+1)%8;
        const float Fraction=Heading-Sector;
        const float Ease=1-FMath::Exp(-12.f*Delta);
        for(int32 I=0;I<8;I++){
            Walks[I]=Anim->WalkDirections.IsValidIndex(I)?Anim->WalkDirections[I]:Walk;
            Runs[I]=Anim->RunDirections.IsValidIndex(I)?Anim->RunDirections[I]:Run;
            Crouches[I]=Anim->CrouchDirections.IsValidIndex(I)?Anim->CrouchDirections[I]:Walk;
            if(LocalVelocity.SizeSquared2D()>4){
                const float Target=I==A?1-Fraction:I==B?Fraction:0;
                DirectionWeights[I]=FMath::Lerp(DirectionWeights[I],Target,Ease);
            }
        }
        Elapsed+=Delta;
        const bool Falling=Movement->IsFalling();bGrounded=Movement->IsMovingOnGround();
        AirTime=Falling?AirTime+Delta:0;
        if(bWasFalling&&!Falling)LandTime=0;else LandTime+=Delta;
        bWasFalling=Falling;VerticalSpeed=Pawn->GetVelocity().Z;
        Jump=Anim->JumpPose;Air=FMath::FInterpTo(Air,Falling?1.f:0.f,Delta,14);
        Crouch=FMath::FInterpTo(Crouch,Pawn->bIsCrouched?1.f:0.f,Delta,10);
        if(Names.IsEmpty()&&Mesh->GetSkeletalMeshAsset()){
            const auto& Ref=Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();
            const int32 Chest=Ref.FindBoneIndex(TEXT("spine_02"));
            for(int32 I=0;I<Ref.GetNum();I++){
                Names.Add(Ref.GetBoneName(I));int32 Ancestor=I;
                while(Ancestor>0&&Ancestor!=Chest)Ancestor=Ref.GetParentIndex(Ancestor);
                UpperBody.Add(Ancestor==Chest);
            }
            for(int32 Side=0;Side<2;Side++){
                int32 Index=Ref.FindBoneIndex(Side==0?TEXT("foot_l"):TEXT("foot_r"));FTransform Rest=FTransform::Identity;
                while(Index>=0){Rest=Rest*Ref.GetRefBonePose()[Index];Index=Ref.GetParentIndex(Index);}
                AnkleHeight[Side]=Rest.GetLocation().Z;
            }
        }
        // Clips and strides are authored in rig centimetres, so normalize
        // world travel for shorter bodies before advancing the gait.
        const float Speed=Pawn->GetVelocity().Size2D()/BodyScale;
        Move=FMath::FInterpTo(Move,FMath::Clamp(Speed/100.f,0.f,1.f),Delta,10);
        Fast=FMath::FInterpTo(Fast,FMath::Clamp((Speed-220.f)/220.f,0.f,1.f),Delta,8);
        float Stride=0;
        for(int32 I=0;I<8;I++)Stride+=DirectionWeights[I]*FMath::Lerp(FMath::Lerp(WildlineMotion::Walk[I].Stride,WildlineMotion::Run[I].Stride,Fast),WildlineMotion::Crouch[I].Stride,Crouch);
        const float NextCycle=Cycle+Delta*Speed/FMath::Max(Stride,50.f);
        bStep=bGrounded&&Speed>25&&FMath::FloorToInt(Cycle*2)!=FMath::FloorToInt(NextCycle*2);
        StepSide=FMath::FloorToInt(NextCycle*2)%2;Cycle=FMath::Fmod(NextCycle,1.f);
        TransitionTime+=Delta;
        auto BeginTransition=[&](bool Start){
            const bool Running=Start?Pawn->bSprinting:LastSpeed>360;
            const auto& Clips=Running?(Start?Anim->RunStarts:Anim->RunStops):(Start?Anim->WalkStarts:Anim->WalkStops);
            const auto& Timing=Running?(Start?WildlineMotion::RunStart[LastDirection]:WildlineMotion::RunStop[LastDirection]):(Start?WildlineMotion::WalkStart[LastDirection]:WildlineMotion::WalkStop[LastDirection]);
            Transition=Clips.IsValidIndex(LastDirection)?Clips[LastDirection]:nullptr;
            TransitionTime=0;TransitionStart=Timing.Start;TransitionDuration=FMath::Min(Timing.Duration,.55f);
        };
        if(Speed>20)LastDirection=FMath::RoundToInt(Heading)%8;
        if(bGrounded&&Crouch<.1f){
            if(LastSpeed<20&&Speed>=20)BeginTransition(true);
            if(LastSpeed>20&&Speed<=20)BeginTransition(false);
        }
        LastSpeed=Speed;
        const FTransform Body=Mesh->GetComponentTransform();
        Forward=Body.InverseTransformVectorNoScale(Pawn->GetActorForwardVector()).GetSafeNormal();
        Right=Body.InverseTransformVectorNoScale(Pawn->GetActorRightVector()).GetSafeNormal();
        const float Yaw=Pawn->GetActorRotation().Yaw;
        const float YawDelta=bYawInitialized?FMath::UnwindDegrees(Yaw-LastYaw):0;
        LastYaw=Yaw;bYawInitialized=true;TurnTime+=Delta;
        if(bGrounded&&Speed<20&&Crouch<.1f){
            RootYaw=FMath::Clamp(RootYaw-YawDelta,-65.f,65.f);
            if(FMath::Abs(RootYaw)>35&&TurnTime>=.65f){
                const bool Left=RootYaw>0;Turn=Left?Anim->TurnLeft:Anim->TurnRight;
                TurnStart=Left?WildlineMotion::TurnLeft.Start:WildlineMotion::TurnRight.Start;TurnTime=0;
                FootLocked[0]=FootLocked[1]=false;
            }
            if(TurnTime<.65f)RootYaw=FMath::FInterpTo(RootYaw,0.f,Delta,5);
        }else RootYaw=FMath::FInterpTo(RootYaw,0.f,Delta,12);
        // Terrain queries run on the game thread. Evaluation only consumes the
        // captured samples, and pins the stance foot in world space.
        for(int32 Side=0;Side<2;Side++){
            const FVector Foot=Mesh->GetSocketLocation(Side==0?TEXT("foot_l"):TEXT("foot_r"));
            FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(HunterFootPlacement),false,Pawn);
            FootHit[Side]=bGrounded&&Pawn->GetWorld()->LineTraceSingleByChannel(Hit,Foot+FVector::UpVector*(50*BodyScale),Foot-FVector::UpVector*(75*BodyScale),ECC_Visibility,Query)&&Hit.ImpactNormal.Z>.55f;
            if(!FootHit[Side]){FootLocked[Side]=false;FootAlpha[Side]=FMath::FInterpTo(FootAlpha[Side],0.f,Delta,18);continue;}
            const FVector Floor=Hit.ImpactPoint+FVector::UpVector*(AnkleHeight[Side]*FootScale*BodyScale);
            FootFloorZ[Side]=Body.InverseTransformPosition(Floor).Z;
            FootNormals[Side]=Body.InverseTransformVectorNoScale(Hit.ImpactNormal).GetSafeNormal();
            const float Phase=FMath::Fmod(Cycle+(Side==0?0.f:.5f),1.f);
            // Pin the heel contact, then release for the captured toe-off.
            // Holding the ankle through toe-off overextends the knee and drags
            // the foot; the last part of stance must be free to roll and lift.
            // Let the captured landing absorb impact before locking the heel.
            // Pinning during that changing squat can overextend shorter legs.
            const bool Stance=LandTime>=.4f&&TransitionTime>=TransitionDuration&&(Speed<20?TurnTime>=.65f:Phase<.30f);
            if(Stance&&!FootLocked[Side]){LockedFeet[Side]=Floor;FootSerial[Side]++;FootAlpha[Side]=0;}
            FootLocked[Side]=Stance;
            if(FVector::Dist2D(LockedFeet[Side],Foot)>32*BodyScale){FootLocked[Side]=false;FootAlpha[Side]=0;}
            LockedFeet[Side].Z=Floor.Z;
            FootTargets[Side]=Body.InverseTransformPosition(LockedFeet[Side]);
            const float Plant=Speed<20?1.f:1-FMath::Clamp((Phase-.20f)/.10f,0.f,1.f);
            FootAlpha[Side]=FMath::FInterpConstantTo(FootAlpha[Side],FootLocked[Side]?Plant:0.f,Delta,15);
        }
        const float FloorShift=FootHit[0]&&FootHit[1]?FMath::Clamp(FMath::Min(FootFloorZ[0]-AnkleHeight[0]*FootScale,FootFloorZ[1]-AnkleHeight[1]*FootScale),-20.f,15.f):0;
        PelvisAdjust=FMath::FInterpTo(PelvisAdjust,FloorShift,Delta,10);
        const float ControlPitch=Pawn->IsLocallyControlled()?Pawn->GetControlRotation().Pitch:Pawn->GetBaseAimRotation().Pitch;
        const float TargetPitch=Pawn->bAiming||Pawn->IsScoped()?FMath::Clamp(FRotator::NormalizeAxis(ControlPitch),-35.f,40.f):-6.f;
        AimPitch=FMath::FInterpTo(AimPitch,TargetPitch,Delta,12);
        const float ShotAge=Pawn->ServerTime()-Pawn->VisualShotAt;
        const float Recoil=ShotAge>=0&&ShotAge<.18f?FMath::Sin(ShotAge/.18f*PI)*(Pawn->SelectedWeapon==1?3.f:1.7f):0;
        WeaponRotation=(Body.GetRotation().Inverse()*FRotator(AimPitch+Recoil,Pawn->GetActorRotation().Yaw,0).Quaternion()).GetNormalized();
        Shotgun=Pawn->SelectedWeapon==1;
        Butt=FVector(-45.6f,0,-4.f);
        if(Shotgun&&Pawn->WorldRifle->GetStaticMesh())Butt=FVector(Pawn->WorldRifle->GetStaticMesh()->GetBoundingBox().Min.X+1.f,0,-4.8f);
        SupportAction=Pawn->WorldBolt->GetRelativeTransform();
    }
    bool Evaluate(FPoseContext& Output) override {
        bWeaponPoseValid=false;
        Output.Pose.ResetToRefPose();if(!Idle||!Walk||!Run)return true;
        FAnimationPoseData IdleData(Output);
        auto Sample=[](UAnimSequence* Clip,FPoseContext& Pose,float Time){
            Pose.ResetToRefPose();if(!Clip)return;
            FAnimationPoseData Data(Pose);Clip->GetAnimationPose(Data,FAnimExtractContext(FMath::Clamp(Time,0.f,Clip->GetPlayLength()),false));
            // Captures can start away from the origin. Every in-place layer
            // shares the capsule origin, including start/stop and landing.
            auto& Root=Pose.Pose[FCompactPoseBoneIndex(0)];Root.SetTranslation(FVector::ZeroVector);Root.SetRotation(FQuat::Identity);
        };
        auto Overlay=[&](UAnimSequence* Clip,float Time,float Alpha){
            if(!Clip||Alpha<.001f)return;FPoseContext Pose(Output);Sample(Clip,Pose,Time);FAnimationPoseData Data(Pose);
            FAnimationRuntime::BlendTwoPosesTogetherInPlace(IdleData,Data,1-FMath::Clamp(Alpha,0.f,1.f));
        };
        Sample(Idle,Output,FMath::Fmod(Elapsed,FMath::Max(Idle->GetPlayLength(),.001f)));
        Overlay(CrouchIdle,FMath::Fmod(Elapsed,10.f),Crouch);
        FPoseContext Moving(Output),Running(Output),Crouching(Output);
        Moving.ResetToRefPose();Running.ResetToRefPose();Crouching.ResetToRefPose();
        FAnimationPoseData MovingData(Moving),RunningData(Running),CrouchingData(Crouching);
        float Accumulated=0;
        for(int32 I=0;I<8;I++){
            if(DirectionWeights[I]<.0001f)continue;
            auto* W=Walks[I]?Walks[I]:Walk;auto* R=Runs[I]?Runs[I]:Run;
            FPoseContext WContext(Output),RContext(Output),CContext(Output);FAnimationPoseData WData(WContext),RData(RContext),CData(CContext);
            Sample(W,WContext,WildlineMotion::Walk[I].Offset+Cycle*WildlineMotion::Walk[I].Period);
            Sample(R,RContext,WildlineMotion::Run[I].Offset+Cycle*WildlineMotion::Run[I].Period);
            Sample(Crouches[I],CContext,WildlineMotion::Crouch[I].Offset+Cycle*WildlineMotion::Crouch[I].Period);
            const float Weight=Accumulated/(Accumulated+DirectionWeights[I]);
            FAnimationRuntime::BlendTwoPosesTogetherInPlace(MovingData,WData,Weight);
            FAnimationRuntime::BlendTwoPosesTogetherInPlace(RunningData,RData,Weight);
            FAnimationRuntime::BlendTwoPosesTogetherInPlace(CrouchingData,CData,Weight);
            Accumulated+=DirectionWeights[I];
        }
        FAnimationRuntime::BlendTwoPosesTogetherInPlace(MovingData,RunningData,1-Fast);
        FAnimationRuntime::BlendTwoPosesTogetherInPlace(MovingData,CrouchingData,1-Crouch);
        FAnimationRuntime::BlendTwoPosesTogetherInPlace(IdleData,MovingData,1-Move);
        if(Transition&&TransitionTime<TransitionDuration&&Air<.1f&&Crouch<.1f){
            const float Alpha=FMath::Min(FMath::Clamp(TransitionTime/.08f,0.f,1.f),FMath::Clamp((TransitionDuration-TransitionTime)/.16f,0.f,1.f));
            Overlay(Transition,TransitionStart+TransitionTime,Alpha);
        }
        if(Turn&&TurnTime<.65f&&Move<.2f&&Crouch<.1f)Overlay(Turn,TurnStart+TurnTime*1.35f,FMath::Sin(TurnTime/.65f*PI)*.85f);
        if(Air>.001f){
            Overlay(Jump,FMath::Fmod(AirTime,FMath::Max(Jump?Jump->GetPlayLength():1.f,.001f)),Air*(1-Crouch));
            // Stepping off a ledge in crouch must retain a compact body. A
            // standing fall pose would expand it beyond the crouched capsule.
            Overlay(CrouchIdle,FMath::Fmod(Elapsed,10.f),Air*Crouch);
            if(JumpStart&&AirTime<.35f&&VerticalSpeed>0)Overlay(JumpStart,WildlineMotion::JumpStart.Start+AirTime,Air*(1-Crouch)*(1-FMath::Clamp((AirTime-.15f)/.2f,0.f,1.f)));
        }
        if(Land&&LandTime<.4f&&Air<.3f)Overlay(Land,WildlineMotion::Land.Start+LandTime,FMath::Sin(LandTime/.4f*PI)*.75f*(1-Crouch));
        if(FMath::Abs(RootYaw)>.01f){auto& Root=Output.Pose[FCompactPoseBoneIndex(0)];Root.SetRotation((FQuat(FVector::UpVector,FMath::DegreesToRadians(RootYaw))*Root.GetRotation()).GetNormalized());}
        FCSPose<FCompactPose> Space;Space.InitPose(Output.Pose);
        const auto& Bones=Output.Pose.GetBoneContainer();
        // A rifle pose has a different pelvis orientation from neutral capture.
        // Layer the held weapon in component space, preserving the captured
        // chest position but not inheriting that incompatible parent rotation.
        if(RifleReady){
            FPoseContext Upper(Output);Sample(RifleReady,Upper,FMath::Fmod(Elapsed,RifleReady->GetPlayLength()));
            FCSPose<FCompactPose> UpperSpace;UpperSpace.InitPose(Upper.Pose);
            const auto Chest=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Names.IndexOfByKey(FName(TEXT("spine_02")))));
            if(Chest.GetInt()!=INDEX_NONE){
                const FVector Shift=Space.GetComponentSpaceTransform(Chest).GetLocation()-UpperSpace.GetComponentSpaceTransform(Chest).GetLocation();
                TArray<FBoneTransform> Held;
                for(int32 I=0;I<Names.Num();I++)if(UpperBody[I]){
                    const auto Compact=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(I));if(Compact.GetInt()==INDEX_NONE)continue;
                    FTransform Target=UpperSpace.GetComponentSpaceTransform(Compact);Target.AddToTranslation(Shift);
                    FTransform Blended;Blended.Blend(Space.GetComponentSpaceTransform(Compact),Target,.96f);Held.Emplace(Compact,Blended);
                }
                Space.SafeSetCSBoneTransforms(Held);
            }
        }
        const int32 PelvisIndex=Names.IndexOfByKey(TEXT("pelvis"));
        if(PelvisIndex!=INDEX_NONE&&FMath::Abs(PelvisAdjust)>.01f){
            const auto Compact=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(PelvisIndex));
            if(Compact.GetInt()!=INDEX_NONE){FTransform Pelvis=Space.GetComponentSpaceTransform(Compact);Pelvis.AddToTranslation(FVector::UpVector*PelvisAdjust);TArray<FBoneTransform> Shift;Shift.Emplace(Compact,Pelvis);Space.SafeSetCSBoneTransforms(Shift);}
        }
        for(int32 Side=0;Side<2;Side++)if(FootHit[Side]&&Air<.1f){
            const TCHAR* Chains[2][3]={{TEXT("thigh_l"),TEXT("calf_l"),TEXT("foot_l")},{TEXT("thigh_r"),TEXT("calf_r"),TEXT("foot_r")}};
            FCompactPoseBoneIndex Indices[3];bool Valid=true;
            for(int32 J=0;J<3;J++){const int32 Index=Names.IndexOfByKey(Chains[Side][J]);if(Index==INDEX_NONE){Valid=false;break;}Indices[J]=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Index));Valid&=Indices[J].GetInt()!=INDEX_NONE;}
            if(!Valid)continue;
            FTransform Hip=Space.GetComponentSpaceTransform(Indices[0]),Knee=Space.GetComponentSpaceTransform(Indices[1]),Foot=Space.GetComponentSpaceTransform(Indices[2]);
            FVector FloorTarget=Foot.GetLocation();FloorTarget.Z+=FMath::Clamp(FootFloorZ[Side]-AnkleHeight[Side],-25.f,25.f);
            const FVector Target=FMath::Lerp(FloorTarget,FootTargets[Side],FootAlpha[Side]);
            AnimationCore::SolveTwoBoneIK(Hip,Knee,Foot,Knee.GetLocation()+Forward*25.f,Target,false,1,1);
            const float Contact=FMath::Clamp(1-(Foot.GetLocation().Z-FootFloorZ[Side])/12.f,0.f,1.f);
            const FQuat Tilt=FQuat::FindBetweenNormals(FVector::UpVector,FootNormals[Side]);
            Foot.SetRotation(FQuat::Slerp(Foot.GetRotation(),Tilt*Foot.GetRotation(),Contact).GetNormalized());
            TArray<FBoneTransform> Solved;Solved.Emplace(Indices[0],Hip);Solved.Emplace(Indices[1],Knee);Solved.Emplace(Indices[2],Foot);Space.SafeSetCSBoneTransforms(Solved);
        }
        if(bPreview){
            ApplyProportions(Space,Bones);
            FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(Space,Output.Pose);
            return true;
        }
        // Share vertical aiming across the chest, neck and head so the hunter
        // follows the sight instead of raising only the arms under a bowed head.
        const FName AimBones[]={TEXT("spine_02"),TEXT("spine_03"),TEXT("neck_01"),TEXT("head")};
        const float AimShares[]={.15f,.20f,.25f,.40f};
        for(int32 I=0;I<4;I++){
            const int32 Index=Names.IndexOfByKey(AimBones[I]);if(Index==INDEX_NONE)continue;
            const auto Compact=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Index));if(Compact.GetInt()==INDEX_NONE)continue;
            FTransform Bone=Space.GetComponentSpaceTransform(Compact);
            Bone.SetRotation((FQuat(Right,-FMath::DegreesToRadians((AimPitch+6.f)*AimShares[I]))*Bone.GetRotation()).GetNormalized());
            TArray<FBoneTransform> Turned;Turned.Emplace(Compact,Bone);Space.SafeSetCSBoneTransforms(Turned);
        }
        const int32 ShoulderIndex=Names.IndexOfByKey(FName(TEXT("upperarm_r")));
        if(ShoulderIndex==INDEX_NONE)return true;
        const auto ShoulderCompact=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(ShoulderIndex));
        if(ShoulderCompact.GetInt()==INDEX_NONE)return true;
        // The pocket sits below and just medial to the shoulder joint, on the
        // padded upper chest rather than on top of the deltoid.
        const FVector Shoulder=Space.GetComponentSpaceTransform(ShoulderCompact).GetLocation()+Forward*5.f-Right*3.f-FVector::UpVector*6.f;
        WeaponPose=FTransform(WeaponRotation,Shoulder-WeaponRotation.RotateVector(Butt));
        bWeaponPoseValid=true;
        BarrelAxis=WeaponRotation.GetForwardVector();
        Hands[0]=WeaponPose.TransformPosition(FVector(-17.f,0,Shotgun?-4.8f:-2.8f));
        const FTransform Support=Shotgun?SupportAction*WeaponPose:WeaponPose;
        Hands[1]=Support.TransformPosition(FVector(Shotgun?18.5f:17.f,-1.f,-3.f));
        Poles[0]=Shoulder+Right*30.f- FVector::UpVector*30.f+Forward*8.f;
        Poles[1]=Shoulder-Right*55.f-FVector::UpVector*28.f+Forward*15.f;
        for(int32 Side=0;Side<2;Side++){
            const TCHAR* Chain[2][3]={{TEXT("upperarm_r"),TEXT("lowerarm_r"),TEXT("hand_r")},{TEXT("upperarm_l"),TEXT("lowerarm_l"),TEXT("hand_l")}};
            FCompactPoseBoneIndex Indices[3];bool Valid=true;
            for(int32 J=0;J<3;J++){
                const int32 Index=Names.IndexOfByKey(FName(Chain[Side][J]));
                if(Index==INDEX_NONE){Valid=false;break;}
                Indices[J]=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Index));
                Valid&=Indices[J].GetInt()!=INDEX_NONE;
            }
            if(!Valid)continue;
            FTransform Upper=Space.GetComponentSpaceTransform(Indices[0]),Lower=Space.GetComponentSpaceTransform(Indices[1]),End=Space.GetComponentSpaceTransform(Indices[2]);
            AnimationCore::SolveTwoBoneIK(Upper,Lower,End,Poles[Side],Hands[Side],false,1,1);
            End.SetRotation((FQuat(Right,-FMath::DegreesToRadians(AimPitch))*End.GetRotation()).GetNormalized());
            TArray<FBoneTransform> Solved;Solved.Emplace(Indices[0],Upper);Solved.Emplace(Indices[1],Lower);Solved.Emplace(Indices[2],End);
            Space.SafeSetCSBoneTransforms(Solved);
        }
        ApplyProportions(Space,Bones);
        FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(Space,Output.Pose);
        // Close the fingers around the narrow sporting stock instead of leaving
        // the mannequin's open support hands underneath it.
        for(int32 Side=0;Side<2;Side++)for(const TCHAR* Finger:{TEXT("index"),TEXT("middle"),TEXT("ring"),TEXT("pinky")})for(int32 Joint=1;Joint<=3;Joint++){
            const FName Name(*FString::Printf(TEXT("%s_0%d_%s"),Finger,Joint,Side==0?TEXT("r"):TEXT("l")));
            const int32 Index=Names.IndexOfByKey(Name);if(Index==INDEX_NONE)continue;
            const auto Compact=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Index));if(Compact.GetInt()==INDEX_NONE)continue;
            const FVector Axis=Space.GetComponentSpaceTransform(Compact).GetRotation().Inverse().RotateVector(BarrelAxis);
            const float Bend=Side==0&&FCString::Strcmp(Finger,TEXT("index"))==0?22.f:Joint==1?42.f:55.f;
            auto& T=Output.Pose[Compact];T.SetRotation((T.GetRotation()*FQuat(Axis,FMath::DegreesToRadians(Bend*(Side==0?-1.f:1.f)))).GetNormalized());
        }
        return true;
    }
};
void UWildlineHunterAnim::NativeInitializeAnimation(){
    Super::NativeInitializeAnimation();
    auto Load=[](const FString& Name){return LoadObject<UAnimSequence>(nullptr,*FString::Printf(TEXT("/Game/Wildline/Hunters/Animations/A_Hunter_%s.A_Hunter_%s"),*Name,*Name));};
    Idle=Load(TEXT("Idle"));CrouchIdle=Load(TEXT("CrouchIdle"));Walk=Load(TEXT("Walk_Fwd"));Run=Load(TEXT("Run_Fwd"));
    RifleReady=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS.MF_Rifle_Idle_ADS"));
    JumpPose=Load(TEXT("Fall"));JumpStart=Load(TEXT("JumpStart"));Land=Load(TEXT("Land"));TurnLeft=Load(TEXT("TurnLeft"));TurnRight=Load(TEXT("TurnRight"));
    WalkDirections.Empty();RunDirections.Empty();CrouchDirections.Empty();WalkStarts.Empty();WalkStops.Empty();RunStarts.Empty();RunStops.Empty();
    const TCHAR* Directions[]={TEXT("Fwd"),TEXT("FwdRight"),TEXT("Right"),TEXT("BwdRight"),TEXT("Bwd"),TEXT("BwdLeft"),TEXT("Left"),TEXT("FwdLeft")};
    for(const auto* D:Directions){
        WalkDirections.Add(Load(FString(TEXT("Walk_"))+D));RunDirections.Add(Load(FString(TEXT("Run_"))+D));CrouchDirections.Add(Load(FString(TEXT("Crouch_"))+D));
        WalkStarts.Add(Load(FString(TEXT("WalkStart_"))+D));WalkStops.Add(Load(FString(TEXT("WalkStop_"))+D));
        RunStarts.Add(Load(FString(TEXT("RunStart_"))+D));RunStops.Add(Load(FString(TEXT("RunStop_"))+D));
    }
    bCapturedMotion=Idle&&Walk&&Run&&CrouchIdle&&JumpStart&&Land&&TurnLeft&&TurnRight;
}
FAnimInstanceProxy* UWildlineHunterAnim::CreateAnimInstanceProxy(){return new FWildlineHunterProxy(this);}
void UWildlineHunterAnim::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy){delete Proxy;}
void UWildlineHunterAnim::NativePostEvaluateAnimation(){
    Super::NativePostEvaluateAnimation();
    const auto& Proxy=GetProxyOnGameThread<FWildlineHunterProxy>();
    GaitPhase=Proxy.Cycle;for(int32 Side=0;Side<2;Side++){FootPlantWeight[Side]=Proxy.FootAlpha[Side];FootPlantSerial[Side]=Proxy.FootSerial[Side];}
    CrouchBlend=Proxy.Crouch;MovementBlend=Proxy.Move;GroundOffset=Proxy.PelvisAdjust;
    auto* Pawn=Cast<AWildlineCharacter>(GetOwningActor());
    if(Pawn&&!Pawn->bSquirrel&&!Pawn->bPreviewHunter&&Proxy.bWeaponPoseValid){
        Pawn->WorldRifle->SetWorldTransform(Proxy.WeaponPose*GetSkelMeshComponent()->GetComponentTransform());
        if(Proxy.bStep&&!Pawn->bDead)WildlineAudio::Play(Pawn,WildlineAudio::Footstep,GetSkelMeshComponent()->GetSocketLocation(Proxy.StepSide==0?TEXT("foot_l"):TEXT("foot_r")),false,Pawn->bSprinting?WildlineAudio::RunningFootstepRate:1.f);
    }
}

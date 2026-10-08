#include "WildlineLivestock.h"
#include "WildlineCharacter.h"
#include "WildlineGame.h"
#include "WildlineRules.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstanceProxy.h"
#include "WildlineAudio.h"

AWildlineLivestock::AWildlineLivestock(){
    bReplicates=true;SetReplicateMovement(true);PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Livestock root"));
    Body=CreateDefaultSubobject<UCapsuleComponent>(TEXT("Solid livestock body"));Body->SetupAttachment(RootComponent);Body->InitCapsuleSize(43,115);Body->SetRelativeRotation(FRotator(90,0,0));Body->SetCollisionProfileName(TEXT("BlockAll"));
    Mesh=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Textured livestock"));Mesh->SetupAttachment(RootComponent);Mesh->SetRelativeLocation(FVector(0,0,-85));Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void AWildlineLivestock::BeginPlay(){Super::BeginPlay();OnRepSpecies();}
void AWildlineLivestock::OnRepSpecies(){Mesh->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,Species==1?TEXT("/Game/Wildline/Farm/Meshes/SK_Horse.SK_Horse"):TEXT("/Game/Wildline/Farm/Meshes/SK_Cow.SK_Cow")));Mesh->SetAnimInstanceClass(UWildlineLivestockAnim::StaticClass());}
void AWildlineLivestock::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AWildlineLivestock,Species);DOREPLIFETIME(AWildlineLivestock,Home);DOREPLIFETIME(AWildlineLivestock,Phase);DOREPLIFETIME(AWildlineLivestock,WalkSpeed);DOREPLIFETIME(AWildlineLivestock,StrikeAt);}
float AWildlineLivestock::ServerTime() const{const auto* State=GetWorld()->GetGameState();return State?State->GetServerWorldTimeSeconds():GetWorld()->GetTimeSeconds();}
void AWildlineLivestock::Tick(float Delta){
    Super::Tick(Delta);if(!HasAuthority())return;
    const auto* State=GetWorld()->GetGameState<AWildlineGameState>();if(!State||!State->bRoundStarted||State->Winner){WalkSpeed=0;return;}
    const float Now=ServerTime();
    auto Exposed=[&](AWildlineCharacter* P){
        if(!State->MatchConfig.bLivestockDamage||!P||!P->bSquirrel||P->bDead||FVector::Dist2D(P->GetActorLocation(),GetActorLocation())>180||P->GetActorLocation().Z-WildlineRules::FarmGround(P->GetActorLocation().X,P->GetActorLocation().Y)>70)return false;
        FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(LivestockSight),false,this);
        return !GetWorld()->LineTraceSingleByChannel(Hit,GetActorLocation()-FVector(0,0,55),P->GetActorLocation(),ECC_Visibility,Query)||Hit.GetActor()==P;
    };
    if(!bStrikeDone){
        WalkSpeed=0;
        if(Now>=StrikeAt){bStrikeDone=true;ReadyAt=Now+2.5f;if(Exposed(Threat.Get())){auto* P=Threat.Get();UGameplayStatics::ApplyDamage(P,Species==1?22.f:18.f,nullptr,this,UDamageType::StaticClass());if(!P->bDead)P->LaunchCharacter((P->GetActorLocation()-GetActorLocation()).GetSafeNormal2D()*160+FVector(0,0,85),true,true);}Threat.Reset();}
        return;
    }
    if(Now>=ReadyAt)for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(Exposed(*It)){Threat=*It;StrikeAt=Now+.9f;bStrikeDone=false;WalkSpeed=0;MulticastWarning();ForceNetUpdate();return;}
    // Slow grazing loops remain in the yard; nearby walls and fences stop travel.
    const float Angle=Now*.045f+Phase;FVector Target=Home+FVector(FMath::Cos(Angle)*340,FMath::Sin(Angle)*230,0);
    const FVector Direction=(Target-GetActorLocation()).GetSafeNormal2D();WalkSpeed=FVector::Dist2D(Target,GetActorLocation())>40?(Species==1?72.f:48.f):0.f;
    const FRotator Facing(0,Direction.Rotation().Yaw,0);SetActorRotation(FMath::RInterpTo(GetActorRotation(),Facing,Delta,1.8f));
    FVector Next=GetActorLocation()+GetActorForwardVector()*WalkSpeed*Delta;Next.Z=WildlineRules::FarmGround(Next.X,Next.Y)+85;
    FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(LivestockMove),false,this);
    const FQuat Axis=GetActorQuat()*FQuat(FVector::RightVector,PI*.5f);
    if(!GetWorld()->SweepSingleByChannel(Hit,GetActorLocation(),Next,Axis,ECC_Pawn,FCollisionShape::MakeCapsule(43,115),Query))SetActorLocation(Next);else WalkSpeed=0;
}

void AWildlineLivestock::MulticastWarning_Implementation(){
    WildlineAudio::Play(this,Species==1?WildlineAudio::Horse:WildlineAudio::Cow,GetActorLocation());
}

struct FWildlineLivestockProxy : FAnimInstanceProxy {
    explicit FWildlineLivestockProxy(UAnimInstance* Instance):FAnimInstanceProxy(Instance){}
    float Time=0,Speed=0,Kick=0;TArray<FName> Names;
    void PreUpdate(UAnimInstance* Instance,float Delta) override{
        FAnimInstanceProxy::PreUpdate(Instance,Delta);auto* P=Cast<AWildlineLivestock>(Instance->GetOwningActor());if(!P)return;
        Time=P->ServerTime()+P->Phase;Speed=FMath::FInterpTo(Speed,P->WalkSpeed/72.f,Delta,6);
        const float Age=P->ServerTime()-P->StrikeAt;Kick=P->StrikeAt>0&&Age>-.9f&&Age<.65f?FMath::Sin(FMath::Clamp((Age+.9f)/1.55f,0.f,1.f)*PI):0;
        if(Names.IsEmpty()&&P->Mesh->GetSkeletalMeshAsset()){const auto& Ref=P->Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();for(int32 I=0;I<Ref.GetNum();I++)Names.Add(Ref.GetBoneName(I));}
    }
    bool Evaluate(FPoseContext& Output) override{
        Output.Pose.ResetToRefPose();const auto& Bones=Output.Pose.GetBoneContainer();
        auto Rotate=[&](const TCHAR* Name,FVector Axis,float Angle){const int32 Index=Names.IndexOfByKey(FName(Name));if(Index<0)return;const auto Compact=Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Index));if(Compact.GetInt()!=INDEX_NONE){auto& Transform=Output.Pose[Compact];Transform.SetRotation(Transform.GetRotation()*FQuat(Axis,Angle));}};
        for(int32 I=0;I<4;I++){const TCHAR* Upper[]={TEXT("LegFL"),TEXT("LegFR"),TEXT("LegBL"),TEXT("LegBR")};const TCHAR* Lower[]={TEXT("KneeFL"),TEXT("KneeFR"),TEXT("KneeBL"),TEXT("KneeBR")};const float Wave=FMath::Sin(Time*2.7f+(I==0||I==3?0:PI));Rotate(Upper[I],FVector::RightVector,Wave*Speed*.20f+(I>=2?-Kick*.45f:0));Rotate(Lower[I],FVector::RightVector,FMath::Max(0.f,-Wave)*Speed*.27f);}
        Rotate(TEXT("Head"),FVector::RightVector,FMath::Sin(Time*.75f)*.035f+Kick*.13f);Rotate(TEXT("Tail"),FVector::UpVector,FMath::Sin(Time*1.4f)*.16f);Rotate(TEXT("Body"),FVector::ForwardVector,FMath::Sin(Time*1.6f)*.006f);
        return true;
    }
};
FAnimInstanceProxy* UWildlineLivestockAnim::CreateAnimInstanceProxy(){return new FWildlineLivestockProxy(this);}
void UWildlineLivestockAnim::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy){delete Proxy;}

#include "WildlineCharacter.h"
#include "WildlineLake.h"
#include "WildlineWorld.h"
#include "WildlineLobby.h"
#include "WildlineGame.h"
#include "WildlineMenu.h"
#include "WildlineAudio.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"

bool AWildlineCharacter::IsHeadUnderwater()const{
    float Surface=0;return !bSquirrel&&AWildlineLake::WaterAt(GetWorld(),GetActorLocation(),Surface)&&GetActorLocation().Z+72*HunterProfile.HeightScale()<Surface;
}
void AWildlineCharacter::StopSwimRise(){bSwimRise=false;StopJumping();}
void AWildlineCharacter::Drown(){
    if(!HasAuthority()||bDead||!bSquirrel)return;
    ReturnCarriedPeanuts();Bag=0;Health=0;bDead=true;bSprinting=false;TreeIndex=INDEX_NONE;RespawnAt=ServerTime()+MatchRules().RespawnSeconds;OnRepDead();ForceNetUpdate();
    if(bInLobby){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[this](){bDead=false;Health=MatchRules().SquirrelHealth;RespawnAt=0;SetActorLocation(AWildlineLobby::Spawn(true),false,nullptr,ETeleportType::TeleportPhysics);OnRepDead();ForceNetUpdate();}),MatchRules().RespawnSeconds,false);}
    else if(auto* GM=GetWorld()->GetAuthGameMode<AWildlineGameMode>())GM->RecordKill(this,nullptr);
}
float AWildlineCharacter::FishingHelpAlpha()const{return FishingStage?1-FMath::Clamp((ServerTime()-FishingEquippedAt-4.f)/1.2f,0.f,1.f):0;}
float AWildlineCharacter::FishingPull()const{return .5f+.5f*FMath::Sin((ServerTime()-FishingStageAt)*1.8f+FishingWeight*2.f);}
FTransform AWildlineCharacter::FishingRodPose()const{
    const float T=ServerTime()-FishingStageAt,Scale=HunterProfile.HeightScale();
    const FVector Forward=GetActorForwardVector(),Right=GetActorRightVector();
    FVector Grip=GetActorLocation()+Forward*38*Scale+Right*17*Scale+FVector(0,0,30*Scale);
    float Pitch=18+FMath::Sin(ServerTime()*1.8f)*1.2f;
    if(FishingStage==5&&FishingChargeAt>0)Pitch+=FMath::Clamp((ServerTime()-FishingChargeAt)/1.1f,0.f,1.f)*55;
    if(FishingStage==6){const float A=FMath::Clamp(T/.95f,0.f,1.f);Pitch=A<.3f?FMath::Lerp(18.f,83.f,A/.3f):FMath::Lerp(83.f,12.f,FMath::SmoothStep(.3f,.85f,A));Grip+=Forward*FMath::Sin(A*PI)*8;}
    if(FishingStage==2)Pitch=20+FMath::Sin(T*12)*2;
    if(FishingStage==3){Pitch=27+FishingTension*15+FMath::Sin(T*8)*FishingPull()*2;Grip-=Forward*FishingTension*5;}
    if(FishingStage==7)Pitch=T<2.5f?FMath::Lerp(28.f,65.f,FMath::SmoothStep(0.f,1.f,T)):FMath::Lerp(65.f,18.f,FMath::SmoothStep(2.5f,3.5f,T));
    const FQuat Rotation=FRotator(Pitch,GetActorRotation().Yaw,0).Quaternion();
    FTransform Result(Rotation,Grip-Rotation.RotateVector(FVector(20,0,0)),FVector(1));
    if(FishingStage==4||FishingStage==8){
        const int32 Spot=AWildlineLake::FishingSpotAt(FishingAnchor);const FVector Rack=AWildlineLake::FishingSpot(FMath::Max(Spot,0))+FVector(-25,0,-40);
        const FTransform Rest(FRotator(65,0,0),Rack);
        if(FishingStage==4)Result.Blend(Rest,Result,FMath::SmoothStep(.35f,1.25f,T));else Result.Blend(Result,Rest,FMath::SmoothStep(0.f,.7f,T));
    }
    return Result;
}
FVector AWildlineCharacter::FishingHand(int32 Side)const{
    const auto Rod=FishingRodPose();
    const float Size=HunterProfile.HandScale()*HunterProfile.HeightScale();
    if(Side==0)return Rod.TransformPosition(FVector(19,2.5f*Size,9*Size));
    const FVector Knob=FVector(29,-9,-6.5f)+FQuat(FVector::YAxisVector,FishingReelPhase).RotateVector(FVector(4,-1,-4.5f));
    return Rod.TransformPosition(Knob+FVector(2.5f*Size,0,9*Size));
}
void AWildlineCharacter::CancelFishing(const FString& Result){
    FishingStage=0;bReeling=bFishingReeling=false;FishingChargeAt=0;FishingProgress=FishingTension=0;FishingUntil=0;
    if(!Result.IsEmpty()){FishingResult=Result;FishingResultUntil=ServerTime()+5;}
    ForceNetUpdate();
}
void AWildlineCharacter::ServerCancelFishing_Implementation(){if(FishingStage&&FishingStage!=8){FishingStage=8;FishingStageAt=ServerTime();FishingUntil=FishingStageAt+.7f;bReeling=bFishingReeling=false;FishingChargeAt=0;ForceNetUpdate();}}
void AWildlineCharacter::ServerReel_Implementation(bool Reeling){
    if(bDead||!bInLobby)return;
    if(FishingStage==5){if(Reeling){if(FishingChargeAt<=0)FishingChargeAt=ServerTime();}else if(FishingChargeAt>0){FishingPower=FMath::Clamp((ServerTime()-FishingChargeAt)/1.1f,.15f,1.f);FishingChargeAt=0;FishingStage=6;FishingStageAt=ServerTime();FishingUntil=FishingStageAt+.95f;}ForceNetUpdate();return;}
    bReeling=bFishingReeling=Reeling&&FishingStage==3;ForceNetUpdate();
}
void AWildlineCharacter::ServerFishInteract_Implementation(){
    const auto* F=AWildlineWorld::Find(GetWorld());const auto* S=GetWorld()->GetGameState<AWildlineGameState>();
    if(!F||!F->bLake||!bInLobby||bSquirrel||bDead||bSwimming||bLobbyReady||(S&&S->Winner))return;
    if(FishingStage==2&&ServerTime()<=FishingUntil){FishingStage=3;FishingStageAt=ServerTime();FishingProgress=.04f;FishingTension=.32f;FishingUntil=ServerTime()+50;MulticastSound(WildlineAudio::Pickup);ForceNetUpdate();return;}
    if(FishingStage==5){FishingPower=.65f;FishingStage=6;FishingStageAt=ServerTime();FishingUntil=FishingStageAt+.95f;FishingChargeAt=0;ForceNetUpdate();return;}
    if(FishingStage)return;
    const int32 Spot=AWildlineLake::FishingSpotAt(GetActorLocation());if(Spot==INDEX_NONE)return;
    for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(*It!=this&&It->FishingStage&&It->bInLobby&&AWildlineLake::FishingSpotAt(It->FishingAnchor)==Spot){FishingResult=TEXT("This rod is in use / try the next station");FishingResultUntil=ServerTime()+3;ForceNetUpdate();return;}
    FishingAnchor=GetActorLocation();FishingStage=4;FishingStageAt=FishingEquippedAt=ServerTime();FishingUntil=FishingStageAt+1.25f;FishingChargeAt=0;FishingProgress=FishingTension=0;FishingResult.Empty();FishingResultUntil=0;bReeling=bFishingReeling=false;bFiring=bAiming=false;
    SetActorRotation(FRotator(0,0,0));GetCharacterMovement()->StopMovementImmediately();MulticastSound(WildlineAudio::Pickup);ForceNetUpdate();
}
void AWildlineCharacter::UpdateLake(float Delta){
    auto* F=AWildlineWorld::Find(GetWorld());if(!F||!F->bLake)return;
    float Surface=0;const FVector P=GetActorLocation();const bool InWater=AWildlineLake::WaterAt(GetWorld(),P,Surface);
    if(HasAuthority()&&bSquirrel&&!bDead&&InWater&&P.Z<Surface+3)Drown();
    if(!bSquirrel&&(HasAuthority()||IsLocallyControlled())){
        auto* M=GetCharacterMovement();const bool Lodge=P.Z>2500;const float Bed=Lodge?Surface-1050*(1-FMath::Square(AWildlineLake::RadiusAt(P,true))):F->GroundHeight(P.X,P.Y);
        const bool Swimming=InWater&&!bDead&&Surface-Bed>145&&(P.Z<Surface-30||(bSwimming&&P.Z<Surface+80));
        if(Swimming!=bSwimming){bSwimming=Swimming;M->SetMovementMode(Swimming?MOVE_Flying:MOVE_Falling);if(Swimming){UnCrouch();M->Velocity*=.4f;bAiming=false;}if(HasAuthority())ForceNetUpdate();}
        if(bSwimming){
            M->MaxFlySpeed=260*MatchRules().MovementScale;M->BrakingDecelerationFlying=420;
            // The movement component transmits the same 3D acceleration used by
            // walking. Local input remains predicted; the server owns water state.
            if(IsLocallyControlled()&&CanControl()){
                if(bSwimRise)AddMovementInput(FVector::UpVector,1);
                else if(bSwimDive)AddMovementInput(-FVector::UpVector,1);
                else if(GetVelocity().Size2D()<15&&P.Z>Surface-100)AddMovementInput(FVector::UpVector,.13f);
            }
            if(P.Z>Surface-48&&M->Velocity.Z>0)M->Velocity.Z=0;
            if(P.Z>Surface-40)SetActorLocation(FVector(P.X,P.Y,Surface-40),false);
        }
    }
    if(IsHeadUnderwater()){bAiming=false;bFiring=false;}
    if(HasAuthority()&&FishingStage){
        auto* PC=Cast<AWildlineController>(Controller);
        if(bDead||!bInLobby||bSquirrel||bSwimming||bLobbyReady||FVector::Dist2D(P,FishingAnchor)>160||(PC&&PC->IsLocalController()&&PC->bMenuOpen)){CancelFishing();}
        else if((FishingStage==4||FishingStage==7)&&ServerTime()>=FishingUntil){FishingStage=5;FishingStageAt=ServerTime();FishingChargeAt=0;ForceNetUpdate();}
        else if(FishingStage==8&&ServerTime()>=FishingUntil)CancelFishing();
        else if(FishingStage==6&&ServerTime()>=FishingUntil){FRandomStream Random(FMath::FloorToInt(ServerTime()*1000)+FishCaught*73);FishingWeight=Random.FRandRange(.65f,6.5f);FishingStage=1;FishingStageAt=ServerTime();FishingUntil=FishingStageAt+Random.FRandRange(3.f,6.f);ForceNetUpdate();}
        else if(FishingStage==1&&ServerTime()>=FishingUntil){FishingStage=2;FishingStageAt=ServerTime();FishingUntil=ServerTime()+1.8f;MulticastSound(WildlineAudio::Feeder);ForceNetUpdate();}
        else if(FishingStage==2&&ServerTime()>FishingUntil){FishingStage=5;FishingStageAt=ServerTime();FishingResult=TEXT("Missed the bite");FishingResultUntil=ServerTime()+3;ForceNetUpdate();}
        else if(FishingStage==3){
            const float Pull=FishingPull();
            FishingTension=FMath::Clamp(FishingTension+Delta*(bReeling?.14f+Pull*.36f+FishingWeight*.012f:-.36f),0.f,1.f);
            const bool Sweet=FishingTension>.22f&&FishingTension<.78f;
            FishingProgress=FMath::Clamp(FishingProgress+Delta*(bReeling?(Sweet?.19f:.065f):FishingTension<.12f?-.09f:-.012f),0.f,1.f);
            if(FishingProgress>=1){FishCaught++;FishingStage=7;FishingStageAt=ServerTime();FishingUntil=FishingStageAt+3.5f;bReeling=bFishingReeling=false;FishingResult=FString::Printf(TEXT("LARGEMOUTH BASS  /  %.2f lb  /  CATCH & RELEASE"),FishingWeight);FishingResultUntil=ServerTime()+6;MulticastSound(WildlineAudio::Deposit);ForceNetUpdate();}
            else if(FishingTension>=1||ServerTime()>FishingUntil){FishingResult=FishingTension>=1?TEXT("Line snapped / ease off during a surge"):TEXT("The fish escaped");FishingResultUntil=ServerTime()+4;FishingStage=5;FishingStageAt=ServerTime();FishingProgress=FishingTension=0;bReeling=bFishingReeling=false;ForceNetUpdate();}
        }
    }
    if(GetNetMode()==NM_DedicatedServer)return;
    if(bFishingReeling)FishingReelPhase=FMath::Fmod(FishingReelPhase+Delta*11.f,2*PI);
    if(!FishingRod){
        auto Make=[&](const TCHAR* Name,const TCHAR* MeshPath){auto* C=NewObject<UStaticMeshComponent>(this,FName(Name));AddInstanceComponent(C);C->SetupAttachment(RootComponent);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,MeshPath));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->RegisterComponent();return C;};
        FishingRod=Make(TEXT("Active fishing rod"),TEXT("/Game/Wildline/Lake/Meshes/SM_FishingRod.SM_FishingRod"));FishingFloat=Make(TEXT("Fishing bobber"),TEXT("/Engine/BasicShapes/Sphere.Sphere"));FishingLine=Make(TEXT("Fishing line"),TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
        FishingCrank=Make(TEXT("Animated reel crank"),TEXT("/Game/Wildline/Lake/Meshes/SM_FishingReelCrank.SM_FishingReelCrank"));FishingCatch=Make(TEXT("Landed largemouth bass"),TEXT("/Game/Wildline/Lake/Meshes/SM_PhotorealBass.SM_PhotorealBass"));
        FishingFloat->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Wildline/Lake/Materials/M_Bobber.M_Bobber")));FishingLine->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Wildline/Lake/Materials/M_LineSpool.M_LineSpool")));
    }
    const bool Equipped=FishingStage>0&&!bDead,Cast=FishingStage==1||FishingStage==2||FishingStage==3||FishingStage==6;
    for(auto* C:{FishingRod,FishingCrank})C->SetVisibility(Equipped&&(FishingStage!=4||ServerTime()-FishingStageAt>.35f));
    for(auto* C:{FishingFloat,FishingLine})C->SetVisibility(Equipped&&Cast);
    FishingCatch->SetVisibility(Equipped&&FishingStage==7);
    if(FishingStage){
        const FTransform Rod=FishingRodPose();FishingRod->SetWorldTransform(Rod);
        const float Spin=FishingReelPhase;
        FishingCrank->SetWorldTransform(FTransform(Rod.GetRotation()*FQuat(FVector::YAxisVector,Spin),Rod.TransformPosition(FVector(29,-9,-6.5f))));
        FVector Float=FVector(FishingAnchor)+FVector(550+FishingPower*900,0,-195+(FishingStage==2?-6+FMath::Sin(ServerTime()*18)*5:FMath::Sin(ServerTime()*2)*2));
        if(FishingStage==3){Float.X-=FishingProgress*650;Float.Y+=FMath::Sin((ServerTime()-FishingStageAt)*1.8f)*FishingPull()*85;}
        const FVector Tip=Rod.TransformPosition(FVector(220,0,6));
        if(FishingStage==6){const float A=FMath::SmoothStep(.35f,.95f,ServerTime()-FishingStageAt);Float=FMath::Lerp(Tip,Float,A)+FVector(0,0,FMath::Sin(A*PI)*180);}
        const FVector D=Float-Tip;
        FishingFloat->SetWorldLocation(Float);FishingFloat->SetWorldScale3D(FVector(.065f,.065f,.13f));FishingLine->SetWorldLocation((Float+Tip)*.5f);FishingLine->SetWorldRotation(FQuat::FindBetweenNormals(FVector::UpVector,D.GetSafeNormal()));FishingLine->SetWorldScale3D(FVector(.0025f,.0025f,D.Size()/100));
        const float LandAge=ServerTime()-FishingStageAt;
        const float Release=FMath::SmoothStep(2.5f,3.5f,LandAge);
        const FVector CatchPoint=FMath::Lerp(P+GetActorForwardVector()*110-GetActorRightVector()*15+FVector(0,0,12-FMath::Sin(LandAge*5)*3),FVector(FishingAnchor)+FVector(500,0,-205),Release)+FVector(0,0,FMath::Sin(Release*PI)*45);
        FishingCatch->SetWorldLocation(CatchPoint);FishingCatch->SetWorldRotation(FRotator(FMath::Lerp(65.f,0.f,Release)+FMath::Sin(LandAge*14)*4,GetActorRotation().Yaw+90,0));FishingCatch->SetWorldScale3D(FVector(.7f+FishingWeight*.07f));
        if(FishingStage==7&&Release==0){const FVector Mouth=CatchPoint+FishingCatch->GetForwardVector()*27*(.7f+FishingWeight*.07f),Line=Mouth-Tip;FishingLine->SetVisibility(true);FishingLine->SetWorldLocation((Mouth+Tip)*.5f);FishingLine->SetWorldRotation(FQuat::FindBetweenNormals(FVector::UpVector,Line.GetSafeNormal()));FishingLine->SetWorldScale3D(FVector(.0025f,.0025f,Line.Size()/100));}
    }
    if(IsLocallyControlled())for(auto* Camera:{FirstCamera,ThirdCamera}){
        float Level=0;const FVector Eye=Camera->GetComponentLocation();const bool Under=AWildlineLake::WaterAt(GetWorld(),Eye,Level)&&Eye.Z<Level;
        auto& Grade=Camera->PostProcessSettings;Grade.bOverride_SceneColorTint=Under;Grade.SceneColorTint=FLinearColor(.45f,.72f,.76f);Grade.bOverride_VignetteIntensity=Under;Grade.VignetteIntensity=.35f;
        Grade.bOverride_AutoExposureBias=Under;Grade.AutoExposureBias=.6f;
        Grade.bOverride_ColorSaturation=Under;Grade.ColorSaturation=FVector4(.6f,.75f,.75f,1);
    }
    if(!bSquirrel){const bool Hidden=IsHeadUnderwater()||FishingStage;WorldRifle->SetVisibility(!Hidden);WorldBolt->SetVisibility(!Hidden);if(Hidden){ViewRifle->SetVisibility(false);ViewBolt->SetVisibility(false);}}
}

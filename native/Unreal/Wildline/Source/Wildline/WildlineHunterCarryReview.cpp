#include "WildlineGame.h"
#include "WildlineCharacter.h"
#include "WildlineRules.h"
#include "WildlineHunterAnim.h"
#include "WildlineSocial.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HUD.h"
#include "TimerManager.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

// An observer views a separate pawn so ADS never hides the hands under review.
void WildlineRunHunterCarryReview(AWildlineController* PC){
    auto At=[PC](float T,TFunction<void()> Work){FTimerHandle H;PC->GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(PC,[Work](){Work();}),T,false);};
    auto Hunter=MakeShared<AWildlineCharacter*>(nullptr);auto Cam=MakeShared<ACameraActor*>(nullptr);
    auto Passed=MakeShared<bool>(true);
    auto Check=[Passed](bool OK,const TCHAR* Label){*Passed&=OK;UE_LOG(LogTemp,Display,TEXT("HUNTER CARRY %s: %s"),OK?TEXT("PASS"):TEXT("FAIL"),Label);};
    auto View=[Hunter,Cam](bool Detail){if(!*Hunter||!*Cam)return;auto* P=*Hunter;const FVector Offset=Detail?FVector(-110,150,135):FVector(-260,340,180);const FVector Target=Detail?FVector(-15,25,130):FVector(5,5,88);const FTransform Body=P->GetMesh()->GetComponentTransform();(*Cam)->SetActorLocation(Body.TransformPosition(Offset));(*Cam)->SetActorRotation((Body.TransformPosition(Target)-(*Cam)->GetActorLocation()).Rotation());(*Cam)->GetCameraComponent()->FieldOfView=Detail?43:45;};
    auto Shot=[Hunter](const TCHAR* Name){if(!*Hunter)return;auto* P=*Hunter;auto* M=P->GetMesh();const auto Gun=P->WorldRifle->GetComponentTransform();
        for(const TCHAR* Bone:{TEXT("hand_r"),TEXT("index_01_r"),TEXT("index_02_r"),TEXT("index_03_r"),TEXT("middle_01_r"),TEXT("middle_03_r"),TEXT("thumb_03_r"),TEXT("hand_l"),TEXT("index_01_l"),TEXT("middle_03_l")})UE_LOG(LogTemp,Display,TEXT("HUNTER GRIP %s %s %s"),Name,Bone,*Gun.InverseTransformPosition(M->GetSocketLocation(Bone)).ToString());
        if(!FParse::Param(FCommandLine::Get(),TEXT("NullRHI")))FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,false,false);
    };
    auto VerifyCarry=[Hunter,Check](){auto* P=*Hunter;auto* M=P->GetMesh();const auto Gun=P->WorldRifle->GetComponentTransform();const float Scale=P->HunterProfile.HeightScale();
        Check(!P->bAiming&&Gun.GetRotation().GetForwardVector().Z<-.7f,TEXT("unzoomed rifle points down at the hunter's side"));
        Check(FMath::Abs(M->GetSocketLocation(TEXT("hand_r")).Z-M->GetSocketLocation(TEXT("pelvis")).Z)<8*Scale,TEXT("carry wrist rests at hip height"));
        Check(Gun.TransformPosition(FVector(0,0,8)).Z<M->GetSocketLocation(TEXT("neck_01")).Z-25*Scale,TEXT("unzoomed scope clears the face"));
    };
    auto VerifyAim=[Hunter,Check](){auto* P=*Hunter;auto* M=P->GetMesh();const auto Gun=P->WorldRifle->GetComponentTransform();
        const FVector Index=Gun.InverseTransformPosition(M->GetSocketLocation(TEXT("index_03_r")));
        const FVector Thumb=Gun.InverseTransformPosition(M->GetSocketLocation(TEXT("thumb_03_r")));
        UE_LOG(LogTemp,Display,TEXT("HUNTER CARRY trigger index=%s thumb=%s"),*Index.ToString(),*Thumb.ToString());
        Check(P->bAiming&&FVector::Dist(Index,FVector(-7.3,0,-5.2))<4.f,TEXT("aimed index finger reaches the trigger guard"));
        Check(Index.Z<0&&Thumb.Z<2,TEXT("trigger hand wraps below the scope"));
    };
    At(2,[PC,Hunter,Cam,View](){
        PC->ChooseRole(false);PC->SetMenu(false);PC->Social->Open(false);if(PC->GetHUD())PC->GetHUD()->bShowHUD=false;
        PC->GetWorld()->GetGameState<AWildlineGameState>()->bRoundStarted=false;
        auto* Player=Cast<AWildlineCharacter>(PC->GetPawn());Player->SetActorHiddenInGame(true);Player->SetActorEnableCollision(false);
        const FVector Location(-900,1400,WildlineRules::Ground(-900,1400)+96);
        auto* P=PC->GetWorld()->SpawnActorDeferred<AWildlineCharacter>(AWildlineCharacter::StaticClass(),FTransform(FRotator::ZeroRotator,Location));
        P->bSquirrel=false;P->bProfileSubmitted=true;P->bThirdPerson=true;P->HunterProfile=FWildlineHunterProfile::Travis();
        FString Preset;FParse::Value(FCommandLine::Get(),TEXT("WildlineHunterPreset="),Preset);
        if(Preset==TEXT("Ethan"))P->HunterProfile=FWildlineHunterProfile::Ethan();else if(Preset==TEXT("Greyson"))P->HunterProfile=FWildlineHunterProfile::Greyson();else if(Preset==TEXT("Mark"))P->HunterProfile=FWildlineHunterProfile::Mark();
        if(FParse::Param(FCommandLine::Get(),TEXT("WildlineCustomBody"))){P->HunterProfile.HeightCm=180;P->HunterProfile.BodyWidth=1.25f;P->HunterProfile.BodyDepth=1.15f;P->HunterProfile.HandSize=1.1f;}
        P->FinishSpawning(FTransform(FRotator::ZeroRotator,Location));P->GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        *Hunter=P;*Cam=PC->GetWorld()->SpawnActor<ACameraActor>();PC->SetViewTarget(*Cam);View(false);
        auto* Light=NewObject<UPointLightComponent>(*Cam);Light->SetupAttachment((*Cam)->GetRootComponent());Light->Intensity=8000;Light->AttenuationRadius=1200;Light->RegisterComponent();
    });
    At(5,[View](){View(false);});At(6,[Shot](){Shot(TEXT("HunterCarrySide.png"));});
    At(6.2f,VerifyCarry);
    At(7,[Hunter](){(*Hunter)->bAiming=true;});At(9,[Shot](){Shot(TEXT("HunterAimFull.png"));});
    At(10,[View](){View(true);});At(11,[Shot](){Shot(TEXT("HunterAimGrip.png"));});
    At(11.2f,VerifyAim);
    At(12,[Hunter](){(*Hunter)->bAiming=false;});At(14,[View](){View(false);});At(15,[Shot](){Shot(TEXT("HunterLoweredAgain.png"));});
    At(15.2f,VerifyCarry);
    At(16,[Hunter,Check](){auto* P=*Hunter;bool Finite=P&&P->WorldRifle->GetComponentTransform().IsValid();if(P)for(int I=0;I<P->GetMesh()->GetNumBones();I++)Finite&=!P->GetMesh()->GetBoneTransform(I).ContainsNaN();Check(Finite,TEXT("evaluated pose and weapon transforms remain finite"));});
    At(17,[Passed](){UE_LOG(LogTemp,Display,TEXT("WILDLINE HUNTER CARRY: %s"),*Passed?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Passed?0:1);});
}

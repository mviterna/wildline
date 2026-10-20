#include "WildlineGame.h"
#include "WildlineCharacter.h"
#include "WildlineHunterGrip.h"
#include "WildlineHunterCreator.h"
#include "WildlineMenu.h"
#include "WildlineUserSettings.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HighResScreenshot.h"
#include "UnrealClient.h"

void AWildlineController::RunHunterBodyReview(){
    auto Passed=MakeShared<bool>(true);const auto Original=UWildlineUserSettings::Get()->HunterProfile;
    auto Check=[Passed](bool Ok,const TCHAR* What){*Passed&=Ok;UE_LOG(LogTemp,Display,TEXT("HUNTER BODY %s: %s"),Ok?TEXT("PASS"):TEXT("FAIL"),What);};
    auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
    auto Shot=[](const TCHAR* Name){if(!FParse::Param(FCommandLine::Get(),TEXT("NullRHI")))FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows")/Name,true,false);};
    auto Adult=FWildlineHunterProfile::Ethan();Adult.HeightCm=180;Adult.BodyWidth=1.25f;Adult.BodyDepth=1.15f;Adult.HandSize=1.1f;Adult.FootSize=1.05f;Adult.FaceSize=1.15f;Adult.Slimness=-.2f;
    At(2,[this,Check](){Menu->CreateHunter();Menu->Creator->LoadEthanPreset();auto* C=Menu->Creator;Check(C->Pending.HeightCm==71.4f&&C->Pending.HandSize==.72f&&C->Pending.FootSize==.75f,TEXT("Ethan starts with his existing dimensions"));C->PreviewAngle=0;C->FullBody();});
    At(4,[Shot](){Shot(TEXT("HunterBodyEthanDefault.png"));});
    At(5,[this](){auto* C=Menu->Creator;C->DimensionSliders[0]->SetValue(180);C->DimensionSliders[1]->SetValue(125);C->DimensionSliders[2]->SetValue(115);C->DimensionSliders[3]->SetValue(110);C->DimensionSliders[4]->SetValue(105);C->DimensionSliders[5]->SetValue(115);C->AppearanceSliders[0]->SetValue(120);if(C->AppearanceScroll)C->AppearanceScroll->ScrollWidgetIntoView(C->DimensionLabels[0],false,EDescendantScrollDestination::TopOrLeft,8);});
    At(6,[this,Check](){auto* C=Menu->Creator;const auto& P=C->Pending;Check(P.Preset==3&&P.HeightCm==180&&FMath::IsNearlyEqual(P.BodyWidth,1.25f)&&FMath::IsNearlyEqual(P.BodyDepth,1.15f)&&FMath::IsNearlyEqual(P.HandSize,1.1f)&&FMath::IsNearlyEqual(P.FootSize,1.05f)&&FMath::IsNearlyEqual(P.FaceSize,1.15f)&&FMath::IsNearlyEqual(P.Slimness,-.2f,.0001f),TEXT("Every body slider updates the live Ethan preset"));Check(FMath::IsNearlyEqual(C->Preview->GetActorScale3D().Z,180.f/174.5f,.001f),TEXT("Preview grows Ethan to adult height"));});
    At(7,[Shot](){Shot(TEXT("HunterBodyEthanAdult.png"));});
    At(8,[this](){auto* C=Menu->Creator;if(C->AppearanceScroll)C->AppearanceScroll->ScrollWidgetIntoView(C->DimensionSliders[5],false,EDescendantScrollDestination::Center);});
    At(9,[Shot](){Shot(TEXT("HunterBodyControls.png"));});
    At(10,[this](){Menu->Creator->FaceView();});At(11,[Shot](){Shot(TEXT("HunterBodyFace.png"));});
    At(12,[this](){Menu->Creator->SaveHunter();});
    At(13,[this,Check](){UWildlineUserSettings::Get()->LoadSettings(true);Menu->CreateHunter();auto* C=Menu->Creator;Check(C->Pending.HeightCm==180&&FMath::IsNearlyEqual(C->Pending.HandSize,1.1f)&&FMath::IsNearlyEqual(C->Pending.FaceSize,1.15f)&&FMath::IsNearlyEqual(C->Pending.BodyWidth,1.25f)&&FMath::IsNearlyEqual(C->Pending.BodyDepth,1.15f)&&FMath::IsNearlyEqual(C->Pending.FootSize,1.05f)&&FMath::IsNearlyEqual(C->Pending.Slimness,-.2f),TEXT("Save, reload and reopening retain all dimensions"));C->ResetPreset();Check(C->Pending.HeightCm==71.4f&&C->Pending.HandSize==.72f&&C->Pending.FootSize==.75f&&C->Pending.FaceSize==1&&C->Pending.BodyWidth==1&&C->Pending.BodyDepth==1,TEXT("Reset restores Ethan without locking later edits"));});
    At(15,[this,Check](){auto* C=Menu->Creator;C->LoadGreysonPreset();Check(C->Pending.HeightCm==110.4f&&C->Pending.HandSize==.82f&&C->Pending.FootSize==.84f,TEXT("Greyson retains original dimensions"));C->LoadMarkPreset();Check(C->Pending.HeightCm==174.5f&&C->Pending.HandSize==1&&C->Pending.Slimness==.25f,TEXT("Mark retains original dimensions and build"));C->LoadTravisPreset();Check(C->Pending.HeightCm==174.5f&&C->Pending.HandSize==1&&C->Pending.Slimness==.85f,TEXT("Travis retains original dimensions and build"));});
    At(17,[this,Adult,Check](){Menu->Creator->Cancel();ChooseRole(false);auto* P=Cast<AWildlineCharacter>(GetPawn());const float Feet=P->GetActorLocation().Z-P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();P->bProfileSubmitted=true;P->ServerSetHunterProfile(Adult,FString());P->bThirdPerson=true;P->bAiming=true;P->GetMesh()->SetOwnerNoSee(false);P->WorldRifle->SetOwnerNoSee(false);Check(FMath::IsNearlyEqual(P->GetActorLocation().Z-P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),Feet,.01f),TEXT("Applying adult height keeps feet anchored"));auto* Cam=GetWorld()->SpawnActor<ACameraActor>();Cam->AttachToComponent(P->GetMesh(),FAttachmentTransformRules::KeepRelativeTransform);Cam->SetActorRelativeLocation(FVector(300,330,246));Cam->SetActorRelativeRotation((FVector(0,15,81)-FVector(300,330,246)).Rotation());Cam->GetCameraComponent()->FieldOfView=48;SetViewTarget(Cam);SetMenu(false);});
    At(20,[this,Check](){auto* P=Cast<AWildlineCharacter>(GetPawn());auto* M=P->GetMesh();bool Sized=true;for(const TCHAR* Bone:{TEXT("hand_l"),TEXT("hand_r"),TEXT("index_01_l")})Sized&=M->GetBoneTransform(Bone,RTS_Component).GetScale3D().Equals(FVector(1.1),.02);Sized&=M->GetBoneTransform(TEXT("head"),RTS_Component).GetScale3D().Equals(FVector(1.15),.02);Sized&=M->GetBoneTransform(TEXT("foot_l"),RTS_Component).GetScale3D().Equals(FVector(1.05),.02);Check(Sized,TEXT("Head, hands, fingers and feet use independent sizes"));const auto Gun=P->WorldRifle->GetComponentTransform();const float Right=FVector::Dist(M->GetSocketLocation(TEXT("hand_r")),Gun.TransformPosition(WildlineHunterGrip::Wrist(0,false,P->HunterProfile.HandScale())));const float Left=FVector::Dist(M->GetSocketLocation(TEXT("hand_l")),Gun.TransformPosition(WildlineHunterGrip::Wrist(1,false,P->HunterProfile.HandScale())));UE_LOG(LogTemp,Display,TEXT("HUNTER BODY grip right=%.4f left=%.4f"),Right,Left);Check(Right<3&&Left<3,TEXT("Both grips align on the widened adult body"));Check(Gun.GetScale3D().Equals(FVector(P->HunterProfile.HeightScale()),.001),TEXT("Body width and depth keep the weapon rigid"));Check(FMath::IsNearlyEqual(M->GetMorphTarget(TEXT("SlimBuild")),-.2f),TEXT("Weight slider reaches heavier builds"));});
    At(21,[Shot](){Shot(TEXT("HunterBodyInGame.png"));});
    At(23,[this](){SetMenu(true);Menu->CreateHunter();auto* C=Menu->Creator;C->Pending.HeightCm=230;C->Pending.BodyWidth=1.5f;C->Pending.BodyDepth=1.6f;C->Pending.HandSize=.5f;C->Pending.FootSize=.5f;C->Pending.FaceSize=1.5f;C->Pending.Slimness=-.5f;C->SyncControls();C->RefreshPreview();C->FullBody();});
    At(25,[this,Check](){auto* M=Menu->Creator->Preview->GetMesh();bool Finite=true;for(int32 I=0;I<M->GetNumBones();I++)Finite&=!M->GetBoneTransform(I).ContainsNaN();Check(Finite,TEXT("Extreme supported proportions retain a finite evaluated rig"));});
    At(26,[Shot](){Shot(TEXT("HunterBodyExtreme.png"));});
    At(28,[this,Passed,Original](){if(Menu&&Menu->Creator)Menu->Creator->Cancel();auto* S=UWildlineUserSettings::Get();S->HunterProfile=Original;S->Persist();UE_LOG(LogTemp,Display,TEXT("WILDLINE HUNTER BODIES: %s"),*Passed?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Passed?0:1);});
}

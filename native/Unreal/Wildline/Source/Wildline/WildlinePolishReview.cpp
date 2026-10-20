#include "WildlineGame.h"
#include "WildlineMenu.h"
#include "WildlineSocial.h"
#include "WildlineGraphics.h"
#include "WildlineCharacter.h"
#include "WildlineWorld.h"
#include "WildlineUserSettings.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HUD.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "UnrealClient.h"

void WildlineRunPolishReview(AWildlineController* PC){
    auto Ok=MakeShared<bool>(true);auto Target=MakeShared<TWeakObjectPtr<AWildlineCharacter>>();
    auto Check=[Ok](bool Value,const TCHAR* Text){*Ok&=Value;UE_LOG(LogTemp,Display,TEXT("POLISH CHECK %s: %s"),Value?TEXT("PASS"):TEXT("FAIL"),Text);};
    auto At=[PC](float Time,TFunction<void()> Work){FTimerHandle H;PC->GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(PC,[Work](){Work();}),Time,false);};
    auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,true,false);};
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlinePolishReload"))){At(2,[Check,Ok](){auto* S=UWildlineUserSettings::Get();float N,V,Min,Max;S->GetResolutionScaleInformationEx(N,V,Min,Max);Check(S->AntiAliasingMethod==1&&S->GetShadowQuality()==1&&FMath::IsNearlyEqual(V,67.f,.1f)&&S->GetFrameRateLimit()==120.f,TEXT("custom graphics survive application restart"));UE_LOG(LogTemp,Display,TEXT("WILDLINE POLISH RELOAD: %s"),*Ok?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Ok?0:1);});return;}
    FApp::SetUseFixedTimeStep(true);FApp::SetFixedDeltaTime(1./60.);
    At(1,[PC,Check](){auto* G=PC->Menu->Graphics;G->AA->SetSelectedIndex(1);for(auto* C:G->Quality)C->SetSelectedIndex(1);G->RenderScale->SetValue(67);G->Sharpness->SetValue(.8f);G->FrameCap->SetSelectedIndex(4);G->VSync->SetIsChecked(false);G->MotionBlur->SetIsChecked(true);G->Apply();auto* S=UWildlineUserSettings::Get();S->LoadSettings(true);S->ApplyNonResolutionSettings();
        auto CVar=[](const TCHAR* Name){return IConsoleManager::Get().FindConsoleVariable(Name)->GetFloat();};
        Check(S->AntiAliasingMethod==1&&S->GetShadowQuality()==1&&S->GetTextureQuality()==1&&FMath::IsNearlyEqual(S->GetFrameRateLimit(),120.f),TEXT("graphics selections survive save and reload"));
        Check(CVar(TEXT("r.AntiAliasingMethod"))==1&&CVar(TEXT("sg.ShadowQuality"))==1&&FMath::IsNearlyEqual(CVar(TEXT("sg.ResolutionQuality")),67.f,.1f)&&FMath::IsNearlyEqual(CVar(TEXT("r.MotionBlur.Amount")),.35f,.01f),TEXT("saved settings drive actual renderer CVars"));PC->Menu->ScrollToSettings();});
    At(2,[Shot](){Shot(TEXT("GraphicsQuality.png"));});
    At(3,[PC](){PC->Menu->FieldScroll->ScrollWidgetIntoView(PC->Menu->Graphics->FrameCap,true,EDescendantScrollDestination::Center);});
    At(4,[Shot](){Shot(TEXT("GraphicsPerformance.png"));});
    At(5,[PC,Check](){PC->Menu->Graphics->Defaults();auto* S=PC->GetWorld()->GetGameState<AWildlineGameState>();S->MatchConfig.DefenderBots=0;S->MatchConfig.SquirrelBots=0;S->MatchConfig.SpawnProtectionSeconds=0;PC->ChooseRole(true);S->bRoundStarted=true;auto* P=Cast<AWildlineCharacter>(PC->GetPawn());auto* F=AWildlineWorld::Find(PC->GetWorld());P->SetActorLocation(F->AcornLocations[0]+FVector(0,0,13));P->Tick(.016f);auto* PS=PC->GetPlayerState<AWildlinePlayerState>();Check(PS->FoodCollected>0,TEXT("actual pickup increments personal food"));const int Picked=PS->FoodCollected;P->SetActorLocation(F->DenLocation+FVector(0,180,13));P->Tick(.016f);Check(PS->FoodDeposited==Picked,TEXT("actual deposit credits carried food once"));P->ProtectedUntil=0;UGameplayStatics::ApplyDamage(P,100,nullptr,nullptr,UDamageType::StaticClass());Check(PS->Deaths==1,TEXT("actual death updates persistent player state"));});
    At(6,[PC](){PC->ChooseRole(false);auto* H=Cast<AWildlineCharacter>(PC->GetPawn());H->SelectWeapon(1);H->SetActorLocation(FVector(0,0,10000));H->GetCharacterMovement()->DisableMovement();H->ServerFire(FVector(0,0,1));});
    At(8,[PC,Target](){auto* H=Cast<AWildlineCharacter>(PC->GetPawn());const FVector Pos=H->GetActorLocation()+FVector(220,2,72*H->HunterProfile.HeightScale());const FTransform T(Pos);auto* A=PC->GetWorld()->SpawnActorDeferred<AWildlineCharacter>(AWildlineCharacter::StaticClass(),T,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);A->bSquirrel=true;UGameplayStatics::FinishSpawningActor(A,T);A->ProtectedUntil=0;A->GetCharacterMovement()->DisableMovement();*Target=A;H->ServerFire(FVector(1,0,0));});
    At(9,[PC,Check](){auto* PS=PC->GetPlayerState<AWildlinePlayerState>();Check(PS->Shots==2&&PS->Hits==1&&PS->Kills==1&&FMath::IsNearlyEqual(PS->HitPercentage(),50.f),TEXT("one missed blast and one multi-pellet kill produce 50 percent accuracy"));PC->SetMenu(true);PC->Social->Open(true);PC->Social->Refresh();Check(PC->Social->Roster->GetText().ToString().Contains(TEXT("Aim 50% (1/2)")),TEXT("Shift Tab shows live stats"));});
    At(10,[Shot](){Shot(TEXT("SharedMatchStats.png"));});
    At(11,[PC,Check](){auto* S=PC->GetWorld()->GetGameState<AWildlineGameState>();S->Winner=1;PC->GetWorld()->GetAuthGameMode<AWildlineGameMode>()->RestartRound();auto* PS=PC->GetPlayerState<AWildlinePlayerState>();Check(PS->Shots==0&&PS->Hits==0&&PS->Kills==0&&PS->Deaths==0&&PS->FoodCollected==0&&PS->FoodDeposited==0,TEXT("round restart resets all personal stats"));PC->Social->Open(false);PC->ChooseRole(true);S->bRoundStarted=false;
        auto* P=Cast<AWildlineCharacter>(PC->GetPawn());P->bThirdPerson=true;P->GetMesh()->SetOwnerNoSee(false);
        // An isolated, collidable runway keeps trees from obscuring the motion
        // evidence. It exists only under this explicit verification flag.
        auto* Stage=PC->GetWorld()->SpawnActor<AActor>();auto* Floor=NewObject<UStaticMeshComponent>(Stage);Stage->SetRootComponent(Floor);Floor->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Floor->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Floor->SetCollisionResponseToAllChannels(ECR_Block);Floor->RegisterComponent();Floor->SetWorldLocation(FVector(0,0,200000));Floor->SetWorldScale3D(FVector(60,12,.1f));
        for(int I=0;I<2;I++){auto* Light=NewObject<UPointLightComponent>(Stage);Light->RegisterComponent();Light->SetWorldLocation(FVector(-1000+I*1200,-200,200250));Light->SetIntensity(1000000);Light->SetAttenuationRadius(2500);Light->SetCastShadows(false);}
        P->SetActorLocation(FVector(-1800,0,200018));P->TreeIndex=INDEX_NONE;P->GetCharacterMovement()->SetMovementMode(MOVE_Walking);P->SetActorRotation(FRotator::ZeroRotator);PC->SetControlRotation(FRotator::ZeroRotator);
        auto* Cam=PC->GetWorld()->SpawnActor<ACameraActor>();Cam->AttachToComponent(P->GetRootComponent(),FAttachmentTransformRules::KeepRelativeTransform);const FVector Offset(65,-85,28);Cam->SetActorRelativeLocation(Offset);Cam->SetActorRelativeRotation((FVector(0,0,7)-Offset).Rotation());Cam->GetCameraComponent()->FieldOfView=46;PC->bAutoManageActiveCameraTarget=false;PC->SetViewTarget(Cam);PC->GetHUD()->bShowHUD=false;UE_LOG(LogTemp,Display,TEXT("POLISH RUNWAY initialized pawn=%s floor=%s"),*P->GetActorLocation().ToString(),*Floor->GetComponentLocation().ToString());
    });
    auto Frame=MakeShared<int32>(0);auto LastPose=MakeShared<FVector>();auto Motion=MakeShared<float>(0);auto Finite=MakeShared<bool>(true);auto Airborne=MakeShared<bool>(false);auto Landed=MakeShared<bool>(false);
    At(12,[PC,Frame,LastPose,Motion,Finite,Airborne,Landed](){
        FApp::SetUseFixedTimeStep(true);FApp::SetFixedDeltaTime(1./60.);
        auto Input=MakeShared<FTimerHandle>();PC->GetWorldTimerManager().SetTimer(*Input,FTimerDelegate::CreateWeakLambda(PC,[PC,Frame,Input](){if(*Frame>=150){PC->GetWorldTimerManager().ClearTimer(*Input);return;}if(auto* P=Cast<AWildlineCharacter>(PC->GetPawn()))P->AddMovementInput(FVector(1,0,0),*Frame<110?1.f:0.f);}),1.f/60,true);
        auto Handle=MakeShared<FTimerHandle>();PC->GetWorldTimerManager().SetTimer(*Handle,FTimerDelegate::CreateWeakLambda(PC,[PC,Frame,LastPose,Motion,Finite,Airborne,Landed,Handle](){
            auto* P=Cast<AWildlineCharacter>(PC->GetPawn());if(!P)return;const int I=(*Frame)++;if(I==25)P->StartSprint();if(I==55)P->ServerJump();if(I==105)P->StopSprint();
            if(I%30==0)UE_LOG(LogTemp,Display,TEXT("POLISH RUNWAY frame=%d pawn=%s camera=%s tree=%d"),I,*P->GetActorLocation().ToString(),*PC->GetViewTarget()->GetActorLocation().ToString(),P->TreeIndex);
            *Finite&=P->GetActorLocation().Z>199900;
            auto* M=P->GetMesh();for(int B=0;B<M->GetNumBones();B++)*Finite&=!M->GetBoneTransform(B).ContainsNaN();const FVector Paw=M->GetSocketTransform(TEXT("SKEL_L_Hand"),RTS_Component).GetLocation();if(I>0)*Motion+=FVector::Dist(Paw,*LastPose);*LastPose=Paw;
            const bool Falling=P->GetCharacterMovement()->IsFalling();*Airborne|=Falling;if(*Airborne&&!Falling&&I>55)*Landed=true;
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/Windows/SquirrelPolish_%03d.png"),I),false,false);
            if(I>=149){PC->GetWorldTimerManager().ClearTimer(*Handle);FApp::SetUseFixedTimeStep(false);}
        }),1.f/30,true);
    });
    At(18,[PC,Check,Ok,Motion,Finite,Airborne,Landed](){Check(*Finite&&*Motion>5,TEXT("150 rendered motion frames retain finite animated bones"));Check(*Airborne&&*Landed,TEXT("actual running jump and landing exercised"));auto* G=PC->Menu->Graphics;G->AA->SetSelectedIndex(1);for(auto* C:G->Quality)C->SetSelectedIndex(1);G->RenderScale->SetValue(67);G->FrameCap->SetSelectedIndex(4);G->Apply();UE_LOG(LogTemp,Display,TEXT("WILDLINE POLISH REVIEW: %s"),*Ok?TEXT("PASS"):TEXT("FAIL"));UE_LOG(LogTemp,Display,TEXT("WILDLINE SETTINGS RESULT: %s"),*Ok?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Ok?0:1);});
}

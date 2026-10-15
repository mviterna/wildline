#include "WildlineGame.h"
#include "WildlineWorld.h"
#include "WildlineCharacter.h"
#include "WildlineMenu.h"
#include "WildlineUserSettings.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HUD.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/Slider.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "InputKeyEventArgs.h"

void AWildlineController::RunGameplayBugsReview(){
    auto* F=AWildlineWorld::Find(GetWorld());auto* S=GetWorld()->GetGameState<AWildlineGameState>();
    if(!F||!S){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    GetGameInstance<UWildlineGameInstance>()->PreferredMode=S->ExpeditionMode;
    auto* Settings=UWildlineUserSettings::Get();Settings->ConfigForMode(S->ExpeditionMode)=S->MatchConfig;
    ChooseRole(true);auto* P=Cast<AWildlineCharacter>(GetPawn());SetMenu(false);S->bRoundStarted=false;S->Winner=0;P->bInLobby=false;P->bThirdPerson=true;
    // Test the actual input stack with W and Shift still held when Space arrives.
    const FVector Start=FVector(0,-600,F->GroundHeight(0,-600)+13);P->SetActorLocation(Start);P->Stamina=100;
    P->GetCharacterMovement()->StopMovementImmediately();P->GetCharacterMovement()->SetMovementMode(MOVE_Falling);SetControlRotation(FRotator(-10,0,0));
    auto Passed=MakeShared<bool>(true);auto JumpZ=MakeShared<float>(0);auto Before=MakeShared<FVector>();auto Speed=MakeShared<float>(0);
    auto Check=[Passed](bool Ok,const TCHAR* Message){*Passed&=Ok;UE_LOG(LogTemp,Display,TEXT("WILDLINE GAMEPLAY BUGS %s: %s"),Ok?TEXT("PASS"):TEXT("FAIL"),Message);};
    auto At=[this](float T,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),T,false);};
    auto Key=[this](FKey K,EInputEvent Event){InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,K,Event,FPlatformTime::Cycles64()));};
    auto Shot=[](FString Name){if(!FParse::Param(FCommandLine::Get(),TEXT("NullRHI")))FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,false,false);};
    At(.6f,[Key](){Key(EKeys::W,IE_Pressed);Key(EKeys::LeftShift,IE_Pressed);});
    At(1.2f,[P,JumpZ,Before,Speed,Check,Key](){
        *JumpZ=P->GetActorLocation().Z;*Before=P->GetActorLocation();*Speed=P->GetVelocity().Size2D();
        Check(P->bSprinting&&*Speed>500,TEXT("W and Shift produce a running squirrel"));Key(EKeys::SpaceBar,IE_Pressed);
    });
    At(1.4f,[P,JumpZ,Before,Speed,Check,Shot](){
        Check(P->GetCharacterMovement()->IsFalling()&&P->GetActorLocation().Z>*JumpZ+30,TEXT("Space while holding W and Shift takes off"));
        Check(FVector::Dist2D(P->GetActorLocation(),*Before)>85&&P->GetVelocity().Size2D()>*Speed-20,TEXT("running jump retains forward momentum"));Shot(TEXT("RunningJump.png"));
    });
    At(1.5f,[Key](){Key(EKeys::SpaceBar,IE_Released);Key(EKeys::LeftShift,IE_Released);Key(EKeys::W,IE_Released);});
    At(2.4f,[P,Check](){
        P->Stamina=100;P->TreeIndex=INDEX_NONE;P->GetCharacterMovement()->PendingLaunchVelocity=FVector::ZeroVector;
        P->GetCharacterMovement()->SetMovementMode(MOVE_Walking);P->Tick(.016f);
        P->GetCharacterMovement()->SetMovementMode(MOVE_Falling);P->GetCharacterMovement()->Velocity=FVector(650,0,40);
        P->ServerJump();const FVector Launch=P->GetCharacterMovement()->PendingLaunchVelocity;
        Check(Launch.Z>350&&Launch.X>=650,TEXT("brief uphill floor-contact loss still permits running jump"));
        const float Stamina=P->Stamina;P->ServerJump();Check(P->GetCharacterMovement()->PendingLaunchVelocity.Equals(Launch)&&FMath::IsNearlyEqual(Stamina,P->Stamina),TEXT("repeated airborne jump cannot add another impulse or spend stamina"));
    });
    const float Original=F->bWinter?Settings->WinterTreeLeafDensity:Settings->TreeLeafDensity;
    At(3.f,[this,F,P](){
        P->GetCharacterMovement()->StopMovementImmediately();
        const auto& T=F->Trees[0];const FVector Target=T.Base+FVector(0,0,1600);
        const FVector Eye=T.Base+FVector(-6500,-8000,6500);
        auto* Camera=GetWorld()->SpawnActor<ACameraActor>(Eye,(Target-Eye).Rotation());
        Camera->GetCameraComponent()->FieldOfView=58;SetViewTarget(Camera);GetHUD()->bShowHUD=false;
    });
    int32 Index=0;
    for(float Value:{0.f,25.f,50.f,75.f,100.f}){
        const float Time=4.f+Index++*2.f;
        At(Time,[this,F,Value,Check](){
            const int32 Count=F->ForestLayers[0]->GetInstanceCount(),Wood=F->ClimbWood[0]->GetInstanceCount();
            Menu->TreeLeaves->SetValue(Value);Menu->TreeLeavesChanged(Value);
            bool Valid=F->ForestLayers[0]->GetStaticMesh()->GetName().EndsWith(Value==0?TEXT("_Bare"):TEXT("_Density"));
            if(Value>0){bool Found=false;for(int32 Slot=0;Slot<F->ForestLayers[0]->GetNumMaterials();Slot++)if(auto* M=Cast<UMaterialInstanceDynamic>(F->ForestLayers[0]->GetMaterial(Slot))){Found=true;Valid&=FMath::IsNearlyEqual(M->K2_GetScalarParameterValue(TEXT("LeafDensity")),Value*.01f);}Valid&=Found;}
            Check(Valid,TEXT("slider applies bare or ranked full-canopy material"));
            bool PropsUpdated=true;int32 VisualTrees=0;
            for(TActorIterator<AActor> It(GetWorld());It;++It){
                TArray<UStaticMeshComponent*> Components;It->GetComponents(Components);
                for(auto* C:Components)if(C->GetStaticMesh()){
                    const FString Name=C->GetStaticMesh()->GetName();
                    if((Name.StartsWith(TEXT("SM_Oak"),ESearchCase::CaseSensitive)||Name.StartsWith(TEXT("SM_Pine"),ESearchCase::CaseSensitive))&&!Name.EndsWith(TEXT("_Wood"))){
                        VisualTrees++;PropsUpdated&=Name.EndsWith(Value==0?TEXT("_Bare"):TEXT("_Density"));
                    }
                }
            }
            Check(VisualTrees>=8&&PropsUpdated,TEXT("yard trees and static lodge scenery also follow the slider"));
            UE_LOG(LogTemp,Display,TEXT("LEAF SLIDER SUMMARY value=%.0f components=%d updated=%d"),Value,VisualTrees,PropsUpdated);
            Check(Count==F->ForestLayers[0]->GetInstanceCount()&&Wood==F->ClimbWood[0]->GetInstanceCount(),TEXT("leaf slider preserves trees and climbable wood"));
            UWildlineUserSettings::Get()->LoadSettings(true);auto* Settings=UWildlineUserSettings::Get();
            Check(FMath::IsNearlyEqual(F->bWinter?Settings->WinterTreeLeafDensity:Settings->TreeLeafDensity,Value),TEXT("leaf slider value survives settings reload"));
        });
        At(Time+1.f,[Value,Shot](){Shot(FString::Printf(TEXT("TreeLeaves%03.0f.png"),Value));});
    }
    At(14.f,[this,Original](){Menu->TreeLeavesChanged(Original);SetMenu(true);Menu->ShowSettings();});
    At(15.f,[Shot](){Shot(TEXT("TreeLeavesSlider.png"));});
    At(16.f,[Passed](){UE_LOG(LogTemp,Display,TEXT("WILDLINE GAMEPLAY BUGS: %s"),*Passed?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Passed?0:1);});
}

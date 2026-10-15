#include "WildlineGame.h"
#include "WildlineCharacter.h"
#include "WildlineWorld.h"
#include "WildlineMenu.h"
#include "WildlineUserSettings.h"
#include "WildlineRules.h"
#include "Components/BoxComponent.h"
#include "Components/SpinBox.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PhysicsVolume.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UnrealClient.h"

void AWildlineController::RunAIBalanceReview(){
    auto Passed=MakeShared<bool>(true);auto Shots=MakeShared<int32>(0),Hits=MakeShared<int32>(0);
    auto Check=[Passed](bool Ok,const TCHAR* What){*Passed&=Ok;UE_LOG(LogTemp,Display,TEXT("AI BALANCE %s: %s"),Ok?TEXT("PASS"):TEXT("FAIL"),What);};
    auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
    auto Hunter=MakeShared<TWeakObjectPtr<AWildlineCharacter>>(),Target=MakeShared<TWeakObjectPtr<AWildlineCharacter>>();
    auto* Settings=UWildlineUserSettings::Get();auto* State=GetWorld()->GetGameState<AWildlineGameState>();const int32 Mode=State->ExpeditionMode;
    const auto Original=Settings->ConfigForMode(Mode);const FVector Origin(5000,4500,4000);
    At(2,[this,Settings,State,Mode,Hunter,Target,Origin,Check](){
        auto C=FWildlineMatchConfig::Preset(Mode,0);C.DefenderBots=C.SquirrelBots=0;C.SquirrelGoal=C.DefenderGoal=30;
        C.BotSightScale=C.BotReactionScale=C.BotAccuracyScale=C.HunterAggression=C.SquirrelCaution=1;
        Settings->ConfigForMode(Mode)=C;State->MatchConfig=C;ChooseRole(false);SetMenu(false);
        GetWorld()->GetDefaultPhysicsVolume()->FluidFriction=0;
        *Hunter=Cast<AWildlineCharacter>(GetPawn());auto* H=Hunter->Get();H->bBot=true;H->BotSlot=0;H->SetActorTickEnabled(false);H->GetCharacterMovement()->DisableMovement();H->SetActorLocation(Origin);
        const FTransform T(Origin+FVector(2500,0,72*H->HunterProfile.HeightScale()));
        auto* A=GetWorld()->SpawnActorDeferred<AWildlineCharacter>(AWildlineCharacter::StaticClass(),T,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        A->bSquirrel=true;UGameplayStatics::FinishSpawningActor(A,T);A->SetActorTickEnabled(false);A->GetCharacterMovement()->SetMovementMode(MOVE_Flying);A->GetCharacterMovement()->MaxFlySpeed=1000;A->GetCharacterMovement()->BrakingDecelerationFlying=0;A->ProtectedUntil=0;A->Health=10000;*Target=A;
        Check(H&&A&&C.AimError(100)==0,TEXT("maximum skill fixtures and zero aim error"));
    });
    int32 Trial=0;
    for(int32 Level=0;Level<3;Level++)for(int32 Weapon=0;Weapon<2;Weapon++)for(float Range:{500.f,2500.f,6000.f})for(float Speed:{0.f,650.f}){
        if(Weapon==1&&Range>3000)continue;
        At(3+Trial++*.2f,[Hunter,Target,Origin,Level,Weapon,Range,Speed,Check,Shots,Hits](){
            auto* H=Hunter->Get();auto* A=Target->Get();if(!H||!A){Check(false,TEXT("accuracy fixtures remain alive"));return;}
            H->LastShot=-100;H->ReloadUntil=0;H->SelectWeapon(Weapon);H->Ammo=WildlineRules::WeaponCapacity(Weapon);H->AIDifficulty=Level;
            A->SetActorLocation(Origin+FVector(Range,Range*.1f,72*H->HunterProfile.HeightScale()));A->GetCharacterMovement()->Velocity=FVector(0,Speed,0);
            H->BotTarget=A;H->BotLastSeenAt=H->ServerTime();H->BotDecisionAt=H->BotShotAt=0;
            const float Before=A->Health;const int32 BeforeShots=H->CosmeticShots;H->PlanBot(H->ServerTime());
            const bool Fired=H->CosmeticShots==BeforeShots+1,Hit=A->Health<Before;*Shots+=Fired;*Hits+=Hit;
            UE_LOG(LogTemp,Display,TEXT("AI ACCURACY level=%d weapon=%d range=%.0f speed=%.0f fired=%d hit=%d"),Level,Weapon,Range,Speed,Fired,Hit);
            Check(Fired&&Hit,TEXT("100 accuracy hits an exposed target across weapons, ranges and difficulty tiers"));
        });
    }
    At(10,[Hunter,Target,Origin](){auto* H=Hunter->Get();auto* A=Target->Get();H->LastShot=-100;H->ReloadUntil=0;H->SelectWeapon(0);H->Ammo=5;H->AIDifficulty=1;A->SetActorLocation(Origin+FVector(2500,-1200,72*H->HunterProfile.HeightScale()));A->GetCharacterMovement()->Velocity=FVector(0,650,0);});
    for(float Time=11;Time<=15;Time++)At(Time,[Hunter,Target,Check,Shots,Hits](){
        auto* H=Hunter->Get();auto* A=Target->Get();H->BotTarget=A;H->BotLastSeenAt=H->ServerTime();H->BotDecisionAt=0;
        const float Before=A->Health;const int32 BeforeShots=H->CosmeticShots;H->PlanBot(H->ServerTime());
        const bool Fired=H->CosmeticShots==BeforeShots+1,Hit=A->Health<Before;*Shots+=Fired;*Hits+=Hit;
        Check(A->GetVelocity().Size2D()>600&&Fired&&Hit,TEXT("live continuously strafing target is hit at 100 accuracy with normal ammo and cadence"));
    });
    At(16,[this,Hunter,Target,Origin,Check](){
        auto* H=Hunter->Get();auto* A=Target->Get();A->GetCharacterMovement()->Velocity=FVector::ZeroVector;A->SetActorLocation(Origin+FVector(500,0,72*H->HunterProfile.HeightScale()));
        auto* Wall=GetWorld()->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Box);Box->SetBoxExtent(FVector(40,300,500));Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();Wall->SetActorLocation(Origin+FVector(250,0,0));
        const float Before=A->Health;const int32 BeforeShots=H->CosmeticShots;H->Ammo=5;H->ReloadUntil=0;H->BotShotAt=H->BotDecisionAt=0;H->PlanBot(H->ServerTime());
        Check(H->CosmeticShots==BeforeShots,TEXT("maximum difficulty cannot acquire or fire at a squirrel through cover"));
        H->LastShot=-100;const float Scale=H->HunterProfile.HeightScale();const FVector MuzzleOrigin=H->GetActorLocation()+FVector(0,0,72*Scale)+H->GetActorRightVector()*(2*Scale);
        H->ServerFire_Implementation((A->GetActorLocation()-MuzzleOrigin).GetSafeNormal());Check(A->Health==Before,TEXT("physical cover blocks even a forced maximum-accuracy shot"));Wall->Destroy();
    });
    At(18,[this,Settings,Mode,Hunter,Check](){
        auto* H=Hunter->Get();H->bBot=false;GetGameInstance<UWildlineGameInstance>()->PreferredMode=Mode;SetMenu(true);Menu->ShowBots();Menu->RefreshConfig();
        for(auto* Box:{Menu->BotSight,Menu->BotReaction,Menu->BotAccuracy,Menu->HunterAggression,Menu->SquirrelCaution}){Box->SetValue(200);Check(Box->GetValue()==100,TEXT("strength controls cap at 100"));Box->SetValue(-10);Check(Box->GetValue()==0,TEXT("strength controls allow zero"));Box->SetValue(100);}
        Settings->LoadSettings(true);const auto& C=Settings->ConfigForMode(Mode);
        Check(C.BotSightScale==1&&C.BotReactionScale==1&&C.BotAccuracyScale==1&&C.HunterAggression==1&&C.SquirrelCaution==1,TEXT("all five maximum values persist as normalized strengths"));
        Check(C.PursuitSpeed(320)==580&&C.SightMultiplier()==3&&C.SearchMemory(4)==20&&C.ReactionDelay(1.4f)==.02f&&C.ShotDelay(3.8f,.85f)==.85f,TEXT("100 means maximum pursuit, sight, memory, reflexes and legal firing speed"));
    });
    if(!FParse::Param(FCommandLine::Get(),TEXT("NullRHI")))At(21,[](){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/AIStrength100.png"),true,false);});
    At(23,[Passed,Shots,Hits,Settings,Mode,Original](){
        Settings->ConfigForMode(Mode)=Original;Settings->Persist();
        UE_LOG(LogTemp,Display,TEXT("WILDLINE AI BALANCE: %s exposedShots=%d hits=%d"),*Passed?TEXT("PASS"):TEXT("FAIL"),*Shots,*Hits);
        FFileHelper::SaveStringToFile(*Passed?TEXT("PASS\n"):TEXT("FAIL\n"),*(FPaths::ProjectSavedDir()/TEXT("AIBalanceResult.txt")));FPlatformMisc::RequestExitWithStatus(false,*Passed?0:1);
    });
}

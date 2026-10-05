#include "WildlineGame.h"
#include "WildlineWorld.h"
#include "WildlineCharacter.h"
#include "WildlineRules.h"
#include "WildlineAudio.h"
#include "WildlineUserSettings.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GameFramework/GameSession.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWildlineMatchTest,"Wildline.Gameplay.MatchConfigsAndAudio",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWildlineMatchTest::RunTest(const FString& Parameters){
    FWildlineMatchConfig C;C.MaxPlayers=99;C.DurationSeconds=-2;C.SquirrelGoal=100;C.DefenderGoal=0;C.Clamp();
    TestTrue(TEXT("Match rules enforce supported players, duration and attainable goals"),C.MaxPlayers==8&&C.DurationSeconds==60&&C.SquirrelGoal==30&&C.DefenderGoal==1);
    TestEqual(TEXT("Hill timeout rewards defending"),WildlineRules::TimeoutWinner(2),1);
    TestTrue(TEXT("Scope sensitivity has an independent saved default"),GetDefault<UWildlineUserSettings>()->ScopeSensitivity<GetDefault<UWildlineUserSettings>()->MouseSensitivity);
    const FString Fixture=FPaths::ProjectSavedDir()/TEXT("MatchSettingsFixture.ini");
    auto* Saved=NewObject<UWildlineUserSettings>();Saved->ClassicConfig.MaxPlayers=2;Saved->FarmConfig.SquirrelGoal=9;Saved->HillConfig.DurationSeconds=1200;Saved->ScopeSensitivity=.42f;Saved->MouseSensitivity=1.3f;Saved->SaveConfig(CPF_Config,*Fixture);
    auto* Loaded=NewObject<UWildlineUserSettings>();Loaded->LoadConfig(UWildlineUserSettings::StaticClass(),*Fixture);
    TestTrue(TEXT("All three mode configurations survive independent settings persistence"),Loaded->ClassicConfig.MaxPlayers==2&&Loaded->FarmConfig.SquirrelGoal==9&&Loaded->HillConfig.DurationSeconds==1200&&Loaded->HillConfig.SquirrelGoal==10);
    TestTrue(TEXT("Normal and scope sensitivity retain separate saved values"),FMath::IsNearlyEqual(Loaded->ScopeSensitivity,.42f)&&FMath::IsNearlyEqual(Loaded->MouseSensitivity,1.3f));
    for(int32 I=WildlineAudio::Rifle;I<=WildlineAudio::Win;I++){
        const auto PCM=WildlineAudio::Samples(static_cast<WildlineAudio::Event>(I));int64 Energy=0;int32 Peak=0;
        for(int16 Sample:PCM){Energy+=int64(Sample)*Sample;Peak=FMath::Max(Peak,FMath::Abs(int32(Sample)));}
        TestTrue(TEXT("Every sound is non-silent finite PCM without clipping"),PCM.Num()>1000&&PCM.Num()<=30000&&Energy>100000&&Peak<32767);
    }
    return true;
}
#endif

void AWildlineController::RunHillSmokeTest(){
    auto Result=MakeShared<bool>(true);auto Check=[Result](bool Ok,const TCHAR* What){*Result&=Ok;UE_LOG(LogTemp,Display,TEXT("HILL CHECK %s: %s"),What,Ok?TEXT("PASS"):TEXT("FAIL"));};
    auto* F=AWildlineWorld::Find(GetWorld());auto* S=GetWorld()->GetGameState<AWildlineGameState>();
    Check(F&&F->bResidential&&!F->bFarm&&S&&S->ExpeditionMode==2,TEXT("Residential map and King of the Hill mode"));if(!F||!S){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    S->MatchConfig=FWildlineMatchConfig();S->MatchConfig.SquirrelGoal=30;S->MatchConfig.DefenderGoal=30;
    ChooseRole(true);auto* P=Cast<AWildlineCharacter>(GetPawn());
    for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It){It->bBot=false;if(*It!=P)It->SetActorLocation(FVector(-7000,7000,1000));}
    Check(F->CoursePlatforms.Num()==8&&F->ResidentialProps.Num()>100,TEXT("Paired obstacles, houses, fences and power lines exist"));
    Check(F->NearestTree(F->FeederLocation,2000)==INDEX_NONE,TEXT("Feeder pole cannot be climbed and trees cannot bypass the course"));
    auto Place=[P](FVector T){P->SetActorLocation(T+FVector(0,0,13),false,nullptr,ETeleportType::TeleportPhysics);P->GetCharacterMovement()->StopMovementImmediately();P->GetCharacterMovement()->SetMovementMode(MOVE_Walking);P->Tick(.01f);};
    Place(F->FeederLocation);Check(S->StoredAcorns==0&&P->FeederVisits==0,TEXT("Direct feeder arrival without checkpoints earns no point"));
    Place(F->CourseTarget(0,0));Place(F->CourseTarget(0,3));Check(P->CourseCheckpoint==1,TEXT("Skipping a checkpoint rejected"));
    Place(F->CourseTarget(0,1));Check(P->CourseCheckpoint==2,TEXT("First platform landing advances progress"));
    Place(F->CourseTarget(0,0));Check(P->CourseCheckpoint==0,TEXT("Falling to the lawn clears progress"));
    for(int32 Lane=0;Lane<2;Lane++){
        Place(F->CourseTarget(Lane,0));for(int32 Step=1;Step<=5;Step++)Place(F->CourseTarget(Lane,Step));
        Check(P->CourseCheckpoint==0&&P->FeederVisits==Lane+1&&S->StoredAcorns==Lane+1,TEXT("Either full route awards exactly one trip"));
        Place(F->FeederLocation);Check(P->FeederVisits==Lane+1,TEXT("Camping on the feeder cannot score again"));
    }
    Place(F->CourseTarget(0,0));P->ServerClimb();Check(P->TreeIndex==INDEX_NONE,TEXT("Course props do not enable trunk climbing"));
    S->Winner=1;GetWorld()->GetAuthGameMode<AWildlineGameMode>()->RestartRound();Check(P->CourseCheckpoint==0&&P->FeederVisits==0&&S->StoredAcorns==0,TEXT("Restart resets trips and progress"));
    S->MatchConfig.SquirrelGoal=30;S->MatchConfig.DefenderGoal=30;
    // An autonomous squirrel must traverse with real movement, gravity, capsule
    // collision and ServerJump, proving that the authored gaps are reachable.
    P->bBot=true;P->Stamina=100;P->SetActorLocation(F->CourseTarget(0,0)+FVector(0,0,13));
    FTimerHandle End;GetWorldTimerManager().SetTimer(End,FTimerDelegate::CreateWeakLambda(this,[this,Check,Result,P,F,S](){
        UE_LOG(LogTemp,Display,TEXT("HILL BOT pos=%s checkpoint=%d visits=%d stamina=%.1f"),*P->GetActorLocation().ToString(),P->CourseCheckpoint,P->FeederVisits,P->Stamina);
        Check(P->FeederVisits>=2,TEXT("Actual squirrel jumps complete repeated course trips"));P->bBot=false;
        S->MatchConfig.SquirrelGoal=S->StoredAcorns+1;GetWorld()->GetAuthGameMode<AWildlineGameMode>()->Deposit(1);Check(S->Winner==2,TEXT("Configured squirrel goal ends the match"));
        ChooseRole(false);auto* H=Cast<AWildlineCharacter>(GetPawn());H->SelectWeapon(1);H->ThermalReadyAt=0;H->ServerThermal();Check(H->ThermalUntil==0&&!H->IsThermalActive(),TEXT("Shotgun thermal rejected by server"));
        H->SelectWeapon(0);H->ServerThermal();const float Until=H->ThermalUntil;Check(FMath::IsNearlyEqual(Until-H->ServerTime(),20,.1f)&&FMath::IsNearlyEqual(H->ThermalReadyAt-Until,120,.1f),TEXT("Rifle thermal lasts 20 seconds then cools for 120"));
        H->SelectWeapon(1);Check(!H->IsThermalActive()&&H->ThermalUntil==0&&H->ThermalReadyAt>H->ServerTime(),TEXT("Changing to shotgun removes thermal but preserves cooldown"));
        FFileHelper::SaveStringToFile(*Result?TEXT("PASS\n"):TEXT("FAIL\n"),*(FPaths::ProjectSavedDir()/TEXT("HillSmokeResult.txt")));
        UE_LOG(LogTemp,Display,TEXT("WILDLINE HILL SMOKE: %s"),*Result?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Result?0:1);
    }),45.f,false);
}

void AWildlineController::RunHillNetworkProbe(){
    auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
    if(HasAuthority()){
        At(5,[this](){auto* S=GetWorld()->GetGameState<AWildlineGameState>();S->MatchConfig.MaxPlayers=2;S->MatchConfig.DurationSeconds=240;S->MatchConfig.SquirrelGoal=6;S->MatchConfig.DefenderGoal=9;S->ForceNetUpdate();GetWorld()->GetAuthGameMode()->GameSession->MaxPlayers=2;});
        At(28,[this](){auto* S=GetWorld()->GetGameState<AWildlineGameState>();S->StoredAcorns=2;S->ForceNetUpdate();for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It){It->SetActorTickEnabled(false);if(!It->bBot){if(It->bSquirrel){It->CourseLane=1;It->CourseCheckpoint=3;It->FeederVisits=2;}else{It->SelectWeapon(0);It->ServerThermal();}It->ForceNetUpdate();}}});
        At(75,[this](){FPlatformMisc::RequestExitWithStatus(false,bNetProbePassed?0:1);});
        At(35,[this](){for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(!It->bBot&&It->bSquirrel)It->MulticastSound(WildlineAudio::Checkpoint);});
    }
    At(40,[this](){
        const auto* F=AWildlineWorld::Find(GetWorld());const auto* S=GetWorld()->GetGameState<AWildlineGameState>();int32 Humans=0;bool Progress=false,Thermal=false,Sound=false;
        for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(!It->bBot&&!It->bPreviewHunter){Humans++;if(It->bSquirrel){Progress=It->CourseLane==1&&It->CourseCheckpoint==3&&It->FeederVisits==2;Sound=It->CosmeticSounds>0;}else Thermal=It->SelectedWeapon==0&&It->ThermalReadyAt>It->ServerTime()&&FMath::IsNearlyEqual(It->ThermalReadyAt-It->ThermalUntil,120.f,.1f);}
        const bool Ok=F&&F->bResidential&&S&&S->ExpeditionMode==2&&S->MatchConfig.MaxPlayers==2&&S->MatchConfig.DurationSeconds==240&&S->MatchConfig.SquirrelGoal==6&&S->MatchConfig.DefenderGoal==9&&S->StoredAcorns==2&&Humans==2&&Progress&&Thermal&&Sound;
        UE_LOG(LogTemp,Display,TEXT("WILDLINE HILL NETWORK %s: %s humans=%d progress=%d thermal=%d sound=%d"),HasAuthority()?TEXT("Host"):TEXT("Client"),Ok?TEXT("PASS"):TEXT("FAIL"),Humans,Progress,Thermal,Sound);if(HasAuthority())bNetProbePassed=Ok;else FPlatformMisc::RequestExitWithStatus(false,Ok?0:1);
    });
}

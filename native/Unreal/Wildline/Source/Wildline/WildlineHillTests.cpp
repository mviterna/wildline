#include "WildlineGame.h"
#include "WildlineWorld.h"
#include "WildlineCharacter.h"
#include "WildlineRules.h"
#include "WildlineAudio.h"
#include "WildlineUserSettings.h"
#include "WildlineLobby.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GameFramework/GameSession.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Sound/SoundWave.h"

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
        if(I==WildlineAudio::Footstep||I==WildlineAudio::Pawstep)continue;
        const auto PCM=WildlineAudio::Samples(static_cast<WildlineAudio::Event>(I));int64 Energy=0;int32 Peak=0;
        for(int16 Sample:PCM){Energy+=int64(Sample)*Sample;Peak=FMath::Max(Peak,FMath::Abs(int32(Sample)));}
        TestTrue(TEXT("Every sound is non-silent finite PCM without clipping"),PCM.Num()>1000&&PCM.Num()<=30000&&Energy>100000&&Peak<32767);
    }
    for(int32 I=0;I<4;I++){
        const auto* Step=WildlineAudio::FootstepSound(I);
        if(TestNotNull(TEXT("Requested recorded footstep is cookable"),Step))TestTrue(TEXT("Single non-looping footfall"),!Step->bLooping&&Step->Duration>.2f&&Step->Duration<.4f&&Step->NumChannels==1);
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
    int32 Hoses=0,Pools=0,Houses=0;bool Mounted=true,Recessed=true,Grounded=true;
    for(auto* Prop:F->ResidentialProps){
        const FString Name=Prop->GetStaticMesh()->GetName();
        if(Name.Contains(TEXT("House"))||Name==TEXT("SM_KitCottage")){
            Houses++;const FBox Bounds=Prop->GetStaticMesh()->GetBoundingBox().TransformBy(Prop->GetComponentTransform());
            Grounded&=FMath::Abs(Bounds.Min.Z-WildlineRules::FarmGround(Bounds.GetCenter().X,Bounds.GetCenter().Y))<3;
        }
        if(Name==TEXT("SM_KitPool")){
            Pools++;const FBox Bounds=Prop->GetStaticMesh()->GetBoundingBox().TransformBy(Prop->GetComponentTransform());
            FHitResult Floor;FCollisionQueryParams Query(SCENE_QUERY_STAT(PoolFloor),true,P);
            const FVector Centre=Bounds.GetCenter();const bool Hit=GetWorld()->LineTraceSingleByChannel(Floor,FVector(Centre.X,Centre.Y,Bounds.Max.Z+20),FVector(Centre.X,Centre.Y,Bounds.Min.Z-20),ECC_Visibility,Query);
            Recessed&=FMath::Abs(Bounds.Max.Z-WildlineRules::FarmGround(Centre.X,Centre.Y)-2)<1&&F->GroundHeight(Centre.X,Centre.Y)<Bounds.Min.Z&&Hit&&Floor.ImpactPoint.Z<Bounds.Max.Z-50;
            UE_LOG(LogTemp,Display,TEXT("HILL POOL rim=%.2f floor=%.2f terrain=%.2f"),Bounds.Max.Z,Floor.ImpactPoint.Z,F->GroundHeight(Centre.X,Centre.Y));
        }
        if(Name==TEXT("SM_garden_hose_wall_mounted_01")){
            Hoses++;const FVector Centre=Prop->Bounds.Origin;const FVector Direction(0,Centre.Y<0?-1.f:1.f,0);
            FHitResult Wall;FCollisionQueryParams Query(SCENE_QUERY_STAT(HoseSupport),true,P);
            const bool Hit=GetWorld()->LineTraceSingleByChannel(Wall,Centre,Centre+Direction*25,ECC_Visibility,Query);
            const auto* Support=Cast<UStaticMeshComponent>(Wall.GetComponent());const FString SupportName=Support&&Support->GetStaticMesh()?Support->GetStaticMesh()->GetName():FString();
            Mounted&=Hit&&(SupportName.Contains(TEXT("House"))||SupportName==TEXT("SM_KitCottage"));
            UE_LOG(LogTemp,Display,TEXT("HILL HOSE mount=%s hit=%d support=%s distance=%.2f"),*Centre.ToString(),Hit,*SupportName,Wall.Distance);
        }
    }
    Check(Hoses==6&&Mounted,TEXT("All six hose reels have a real supporting house wall"));
    Check(Pools==1&&Recessed,TEXT("Pool coping meets lawn and terrain clears the basin"));
    Check(Houses==6&&Grounded,TEXT("All six neighborhood houses meet the ground"));
    Check(F->NearestTree(F->FeederLocation,200)==INDEX_NONE,TEXT("Smooth feeder pole remains unclimbable"));
    bool RecordedFootsteps=true;for(int32 I=0;I<4;I++){const auto* Step=WildlineAudio::FootstepSound(I);RecordedFootsteps&=Step&&!Step->bLooping&&Step->Duration>.2f&&Step->Duration<.4f;}
    Check(RecordedFootsteps,TEXT("All four requested concrete footstep samples exist in the packaged map"));
    auto Place=[P](FVector T){P->SetActorLocation(T+FVector(0,0,13),false,nullptr,ETeleportType::TeleportPhysics);P->GetCharacterMovement()->StopMovementImmediately();P->GetCharacterMovement()->SetMovementMode(MOVE_Walking);P->Tick(.01f);};
    Place(F->FeederLocation);Check(S->StoredAcorns==0&&P->FeederVisits==0,TEXT("Direct feeder arrival without checkpoints earns no point"));
    Place(F->CourseTarget(0,0));Place(F->CourseTarget(0,3));Check(P->CourseCheckpoint==1,TEXT("Skipping a checkpoint rejected"));
    Place(F->CourseTarget(0,1));Check(P->CourseCheckpoint==2,TEXT("First platform landing advances progress"));
    Place(F->CourseTarget(0,0));Check(P->CourseCheckpoint==0,TEXT("Falling to the lawn clears progress"));
    for(int32 Lane=0;Lane<2;Lane++){
        Place(F->CourseTarget(Lane,0));for(int32 Step=1;Step<=5;Step++){
            Place(F->CourseTarget(Lane,Step));FHitResult Landing;FCollisionQueryParams Q(SCENE_QUERY_STAT(HillProbeLanding),false,P);GetWorld()->LineTraceSingleByChannel(Landing,P->GetActorLocation(),P->GetActorLocation()-FVector(0,0,30),ECC_Visibility,Q);
            UE_LOG(LogTemp,Display,TEXT("HILL LAND lane=%d step=%d progress=%d pos=%s support=%s mesh=%s"),Lane,Step,P->CourseCheckpoint,*P->GetActorLocation().ToString(),*Landing.ImpactPoint.ToString(),*GetNameSafe(Landing.Component.Get()));
        }
        Check(P->CourseCheckpoint==0&&P->FeederVisits==Lane+1&&S->StoredAcorns==Lane+1,TEXT("Either full route awards exactly one trip"));
        Place(F->FeederLocation);Check(P->FeederVisits==Lane+1,TEXT("Camping on the feeder cannot score again"));
    }
    Place(F->CourseTarget(0,0));P->ServerClimb();Check(P->TreeIndex==INDEX_NONE,TEXT("Course props do not enable trunk climbing"));
    S->Winner=1;GetWorld()->GetAuthGameMode<AWildlineGameMode>()->RestartRound();Check(P->CourseCheckpoint==0&&P->FeederVisits==0&&S->StoredAcorns==0,TEXT("Restart resets trips and progress"));
    S->MatchConfig.SquirrelGoal=30;S->MatchConfig.DefenderGoal=30;
    // Restart refreshes the match bots. Isolate the physical traversal probe
    // again so a defender cannot interrupt it with a shot or a respawn.
    for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(*It!=P){It->bBot=false;It->SetActorLocation(FVector(-7000,7000,1000));}
    // An autonomous squirrel must traverse with real movement, gravity, capsule
    // collision and ServerJump, proving that the authored gaps are reachable.
    int32 ProbeLane=0;FParse::Value(FCommandLine::Get(),TEXT("WildlineCourseLane="),ProbeLane);ProbeLane=FMath::Clamp(ProbeLane,0,1);
    P->bBot=true;P->BotSlot=ProbeLane;P->Stamina=100;P->SetActorLocation(F->CourseTarget(ProbeLane,0)+FVector(0,0,13));
    FTimerHandle Trace;GetWorldTimerManager().SetTimer(Trace,FTimerDelegate::CreateWeakLambda(this,[P](){const auto& Floor=P->GetCharacterMovement()->CurrentFloor.HitResult;UE_LOG(LogTemp,Display,TEXT("HILL SUPPORT component=%s item=%d point=%s"),*GetNameSafe(Floor.Component.Get()),Floor.Item,*Floor.ImpactPoint.ToString());UE_LOG(LogTemp,Display,TEXT("HILL ROUTE actor=%s valid=%d bot=%d pos=%s checkpoint=%d visits=%d tree=%d falling=%d stamina=%.1f dead=%d delta=%.6f dilation=%.3f"),*P->GetName(),IsValid(P),P->bBot,*P->GetActorLocation().ToString(),P->CourseCheckpoint,P->FeederVisits,P->TreeIndex,P->GetCharacterMovement()->IsFalling(),P->Stamina,P->bDead,P->GetWorld()->GetDeltaSeconds(),P->CustomTimeDilation);}),5.f,true);
    FTimerHandle End;GetWorldTimerManager().SetTimer(End,FTimerDelegate::CreateWeakLambda(this,[this,Check,Result,P,F,S](){
        const auto* Lobby=AWildlineLobby::Find(GetWorld());Check(Lobby&&Lobby->IsHidden(),TEXT("Elevated waiting-room architecture is hidden during outdoor play"));
        UE_LOG(LogTemp,Display,TEXT("HILL BOT pos=%s checkpoint=%d visits=%d stamina=%.1f"),*P->GetActorLocation().ToString(),P->CourseCheckpoint,P->FeederVisits,P->Stamina);
        Check(P->FeederVisits>=2,TEXT("Actual squirrel jumps complete repeated course trips"));P->bBot=false;
        S->MatchConfig.SquirrelGoal=S->StoredAcorns+1;GetWorld()->GetAuthGameMode<AWildlineGameMode>()->Deposit(1);Check(S->Winner==2,TEXT("Configured squirrel goal ends the match"));
        ChooseRole(false);auto* H=Cast<AWildlineCharacter>(GetPawn());H->SelectWeapon(1);H->ThermalReadyAt=0;H->ServerThermal();Check(H->ThermalUntil==0&&!H->IsThermalActive(),TEXT("Shotgun thermal rejected by server"));
        H->SelectWeapon(0);H->ServerThermal();const float Until=H->ThermalUntil;Check(FMath::IsNearlyEqual(Until-H->ServerTime(),20,.1f)&&FMath::IsNearlyEqual(H->ThermalReadyAt-Until,120,.1f),TEXT("Rifle thermal lasts 20 seconds then cools for 120"));
        H->SelectWeapon(1);Check(!H->IsThermalActive()&&H->ThermalUntil==0&&H->ThermalReadyAt>H->ServerTime(),TEXT("Changing to shotgun removes thermal but preserves cooldown"));
        FFileHelper::SaveStringToFile(*Result?TEXT("PASS\n"):TEXT("FAIL\n"),*(FPaths::ProjectSavedDir()/TEXT("HillSmokeResult.txt")));
        UE_LOG(LogTemp,Display,TEXT("WILDLINE HILL SMOKE: %s"),*Result?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Result?0:1);
    }),75.f,false);
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

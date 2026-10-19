#include "WildlineGame.h"
#include "Misc/App.h"
#include "WildlineLobby.h"
#include "WildlineRules.h"
#include "WildlineWorld.h"
#include "WildlineLivestock.h"
#include "WildlineCharacter.h"
#include "WildlineSquirrelAnim.h"
#include "WildlineHunterAnim.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "WildlineHUD.h"
#include "WildlineMenu.h"
#include "WildlineIntro.h"
#include "WildlineHunterCreator.h"
#include "WildlineUserSettings.h"
#include "GameFramework/GameSession.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "AIController.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "GameFramework/HUD.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HighResScreenshot.h"
#include "Engine/GameViewportClient.h"
#include "PhysicsEngine/BodySetup.h"
#include "ImageUtils.h"
#include "HAL/IConsoleManager.h"
#include "UnrealClient.h"
#include "ImageCore.h"
#include "Misc/Base64.h"
#include "Materials/MaterialInstanceDynamic.h"
#if WITH_EDITOR
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#endif

void AWildlineGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AWildlineGameState,MatchConfig);DOREPLIFETIME(AWildlineGameState,HunterKills);DOREPLIFETIME(AWildlineGameState,StoredAcorns);DOREPLIFETIME(AWildlineGameState,Winner);DOREPLIFETIME(AWildlineGameState,RoundStartedAt);DOREPLIFETIME(AWildlineGameState,bRoundStarted);DOREPLIFETIME(AWildlineGameState,LobbyPlayers);DOREPLIFETIME(AWildlineGameState,LobbyReadyPlayers);DOREPLIFETIME(AWildlineGameState,HunterDifficulty);DOREPLIFETIME(AWildlineGameState,ExpeditionMode);DOREPLIFETIME(AWildlineGameState,TakenPeanuts);}
float AWildlineGameState::RoundRemaining()const{return bRoundStarted?FMath::Max(0.f,MatchConfig.DurationSeconds-(GetServerWorldTimeSeconds()-RoundStartedAt)):MatchConfig.DurationSeconds;}
AWildlineGameMode::AWildlineGameMode(){PrimaryActorTick.bCanEverTick=true;DefaultPawnClass=AWildlineCharacter::StaticClass();PlayerControllerClass=AWildlineController::StaticClass();HUDClass=AWildlineHUD::StaticClass();GameStateClass=AWildlineGameState::StaticClass();}
void AWildlineGameMode::InitGame(const FString& MapName,const FString& Options,FString& ErrorMessage){
    Super::InitGame(MapName,Options,ErrorMessage);if(auto* Settings=UWildlineUserSettings::Get()){Settings->LoadSettings(false);const int32 Mode=MapName.Contains(TEXT("Neighborhood"))?2:MapName.Contains(TEXT("Farmstead"))?1:0;auto Config=Settings->ConfigForMode(Mode);Config.Clamp();if(GameSession)GameSession->MaxPlayers=Config.MaxPlayers;}
}
void AWildlineGameMode::BeginPlay(){Super::BeginPlay();
    if(auto* Settings=UWildlineUserSettings::Get()){Settings->LoadSettings(false);if(auto* State=GetGameState<AWildlineGameState>())State->HunterDifficulty=FMath::Clamp(Settings->HunterDifficulty,0,2);}
    if(!AWildlineWorld::Find(GetWorld()))GetWorld()->SpawnActor<AWildlineWorld>();
    if(auto* Forest=AWildlineWorld::Find(GetWorld());Forest){auto* State=GetGameState<AWildlineGameState>();State->ExpeditionMode=Forest->bResidential?2:Forest->bFarm?1:0;if(auto* Settings=UWildlineUserSettings::Get())State->MatchConfig=Settings->ConfigForMode(State->ExpeditionMode);State->MatchConfig.Clamp();State->MatchConfig.ClassicLayout=Forest->bWinter?1:0;if(Forest->bResidential)State->MatchConfig.NeighborhoodLayout=GetWorld()->GetMapName().Contains(TEXT("Cedar"))?1:0;}
    if(auto* Forest=AWildlineWorld::Find(GetWorld());Forest&&Forest->bFarm){
        GetGameState<AWildlineGameState>()->ExpeditionMode=WildlineRules::FarmMode;
        for(int32 I=0;I<6;I++){
            const FVector Points[]={FVector(-1000,650,0),FVector(250,620,0),FVector(900,2050,0),FVector(2400,2450,0),FVector(0,-650,0),FVector(1900,-2000,0)};
            FVector Position=Points[I];Position.Z=Forest->GroundHeight(Position.X,Position.Y)+85;
            const FTransform Transform(FRotator(0,I*47,0),Position);
            auto* Animal=GetWorld()->SpawnActorDeferred<AWildlineLivestock>(AWildlineLivestock::StaticClass(),Transform,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
            if(Animal){Animal->Species=I>=4?1:0;Animal->Home=Position;Animal->Phase=I*1.7f;UGameplayStatics::FinishSpawningActor(Animal,Transform);}
        }
    }
}
void AWildlineGameMode::Tick(float Delta){Super::Tick(Delta);UpdateWaitingRooms();auto* S=GetGameState<AWildlineGameState>();if(S&&S->bRoundStarted&&!S->Winner&&S->RoundRemaining()<=0)S->Winner=S->MatchConfig.TimeWinner?S->MatchConfig.TimeWinner:WildlineRules::TimeoutWinner(S->ExpeditionMode);}
void AWildlineGameMode::PickRole(APlayerController* PC,bool bAnimal){
    if(!PC)return;
    if(auto* P=Cast<AWildlineCharacter>(PC->GetPawn());P&&!P->bInLobby&&P->bSquirrel==bAnimal&&GetGameState<AWildlineGameState>()->bRoundStarted)return;
    APawn* Old=PC->GetPawn();if(auto* OldAnimal=Cast<AWildlineCharacter>(Old))OldAnimal->ReturnCarriedPeanuts();FVector P=SpawnLocation(bAnimal,GetNumPlayers());
    auto* New=GetWorld()->SpawnActorDeferred<AWildlineCharacter>(AWildlineCharacter::StaticClass(),FTransform(FRotator::ZeroRotator,P),PC,nullptr,ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    if(!New)return;New->bSquirrel=bAnimal;UGameplayStatics::FinishSpawningActor(New,FTransform(FRotator::ZeroRotator,P));PC->Possess(New);if(Old)Old->Destroy();
    auto* S=GetGameState<AWildlineGameState>();if(S&&!S->bRoundStarted){S->bRoundStarted=true;S->RoundStartedAt=S->GetServerWorldTimeSeconds();}RefreshBots();
}
void AWildlineGameMode::RefreshBots(){
    if(GetWorld()->bIsTearingDown)return;const auto* State=GetGameState<AWildlineGameState>();if(!State||!State->bRoundStarted)return;
    const auto& C=State->MatchConfig;
    TArray<AWildlineCharacter*> Existing;
    for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(It->bBot&&!It->bPreviewHunter)Existing.Add(*It);
    // Human joins/leaves retain existing bots and their carried food and health.
    for(auto* B:Existing){const int32 Count=B->bSquirrel?C.SquirrelBots:C.DefenderBots;if(B->BotSlot<0||B->BotSlot>=Count){B->ReturnCarriedPeanuts();if(B->Controller)B->Controller->Destroy();B->Destroy();}}
    for(bool Animal:{false,true})for(int32 I=0;I<(Animal?C.SquirrelBots:C.DefenderBots);I++){
        const auto& Setup=Animal?C.Squirrels[I]:C.Defenders[I];
        AWildlineCharacter* B=nullptr;for(auto* E:Existing)if(IsValid(E)&&E->bSquirrel==Animal&&E->BotSlot==I){B=E;break;}
        if(!B){const FTransform T(FRotator::ZeroRotator,SpawnLocation(Animal,I+1));B=GetWorld()->SpawnActorDeferred<AWildlineCharacter>(AWildlineCharacter::StaticClass(),T,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);if(!B)continue;B->bSquirrel=Animal;B->bBot=true;B->BotSlot=I;B->AIDifficulty=Setup.Difficulty;UGameplayStatics::FinishSpawningActor(B,T);}
        B->AIDifficulty=Setup.Difficulty;if(!Animal)B->SelectWeapon(Setup.Weapon);B->ForceNetUpdate();
    }
}
FVector AWildlineGameMode::SpawnLocation(bool Animal,int32 Index) const{
    const auto* Forest=AWildlineWorld::Find(GetWorld());const bool Farm=Forest&&Forest->bFarm;
    if(Forest&&Forest->bResidential){FVector P=Animal?Forest->CourseStart+FVector(-160,Index*40-160,13):FVector(1500,-1600+Index*100,320);return P;}
    FVector P=Farm?(Animal?FVector(-3900,Index*80-160,0):FVector(1250,700+Index*90,0)):(Animal?FVector(350+Index*65,200+Index*65,0):FVector(0,750+Index*65,0));
    P.Z=(Forest?Forest->GroundHeight(P.X,P.Y):WildlineRules::Ground(P.X,P.Y))+(Animal?13:110);return P;
}
void AWildlineGameMode::Logout(AController* Exiting){if(Exiting)if(auto* P=Cast<AWildlineCharacter>(Exiting->GetPawn()))P->ReturnCarriedPeanuts();Super::Logout(Exiting);RefreshBots();}
void AWildlineGameMode::RecordKill(AWildlineCharacter* Victim,AController* Hunter){
    auto* S=GetGameState<AWildlineGameState>();if(!S||S->Winner)return;S->HunterKills++;if(S->HunterKills>=S->MatchConfig.DefenderGoal)S->Winner=1;
    FTimerHandle Handle;const TWeakObjectPtr<AWildlineCharacter> Weak=Victim;const float Expected=Victim->RespawnAt;GetWorldTimerManager().SetTimer(Handle,FTimerDelegate::CreateWeakLambda(this,[this,Weak,Expected](){if(Weak.IsValid()&&Weak->bDead&&FMath::IsNearlyEqual(Weak->RespawnAt,Expected))RespawnAnimal(Weak);}),S->MatchConfig.RespawnSeconds,false);
}
void AWildlineGameMode::RespawnAnimal(TWeakObjectPtr<AWildlineCharacter> Weak){auto* P=Weak.Get();if(!P)return;auto* S=GetGameState<AWildlineGameState>();if(S&&S->Winner)return;P->bDead=false;P->Health=S?S->MatchConfig.SquirrelHealth:40;P->ProtectedUntil=P->ServerTime()+(S?S->MatchConfig.SpawnProtectionSeconds:0);P->Bag=0;P->CourseCheckpoint=0;P->CourseLane=-1;P->Stamina=100;P->RespawnAt=0;P->TreeIndex=INDEX_NONE;P->ResetBotAwareness();FVector Pos=SpawnLocation(true,P->bBot?P->BotSlot+1:0);P->SetActorLocation(Pos,false,nullptr,ETeleportType::TeleportPhysics);P->OnRepDead();P->ForceNetUpdate();}
void AWildlineGameMode::Deposit(int32 Count){auto* S=GetGameState<AWildlineGameState>();if(!S||S->Winner||!S->bRoundStarted)return;S->StoredAcorns+=Count;if(S->StoredAcorns>=S->MatchConfig.SquirrelGoal)S->Winner=2;}
void AWildlineGameMode::RestartRound(){auto* S=GetGameState<AWildlineGameState>();if(!S||!S->Winner)return;if(auto* Settings=UWildlineUserSettings::Get())S->MatchConfig=Settings->ConfigForMode(S->ExpeditionMode);S->MatchConfig.Clamp();if(S->ExpeditionMode==0)S->MatchConfig.ClassicLayout=GetWorld()->GetMapName().Contains(TEXT("Frostwood"))?1:0;if(S->ExpeditionMode==2)S->MatchConfig.NeighborhoodLayout=GetWorld()->GetMapName().Contains(TEXT("Cedar"))?1:0;if(GameSession)GameSession->MaxPlayers=S->MatchConfig.MaxPlayers;S->HunterKills=0;S->StoredAcorns=0;S->TakenPeanuts.Empty();S->Winner=0;S->RoundStartedAt=S->GetServerWorldTimeSeconds();for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It){auto* P=*It;P->bDead=false;P->Health=P->bSquirrel?S->MatchConfig.SquirrelHealth:100;P->ProtectedUntil=P->bSquirrel?P->ServerTime()+S->MatchConfig.SpawnProtectionSeconds:0;P->RifleAmmo=5;P->ShotgunAmmo=2;P->Ammo=WildlineRules::WeaponCapacity(P->SelectedWeapon);P->Bag=0;P->BagPeanuts.Empty();P->CourseCheckpoint=0;P->CourseLane=-1;P->FeederVisits=0;P->Collected.Empty();P->ThermalReadyAt=P->ThermalUntil=P->ReloadUntil=P->RespawnAt=0;P->TreeIndex=INDEX_NONE;P->Stamina=100;P->ResetBotAwareness();FVector Pos=SpawnLocation(P->bSquirrel,P->bBot?P->BotSlot+1:0);P->SetActorLocation(Pos,false,nullptr,ETeleportType::TeleportPhysics);P->OnRepDead();P->ForceNetUpdate();}if(bWaitingRoomsEnabled)ResetWaitingRooms();else RefreshBots();S->ForceNetUpdate();}

AWildlineController::AWildlineController(){bShowMouseCursor=true;}
void AWildlineController::BeginPlay(){
    Super::BeginPlay();if(!IsLocalController())return;
    if(auto* S=UWildlineUserSettings::Get()){S->LoadSettings(false);S->ApplyNonResolutionSettings();S->Persist();}
    PlayerCameraManager->ViewPitchMin=-80;PlayerCameraManager->ViewPitchMax=80;
    Menu=CreateWidget<UWildlineMenu>(this,UWildlineMenu::StaticClass());if(Menu)Menu->AddToViewport();
    auto* GI=GetGameInstance<UWildlineGameInstance>();if(GI&&GI->bStartAfterTravel){GI->bStartAfterTravel=false;EnterWaitingRoom(GI->bPreferredSquirrel);}else SetMenu(true);
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineLobbySmoke"))){FTimerHandle H;GetWorldTimerManager().SetTimer(H,this,&AWildlineController::RunLobbySmokeTest,2.f,false);}
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineLobbyReview")))RunLobbyReview();
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineLobbyNetProbe")))RunLobbyNetworkProbe();
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineAutoStart"))){ChooseRole(FParse::Param(FCommandLine::Get(),TEXT("WildlineSquirrel")));}
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineHillNetProbe")))RunHillNetworkProbe();
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineHillSmoke"))){FApp::SetUseFixedTimeStep(true);FApp::SetFixedDeltaTime(1./60.);FTimerHandle H;GetWorldTimerManager().SetTimer(H,this,&AWildlineController::RunHillSmokeTest,2.f,false);}
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineFarmSmoke"))){FTimerHandle H;GetWorldTimerManager().SetTimer(H,this,&AWildlineController::RunFarmSmokeTest,2.f,false);}
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineConfigSmoke"))){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[this](){RunConfigSmokeTest();}),2.f,false);}
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineConfigNetProbe")))RunConfigNetworkProbe();
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineConfigReview")))RunConfigReview();
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineAIReview")))RunAIReview();
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineAIBalanceReview")))RunAIBalanceReview();
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineFarmBotProbe"))){
        ChooseRole(false);
        FTimerHandle Probe;GetWorldTimerManager().SetTimer(Probe,FTimerDelegate::CreateWeakLambda(this,[this](){
            auto* S=GetWorld()->GetGameState<AWildlineGameState>();bool Ok=S&&S->StoredAcorns>=4;
            for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(It->bBot){Ok&=It->Collected.Num()>0;UE_LOG(LogTemp,Display,TEXT("FARM BOT pos=%s bag=%d collected=%d dead=%d"),*It->GetActorLocation().ToString(),It->Bag,It->Collected.Num(),It->bDead);}
            UE_LOG(LogTemp,Display,TEXT("WILDLINE FARM BOTS: %s stored=%d"),Ok?TEXT("PASS"):TEXT("FAIL"),S?S->StoredAcorns:-1);FPlatformMisc::RequestExitWithStatus(false,Ok?0:1);
        }),85.f,false);
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineHillReview"))){
        auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
        auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,true,false);};
        At(2,[this](){GetGameInstance<UWildlineGameInstance>()->PreferredMode=2;Menu->RefreshDifficulty();});At(5,[Shot](){Shot(TEXT("HillMenu.png"));});
        At(6,[this](){ChooseRole(true);auto* P=Cast<AWildlineCharacter>(GetPawn());auto* F=AWildlineWorld::Find(GetWorld());P->SetActorLocation(F->CourseTarget(0,0)+FVector(0,0,13));SetControlRotation(FRotator(-2,0,0));GetWorld()->GetGameState<AWildlineGameState>()->bRoundStarted=false;});
        At(10,[Shot](){Shot(TEXT("HillSquirrel.png"));});
        At(11,[this](){auto* Camera=GetWorld()->SpawnActor<ACameraActor>(FVector(-1800,-1400,1100),FRotator::ZeroRotator);Camera->SetActorRotation((FVector(-150,0,400)-Camera->GetActorLocation()).Rotation());Camera->GetCameraComponent()->SetFieldOfView(60);SetViewTarget(Camera);GetHUD()->bShowHUD=false;});
        At(15,[Shot](){Shot(TEXT("HillCourse.png"));});
        At(16,[this](){auto* Camera=GetWorld()->SpawnActor<ACameraActor>(FVector(-4200,-5800,4000),FRotator::ZeroRotator);Camera->SetActorRotation((FVector(0,-200,450)-Camera->GetActorLocation()).Rotation());Camera->GetCameraComponent()->SetFieldOfView(65);SetViewTarget(Camera);});
        At(21,[Shot](){Shot(TEXT("HillNeighborhood.png"));});
        At(22,[this](){const auto* F=AWildlineWorld::Find(GetWorld());const FVector Eye=F->FeederLocation+FVector(220,-280,160);auto* Camera=GetWorld()->SpawnActor<ACameraActor>(Eye,(F->FeederLocation+FVector(0,0,15)-Eye).Rotation());Camera->GetCameraComponent()->SetFieldOfView(50);SetViewTarget(Camera);});
        At(26,[Shot](){Shot(TEXT("HillFeeder.png"));});At(27,[this](){SetMenu(true);Menu->ScrollToSettings();});At(31,[Shot](){Shot(TEXT("HillSettings.png"));});
        At(32,[this](){SetMenu(false);const auto* F=AWildlineWorld::Find(GetWorld());for(auto* Prop:F->ResidentialProps)if(Prop->GetStaticMesh()->GetName()==TEXT("SM_KitPool")){const FVector Target=Prop->Bounds.Origin;const FVector Eye=Target+FVector(440,-660,220);auto* Camera=GetWorld()->SpawnActor<ACameraActor>(Eye,(Target-Eye).Rotation());Camera->GetCameraComponent()->SetFieldOfView(65);SetViewTarget(Camera);break;}});
        At(36,[Shot](){Shot(TEXT("HillPool.png"));});
        At(37,[this](){const auto* F=AWildlineWorld::Find(GetWorld());for(auto* Prop:F->ResidentialProps)if(Prop->GetStaticMesh()->GetName()==TEXT("SM_garden_hose_wall_mounted_01")&&Prop->GetComponentLocation().Y>0&&Prop->GetComponentLocation().X>3000){const FVector Target=Prop->Bounds.Origin;const FVector Eye=Target+FVector(-80,-180,30);auto* Camera=GetWorld()->SpawnActor<ACameraActor>(Eye,(Target-Eye).Rotation());Camera->GetCameraComponent()->SetFieldOfView(60);SetViewTarget(Camera);break;}});
        At(41,[Shot](){Shot(TEXT("HillHose.png"));});
        At(42,[this](){const FVector Eye(0,-200,380);auto* Camera=GetWorld()->SpawnActor<ACameraActor>(Eye,(AWildlineLobby::Centre(false)-Eye).Rotation());Camera->GetCameraComponent()->SetFieldOfView(65);SetViewTarget(Camera);});
        At(46,[Shot](){Shot(TEXT("HillSky.png"));});At(48,[](){FPlatformMisc::RequestExit(false);});
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineFarmReview"))){
        auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
        auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,true,false);};
        At(2,[this](){GetGameInstance<UWildlineGameInstance>()->PreferredMode=1;Menu->RefreshDifficulty();});At(5,[Shot](){Shot(TEXT("FarmMenu.png"));});
        At(6,[this](){ChooseRole(false);auto* P=Cast<AWildlineCharacter>(GetPawn());P->SelectWeapon(1);P->SetActorLocation(FVector(1050,-1600,320));SetControlRotation(FRotator(-4,52,0));GetWorld()->GetGameState<AWildlineGameState>()->bRoundStarted=false;});
        At(10,[Shot](){Shot(TEXT("FarmShotgun.png"));});
        At(11,[this](){auto* Camera=GetWorld()->SpawnActor<ACameraActor>(FVector(500,-1700,820),FRotator::ZeroRotator);Camera->SetActorRotation((FVector(2200,250,500)-Camera->GetActorLocation()).Rotation());Camera->GetCameraComponent()->SetFieldOfView(60);SetViewTarget(Camera);GetHUD()->bShowHUD=false;});
        At(15,[Shot](){Shot(TEXT("Farmhouse.png"));});
        At(16,[this](){auto* Camera=GetWorld()->SpawnActor<ACameraActor>(FVector(-4300,-6200,3450),FRotator::ZeroRotator);Camera->SetActorRotation((FVector(950,550,450)-Camera->GetActorLocation()).Rotation());Camera->GetCameraComponent()->SetFieldOfView(65);SetViewTarget(Camera);});
        At(20,[Shot](){Shot(TEXT("FarmOverview.png"));});
        At(21,[this](){ChooseRole(true);GetHUD()->bShowHUD=true;auto* P=Cast<AWildlineCharacter>(GetPawn());P->SetActorLocation(FVector(2420,-80,284));SetControlRotation(FRotator(0,-15,0));bMapZoomed=true;auto* S=GetWorld()->GetGameState<AWildlineGameState>();S->HunterDifficulty=0;S->bRoundStarted=false;});
        At(25,[Shot](){Shot(TEXT("FarmPantry.png"));});
        At(26,[this](){auto* P=Cast<AWildlineCharacter>(GetPawn());P->SetActorLocation(FVector(-1400,250,223));SetControlRotation(FRotator(7,45,0));P->ToggleCamera();});
        At(30,[Shot](){Shot(TEXT("FarmLivestock.png"));});At(32,[](){FPlatformMisc::RequestExit(false);});
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineSmoke"))){FTimerHandle H;GetWorldTimerManager().SetTimer(H,this,&AWildlineController::RunSmokeTest,1.5f,false);}
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineTraversalSmoke"))){FTimerHandle H;GetWorldTimerManager().SetTimer(H,this,&AWildlineController::RunTraversalSmokeTest,2.f,false);}
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineGameplayBugsReview"))){FTimerHandle H;GetWorldTimerManager().SetTimer(H,this,&AWildlineController::RunGameplayBugsReview,2.f,false);}
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineThermalReview"))){FTimerHandle H;GetWorldTimerManager().SetTimer(H,this,&AWildlineController::RunThermalReview,2.f,false);}
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineWorldUpgradeReview"))){FTimerHandle H;GetWorldTimerManager().SetTimer(H,this,&AWildlineController::RunWorldUpgradeReview,2.f,false);}
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineTraversalNetProbe"))){FTimerHandle H;GetWorldTimerManager().SetTimer(H,this,&AWildlineController::RunTraversalNetProbe,2.f,false);}
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineSettingsReview"))){
        auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
        auto Passed=MakeShared<bool>(true);const FIntPoint Original=UWildlineUserSettings::Get()->GetScreenResolution();
        auto Check=[Passed](bool Ok,const TCHAR* Message){*Passed&=Ok;UE_LOG(LogTemp,Display,TEXT("WILDLINE SETTINGS %s: %s"),Ok?TEXT("PASS"):TEXT("FAIL"),Message);};
        auto Quality=[Check](){
            auto* S=UWildlineUserSettings::Get();
            for(const TCHAR* Name:{TEXT("sg.ViewDistanceQuality"),TEXT("sg.AntiAliasingQuality"),TEXT("sg.PostProcessQuality"),TEXT("sg.ShadowQuality"),TEXT("sg.GlobalIlluminationQuality"),TEXT("sg.ReflectionQuality"),TEXT("sg.TextureQuality"),TEXT("sg.EffectsQuality"),TEXT("sg.FoliageQuality"),TEXT("sg.ShadingQuality"),TEXT("sg.LandscapeQuality")}){
                const auto* CVar=IConsoleManager::Get().FindConsoleVariable(Name);Check(CVar&&CVar->GetInt()==3,Name);
            }
            const auto* Scale=IConsoleManager::Get().FindConsoleVariable(TEXT("sg.ResolutionQuality"));
            Check(Scale&&FMath::IsNearlyEqual(Scale->GetFloat(),100.f)&&!S->IsDynamicResolutionEnabled(),TEXT("native render resolution and dynamic resolution disabled"));
        };
        At(2,[this,Quality,Check,Original](){
            Quality();Check(Menu&&Menu->Resolution&&Menu->Resolution->GetSelectedOption()==FString::Printf(TEXT("%d x %d"),Original.X,Original.Y),TEXT("saved display resolution selected in menu"));
            auto* S=UWildlineUserSettings::Get();S->SetOverallScalabilityLevel(0);S->SetResolutionScaleValueEx(50);S->SetDynamicResolutionEnabled(true);S->ApplyNonResolutionSettings();Quality();
            Menu->ScrollToSettings();const int32 Index=Menu->ResolutionOptions.IndexOfByKey(FIntPoint(1280,720));Check(Index!=INDEX_NONE,TEXT("720p resolution offered"));
            if(Index!=INDEX_NONE){Menu->Resolution->SetSelectedIndex(Index);Menu->ResolutionChanged(Menu->Resolution->GetSelectedOption(),ESelectInfo::OnMouseClick);}
        });
        At(5,[Quality,Check](){
            auto* S=UWildlineUserSettings::Get();Check(S->GetScreenResolution()==FIntPoint(1280,720)&&GEngine->GameViewport->Viewport->GetSizeXY()==FIntPoint(1280,720),TEXT("resolution selector resizes live viewport to 720p"));
            S->LoadSettings(true);Check(S->GetScreenResolution()==FIntPoint(1280,720)&&S->GetLastConfirmedScreenResolution()==FIntPoint(1280,720),TEXT("720p selection and confirmation persist after reload"));Quality();
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/HighQualitySettings720.png"),true,false);
        });
        At(6,[this,Original](){const int32 Index=Menu->ResolutionOptions.IndexOfByKey(Original);Menu->Resolution->SetSelectedIndex(Index);Menu->ResolutionChanged(Menu->Resolution->GetSelectedOption(),ESelectInfo::OnMouseClick);});
        At(9,[Quality,Check,Original](){
            auto* S=UWildlineUserSettings::Get();S->LoadSettings(true);Check(S->GetScreenResolution()==Original&&GEngine->GameViewport->Viewport->GetSizeXY()==Original,TEXT("original resolution restored and retained"));Quality();
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/HighQualitySettings.png"),true,false);
        });
        At(11,[Passed](){UE_LOG(LogTemp,Display,TEXT("WILDLINE SETTINGS RESULT: %s"),*Passed?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Passed?0:1);});
    }
#if WITH_EDITOR
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineUVProbe"))){FTimerHandle Probe;GetWorldTimerManager().SetTimer(Probe,FTimerDelegate::CreateWeakLambda(this,[](){
        auto* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Wildline/Hunters/Meshes/SK_HunterFemale.SK_HunterFemale"));const auto& L=Mesh->GetResourceForRendering()->LODRenderData[0];const auto& B=L.StaticVertexBuffers;
        const int32 Skin=Mesh->GetMaterials().IndexOfByPredicate([](const FSkeletalMaterial& M){return M.MaterialSlotName==TEXT("H_Skin");});float Crown=-9999;TArray<int32> Vertices;
        for(const auto& S:L.RenderSections)if(S.MaterialIndex==Skin)for(uint32 I=S.BaseVertexIndex;I<S.BaseVertexIndex+S.NumVertices;I++){Vertices.Add(I);Crown=FMath::Max(Crown,B.PositionVertexBuffer.VertexPosition(I).Z);}
        UE_LOG(LogTemp,Display,TEXT("UV_RUNTIME crown=%f channels=%u skin=%d vertices=%d"),Crown,B.StaticMeshVertexBuffer.GetNumTexCoords(),Skin,Vertices.Num());
        for(float Offset:{4.f,8.f,12.f,16.f,20.f})for(int32 Side:{-1,1}){int32 Best=-1;float Front=-99999;for(int32 I:Vertices){const auto V=B.PositionVertexBuffer.VertexPosition(I);if(FMath::Abs(V.X)<2&&FMath::Abs(V.Z-(Crown-Offset))<.9f&&Side*V.Y>Front){Front=Side*V.Y;Best=I;}}if(Best>=0){const auto V=B.PositionVertexBuffer.VertexPosition(Best);const auto U0=B.StaticMeshVertexBuffer.GetVertexUV(Best,0),U1=B.StaticMeshVertexBuffer.GetVertexUV(Best,1);UE_LOG(LogTemp,Display,TEXT("UV_RUNTIME offset=%.1f side=%d pos=%s uv0=%s uv1=%s"),Offset,Side,*V.ToString(),*U0.ToString(),*U1.ToString());}}
        FPlatformMisc::RequestExit(false);
    }),2.f,false);}
#endif
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineWeaponFXReview"))){
        for(const TCHAR* Name:{TEXT("M_Smoke"),TEXT("M_Flash"),TEXT("M_Impact"),TEXT("M_Chip"),TEXT("M_Spark")})LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Wildline/FX/%s.%s"),Name,Name));
        auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
        auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,true,false);};
        At(2,[this](){ChooseRole(false);auto* P=Cast<AWildlineCharacter>(GetPawn());P->SelectWeapon(1);P->SetActorLocation(FVector(1250,250,320));P->GetCharacterMovement()->DisableMovement();SetControlRotation(FRotator(0,0,0));GetWorld()->GetGameState<AWildlineGameState>()->bRoundStarted=false;for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)It->bBot=false;});
        At(8,[Shot](){Shot(TEXT("ShotgunReady.png"));});
        At(9,[this,Shot](){Cast<AWildlineCharacter>(GetPawn())->ServerFire(FVector::ForwardVector);Shot(TEXT("ShotgunFlash.png"));});
        At(9.24f,[Shot](){Shot(TEXT("ShotgunSmoke.png"));});
        At(9.40f,[Shot](){Shot(TEXT("ShotgunPump.png"));});
        At(11,[this](){auto* C=GetWorld()->SpawnActor<ACameraActor>(FVector(1390,250,392),FRotator(0,0,0));C->GetCameraComponent()->SetFieldOfView(42);SetViewTarget(C);GetHUD()->bShowHUD=false;});
        At(14,[Shot](){Shot(TEXT("ShotgunImpacts.png"));});
        At(15,[this](){auto* P=Cast<AWildlineCharacter>(GetPawn());SetViewTarget(P);GetHUD()->bShowHUD=true;P->ServerReload();});
        At(16,[Shot](){Shot(TEXT("ShotgunReload.png"));});At(18,[](){FPlatformMisc::RequestExit(false);});
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineNetProbe"))){
        FTimerHandle Activate,Verify;
        if(HasAuthority()){
            FTimerHandle FXShot;GetWorldTimerManager().SetTimer(FXShot,FTimerDelegate::CreateWeakLambda(this,[this](){if(auto* F=AWildlineWorld::Find(GetWorld());F&&F->bFarm){auto* P=Cast<AWildlineCharacter>(GetPawn());P->SetActorLocation(FVector(1250,250,320));P->Ammo=2;P->ServerFire(FVector::ForwardVector);P->ForceNetUpdate();}}),32.f,false);
        }
        GetWorldTimerManager().SetTimer(Activate,FTimerDelegate::CreateWeakLambda(this,[this](){if(HasAuthority())if(auto* P=Cast<AWildlineCharacter>(GetPawn())){if(auto* State=GetWorld()->GetGameState<AWildlineGameState>()){State->HunterDifficulty=2;State->ForceNetUpdate();}if(auto* F=AWildlineWorld::Find(GetWorld());F&&F->bFarm){P->SelectWeapon(1);P->Ammo=1;GetWorld()->GetGameState<AWildlineGameState>()->TakenPeanuts.AddUnique(29);}P->ServerThermal();P->ServerAim(true);auto Look=P->HunterProfile;Look.Body=1;Look.Outfit=2;Look.FaceWidth=.4f;Look.DimensionsVersion=1;Look.HeightCm=183;Look.BodyWidth=1.3f;Look.BodyDepth=.9f;Look.HandSize=1.2f;Look.FootSize=.9f;Look.FaceSize=1.1f;Look.Slimness=-.15f;Look.bUsePhoto=true;FImage Img(512,512,ERawImageFormat::BGRA8,EGammaSpace::sRGB);for(int32 Y=0;Y<512;Y++)for(int32 X=0;X<512;X++){const int32 I=(Y*512+X)*4,N=(X*37+Y*19)%17-8;Img.RawData[I]=120+N;Img.RawData[I+1]=60+Y/4+N;Img.RawData[I+2]=70+X/4+N;Img.RawData[I+3]=255;}TArray64<uint8> Data;FImageUtils::CompressImage(Data,TEXT("jpg"),Img,75);UE_LOG(LogTemp,Display,TEXT("WILDLINE NETWORK portrait fixture: %lld bytes"),Data.Num());P->ServerSetHunterProfile(Look,FBase64::Encode(Data.GetData(),Data.Num()));}}),12.f,false);
        GetWorldTimerManager().SetTimer(Verify,FTimerDelegate::CreateWeakLambda(this,[this](){
            int32 Humans=0;bool Hunter=false,Animal=false,ReplicatedThermal=false,ReplicatedLook=false,ReplicatedAim=false;
            for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(!It->bBot&&!It->bPreviewHunter){ReplicatedAim|=!It->bSquirrel&&It->bAiming;Humans++;Hunter|=!It->bSquirrel;Animal|=It->bSquirrel;ReplicatedThermal|=!It->bSquirrel&&(It->SelectedWeapon==1?!It->IsThermalActive()&&It->ThermalUntil==0:It->ThermalReadyAt>It->ServerTime());ReplicatedLook|=!It->bSquirrel&&It->HunterProfile.Body==1&&It->HunterProfile.Outfit==2&&It->HunterFaceTexture&&It->HunterFaceData.Len()>2000&&FMath::IsNearlyEqual(It->HunterProfile.FaceWidth,.4f)&&It->HunterProfile.HeightCm==183&&FMath::IsNearlyEqual(It->HunterProfile.BodyWidth,1.3f)&&FMath::IsNearlyEqual(It->HunterProfile.BodyDepth,.9f)&&FMath::IsNearlyEqual(It->HunterProfile.HandSize,1.2f)&&FMath::IsNearlyEqual(It->HunterProfile.FootSize,.9f)&&FMath::IsNearlyEqual(It->HunterProfile.FaceSize,1.1f)&&FMath::IsNearlyEqual(It->GetActorScale3D().Z,183.f/174.5f);}
            const auto* S=GetWorld()->GetGameState<AWildlineGameState>();const auto* P=Cast<AWildlineCharacter>(GetPawn());
            const bool IsHost=!FParse::Param(FCommandLine::Get(),TEXT("WildlineSquirrel"));
            bool FarmReplication=true;
            if(auto* F=AWildlineWorld::Find(GetWorld());F&&F->bFarm){
                int32 Livestock=0;bool Weapon=false,Peanuts=false,Effects=false;for(TActorIterator<AWildlineLivestock> It(GetWorld());It;++It)Livestock++;
                for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It){
                    if(HasAuthority()&&It->bBot)It->SetActorTickEnabled(false);
                    if(!It->bBot&&!It->bPreviewHunter){Weapon|=!It->bSquirrel&&It->SelectedWeapon==1&&It->Ammo==1;
                        Effects|=!It->bSquirrel&&It->CosmeticShots>0&&It->CosmeticImpacts==12;
                        if(HasAuthority()&&It->bSquirrel&&It->Bag==0){It->SetActorLocation(F->AcornLocations[0]+FVector(0,0,13));}
                        Peanuts|=It->bSquirrel&&It->Bag>0&&It->BagPeanuts.Num()==It->Bag;
                    }
                }
                FarmReplication=S&&S->ExpeditionMode==1&&S->TakenPeanuts.Contains(29)&&Weapon&&Peanuts&&Livestock==6&&Effects;
                if(FarmReplication&&!bNetProbePassed)UE_LOG(LogTemp,Display,TEXT("WILDLINE FARM NETWORK: PASS shared pantry, carried peanuts, shotgun ammo, six livestock and mode"));
            }
            const bool Ok=FarmReplication&&Humans==2&&Hunter&&Animal&&ReplicatedThermal&&ReplicatedLook&&ReplicatedAim&&S&&S->bRoundStarted&&S->HunterDifficulty==2&&P&&P->bSquirrel==!IsHost&&GetNetMode()==(IsHost?NM_ListenServer:NM_Client);
            const FString Side=IsHost?TEXT("Host"):TEXT("Client");
            if(IsHost){
                if(Ok&&!bNetProbePassed){bNetProbePassed=true;FFileHelper::SaveStringToFile(TEXT("PASS\n"),*(FPaths::ProjectSavedDir()/TEXT("NetworkHostResult.txt")));UE_LOG(LogTemp,Display,TEXT("WILDLINE NETWORK Host: PASS humans=%d hunter=%d squirrel=%d thermal=%d"),Humans,Hunter,Animal,ReplicatedThermal);}
                if(GetWorld()->GetTimeSeconds()<75)return;
                if(!bNetProbePassed)FFileHelper::SaveStringToFile(TEXT("FAIL\n"),*(FPaths::ProjectSavedDir()/TEXT("NetworkHostResult.txt")));
                FPlatformMisc::RequestExitWithStatus(false,bNetProbePassed?0:1);return;
            }
            UE_LOG(LogTemp,Display,TEXT("WILDLINE NETWORK %s: %s humans=%d hunter=%d squirrel=%d thermal=%d appearance=%d aim=%d difficulty=%d"),*Side,Ok?TEXT("PASS"):TEXT("FAIL"),Humans,Hunter,Animal,ReplicatedThermal,ReplicatedLook,ReplicatedAim,S?S->HunterDifficulty:-1);
            FFileHelper::SaveStringToFile(Ok?TEXT("PASS\n"):TEXT("FAIL\n"),*(FPaths::ProjectSavedDir()/(TEXT("Network")+Side+TEXT("Result.txt"))));
            FPlatformMisc::RequestExitWithStatus(false,Ok?0:1);
        }),HasAuthority()?1.f:18.f,HasAuthority());
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlinePolishReview"))){
        auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
        auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,false,false);};
        At(3,[this](){ChooseRole(false);GetWorld()->GetGameState<AWildlineGameState>()->bRoundStarted=false;if(GetHUD())GetHUD()->bShowHUD=false;
            auto* P=Cast<AWildlineCharacter>(GetPawn());P->bThirdPerson=true;P->HunterProfile=FWildlineHunterProfile::Travis();P->bProfileSubmitted=true;P->ApplyHunterProfile();
            auto* Cam=GetWorld()->SpawnActor<ACameraActor>();Cam->AttachToComponent(P->GetRootComponent(),FAttachmentTransformRules::KeepRelativeTransform);
            Cam->SetActorRelativeLocation(FVector(185,-175,55));Cam->SetActorRelativeRotation((FVector(22,0,25)-FVector(185,-175,55)).Rotation());Cam->GetCameraComponent()->FieldOfView=50;SetViewTarget(Cam);
        });
        At(10,[Shot](){Shot(TEXT("PolishHunterGrip.png"));});
        At(12,[this](){ChooseRole(true);GetWorld()->GetGameState<AWildlineGameState>()->bRoundStarted=false;auto* P=Cast<AWildlineCharacter>(GetPawn());auto* F=AWildlineWorld::Find(GetWorld());
            P->bThirdPerson=true;const auto& T=F->Trees[0];float Radius;const FVector Centre=F->TrunkCentre(0,T.Base.Z+600,Radius);P->SetActorLocation(Centre+FVector(Radius+20,0,0));P->ServerClimb();P->SetActorRotation(FRotator(0,180,0));SetControlRotation(FRotator(0,180,0));SetViewTarget(P);
        });
        At(19,[Shot](){Shot(TEXT("PolishSquirrelClimb.png"));});
        At(21,[this](){SetControlRotation(FRotator(0,0,0));Cast<AWildlineCharacter>(GetPawn())->ServerClimbInput(0,1);});
        At(23,[this](){Cast<AWildlineCharacter>(GetPawn())->ServerClimbInput(0,0);});
        At(26,[Shot](){Shot(TEXT("PolishSquirrelOrbit.png"));});
        At(28,[this](){ChooseRole(false);GetWorld()->GetGameState<AWildlineGameState>()->bRoundStarted=false;auto* P=Cast<AWildlineCharacter>(GetPawn());const float X=WildlineRules::MapRadius-500;
            P->SetActorLocation(FVector(X,0,WildlineRules::Ground(X,0)+100));SetControlRotation(FRotator(-2,0,0));SetViewTarget(P);
        });
        At(35,[Shot](){Shot(TEXT("PolishReserveBoundary.png"));});
        At(37,[](){FPlatformMisc::RequestExit(false);});
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineSquirrelReview"))){
        FTimerHandle Enter,Idle,Walk,Run,Jump,Exit;
        GetWorldTimerManager().SetTimer(Enter,FTimerDelegate::CreateWeakLambda(this,[this](){
            ChooseRole(true);if(auto* S=GetWorld()->GetGameState<AWildlineGameState>())S->bRoundStarted=false;
            auto* P=Cast<AWildlineCharacter>(GetPawn());if(!P)return;P->bThirdPerson=true;P->GetMesh()->SetOwnerNoSee(false);
            FVector Pos(0,750,WildlineRules::Ground(0,750)+13);P->SetActorLocation(Pos);P->SetActorRotation(FRotator::ZeroRotator);SetControlRotation(FRotator::ZeroRotator);
            if(auto* Forest=AWildlineWorld::Find(GetWorld()))Forest->Grass->SetVisibility(false);
            auto* Cam=GetWorld()->SpawnActor<ACameraActor>();Cam->AttachToComponent(P->GetRootComponent(),FAttachmentTransformRules::KeepRelativeTransform);
            const FVector Offset(54,-57,21);Cam->SetActorRelativeLocation(Offset);Cam->SetActorRelativeRotation((FVector(0,0,8)-Offset).Rotation());Cam->GetCameraComponent()->FieldOfView=46;SetViewTarget(Cam);
            if(GetHUD())GetHUD()->bShowHUD=false;
        }),3.f,false);
        auto Shot=[this](const TCHAR* Name){if(auto* P=Cast<AWildlineCharacter>(GetPawn()))UE_LOG(LogTemp,Display,TEXT("SQUIRREL REVIEW %s pawn=%s camera=%s mesh=%s pelvis=%s"),Name,*P->GetActorLocation().ToString(),*GetViewTarget()->GetActorLocation().ToString(),*P->GetMesh()->GetRelativeLocation().ToString(),*P->GetMesh()->GetBoneTransform(TEXT("SKEL_Pelvis"),RTS_Component).ToString());FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,false,false);};
        GetWorldTimerManager().SetTimer(Idle,FTimerDelegate::CreateWeakLambda(this,[Shot](){Shot(TEXT("SquirrelIdle.png"));}),6.f,false);
        GetWorldTimerManager().SetTimer(Walk,FTimerDelegate::CreateWeakLambda(this,[Shot](){Shot(TEXT("SquirrelWalk.png"));}),8.f,false);
        GetWorldTimerManager().SetTimer(Run,FTimerDelegate::CreateWeakLambda(this,[Shot](){Shot(TEXT("SquirrelRun.png"));}),10.f,false);
        GetWorldTimerManager().SetTimer(Jump,FTimerDelegate::CreateWeakLambda(this,[this,Shot](){if(auto* P=Cast<AWildlineCharacter>(GetPawn()))P->ServerJump();}),11.f,false);
        GetWorldTimerManager().SetTimer(Exit,FTimerDelegate::CreateWeakLambda(this,[](){FPlatformMisc::RequestExit(false);}),13.f,false);
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineForageReview"))){
        auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
        auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,true,false);};
        At(4,[Shot](){Shot(TEXT("ForageCamp.png"));});
        At(6,[this](){ChooseRole(true);auto* S=GetWorld()->GetGameState<AWildlineGameState>();S->HunterDifficulty=0;S->bRoundStarted=false;
            auto* F=AWildlineWorld::Find(GetWorld());auto* P=Cast<AWildlineCharacter>(GetPawn());FVector Position=F->AcornLocations[0]-FVector(650,0,0);Position.Z=WildlineRules::Ground(Position.X,Position.Y)+13;P->SetActorLocation(Position);SetControlRotation((F->AcornLocations[0]+FVector(0,0,18)-Position).Rotation());
            for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(It->bBot&&!It->bSquirrel){FVector Far(-6000,5000,0);Far.Z=WildlineRules::Ground(Far.X,Far.Y)+110;It->SetActorLocation(Far);}
        });
        At(9,[Shot](){Shot(TEXT("ForageEasyMap.png"));});
        At(10,[this](){bMapZoomed=true;});At(13,[Shot](){Shot(TEXT("ForageZoomMap.png"));});
        At(14,[this](){bMapZoomed=false;auto* F=AWildlineWorld::Find(GetWorld());auto* P=Cast<AWildlineCharacter>(GetPawn());GetWorld()->GetGameState<AWildlineGameState>()->bRoundStarted=true;P->SetActorLocation(F->AcornLocations[0]+FVector(0,0,13));});
        At(15,[Shot](){Shot(TEXT("ForagePickup.png"));});
        At(16,[this](){GetWorld()->GetGameState<AWildlineGameState>()->bRoundStarted=false;});
        At(18,[](){FPlatformMisc::RequestExit(false);});
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineCapture"))){
        FTimerHandle MenuShot,Enter,GameShot,Exit;
        GetWorldTimerManager().SetTimer(MenuShot,FTimerDelegate::CreateWeakLambda(this,[this](){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/Menu.png"),true,false);}),4.f,false);
        GetWorldTimerManager().SetTimer(Enter,FTimerDelegate::CreateWeakLambda(this,[this](){ChooseRole(FParse::Param(FCommandLine::Get(),TEXT("WildlineSquirrel")));if(auto* P=Cast<AWildlineCharacter>(GetPawn());P&&FParse::Param(FCommandLine::Get(),TEXT("WildlineThird")))P->ToggleCamera();SetControlRotation(FRotator(-7,-20,0));}),7.f,false);
        GetWorldTimerManager().SetTimer(GameShot,FTimerDelegate::CreateWeakLambda(this,[this](){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/Gameplay.png"),true,false);}),11.f,false);
        GetWorldTimerManager().SetTimer(Exit,FTimerDelegate::CreateWeakLambda(this,[](){FPlatformMisc::RequestExit(false);}),13.f,false);
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineHunterBodies")))RunHunterBodyReview();
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineOpenCreator"))){FTimerHandle Open;GetWorldTimerManager().SetTimer(Open,FTimerDelegate::CreateWeakLambda(this,[this](){Menu->CreateHunter();if(FParse::Param(FCommandLine::Get(),TEXT("WildlineOpenTravis")))Menu->Creator->LoadTravisPreset();}),2.f,false);}
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineHunterMotionReview"))){
        auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
        auto Shot=[this](const TCHAR* Name){
            auto* P=Cast<AWildlineCharacter>(GetPawn());auto* M=P->GetMesh();const auto T=P->WorldRifle->GetComponentTransform();
            const FVector Pocket=M->GetSocketLocation(TEXT("upperarm_r"))+P->GetActorForwardVector()*5-P->GetActorRightVector()*3-FVector::UpVector*6;
            UE_LOG(LogTemp,Display,TEXT("HUNTER MOTION %s shoulder=%.2f right=%.2f left=%.2f pitch=%.2f"),Name,FVector::Dist(Pocket,T.TransformPosition(FVector(P->SelectedWeapon==1?P->WorldRifle->GetStaticMesh()->GetBoundingBox().Min.X+1:-45.6f,0,P->SelectedWeapon==1?-4.8f:-4.f))),FVector::Dist(M->GetSocketLocation(TEXT("hand_r")),T.TransformPosition(FVector(-17,0,P->SelectedWeapon==1?-4.8f:-2.8f))),FVector::Dist(M->GetSocketLocation(TEXT("hand_l")),(P->SelectedWeapon==1?P->WorldBolt->GetComponentTransform():T).TransformPosition(FVector(P->SelectedWeapon==1?18.5f:17,-1,-3))),T.Rotator().Pitch);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,false,false);
        };
        At(3,[this](){ChooseRole(false);GetWorld()->GetGameState<AWildlineGameState>()->bRoundStarted=false;if(GetHUD())GetHUD()->bShowHUD=false;
            auto* P=Cast<AWildlineCharacter>(GetPawn());P->bThirdPerson=true;P->HunterProfile=FWildlineHunterProfile::Travis();P->bProfileSubmitted=true;P->ApplyHunterProfile();SetControlRotation(FRotator::ZeroRotator);
            auto* Cam=GetWorld()->SpawnActor<ACameraActor>();Cam->AttachToComponent(P->GetRootComponent(),FAttachmentTransformRules::KeepRelativeTransform);Cam->SetActorRelativeLocation(FVector(185,-175,55));Cam->SetActorRelativeRotation((FVector(22,0,25)-FVector(185,-175,55)).Rotation());Cam->GetCameraComponent()->FieldOfView=50;SetViewTarget(Cam);
        });
        At(7,[Shot](){Shot(TEXT("HunterMotionReady.png"));});
        At(8,[this](){Cast<AWildlineCharacter>(GetPawn())->StartAim();SetControlRotation(FRotator(25,0,0));});
        At(12,[Shot](){Shot(TEXT("HunterMotionAimUp.png"));});
        At(13,[this](){SetControlRotation(FRotator(-25,0,0));});
        At(17,[Shot](){Shot(TEXT("HunterMotionAimDown.png"));});
        At(18,[this](){auto* P=Cast<AWildlineCharacter>(GetPawn());P->StopAim();P->SelectWeapon(1);SetControlRotation(FRotator::ZeroRotator);});
        At(22,[Shot](){Shot(TEXT("HunterMotionShotgun.png"));});
        At(23,[this](){auto* P=Cast<AWildlineCharacter>(GetPawn());P->SelectWeapon(0);P->HunterProfile.Body=1;P->ApplyHunterProfile();});
        At(27,[Shot](){Shot(TEXT("HunterMotionFemale.png"));});
        At(29,[](){FPlatformMisc::RequestExit(false);});
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineHunterLocomotionReview"))||FParse::Param(FCommandLine::Get(),TEXT("WildlineHunterAnimationProbe"))){
        const bool Render=FParse::Param(FCommandLine::Get(),TEXT("WildlineHunterLocomotionReview"));
        auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
        auto Passed=MakeShared<bool>(true);auto MaxGrip=MakeShared<float>(0);auto MaxShoulder=MakeShared<float>(0);auto MaxFootDrift=MakeShared<float>(0);
        auto PreviousFeet=MakeShared<TArray<FVector>>();PreviousFeet->SetNum(2);auto PreviousPlant=MakeShared<TArray<float>>();PreviousPlant->Init(0.f,2);
        auto PreviousSerial=MakeShared<TArray<int32>>();PreviousSerial->Init(-1,2);
        auto MovingPlants=MakeShared<int32>(0);
        auto PreviousScale=MakeShared<float>(0);auto PlantWarmupUntil=MakeShared<float>(0);
        auto LastSampleFrame=MakeShared<uint64>(MAX_uint64);
        auto Check=[Passed](bool Ok,const TCHAR* Message){*Passed&=Ok;UE_LOG(LogTemp,Display,TEXT("HUNTER ANIMATION %s: %s"),Ok?TEXT("PASS"):TEXT("FAIL"),Message);};
        At(3,[this,Check](){
            ChooseRole(false);GetWorld()->GetGameState<AWildlineGameState>()->bRoundStarted=false;if(GetHUD())GetHUD()->bShowHUD=false;
            auto* P=Cast<AWildlineCharacter>(GetPawn());P->bThirdPerson=true;P->HunterProfile=FWildlineHunterProfile::Travis();P->bProfileSubmitted=true;P->ApplyHunterProfile();SetControlRotation(FRotator::ZeroRotator);
            FString Preset;FParse::Value(FCommandLine::Get(),TEXT("WildlineHunterPreset="),Preset);
            const bool CustomBody=FParse::Param(FCommandLine::Get(),TEXT("WildlineCustomBody"));
            const float ExpectedHeight=CustomBody?180.f:Preset==TEXT("Ethan")?71.4f:Preset==TEXT("Greyson")?110.4f:174.5f;
            if(Preset==TEXT("Ethan"))P->HunterProfile=FWildlineHunterProfile::Ethan();
            else if(Preset==TEXT("Mark"))P->HunterProfile=FWildlineHunterProfile::Mark();
            else if(Preset==TEXT("Greyson"))P->HunterProfile=FWildlineHunterProfile::Greyson();
            if(CustomBody){auto& B=P->HunterProfile;B.HeightCm=180;B.HandSize=1.1f;B.FootSize=1.05f;B.BodyWidth=1.25f;B.BodyDepth=1.15f;B.FaceSize=1.15f;B.Slimness=-.2f;}
            const float FeetZ=P->GetActorLocation().Z-P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();P->ApplyHunterProfile();
            const float Scale=P->HunterProfile.HeightScale();
            Check(FMath::IsNearlyEqual(static_cast<float>(P->GetActorScale3D().Z)*174.5f,ExpectedHeight,.01f),TEXT("selected hunter uses its intended standing height"));
            Check(FMath::IsNearlyEqual(P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),96.f*Scale,.01f)&&FMath::IsNearlyEqual(P->GetActorLocation().Z-P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),FeetZ,.01),TEXT("size changes scale collision and retain ground contact"));
            Check(FMath::IsNearlyEqual(P->FirstCamera->GetComponentLocation().Z-P->GetActorLocation().Z,72.f*Scale,.01)&&FMath::IsNearlyEqual(static_cast<float>(P->ViewRifle->GetComponentScale().Z),Scale,.001f),TEXT("eye level and first-person equipment follow body height"));
            P->SetActorLocation(FVector(-900,1400,WildlineRules::Ground(-900,1400)+P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+4));P->GetCharacterMovement()->StopMovementImmediately();
            auto* Cam=GetWorld()->SpawnActor<ACameraActor>();Cam->AttachToComponent(P->GetMesh(),FAttachmentTransformRules::KeepRelativeTransform);Cam->SetActorRelativeLocation(FVector(300,330,246));Cam->SetActorRelativeRotation((FVector(0,15,81)-FVector(300,330,246)).Rotation());Cam->GetCameraComponent()->FieldOfView=48;SetViewTarget(Cam);
            auto* A=Cast<UWildlineHunterAnim>(P->GetMesh()->GetAnimInstance());bool Assets=A&&A->bCapturedMotion&&A->CrouchDirections.Num()==8&&A->WalkStarts.Num()==8&&A->RunStops.Num()==8;
            if(A)for(const auto* Clips:{&A->WalkDirections,&A->RunDirections,&A->CrouchDirections,&A->WalkStarts,&A->WalkStops,&A->RunStarts,&A->RunStops})for(auto* Clip:*Clips)Assets&=Clip!=nullptr;
            Check(Assets,TEXT("captured locomotion, crouch, starts and stops loaded"));
        });
        auto PoseShot=[this,Render](const TCHAR* Name){if(Render)FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,false,false);};
        At(6,[this,Check,PoseShot](){
            Check(GetPawn()->GetVelocity().Size2D()>200,TEXT("walking uses real character movement"));
            auto* P=Cast<AWildlineCharacter>(GetPawn());auto* M=P->GetMesh();bool Sizes=true;
            for(const TCHAR* Bone:{TEXT("hand_l"),TEXT("hand_r"),TEXT("index_01_l"),TEXT("index_01_r")})Sizes&=M->GetBoneTransform(Bone,RTS_Component).GetScale3D().Equals(FVector(P->HunterProfile.HandScale()),.001);
            for(const TCHAR* Bone:{TEXT("foot_l"),TEXT("foot_r"),TEXT("ball_l"),TEXT("ball_r")})Sizes&=M->GetBoneTransform(Bone,RTS_Component).GetScale3D().Equals(FVector(P->HunterProfile.FootScale()),.001);
            Check(Sizes,TEXT("hands, fingers, feet and toes use the selected body proportions"));
            PoseShot(TEXT("HunterCaptureWalk.png"));
        });
        // These authored teleports are scene resets, not stance-foot travel.
        At(7.5f,[this,PreviousPlant](){PreviousPlant->Init(0.f,2);auto* P=Cast<AWildlineCharacter>(GetPawn());P->SetActorLocation(FVector(-900,1400,WildlineRules::Ground(-900,1400)+P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+4));});
        At(9,[this,Check,PoseShot](){Check(GetPawn()->GetVelocity().Size2D()>480,TEXT("sprint reaches full movement speed"));PoseShot(TEXT("HunterCaptureRun.png"));});
        At(10.5f,[this,PreviousPlant](){PreviousPlant->Init(0.f,2);auto* P=Cast<AWildlineCharacter>(GetPawn());P->SetActorLocation(FVector(-500,800,WildlineRules::Ground(-500,800)+P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+4));});
        At(12,[this,Check,PoseShot](){Check(GetPawn()->GetVelocity().Size2D()>200,TEXT("strafe uses directional captured motion"));PoseShot(TEXT("HunterCaptureStrafe.png"));});
        At(14,[this](){SetControlRotation(FRotator(0,75,0));});At(14.3f,[PoseShot](){PoseShot(TEXT("HunterCaptureTurn.png"));});
        At(15.5f,[this,PreviousPlant](){PreviousPlant->Init(0.f,2);auto* P=Cast<AWildlineCharacter>(GetPawn());P->SetActorLocation(FVector(-500,800,WildlineRules::Ground(-500,800)+P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+4));});
        At(16,[this](){Cast<AWildlineCharacter>(GetPawn())->DoCrouch();});
        At(18,[this,Check,PoseShot](){auto* P=Cast<AWildlineCharacter>(GetPawn());auto* A=Cast<UWildlineHunterAnim>(P->GetMesh()->GetAnimInstance());const float H=P->GetMesh()->GetBoneTransform(TEXT("pelvis"),RTS_Component).GetLocation().Z;UE_LOG(LogTemp,Display,TEXT("HUNTER CROUCH active=%d wants=%d pelvis=%.2f meshZ=%.2f speed=%.1f crouchBlend=%.3f moveBlend=%.3f ground=%.2f phase=%.2f"),P->bIsCrouched,P->GetCharacterMovement()->bWantsToCrouch,H,P->GetMesh()->GetRelativeLocation().Z,P->GetVelocity().Size2D(),A?A->CrouchBlend:0,A?A->MovementBlend:0,A?A->GroundOffset:0,A?A->GaitPhase:0);Check(P->bIsCrouched&&H<75,TEXT("crouching lowers the captured pelvis and articulates the legs"));PoseShot(TEXT("HunterCaptureCrouch.png"));});
        At(21,[this](){Cast<AWildlineCharacter>(GetPawn())->DoUnCrouch();});At(21.4f,[this](){Cast<AWildlineCharacter>(GetPawn())->DoJump();});
        At(20.3f,[this](){auto* P=Cast<AWildlineCharacter>(GetPawn());P->SetActorLocation(P->GetActorLocation()+FVector::UpVector*130);P->GetCharacterMovement()->SetMovementMode(MOVE_Falling);});
        At(20.55f,[this,Check](){auto* P=Cast<AWildlineCharacter>(GetPawn());Check(P->bIsCrouched&&P->GetCharacterMovement()->IsFalling()&&P->GetMesh()->GetBoneTransform(TEXT("pelvis"),RTS_Component).GetLocation().Z<75,TEXT("falling from a ledge preserves the compact crouched body"));});
        At(21.65f,[this,Check,PoseShot](){Check(Cast<AWildlineCharacter>(GetPawn())->GetCharacterMovement()->IsFalling(),TEXT("captured takeoff follows an actual physics jump"));PoseShot(TEXT("HunterCaptureJump.png"));});
        At(22.4f,[PoseShot](){PoseShot(TEXT("HunterCaptureLand.png"));});
        At(25,[this](){Cast<AWildlineCharacter>(GetPawn())->StartAim();SetControlRotation(FRotator(20,75,0));});At(25.5f,[PoseShot](){PoseShot(TEXT("HunterCaptureAim.png"));});
        At(26,[this](){Cast<AWildlineCharacter>(GetPawn())->ServerFire(GetControlRotation().Vector());});
        At(28,[this](){auto* P=Cast<AWildlineCharacter>(GetPawn());P->StopAim();P->SelectWeapon(1);SetControlRotation(FRotator(0,75,0));});
        At(28.5f,[this](){Cast<AWildlineCharacter>(GetPawn())->ServerFire(GetControlRotation().Vector());});At(28.85f,[PoseShot](){PoseShot(TEXT("HunterCapturePump.png"));});
        At(31,[this,PreviousPlant](){PreviousPlant->Init(0.f,2);auto* P=Cast<AWildlineCharacter>(GetPawn());P->HunterProfile.Body=1;P->ApplyHunterProfile();});At(32,[PoseShot](){PoseShot(TEXT("HunterCaptureFemale.png"));});
        auto Frame=MakeShared<int32>(0);FTimerHandle Monitor;
        FTimerManagerTimerParameters ContactTimer;ContactTimer.bLoop=true;ContactTimer.bMaxOncePerFrame=true;
        GetWorldTimerManager().SetTimer(Monitor,FTimerDelegate::CreateWeakLambda(this,[this,Render,Frame,Passed,MaxGrip,MaxShoulder,MaxFootDrift,PreviousFeet,PreviousPlant,PreviousSerial,MovingPlants,LastSampleFrame,PreviousScale,PlantWarmupUntil](){
            const float Time=GetWorld()->GetTimeSeconds();if(Time<4||Time>33)return;
            if(*LastSampleFrame==GFrameCounter)return;*LastSampleFrame=GFrameCounter;
            auto* P=Cast<AWildlineCharacter>(GetPawn());auto* M=P->GetMesh();auto* A=Cast<UWildlineHunterAnim>(M->GetAnimInstance());const FTransform Gun=P->WorldRifle->GetComponentTransform();
            if(!FMath::IsNearlyEqual(*PreviousScale,P->HunterProfile.HeightScale())){*PreviousScale=P->HunterProfile.HeightScale();*PlantWarmupUntil=Time+.5f;}
            if(Time<*PlantWarmupUntil)PreviousPlant->Init(0.f,2);
            const FVector Pocket=M->GetSocketLocation(TEXT("upperarm_r"))+(P->GetActorForwardVector()*5-P->GetActorRightVector()*3-FVector::UpVector*6)*P->HunterProfile.HeightScale();
            const FVector Butt(P->SelectedWeapon==1?P->WorldRifle->GetStaticMesh()->GetBoundingBox().Min.X+1:-45.6f,0,P->SelectedWeapon==1?-4.8f:-4.f);
            if(A&&P->GetVelocity().Size2D()>100)for(int32 Side=0;Side<2;Side++)if(A->FootPlantWeight[Side]>.98f)(*MovingPlants)++;
            const float ShoulderError=FVector::Dist(Pocket,Gun.TransformPosition(Butt));
            if(ShoulderError>*MaxShoulder+.5f)UE_LOG(LogTemp,Display,TEXT("HUNTER CONTACT shoulder=%.2f time=%.3f speed=%.1f crouch=%d"),ShoulderError,Time,P->GetVelocity().Size2D(),P->bIsCrouched);
            *MaxShoulder=FMath::Max(*MaxShoulder,ShoulderError);
            *MaxGrip=FMath::Max(*MaxGrip,static_cast<float>(FVector::Dist(M->GetSocketLocation(TEXT("hand_r")),Gun.TransformPosition(FVector(-17,0,P->SelectedWeapon==1?-4.8f:-2.8f)))));
            *MaxGrip=FMath::Max(*MaxGrip,static_cast<float>(FVector::Dist(M->GetSocketLocation(TEXT("hand_l")),(P->SelectedWeapon==1?P->WorldBolt->GetComponentTransform():Gun).TransformPosition(FVector(P->SelectedWeapon==1?18.5f:17,-1,-3)))));
            for(int32 Side=0;Side<2;Side++){const FVector Foot=M->GetSocketLocation(Side==0?TEXT("foot_l"):TEXT("foot_r"));if(A&&A->FootPlantSerial[Side]==(*PreviousSerial)[Side]&&A->FootPlantWeight[Side]>.98f&&(*PreviousPlant)[Side]>.98f){const float Drift=FVector::Dist2D(Foot,(*PreviousFeet)[Side]);if(Drift>*MaxFootDrift+.5f)UE_LOG(LogTemp,Display,TEXT("HUNTER CONTACT foot=%d drift=%.2f time=%.3f phase=%.3f speed=%.1f crouch=%d"),Side,Drift,Time,A->GaitPhase,P->GetVelocity().Size2D(),P->bIsCrouched);*MaxFootDrift=FMath::Max(*MaxFootDrift,Drift);}(*PreviousFeet)[Side]=Foot;(*PreviousPlant)[Side]=A?A->FootPlantWeight[Side]:0;(*PreviousSerial)[Side]=A?A->FootPlantSerial[Side]:-1;}
            for(const TCHAR* Bone:{TEXT("pelvis"),TEXT("foot_l"),TEXT("foot_r"),TEXT("hand_l"),TEXT("hand_r")})*Passed&=!M->GetBoneTransform(Bone,RTS_Component).ContainsNaN();
            if(Render){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/Windows/HunterLocomotion/Frame%04d.png"),(*Frame)++),false,false);}
        }),1.f/60.f,ContactTimer);
        At(33.2f,[Check,Passed,MaxGrip,MaxShoulder,MaxFootDrift,MovingPlants](){Check(*MaxShoulder<.5f,TEXT("shoulder contact remains stable throughout movement and stance changes"));Check(*MaxGrip<3.f,TEXT("both weapon grips stay within three centimetres throughout the sequence"));Check(*MaxFootDrift<3.f&&*MovingPlants>10,TEXT("moving stance feet plant and remain stable in world space"));UE_LOG(LogTemp,Display,TEXT("HUNTER ANIMATION RESULT: %s shoulder=%.2f grip=%.2f footDrift=%.2f movingPlants=%d"),*Passed?TEXT("PASS"):TEXT("FAIL"),*MaxShoulder,*MaxGrip,*MaxFootDrift,*MovingPlants);FPlatformMisc::RequestExitWithStatus(false,*Passed?0:1);});
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineHunterReview"))){
        auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
        auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,true,false);};
        At(3,[this](){Menu->CreateHunter();});
        At(5,[this](){auto* C=Menu->Creator;C->Pending.Outfit=1;C->SyncControls();C->RefreshPreview();});
        At(8,[Shot](){Shot(TEXT("HunterCreator.png"));});
        At(10,[this](){Menu->Creator->FaceView();});
        At(13,[Shot](){Shot(TEXT("HunterFace.png"));});
        At(15,[this](){auto* C=Menu->Creator;C->Pending.Body=1;C->SyncControls();C->RefreshPreview();C->FullBody();});
        At(18,[Shot](){Shot(TEXT("HunterFemale.png"));});
        At(20,[this](){FString Photo;if(FParse::Value(FCommandLine::Get(),TEXT("WildlineHunterPhoto="),Photo))Menu->Creator->ImportPortraitPath(Photo);});
        At(23,[Shot](){Shot(TEXT("HunterPhoto.png"));});
        At(25,[this](){Menu->Creator->Cancel();ChooseRole(false);SetControlRotation(FRotator(-7,-20,0));GetWorld()->GetGameState<AWildlineGameState>()->bRoundStarted=false;});
        At(28,[Shot](){Shot(TEXT("BoltRifle.png"));});
        At(29,[this](){Cast<AWildlineCharacter>(GetPawn())->StartAim();});
        At(32,[Shot](){Shot(TEXT("HunterScope.png"));});
        At(34,[this](){Cast<AWildlineCharacter>(GetPawn())->ServerThermal();});
        At(37,[Shot](){Shot(TEXT("HunterThermal.png"));});
        At(39,[](){FPlatformMisc::RequestExit(false);});
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlinePhotoHuntersReview"))){
        auto At=[this](float Seconds,TFunction<void()> Action){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Action](){Action();}),Seconds,false);};
        auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows")/Name,true,false);};
        At(3,[this](){Menu->CreateHunter();});
        At(5,[this](){Menu->Creator->LoadTravisPreset();Menu->Creator->FaceView();});
        At(10,[Shot](){Shot(TEXT("TravisFace.png"));});
        At(12,[this](){Menu->Creator->TurnRight();Menu->Creator->TurnRight();});
        At(16,[Shot](){Shot(TEXT("TravisThreeQuarter.png"));});
        At(18,[this](){Menu->Creator->LoadEthanPreset();Menu->Creator->FaceView();});
        At(23,[Shot](){Shot(TEXT("EthanFace.png"));});
        At(25,[this](){Menu->Creator->TurnRight();Menu->Creator->TurnRight();});
        At(30,[Shot](){Shot(TEXT("EthanThreeQuarter.png"));});
        At(32,[this](){Menu->Creator->TurnRight();Menu->Creator->TurnRight();});
        At(37,[Shot](){Shot(TEXT("EthanProfile.png"));});
        At(39,[this](){Menu->Creator->PreviewAngle=0;Menu->Creator->FullBody();});
        At(44,[Shot](){Shot(TEXT("EthanCreator.png"));});
        At(46,[](){FPlatformMisc::RequestExit(false);});
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineHunterFacesReview"))){
        auto At=[this](float Seconds,TFunction<void()> Action){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Action](){Action();}),Seconds,false);};
        auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows")/Name,true,false);};
        At(3,[this](){Menu->CreateHunter();Menu->Creator->LoadTravisPreset();Menu->Creator->FaceView();});
        At(8,[Shot](){Shot(TEXT("HunterFacesTravis.png"));});
        At(10,[this](){Menu->Creator->LoadEthanPreset();});
        At(15,[Shot](){Shot(TEXT("HunterFacesEthan.png"));});
        At(17,[this](){Menu->Creator->LoadGreysonPreset();});
        At(22,[Shot](){Shot(TEXT("HunterFacesGreyson.png"));});
        At(24,[this](){Menu->Creator->PresetChanged(TEXT("Mark"),ESelectInfo::Direct);});
        At(29,[Shot](){Shot(TEXT("MarkFace.png"));});
        At(31,[this](){Menu->Creator->TurnRight();Menu->Creator->TurnRight();});
        At(36,[Shot](){Shot(TEXT("MarkThreeQuarter.png"));});
        At(38,[this](){auto* C=Menu->Creator;C->PreviewAngle=0;C->FullBody();Cast<UWildlineChoice>(C->PresetCombo)->SetMenuOpen(true);});
        At(43,[Shot](){Shot(TEXT("MarkCreator.png"));});
        At(45,[this](){auto* C=Menu->Creator;Cast<UWildlineChoice>(C->PresetCombo)->SetMenuOpen(false);C->Pending.Outfit=2;C->Pending.bCap=true;C->SyncControls();C->RefreshPreview();C->FaceView();});
        At(50,[Shot](){Shot(TEXT("MarkCustomized.png"));});
        At(52,[this](){
            Menu->Creator->ResetPreset();Menu->Creator->Cancel();ChooseRole(false);
            auto* P=Cast<AWildlineCharacter>(GetPawn());P->bProfileSubmitted=true;P->bThirdPerson=true;
            P->HunterProfile=FWildlineHunterProfile::Mark();P->ApplyHunterProfile();
            P->SetActorLocation(FVector(-900,1400,WildlineRules::Ground(-900,1400)+96));P->SetActorRotation(FRotator::ZeroRotator);SetControlRotation(FRotator::ZeroRotator);
            P->GetMesh()->SetOwnerNoSee(false);P->WorldRifle->SetVisibility(false);P->WorldBolt->SetVisibility(false);
            GetWorld()->GetGameState<AWildlineGameState>()->bRoundStarted=false;
            auto* Camera=GetWorld()->SpawnActor<ACameraActor>();Camera->SetActorLocation(P->GetActorLocation()+FVector(105,0,68));
            Camera->SetActorRotation((P->GetActorLocation()+FVector(0,0,65)-Camera->GetActorLocation()).Rotation());SetViewTarget(Camera);
        });
        At(58,[Shot](){Shot(TEXT("MarkInGame.png"));});
        At(60,[](){FPlatformMisc::RequestExit(false);});
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineHunterHeightReview"))){
        auto At=[this](float Seconds,TFunction<void()> Action){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Action](){Action();}),Seconds,false);};
        auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows")/Name,true,false);};
        At(3,[this](){Menu->CreateHunter();Menu->Creator->LoadTravisPreset();Menu->Creator->PreviewAngle=0;Menu->Creator->FullBody();});
        At(8,[Shot](){Shot(TEXT("HunterHeightTravis.png"));});
        At(10,[this](){Menu->Creator->LoadEthanPreset();});
        At(15,[Shot](){Shot(TEXT("HunterHeightEthan.png"));});
        At(17,[this](){Menu->Creator->LoadGreysonPreset();});
        At(22,[Shot](){Shot(TEXT("HunterHeightGreyson.png"));});
        At(24,[this](){Menu->Creator->FaceView();});
        At(29,[Shot](){Shot(TEXT("HunterHeightGreysonFace.png"));});
        At(31,[this](){Menu->Creator->LoadEthanPreset();Menu->Creator->FaceView();});
        At(36,[Shot](){Shot(TEXT("HunterHeightEthanFace.png"));});
        At(38,[](){FPlatformMisc::RequestExit(false);});
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineDropdownReview"))){
        auto At=[this](float Seconds,TFunction<void()> Action){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Action](){Action();}),Seconds,false);};
        auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows")/Name,true,false);};
        At(3,[this](){Menu->CreateHunter();Menu->Creator->LoadGreysonPreset();Menu->Creator->PreviewAngle=0;Menu->Creator->FullBody();});
        At(8,[Shot](){Shot(TEXT("GreysonShorterCreator.png"));});
        At(10,[this](){Cast<UWildlineChoice>(Menu->Creator->PresetCombo)->SetMenuOpen(true);});
        At(12,[Shot](){Shot(TEXT("HunterPresetDropdown.png"));});
        At(14,[this](){Cast<UWildlineChoice>(Menu->Creator->PresetCombo)->SetMenuOpen(false);Cast<UWildlineChoice>(Menu->Creator->OutfitCombo)->SetMenuOpen(true);});
        At(16,[Shot](){Shot(TEXT("HunterOutfitDropdown.png"));});
        At(18,[this](){Cast<UWildlineChoice>(Menu->Creator->OutfitCombo)->SetMenuOpen(false);Menu->Creator->Cancel();Menu->SetWorkshop(false);});
        At(20,[this](){Cast<UWildlineChoice>(Menu->Mode)->SetMenuOpen(true);});
        At(22,[Shot](){Shot(TEXT("GameModeDropdown.png"));});
        At(24,[this](){Cast<UWildlineChoice>(Menu->Mode)->SetMenuOpen(false);FPlatformMisc::RequestExit(false);});
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineGreysonReview"))){
        auto At=[this](float Seconds,TFunction<void()> Action){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Action](){Action();}),Seconds,false);};
        auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows")/Name,true,false);};
        At(3,[this](){Menu->CreateHunter();Menu->Creator->LoadGreysonPreset();});
        At(8,[Shot](){Shot(TEXT("GreysonCreator.png"));});
        At(10,[this](){Menu->Creator->PreviewAngle=0;Menu->Creator->FaceView();});
        At(15,[Shot](){Shot(TEXT("GreysonFace.png"));});
        At(17,[this](){Menu->Creator->TurnRight();Menu->Creator->TurnRight();});
        At(22,[Shot](){Shot(TEXT("GreysonThreeQuarter.png"));});
        At(24,[this](){Menu->Creator->TurnRight();Menu->Creator->TurnRight();});
        At(29,[Shot](){Shot(TEXT("GreysonProfile.png"));});
        At(31,[this](){auto* C=Menu->Creator;C->Pending.Outfit=0;C->Pending.bCap=true;C->Pending.Slimness=.3f;C->SyncControls();C->RefreshPreview();C->FullBody();});
        At(36,[Shot](){Shot(TEXT("GreysonCustomized.png"));});
        At(38,[](){FPlatformMisc::RequestExit(false);});
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineTravisReview"))){
        auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
        auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,true,false);};
        At(3,[this](){Menu->CreateHunter();});
        At(5,[this](){Menu->Creator->LoadTravisPreset();});
        At(10,[Shot](){Shot(TEXT("TravisCreator.png"));});
        At(12,[this](){Menu->Creator->FaceView();});
        At(16,[Shot](){Shot(TEXT("TravisFace.png"));});
        At(18,[this](){Menu->Creator->TurnRight();Menu->Creator->TurnRight();});
        At(22,[Shot](){Shot(TEXT("TravisThreeQuarter.png"));});
        At(24,[this](){Menu->Creator->TurnRight();Menu->Creator->TurnRight();});
        At(28,[Shot](){Shot(TEXT("TravisProfile.png"));});
        At(36,[this](){auto* C=Menu->Creator;C->LoadTravisPreset();C->Pending.Outfit=0;C->Pending.bCap=true;C->Pending.Slimness=.2f;C->SyncControls();C->RefreshPreview();C->FullBody();});
        At(40,[Shot](){Shot(TEXT("TravisCustomized.png"));});
        At(42,[this](){Menu->Creator->Cancel();ChooseRole(false);auto* P=Cast<AWildlineCharacter>(GetPawn());P->HunterProfile=FWildlineHunterProfile::Travis();P->bProfileSubmitted=true;P->ApplyHunterProfile();P->ToggleCamera();SetControlRotation(FRotator(-7,160,0));GetWorld()->GetGameState<AWildlineGameState>()->bRoundStarted=false;});
        At(46,[Shot](){Shot(TEXT("TravisGameplay.png"));});
        At(48,[](){FPlatformMisc::RequestExit(false);});
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineForestCapture"))){
        FTimerHandle Enter,ShotA,Move,ShotB,Low,ShotC,Exit;
        auto View=[this](FVector P,FVector Target){
            P.Z+=WildlineRules::Ground(P.X,P.Y);Target.Z+=WildlineRules::Ground(Target.X,Target.Y);
            auto* Cam=GetWorld()->SpawnActor<ACameraActor>();Cam->SetActorLocation(P);Cam->SetActorRotation((Target-P).Rotation());Cam->GetCameraComponent()->FieldOfView=65;SetViewTarget(Cam);
        };
        auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,false,false);};
        GetWorldTimerManager().SetTimer(Enter,FTimerDelegate::CreateWeakLambda(this,[this,View](){ChooseRole(false);GetWorld()->GetGameState<AWildlineGameState>()->bRoundStarted=false;if(GetHUD())GetHUD()->bShowHUD=false;View(FVector(0,750,170),FVector(2100,-500,320));}),3.f,false);
        GetWorldTimerManager().SetTimer(ShotA,FTimerDelegate::CreateWeakLambda(this,[Shot](){Shot(TEXT("ForestCamp.png"));}),12.f,false);
        GetWorldTimerManager().SetTimer(Move,FTimerDelegate::CreateWeakLambda(this,[View](){View(FVector(-2200,-1400,165),FVector(900,-2800,340));}),14.f,false);
        GetWorldTimerManager().SetTimer(ShotB,FTimerDelegate::CreateWeakLambda(this,[Shot](){Shot(TEXT("ForestUnderstory.png"));}),23.f,false);
        GetWorldTimerManager().SetTimer(Low,FTimerDelegate::CreateWeakLambda(this,[View](){View(FVector(370,350,24),FVector(750,0,60));}),25.f,false);
        GetWorldTimerManager().SetTimer(ShotC,FTimerDelegate::CreateWeakLambda(this,[Shot](){Shot(TEXT("ForestFloor.png"));}),33.f,false);
        GetWorldTimerManager().SetTimer(Exit,FTimerDelegate::CreateWeakLambda(this,[](){FPlatformMisc::RequestExit(false);}),35.f,false);
    }
    const bool ReviewIntro=FParse::Param(FCommandLine::Get(),TEXT("WildlineIntroReview"));
    const bool StartupProbe=FParse::Param(FCommandLine::Get(),TEXT("WildlineIntroSmoke"));
    const bool SkipIntro=FParse::Param(FCommandLine::Get(),TEXT("WildlineNoIntro"))||FParse::Param(FCommandLine::Get(),TEXT("WildlineAutoStart"))||(FParse::Param(FCommandLine::Get(),TEXT("unattended"))&&!ReviewIntro&&!StartupProbe);
    if(bMenuOpen&&GI&&!GI->bIntroPlayed&&!SkipIntro){StartIntro();}
    if(ReviewIntro){
        auto At=[this](float Delay,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Delay,false);};
        At(AWildlineIntro::Duration+2,[](){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/PresentationMenu.png"),true,false);});
        At(AWildlineIntro::Duration+3,[this](){Menu->ShowSettings();});
        At(AWildlineIntro::Duration+4,[](){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/PresentationSettings.png"),true,false);});
        At(AWildlineIntro::Duration+6,[](){FPlatformMisc::RequestExit(false);});
    }
    if(StartupProbe)RunIntroSmokeTest();
}
void AWildlineController::SetupInputComponent(){Super::SetupInputComponent();InputComponent->BindAction(TEXT("Menu"),IE_Pressed,this,&AWildlineController::ToggleMenu);InputComponent->BindAction(TEXT("Restart"),IE_Pressed,this,&AWildlineController::RequestRestart);InputComponent->BindAction(TEXT("MapZoom"),IE_Pressed,this,&AWildlineController::ToggleMapZoom);}
void AWildlineController::SetMenu(bool Open){
    if(Intro){Intro->Finish();if(Open)return;}
    bMenuOpen=Open;bShowMouseCursor=Open;
    if(Open)StartMenuMusic();
    if(Menu){if(Open)Menu->RefreshDifficulty();Menu->SetVisibility(Open?ESlateVisibility::Visible:ESlateVisibility::Collapsed);}
    if(Open){if(auto* P=Cast<AWildlineCharacter>(GetPawn())){P->bFiring=false;P->StopAim();P->StopSprint();}FInputModeGameAndUI Mode;if(Menu)Mode.SetWidgetToFocus(Menu->TakeWidget());Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);}
    else {FInputModeGameOnly Mode;SetInputMode(Mode);SetIgnoreLookInput(false);SetIgnoreMoveInput(false);}
}
void AWildlineController::ToggleMapZoom(){const auto* S=GetWorld()->GetGameState<AWildlineGameState>();if(!bMenuOpen&&S&&S->MatchConfig.bAllowMinimap)bMapZoomed=!bMapZoomed;}
void AWildlineController::ToggleMenu(){if(Intro){Intro->Finish();return;}SetMenu(!bMenuOpen);}
void AWildlineController::ChooseRole(bool bAnimal){auto* GI=GetGameInstance<UWildlineGameInstance>();if(GI)GI->bPreferredSquirrel=bAnimal;if(Menu){if(bAnimal)Menu->Squirrel();else Menu->Hunter();}ServerPickRole(bAnimal);if(!bAnimal)if(const auto* Settings=UWildlineUserSettings::Get())ServerSelectWeapon(Settings->SelectedWeapon);SetMenu(false);}
void AWildlineController::ServerPickRole_Implementation(bool bAnimal){if(auto* GM=GetWorld()->GetAuthGameMode<AWildlineGameMode>())GM->PickRole(this,bAnimal);}
static const TCHAR* SelectedExpeditionMap(int32 Mode){const auto* Settings=UWildlineUserSettings::Get();return Mode==0&&Settings&&Settings->ClassicConfig.ClassicLayout==1?TEXT("Frostwood"):Mode==2&&Settings&&Settings->HillConfig.NeighborhoodLayout==1?TEXT("NeighborhoodCedar"):WildlineRules::ModeMap(Mode);}
void AWildlineController::Host(bool bAnimal){auto* GI=GetGameInstance<UWildlineGameInstance>();if(GI){GI->bPreferredSquirrel=bAnimal;GI->bStartAfterTravel=true;}UGameplayStatics::OpenLevel(this,FName(SelectedExpeditionMap(GI?GI->PreferredMode:0)),true,TEXT("listen"));}
void AWildlineController::Join(FString Address,bool bAnimal){Address=Address.TrimStartAndEnd();if(Address.IsEmpty())return;auto* GI=GetGameInstance<UWildlineGameInstance>();if(GI){GI->bPreferredSquirrel=bAnimal;GI->bStartAfterTravel=true;GI->JoinAddress=Address;}ClientTravel(Address,TRAVEL_Absolute);}
void AWildlineController::ServerSelectWeapon_Implementation(int32 Weapon){const auto* GM=GetWorld()->GetAuthGameMode<AWildlineGameMode>();if(GM&&GM->bWaitingRoomsEnabled)return;if(auto* P=Cast<AWildlineCharacter>(GetPawn()))P->SelectWeapon(Weapon);}
void AWildlineController::PlaySelected(bool Animal){
    auto* GI=GetGameInstance<UWildlineGameInstance>();const auto* State=GetWorld()->GetGameState<AWildlineGameState>();
    if(HasAuthority()&&GI&&State&&(GI->PreferredMode!=State->ExpeditionMode||(UWildlineUserSettings::Get()&&UWildlineUserSettings::Get()->ConfigForMode(GI->PreferredMode)!=State->MatchConfig))){GI->bPreferredSquirrel=Animal;GI->bStartAfterTravel=true;UGameplayStatics::OpenLevel(this,FName(SelectedExpeditionMap(GI->PreferredMode)),true,GetNetMode()==NM_ListenServer?TEXT("listen"):TEXT(""));return;}
    if(auto* P=Cast<AWildlineCharacter>(GetPawn());P&&P->bSquirrel==Animal&&(P->bInLobby||(State&&State->bRoundStarted)))SetMenu(false);
    else EnterWaitingRoom(Animal);
}
void AWildlineController::MenuClick(){}
void AWildlineController::RequestRestart(){ServerRestartRound();}
void AWildlineController::ServerRestartRound_Implementation(){if(auto* GM=GetWorld()->GetAuthGameMode<AWildlineGameMode>())GM->RestartRound();}

void AWildlineController::RunSmokeTest(){
    if(!HasAuthority())return;
    ChooseRole(false);
    auto* P=Cast<AWildlineCharacter>(GetPawn());auto* State=GetWorld()->GetGameState<AWildlineGameState>();auto* Forest=AWildlineWorld::Find(GetWorld());
    if(P)P->SelectWeapon(0);
    bool Ok=P&&State&&Forest;
    auto Check=[&](bool Passed,const TCHAR* Message){Ok=Ok&&Passed;UE_LOG(LogTemp,Display,TEXT("WILDLINE SMOKE %s: %s"),Passed?TEXT("PASS"):TEXT("FAIL"),Message);};
    if(!Ok){UE_LOG(LogTemp,Error,TEXT("WILDLINE SMOKE FAILED: missing world or player"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
    State->bRoundStarted=false; // Freeze AI while checking live physics and RPC behavior.
    Check(Forest->Trees.Num()>100,TEXT("native forest generated"));
    bool GroundedRoots=true;for(const auto& T:Forest->Trees){const float Collar=T.Species==3?460.f:0.f;GroundedRoots&=T.MeshTransform.TransformPosition(FVector(0,0,Collar)).Z<T.Base.Z-6;}
    Check(GroundedRoots,TEXT("all gameplay root collars are planted below terrain"));
    Check(Forest->ReserveFence->GetInstanceCount()==720&&Forest->ReserveFence->GetCollisionEnabled()!=ECollisionEnabled::NoCollision,TEXT("visible collidable perimeter spans the reserve"));
    Check(WildlineRules::Ground(20500,0)-WildlineRules::Ground(14500,0)>1800,TEXT("surrounding ridges close the horizon"));
    FHitResult FenceHit;FCollisionQueryParams FenceQuery(SCENE_QUERY_STAT(ReserveFence));
    const float FenceX=WildlineRules::MapRadius-60.f;const float FenceZ=WildlineRules::Ground(FenceX,0)+52;
    Check(GetWorld()->LineTraceSingleByChannel(FenceHit,FVector(FenceX-100,0,FenceZ),FVector(FenceX+100,0,FenceZ),ECC_Visibility,FenceQuery)&&FenceHit.Component==Forest->ReserveFence,TEXT("boundary rail blocks live physics traces"));
    bool PlantAssets=true;
    for(auto* Layer:Forest->ForestLayers)PlantAssets&=Layer&&Layer->GetStaticMesh()&&Layer->GetStaticMesh()->GetPathName().StartsWith(TEXT("/Game/Wildline/Forest/"))&&Layer->GetInstanceCount()>0;
    Check(PlantAssets,TEXT("licensed forest meshes cooked and instanced for every vegetation layer"));
    bool SolidWood=Forest->ClimbWood.Num()==4;for(auto* Wood:Forest->ClimbWood)SolidWood&=Wood&&Wood->GetCollisionEnabled()!=ECollisionEnabled::NoCollision;Check(!Forest->Trunks->IsVisible()&&SolidWood,TEXT("wood collision stays active without placeholder cylinders rendering"));
    UStaticMesh* Stone=Forest->Rocks->GetStaticMesh();
    Check(Stone&&Stone->GetBodySetup()&&Stone->GetBodySetup()->AggGeom.ConvexElems.Num()>0&&Forest->Rocks->GetCollisionEnabled()==ECollisionEnabled::QueryAndPhysics,TEXT("rocks have cooked convex hulls and physical collision"));
    int32 SolidRockTraces=0,SolidRockSweeps=0;
    if(Stone)for(int32 I=0;I<Forest->Rocks->GetInstanceCount()&&SolidRockTraces<8;I++){
        FTransform Transform;Forest->Rocks->GetInstanceTransform(I,Transform,true);if(Transform.GetScale3D().X<.8f)continue;
        const FVector Centre=Transform.TransformPosition(Stone->GetBoundingBox().GetCenter());const float Reach=Stone->GetBoundingBox().GetExtent().Size()*Transform.GetScale3D().GetMax()+60;
        FHitResult RockHit,SweepHit;FCollisionQueryParams Query(SCENE_QUERY_STAT(RockSmoke),false);
        if(GetWorld()->LineTraceSingleByChannel(RockHit,Centre+FVector(0,0,Reach),Centre,ECC_Visibility,Query)&&RockHit.Component==Forest->Rocks)SolidRockTraces++;
        if(GetWorld()->SweepSingleByChannel(SweepHit,Centre+FVector(Reach,0,0),Centre,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(12,12),Query)&&SweepHit.Component==Forest->Rocks)SolidRockSweeps++;
    }
    Check(SolidRockTraces>=8&&SolidRockSweeps>=4,TEXT("multiple scaled rocks block live bullet traces and squirrel capsule sweeps"));
    Check(Forest->AcornLocations.Num()==30&&Forest->Acorns->GetInstanceCount()==30&&Forest->AcornCaps->GetInstanceCount()==30,TEXT("thirty enlarged acorns have matching cap meshes"));
    bool CloseForage=true;for(int32 I=0;I<12;I++)CloseForage&=Forest->AcornLocations[I].Size2D()<3000;
    Check(CloseForage,TEXT("twelve forage pickups are within thirty metres of camp"));
    UStaticMesh* LogMesh=Forest->DenMesh->GetStaticMesh();Check(LogMesh&&LogMesh->GetBodySetup()&&LogMesh->GetBodySetup()->AggGeom.ConvexElems.Num()>=12,TEXT("hollow logs have cooked convex wall collision"));
    FHitResult Wall,Tunnel;FCollisionQueryParams LQ(SCENE_QUERY_STAT(LogWalls));
    const FTransform LT=Forest->DenMesh->GetComponentTransform();const bool HitWall=GetWorld()->LineTraceSingleByChannel(Wall,LT.TransformPosition(FVector(0,80,0)),LT.TransformPosition(FVector(0,0,0)),ECC_Visibility,LQ);
    const bool HitTunnel=GetWorld()->LineTraceSingleByChannel(Tunnel,LT.TransformPosition(FVector(-175,0,0)),LT.TransformPosition(FVector(175,0,0)),ECC_Visibility,LQ);
    Check(HitWall&&Wall.Component==Forest->DenMesh&&(!HitTunnel||Tunnel.Component!=Forest->DenMesh),TEXT("log walls block movement traces while the hollow tunnel stays open"));
    Check(Forest->ForestLayers[18]->GetCollisionEnabled()!=ECollisionEnabled::NoCollision&&Forest->DenMesh->GetCollisionEnabled()!=ECollisionEnabled::NoCollision,TEXT("fallen logs and den both enable collision"));
    Check(P->GetMesh()->GetSkeletalMeshAsset()&&P->GetMesh()->GetSkeletalMeshAsset()->GetPathName().StartsWith(TEXT("/Game/Wildline/Hunters/")),TEXT("detailed outdoor hunter replaces mannequin"));
    Check(P->GetMesh()->GetSkeletalMeshAsset()->FindMorphTarget(TEXT("FaceWidth"))&&P->GetMesh()->GetSkeletalMeshAsset()->FindMorphTarget(TEXT("NoseDepth")),TEXT("facial width and nose shape controls have cooked morph targets"));
    Check(P->ViewRifle->GetStaticMesh()&&P->ViewBolt->GetStaticMesh()&&P->ViewRifle->GetStaticMesh()->GetName()==TEXT("SM_BoltRifle"),TEXT("sporting rifle and separate working bolt are cooked"));
    P->SelectWeapon(1);P->Tick(.016f);
    Check(P->SelectedWeapon==1&&P->Ammo==2&&P->ViewRifle->GetStaticMesh()&&P->ViewRifle->GetStaticMesh()->GetName()==TEXT("SM_Shotgun")&&P->ViewBolt->GetStaticMesh()&&P->ViewBolt->GetStaticMesh()->GetBoundingBox().GetSize().X>15&&P->ViewBolt->GetStaticMesh()->GetBoundingBox().GetSize().X<30,TEXT("Classic also loads the textured shotgun with a separate wooden pump"));
    P->SelectWeapon(0);P->Tick(.016f);
    auto CheckGrip=[&](){auto* M=P->GetMesh();M->TickAnimation(.05f,false);M->RefreshBoneTransforms();
        const auto T=P->WorldRifle->GetComponentTransform();
        auto* Anim=Cast<UWildlineHunterAnim>(M->GetAnimInstance());
        return Anim&&Anim->Idle&&Anim->Walk&&Anim->Run&&FVector::Dist(M->GetSocketLocation(TEXT("hand_r")),T.TransformPosition(FVector(-17,0,-2.8f)))<3&&FVector::Dist(M->GetSocketLocation(TEXT("hand_l")),T.TransformPosition(FVector(17,-1,-3)))<3;
    };
    Check(CheckGrip(),TEXT("both hunter wrists reach sporting rifle stock"));
    auto CheckShoulder=[&](){auto* M=P->GetMesh();const FVector Pocket=M->GetSocketLocation(TEXT("upperarm_r"))+(P->GetActorForwardVector()*5-P->GetActorRightVector()*3-FVector::UpVector*6)*P->HunterProfile.HeightScale();
        return FVector::Dist(Pocket,P->WorldRifle->GetComponentTransform().TransformPosition(FVector(-45.6f,0,-4)))<.5f;
    };
    Check(CheckShoulder(),TEXT("rifle recoil pad contacts the animated shoulder pocket"));
    auto* HunterAnim=Cast<UWildlineHunterAnim>(P->GetMesh()->GetAnimInstance());bool Directions=HunterAnim&&HunterAnim->JumpPose&&HunterAnim->WalkDirections.Num()==8&&HunterAnim->RunDirections.Num()==8;
    if(HunterAnim){for(auto* A:HunterAnim->WalkDirections)Directions&=A!=nullptr;for(auto* A:HunterAnim->RunDirections)Directions&=A!=nullptr;}
    Check(Directions,TEXT("eight rifle locomotion directions and airborne pose are cooked"));
    P->StartAim();P->Tick(.016f);Check(CheckGrip()&&CheckShoulder(),TEXT("scoped posture retains shoulder contact and both wrist grips"));P->StopAim();P->Tick(.016f);
    P->GetCharacterMovement()->Velocity=FVector(0,-580,0);Check(CheckGrip(),TEXT("sprinting sideways retains both rifle grips"));P->GetCharacterMovement()->Velocity=FVector::ZeroVector;
    FWildlineHunterProfile TestProfile=P->HunterProfile;TestProfile.Body=1;TestProfile.Outfit=1;P->ServerSetHunterProfile(TestProfile,FString());
    bool FieldMaterials=P->HunterMaterials.Num()>=7;for(auto* M:P->HunterMaterials)FieldMaterials&=M&&M->Parent&&M->Parent->GetPathName().StartsWith(TEXT("/Game/Wildline/Hunters/Materials/"));
    Check(P->GetMesh()->GetSkeletalMeshAsset()->GetName()==TEXT("SK_HunterFemale")&&FieldMaterials,TEXT("hunter body and real field materials apply to live pawn"));
    Check(CheckGrip(),TEXT("female hunter maintains both rifle grips"));
    TestProfile.Body=0;P->ServerSetHunterProfile(TestProfile,FString());
    FString TestPhoto,PhotoError;const FString Fixture=FPaths::ProjectSavedDir()/TEXT("PortraitTest.png");
    FImage Portrait(64,64,ERawImageFormat::BGRA8,EGammaSpace::sRGB);FMemory::Memset(Portrait.RawData.GetData(),127,Portrait.RawData.Num());FImageUtils::SaveImageByExtension(*Fixture,Portrait);
    const bool PhotoOK=WildlinePortrait::ImportPhoto(Fixture,TestPhoto,PhotoError);TestProfile.bUsePhoto=true;P->ServerSetHunterProfile(TestProfile,TestPhoto);
    Check(PhotoOK&&P->HunterFaceTexture&&P->HunterFaceData==TestPhoto,TEXT("PNG portrait decodes and reaches the live 3D face material"));
    const FString ValidPhoto=P->HunterFaceData;P->ServerSetHunterProfile(TestProfile,TEXT("invalid image"));Check(P->HunterFaceData==ValidPhoto,TEXT("invalid portrait data is rejected without replacing the face"));
    TestProfile.bUsePhoto=false;P->ServerSetHunterProfile(TestProfile,FString());
    const auto BeforeTravis=P->HunterProfile;P->ServerSetHunterProfile(FWildlineHunterProfile::Travis(),FString());
    Check(P->GetMesh()->GetSkeletalMeshAsset()->GetName()==TEXT("SK_HunterTravis")&&!P->HunterFaceTexture&&P->HunterMaterials.Num()>=7,TEXT("Travis selects the featureless photo head without an upload"));
    Check(P->GetMesh()->GetSkeletalMeshAsset()->FindMorphTarget(TEXT("SlimBuild"))&&P->GetMesh()->GetMorphTarget(TEXT("SlimBuild"))>.7f,TEXT("Travis slim body uses a cooked customizable morph"));
    Check(CheckGrip(),TEXT("Travis hunter maintains both rifle grips"));
    auto EditedTravis=P->HunterProfile;EditedTravis.Outfit=2;EditedTravis.Slimness=.2f;EditedTravis.bCap=true;EditedTravis.bBeard=false;P->ServerSetHunterProfile(EditedTravis,FString());
    Check(P->HunterProfile.Preset==1&&P->HunterProfile.Outfit==2&&FMath::IsNearlyEqual(P->GetMesh()->GetMorphTarget(TEXT("SlimBuild")),.2f)&&!P->HunterFaceTexture,TEXT("customizing Travis retains its built-in photo head"));
    P->ServerSetHunterProfile(FWildlineHunterProfile::Ethan(),FString());
    Check(P->GetMesh()->GetSkeletalMeshAsset()->GetName()==TEXT("SK_HunterEthan")&&!P->HunterFaceTexture&&!P->HunterProfile.bBeard,TEXT("Ethan selects its own featureless photo head without an upload"));
    bool PhotoMaterial=false;for(auto* M:P->HunterMaterials)if(M&&M->GetBaseMaterial()->GetName()==TEXT("M_EthanPhoto"))PhotoMaterial=true;
    Check(PhotoMaterial&&P->GetMesh()->GetSkeletalMeshAsset()->FindMorphTarget(TEXT("SlimBuild"))&&CheckGrip(),TEXT("Ethan retains its cooked photo material, build morph and rifle grips"));
    P->ServerSetHunterProfile(FWildlineHunterProfile::Greyson(),FString());
    Check(P->GetMesh()->GetSkeletalMeshAsset()->GetName()==TEXT("SK_HunterGreyson")&&!P->HunterFaceTexture&&!P->HunterProfile.bBeard,TEXT("Greyson selects its own featureless photo head without an upload"));
    bool GreysonPhoto=false;for(auto* M:P->HunterMaterials)if(M&&M->GetBaseMaterial()->GetName()==TEXT("M_GreysonPhoto"))GreysonPhoto=true;
    Check(GreysonPhoto&&P->GetMesh()->GetSkeletalMeshAsset()->FindMorphTarget(TEXT("SlimBuild"))&&CheckGrip(),TEXT("Greyson retains its cooked photo material, build morph and rifle grips"));
    auto EditedGreyson=P->HunterProfile;EditedGreyson.Outfit=0;EditedGreyson.Slimness=.3f;EditedGreyson.bCap=true;P->ServerSetHunterProfile(EditedGreyson,FString());
    Check(P->HunterProfile.Preset==4&&P->HunterProfile.Outfit==0&&FMath::IsNearlyEqual(P->GetMesh()->GetMorphTarget(TEXT("SlimBuild")),.3f),TEXT("customizing Greyson retains its built-in photo head"));
    P->ServerSetHunterProfile(FWildlineHunterProfile::Mark(),FString());
    Check(P->GetMesh()->GetSkeletalMeshAsset()->GetName()==TEXT("SK_HunterMark")&&!P->HunterFaceTexture&&!P->HunterProfile.bBeard,TEXT("Mark selects its cooked face without an upload"));
    bool MarkPhoto=false;for(auto* M:P->HunterMaterials)if(M&&M->GetBaseMaterial()->GetName()==TEXT("M_MarkPhoto"))MarkPhoto=true;
    Check(MarkPhoto&&P->GetMesh()->GetSkeletalMeshAsset()->FindMorphTarget(TEXT("SlimBuild"))&&CheckGrip(),TEXT("Mark retains its cooked portrait, build morph and rifle grips"));
    auto EditedMark=P->HunterProfile;EditedMark.Outfit=2;EditedMark.bCap=true;EditedMark.Slimness=.6f;P->ServerSetHunterProfile(EditedMark,FString());
    Check(P->HunterProfile.Preset==5&&P->HunterProfile.Outfit==2&&P->HunterProfile.bCap&&FMath::IsNearlyEqual(P->GetMesh()->GetMorphTarget(TEXT("SlimBuild")),.6f),TEXT("customizing Mark retains its built-in face"));
    auto LegacyTravis=FWildlineHunterProfile::Travis();LegacyTravis.Preset=2;LegacyTravis.Outfit=2;P->ServerSetHunterProfile(LegacyTravis,FString());
    Check(P->HunterProfile.Preset==1&&P->HunterProfile.Outfit==2&&P->GetMesh()->GetSkeletalMeshAsset()->GetName()==TEXT("SK_HunterTravis"),TEXT("saved Travis 2 migrates to Travis while retaining outfit"));
    bool TravisPhoto=false;for(auto* M:P->HunterMaterials)if(M&&M->GetBaseMaterial()->GetName()==TEXT("M_TravisPhoto"))TravisPhoto=true;
    Check(TravisPhoto,TEXT("Travis retains its cooked photo material after preset switching"));
    P->ServerSetHunterProfile(BeforeTravis,FString());
    Check(!P->bThirdPerson,TEXT("first-person default"));
    P->ToggleCamera();Check(P->bThirdPerson,TEXT("third-person toggle"));P->ToggleCamera();
    const float Now=P->ServerTime();P->ServerThermal();Check(FMath::IsNearlyEqual(P->ThermalUntil-Now,20,.1f),TEXT("20-second thermal"));Check(FMath::IsNearlyEqual(P->ThermalReadyAt-P->ThermalUntil,120,.1f),TEXT("120-second cooldown after thermal expires"));const float Ready=P->ThermalReadyAt;P->ServerThermal();Check(P->ThermalReadyAt==Ready,TEXT("early thermal reuse rejected"));P->ThermalUntil=0;
    P->Ammo=0;P->ServerReload();Check(P->ReloadUntil>P->ServerTime(),TEXT("server reload starts"));P->ReloadUntil=P->ServerTime()-.1f;P->Tick(.01f);Check(P->Ammo==5,TEXT("reload restores magazine"));
    // An actual rifle round is swept through the physics scene toward a stationary squirrel.
    FVector Origin(0,0,WildlineRules::Ground(0,0)+110);P->SetActorLocation(Origin,false,nullptr,ETeleportType::TeleportPhysics);
    const FVector Target(650,0,WildlineRules::Ground(650,0)+13);
    auto* Animal=GetWorld()->SpawnActorDeferred<AWildlineCharacter>(AWildlineCharacter::StaticClass(),FTransform(Target),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    Animal->bSquirrel=true;UGameplayStatics::FinishSpawningActor(Animal,FTransform(Target));Animal->GetCharacterMovement()->DisableMovement();
    const FVector Dir=(Target-(Origin+FVector(0,0,72))).GetSafeNormal();P->ServerFire(Dir);Check(P->Ammo==4,TEXT("rifle consumes a live round"));
    const TWeakObjectPtr<AWildlineCharacter> Weak=Animal;FTimerHandle Verify;
    GetWorldTimerManager().SetTimer(Verify,FTimerDelegate::CreateWeakLambda(this,[this,Weak,Ok](){
        bool Success=Ok;auto* A=Weak.Get();auto* F=AWildlineWorld::Find(GetWorld());
        auto C=[&](bool Passed,const TCHAR* Message){Success=Success&&Passed;UE_LOG(LogTemp,Display,TEXT("WILDLINE SMOKE %s: %s"),Passed?TEXT("PASS"):TEXT("FAIL"),Message);};
        C(A&&A->bDead,TEXT("ballistic hit causes lethal death"));
        if(auto* HunterPawn=Cast<AWildlineCharacter>(GetPawn()))C(HunterPawn->ViewBolt->GetRelativeLocation().Size()>1,TEXT("firing cycles the visible bolt before the next round"));
        if(A&&F&&F->Trees.Num()){
            A->bDead=false;A->OnRepDead();const auto& T=F->Trees[0];float Radius;const FVector Centre=F->TrunkCentre(0,T.Base.Z+18,Radius);A->SetActorLocation(Centre+FVector(Radius+20,0,0),false,nullptr,ETeleportType::TeleportPhysics);A->ServerClimb();C(A->TreeIndex==0,TEXT("squirrel grabs a real trunk"));A->ServerClimbInput(1,0);const float Z=A->GetActorLocation().Z;A->Tick(.5f);C(A->GetActorLocation().Z>Z+80,TEXT("squirrel climbs vertically"));A->ServerJump();C(A->TreeIndex==INDEX_NONE,TEXT("launch releases the tree"));C(A->GetCharacterMovement()->PendingLaunchVelocity.Size2D()>850,TEXT("long launch has canopy travel velocity"));
        }
        ChooseRole(true);const auto* Squirrel=Cast<AWildlineCharacter>(GetPawn());
        C(Squirrel&&Squirrel->bSquirrel&&!Squirrel->bThirdPerson,TEXT("squirrel role starts in first person"));
        if(Squirrel){
            C(Squirrel->GetMesh()->GetSkeletalMeshAsset()&&Squirrel->GetMesh()->GetBoneIndex(TEXT("SKEL_Tail3"))!=INDEX_NONE,TEXT("authored skeletal anatomy and segmented tail loaded"));
            C(Squirrel->GetMesh()->GetAnimInstance()&&Squirrel->GetMesh()->GetAnimInstance()->IsA(UWildlineSquirrelAnim::StaticClass()),TEXT("native articulated squirrel animation active"));
            C(Squirrel->FurSections.Num()>10,TEXT("bone-attached silhouette fur generated"));
            C(FMath::IsNearlyEqual(Squirrel->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),12.f,.01f),TEXT("real animal collision and eye height"));
            const float Length=Squirrel->GetMesh()->GetSkeletalMeshAsset()->GetImportedBounds().BoxExtent.Y*2*Squirrel->GetMesh()->GetRelativeScale3D().X;
            C(Length>45&&Length<52,TEXT("squirrel nose-to-tail length is 45 to 52 centimetres"));
            // The gait probe needs a grounded pose; a freshly spawned capsule may still be falling.
            Squirrel->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
            auto* Mesh=Squirrel->GetMesh();Mesh->TickAnimation(.05f,false);Mesh->RefreshBoneTransforms();
            const FQuat Before=Mesh->GetBoneTransform(TEXT("SKEL_L_UpperArm"),RTS_Component).GetRotation();
            auto* Moving=const_cast<AWildlineCharacter*>(Squirrel);Moving->SetActorLocation(Moving->GetActorLocation()+FVector(12,0,0));
            Mesh->TickAnimation(.05f,false);Mesh->RefreshBoneTransforms();
            C(Before.AngularDistance(Mesh->GetBoneTransform(TEXT("SKEL_L_UpperArm"),RTS_Component).GetRotation())>.01f,TEXT("travel articulates the paw joints"));
            C(Mesh->GetBoneTransform(TEXT("SKEL_Pelvis"),RTS_Component).GetTranslation().Z<20,TEXT("bounding stays within anatomical height despite FBX unit scale"));
            Moving->bThirdPerson=true;float Radius;const FVector Centre=F->TrunkCentre(0,F->Trees[0].Base.Z+600,Radius);Moving->SetActorLocation(Centre+FVector(Radius+20,0,0));Moving->ServerClimb();
            bool Clear=true;
            for(int32 Angle=0;Angle<360;Angle+=45){SetControlRotation(FRotator(0,Angle,0));Moving->Tick(.016f);const FVector Camera=Moving->ThirdCamera->GetComponentLocation();
                FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(VisibleClimber),false,Moving);
                Clear&=FVector::Dist(Camera,Moving->GetActorLocation())>60&&!GetWorld()->LineTraceSingleByChannel(H,Camera,Moving->GetActorLocation()+FVector(0,0,8),ECC_Camera,Q);
            }
            C(Clear&&!Mesh->bOwnerNoSee,TEXT("third person keeps climber visible through a full look orbit"));
            bool SpeciesViews=true;
            for(int32 Species=0;Species<4;Species++){
                const int32 Index=F->Trees.IndexOfByPredicate([Species](const FWildlineTree& T){return T.Species==Species;});
                if(Index==INDEX_NONE){SpeciesViews=false;continue;}
                const auto& T=F->Trees[Index];
                for(float Height:{.15f,.45f,.8f}){
                    const float Z=FMath::Lerp(T.Base.Z+25,T.ClimbTop-25,Height);float R;
                    const FVector CentreAtHeight=F->TrunkCentre(Index,Z,R);
                    FHitResult Bark;int32 BarkTree;const bool BarkFound=F->TraceWood(CentreAtHeight+FVector(R+250,0,0),CentreAtHeight-FVector(R+30,0,0),Bark,BarkTree)&&BarkTree==Index;Moving->TreeIndex=INDEX_NONE;Moving->SetActorLocation(BarkFound?Bark.ImpactPoint+Bark.ImpactNormal*16:CentreAtHeight+FVector(R+20,0,0));Moving->ServerClimb();
                    SpeciesViews&=Moving->TreeIndex==Index;
                    for(int32 Angle=0;Angle<360;Angle+=45){SetControlRotation(FRotator(0,Angle,0));Moving->Tick(.016f);const FVector Camera=Moving->ThirdCamera->GetComponentLocation();FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(SpeciesClimbCamera),false,Moving);
                        const bool Visible=FVector::Dist(Camera,Moving->GetActorLocation())>60&&!GetWorld()->LineTraceSingleByChannel(H,Camera,Moving->GetActorLocation()+FVector(0,0,8),ECC_Camera,Q);if(!Visible||Moving->TreeIndex!=Index)UE_LOG(LogTemp,Display,TEXT("CLIMB CAMERA obstruction species=%d height=%.2f yaw=%d distance=%.1f component=%s grip=%d"),Species,Height,Angle,FVector::Dist(Camera,Moving->GetActorLocation()),*GetNameSafe(H.Component.Get()),Moving->TreeIndex);SpeciesViews&=Visible;
                    }
                }
            }
            C(SpeciesViews,TEXT("climb camera stays clear on all four species at low middle and upper heights"));
            Moving->ServerJump();Moving->Tick(.016f);C(Moving->ThirdCamera->GetAttachParent()==Moving->Boom,TEXT("trunk launch restores chase camera"));
            auto* ForageState=GetWorld()->GetGameState<AWildlineGameState>();ForageState->bRoundStarted=true;
            Moving->Bag=0;Moving->Collected.Empty();
            const FVector Pickup=F->AcornLocations[0];Moving->SetActorLocation(Pickup+FVector(0,0,13));Moving->Tick(.016f);
            C(Moving->Bag==1&&Moving->Collected.Contains(0),TEXT("server collects an accessible acorn on approach"));
            FTransform HiddenAcorn,HiddenCap;F->Acorns->GetInstanceTransform(0,HiddenAcorn);F->AcornCaps->GetInstanceTransform(0,HiddenCap);
            C(HiddenAcorn.GetScale3D().IsNearlyZero()&&HiddenCap.GetScale3D().IsNearlyZero(),TEXT("collected acorn body and cap disappear together locally"));
            Moving->Tick(.016f);C(Moving->Bag==1,TEXT("the same acorn cannot be collected twice"));
            Moving->SetActorLocation(F->AcornLocations[1]+FVector(0,0,300));Moving->Tick(.016f);C(Moving->Bag==1,TEXT("airborne squirrel cannot pick forage far below"));
            const int32 BeforeDeposit=ForageState->StoredAcorns;Moving->SetActorLocation(F->DenLocation+FVector(0,180,13));Moving->Tick(.016f);
            C(Moving->Bag==0&&ForageState->StoredAcorns==BeforeDeposit+1,TEXT("ground-level den visit deposits the carried acorn"));ForageState->bRoundStarted=false;
            Moving->TreeIndex=INDEX_NONE;Moving->SetActorLocation(FVector(WildlineRules::MapRadius+200,0,900));Moving->GetCharacterMovement()->Velocity=FVector(300,0,0);Moving->Tick(.016f);
            C(Moving->GetActorLocation().Size2D()<=WildlineRules::MapRadius+.1f&&Moving->GetVelocity().X<=0,TEXT("reserve edge removes outward velocity and contains player"));
        }
        C(Menu&&Menu->bSquirrelSelected,TEXT("camp retains the active squirrel role"));
        ChooseRole(false);auto* Defender=Cast<AWildlineCharacter>(GetPawn());Defender->SelectWeapon(1);
        C(Defender->SelectedWeapon==1&&Defender->Ammo==2&&Defender->ViewRifle->GetStaticMesh()&&Defender->ViewBolt->GetStaticMesh(),TEXT("Classic supports a selectable two-shell shotgun with reload action mesh"));
        Defender->SelectWeapon(0);C(Defender->SelectedWeapon==0&&Defender->Ammo==5,TEXT("Switching back restores the Classic rifle magazine"));
        FString Result=Success?TEXT("PASS\n"):TEXT("FAIL\n");FFileHelper::SaveStringToFile(Result,*(FPaths::ProjectSavedDir()/TEXT("SmokeResult.txt")));UE_LOG(LogTemp,Display,TEXT("WILDLINE SMOKE RESULT: %s"),*Result);FPlatformMisc::RequestExitWithStatus(false,Success?0:1);
    }),.5f,false);
}



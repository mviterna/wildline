#include "WildlineGame.h"
#include "WildlineLobby.h"
#include "WildlineCharacter.h"
#include "WildlineMenu.h"
#include "WildlineUserSettings.h"
#include "WildlineRules.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/HUD.h"
#include "TimerManager.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWildlineLobbyGeometry,"Wildline.Lobby.PortalAndRack",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWildlineLobbyGeometry::RunTest(const FString&){
    for(bool S:{false,true}){
        TestFalse(TEXT("Spawn is outside ready portal"),AWildlineLobby::OnPortal(S,AWildlineLobby::Spawn(S)));
        TestTrue(TEXT("Standing on portal is ready"),AWildlineLobby::OnPortal(S,AWildlineLobby::Portal(S)+FVector(0,0,S?17:110)));
        TestFalse(TEXT("Jumping over portal is not ready"),AWildlineLobby::OnPortal(S,AWildlineLobby::Portal(S)+FVector(0,0,300)));
        TestFalse(TEXT("Standing under portal is not ready"),AWildlineLobby::OnPortal(S,AWildlineLobby::Portal(S)-FVector(0,0,100)));
    }
    TestEqual(TEXT("Rifle rack selects rifle"),AWildlineLobby::WeaponAt(AWildlineLobby::Rack(0)+FVector(0,100,110)),0);
    TestEqual(TEXT("Shotgun rack selects shotgun"),AWildlineLobby::WeaponAt(AWildlineLobby::Rack(1)+FVector(0,100,110)),1);
    TestEqual(TEXT("Equipment cannot change across rooms"),AWildlineLobby::WeaponAt(AWildlineLobby::Spawn(true)),INDEX_NONE);
    TestEqual(TEXT("Equipment cannot change through floor"),AWildlineLobby::WeaponAt(AWildlineLobby::Rack(0)-FVector(0,0,1000)),INDEX_NONE);
    return true;
}
#endif

void AWildlineController::RunLobbySmokeTest(){
    bool Passed=true;auto Check=[&](bool Ok,const TCHAR* Label){Passed&=Ok;UE_LOG(LogTemp,Display,TEXT("WILDLINE LOBBY %s: %s"),Ok?TEXT("PASS"):TEXT("FAIL"),Label);};
    auto* GM=GetWorld()->GetAuthGameMode<AWildlineGameMode>();auto* S=GetWorld()->GetGameState<AWildlineGameState>();
    if(!GM||!S){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    EnterWaitingRoom(false);auto* Hunter=Cast<AWildlineCharacter>(GetPawn());
    Check(Hunter&&Hunter->bInLobby&&!S->bRoundStarted,TEXT("Menu entry goes to lodge without starting timer"));
    if(!Hunter){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    auto* Second=GetWorld()->SpawnActor<APlayerController>();GM->EnterWaitingRoom(Second,true,0);auto* Animal=Cast<AWildlineCharacter>(Second->GetPawn());
    if(!Animal){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    auto BotCount=[this](){int32 N=0;for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(It->bBot)N++;return N;};
    Check(BotCount()==0,TEXT("Bots wait until humans are ready"));
    bool HasLodge=false,HasTree=false;
    if(auto* Room=AWildlineLobby::Find(GetWorld()))for(auto* Prop:Room->Props)if(Prop&&Prop->GetStaticMesh()){HasLodge|=Prop->GetStaticMesh()->GetName()==TEXT("SM_LodgeArchitecture");HasTree|=Prop->GetStaticMesh()->GetName()==TEXT("SM_HollowArchitecture");}
    Check(HasLodge&&HasTree,TEXT("Both textured walkable interiors exist"));
    if(auto* Room=AWildlineLobby::Find(GetWorld())){
        TArray<UPostProcessComponent*> Grades;Room->GetComponents(Grades);bool HunterLit=false,AnimalLit=false,Outside=false;
        for(auto* Grade:Grades){HunterLit|=Grade->EncompassesPoint(Hunter->GetActorLocation(),0,nullptr);AnimalLit|=Grade->EncompassesPoint(Animal->GetActorLocation(),0,nullptr);Outside|=Grade->EncompassesPoint(GM->SpawnLocation(false),0,nullptr);}
        Check(HunterLit&&AnimalLit&&!Outside,TEXT("Interior exposure volumes cover both rooms and exclude gameplay"));
    }
    const float Health=Animal->Health;FDamageEvent Damage;Animal->TakeDamage(100,Damage,this,Hunter);Check(Animal->Health==Health&&!Animal->bDead,TEXT("Waiting squirrels cannot be killed"));
    Hunter->SelectWeapon(0);const int32 Ammo=Hunter->Ammo;Hunter->ServerFire(FVector::UpVector);Hunter->ServerThermal();Check(Hunter->Ammo==Ammo&&Hunter->ThermalUntil==0,TEXT("Lobby cannot fire or activate thermal"));
    Hunter->SetActorLocation(AWildlineLobby::Rack(1)+FVector(0,100,110));ServerLobbyInteract();
    Check(Hunter->SelectedWeapon==1&&Hunter->Ammo==2,TEXT("Physical shotgun rack equips weapon"));
    Hunter->SetActorLocation(AWildlineLobby::Spawn(false));ServerSelectWeapon(0);ServerLobbyInteract();Check(Hunter->SelectedWeapon==1,TEXT("Remote and out-of-range weapon changes rejected"));
    Hunter->SetActorLocation(AWildlineLobby::Portal(false)+FVector(0,0,110));GM->UpdateWaitingRooms();Check(Hunter->bLobbyReady&&!S->bRoundStarted,TEXT("One ready human waits for other team"));
    Hunter->SetActorLocation(AWildlineLobby::Spawn(false));GM->UpdateWaitingRooms();Check(!Hunter->bLobbyReady,TEXT("Stepping off cancels ready"));
    for(const FVector Offset:{FVector(620,620,110),FVector(-620,620,110),FVector(620,-620,110),FVector(-620,-620,110),FVector(660,200,110)}){
        const FVector Position=AWildlineLobby::Centre(false)+Offset;Hunter->SetActorLocation(Position);SetControlRotation(FRotator(-12,143,0));Hunter->bThirdPerson=true;
        GM->UpdateWaitingRooms();Check(Hunter->GetActorLocation().Equals(Position,.1)&&GetControlRotation().Equals(FRotator(-12,143,0),.1)&&Hunter->bThirdPerson,TEXT("Lodge wall and corner preserve player position and camera"));
    }
    Hunter->SetActorLocation(AWildlineLobby::Centre(false)+FVector(800,0,110));GM->UpdateWaitingRooms();Check(Hunter->GetActorLocation().Equals(AWildlineLobby::Spawn(false)),TEXT("Escaped lodge player is still recovered"));
    Animal->SetActorLocation(AWildlineLobby::Centre(true)+FVector(0,0,-100));GM->UpdateWaitingRooms();Check(Animal->GetActorLocation().Z>AWildlineLobby::Centre(true).Z,TEXT("Escaped player is recovered into room"));
    Hunter->SetActorLocation(AWildlineLobby::Portal(false)+FVector(0,0,110));Animal->SetActorLocation(AWildlineLobby::Portal(true)+FVector(0,0,17));GM->UpdateWaitingRooms();
    Check(S->bRoundStarted&&!Hunter->bInLobby&&!Animal->bInLobby&&Hunter->SelectedWeapon==1,TEXT("Both portals launch the match with chosen equipment"));
    Check(S->RoundRemaining()>S->MatchConfig.DurationSeconds-1&&Animal->ProtectedUntil>=Animal->ServerTime(),TEXT("Timer and spawn protection begin at portal launch"));
    const int32 Expected=S->MatchConfig.DefenderBots+S->MatchConfig.SquirrelBots;Check(BotCount()==Expected,TEXT("Configured bot roster starts once"));
    GM->EnterWaitingRoom(Second,false,0);auto* Late=Cast<AWildlineCharacter>(Second->GetPawn());Check(Late&&Late->bInLobby&&S->bRoundStarted&&BotCount()==Expected,TEXT("Role change enters lodge without resetting active match or bots"));
    if(Late){Late->SetActorLocation(AWildlineLobby::Portal(false)+FVector(0,0,110));GM->UpdateWaitingRooms();Check(!Late->bInLobby,TEXT("Late join enters active round through portal"));}
    S->Winner=1;GM->RestartRound();Check(!S->bRoundStarted&&Hunter->bInLobby&&Late&&Late->bInLobby&&BotCount()==0,TEXT("Next round returns humans to waiting rooms and pauses bots"));
    if(Second->GetPawn())Second->GetPawn()->Destroy();Second->Destroy();
    UE_LOG(LogTemp,Display,TEXT("WILDLINE LOBBY SMOKE: %s mode=%d"),Passed?TEXT("PASS"):TEXT("FAIL"),S->ExpeditionMode);FPlatformMisc::RequestExitWithStatus(false,Passed?0:1);
}

void AWildlineController::RunLobbyReview(){
    auto At=[this](float T,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),T,false);};
    auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,true,false);};
    auto View=[this](bool Animal,FVector Position,FVector Target){auto* Cam=GetWorld()->SpawnActor<ACameraActor>(AWildlineLobby::Centre(Animal)+Position,FRotator::ZeroRotator);Cam->SetActorRotation((AWildlineLobby::Centre(Animal)+Target-Cam->GetActorLocation()).Rotation());Cam->GetCameraComponent()->FieldOfView=83;SetViewTarget(Cam);};
    At(2,[this](){EnterWaitingRoom(false);});At(6,[Shot](){Shot(TEXT("LobbyHunterPlayer.png"));});
    At(7,[this,View](){GetHUD()->bShowHUD=false;View(false,FVector(500,-480,260),FVector(-190,250,220));});At(11,[Shot](){Shot(TEXT("LobbyHunterRoom.png"));});
    At(12,[this,View](){View(false,FVector(-130,-115,300),FVector(-680,-115,298));});At(16,[Shot](){Shot(TEXT("LobbyTrophies.png"));});
    At(17,[this](){EnterWaitingRoom(true);SetViewTarget(GetPawn());GetHUD()->bShowHUD=true;SetControlRotation(FRotator(5,0,0));});At(21,[Shot](){Shot(TEXT("LobbySquirrelPlayer.png"));});
    At(22,[this,View](){GetHUD()->bShowHUD=false;View(true,FVector(-315,-285,160),FVector(70,70,160));});At(26,[Shot](){Shot(TEXT("LobbySquirrelRoom.png"));});
    At(27,[this,View](){EnterWaitingRoom(false);GetHUD()->bShowHUD=false;View(false,FVector(-60,-230,210),FVector(-60,-600,140));});At(31,[Shot](){Shot(TEXT("LobbyWeaponRack.png"));});
    At(32,[this,View](){EnterWaitingRoom(true);GetHUD()->bShowHUD=false;View(true,FVector(-50,-80,90),FVector(-220,-275,25));});At(36,[Shot](){Shot(TEXT("LobbyAcornPantry.png"));});
    At(37,[this,View](){EnterWaitingRoom(false);GetHUD()->bShowHUD=false;View(false,FVector(-410,-410,235),FVector(460,690,260));});At(41,[Shot](){Shot(TEXT("LobbyHunterWindows.png"));});
    At(42,[](){FPlatformMisc::RequestExit(false);});
}

void AWildlineController::RunLobbyNetworkProbe(){
    auto At=[this](float T,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),T,false);};
    auto Passed=MakeShared<bool>(true);
    auto Check=[Passed](bool Ok,const TCHAR* Label){*Passed&=Ok;UE_LOG(LogTemp,Display,TEXT("WILDLINE LOBBY NETWORK %s: %s"),Ok?TEXT("PASS"):TEXT("FAIL"),Label);};
    auto Finish=[this,Check,Passed](){auto* P=Cast<AWildlineCharacter>(GetPawn());const auto* S=GetWorld()->GetGameState<AWildlineGameState>();Check(S&&S->bRoundStarted&&P&&!P->bInLobby,TEXT("Portal starts and replicates match entry"));UE_LOG(LogTemp,Display,TEXT("WILDLINE LOBBY NETWORK %s: %s"),HasAuthority()?TEXT("Host"):TEXT("Client"),*Passed?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Passed?0:1);};
    At(2,[this](){EnterWaitingRoom(!HasAuthority());});
    At(6,[this,Check](){auto* P=Cast<AWildlineCharacter>(GetPawn());Check(P&&P->bInLobby&&AWildlineLobby::Find(GetWorld()),TEXT("Local room and replicated lobby state exist"));});
    if(HasAuthority()){
        At(12,[this](){auto* P=Cast<AWildlineCharacter>(GetPawn());if(P){P->SetActorLocation(AWildlineLobby::Rack(1)+FVector(0,100,110));ServerLobbyInteract();}});
        At(14,[this,Check](){
            auto Poll=MakeShared<FTimerHandle>();
            GetWorldTimerManager().SetTimer(*Poll,FTimerDelegate::CreateWeakLambda(this,[this,Check,Poll](){
                bool Other=false;for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(It->bSquirrel&&It->bInLobby&&!It->bBot)Other=true;
                if(!Other)return;FTimerHandle Handle=*Poll;
                auto* P=Cast<AWildlineCharacter>(GetPawn());auto* GM=GetWorld()->GetAuthGameMode<AWildlineGameMode>();auto* S=GetWorld()->GetGameState<AWildlineGameState>();
                if(P&&GM){P->SetActorLocation(AWildlineLobby::Portal(false)+FVector(0,0,110));GM->UpdateWaitingRooms();}
                Check(P&&P->bLobbyReady&&S&&!S->bRoundStarted,TEXT("Host remains ready until client portal entry"));
                // Clearing this timer destroys the delegate's captures: do it last.
                GetWorldTimerManager().ClearTimer(Handle);
            }),.1f,true);
        });
        // First-load asset preparation can take longer than client connection.
        // Keep the server alive through the client's replicated entry assertion.
        At(14,[this,At,Finish](){
            auto Poll=MakeShared<FTimerHandle>();
            GetWorldTimerManager().SetTimer(*Poll,FTimerDelegate::CreateWeakLambda(this,[this,At,Finish,Poll](){
                const auto* S=GetWorld()->GetGameState<AWildlineGameState>();auto* P=Cast<AWildlineCharacter>(GetPawn());
                if(!S||!S->bRoundStarted||!P||P->bInLobby)return;
                FTimerHandle Handle=*Poll;At(12,Finish);
                GetWorldTimerManager().ClearTimer(Handle);
            }),.2f,true);
        });
        At(140,Finish);
    }else{
        At(8,[this,Check](){bool Shotgun=false,Ready=false;for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(!It->bBot&&!It->bSquirrel&&It->bInLobby){Shotgun|=It->SelectedWeapon==1;Ready|=It->bLobbyReady;}Check(Shotgun&&Ready,TEXT("Host rack selection and portal readiness replicate"));});
        // Exercise real replicated movement instead of a client-only teleport.
        At(10,[this](){SetControlRotation(FRotator::ZeroRotator);LobbyProbeWalkAt=GetWorld()->GetTimeSeconds();});
        At(18,Finish);
    }
}

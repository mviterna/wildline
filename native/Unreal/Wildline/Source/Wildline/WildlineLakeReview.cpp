#include "WildlineGame.h"
#include "WildlineCharacter.h"
#include "WildlineLake.h"
#include "WildlineWorld.h"
#include "WildlineKnobs.h"
#include "WildlineLobby.h"
#include "WildlineMenu.h"
#include "WildlineUserSettings.h"
#include "WildlineRules.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HUD.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ComboBoxString.h"
#include "Components/WidgetSwitcher.h"
#include "Components/ScrollBox.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

namespace {
void At(AWildlineController* PC,float Time,TFunction<void()> Work){FTimerHandle H;PC->GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(PC,[Work](){Work();}),Time,false);}
void View(AWildlineController* PC,FVector Eye,FVector Target,float FOV=70){auto* C=PC->GetWorld()->SpawnActor<ACameraActor>(Eye,(Target-Eye).Rotation());C->GetCameraComponent()->SetFieldOfView(FOV);PC->SetViewTarget(C);}
void Shot(const FString& Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/Lake/")+Name+TEXT(".png"),true,false);}
}
void WildlineRunMapPreview(AWildlineController* PC){
    At(PC,2, [PC](){PC->SetMenu(false);PC->GetHUD()->bShowHUD=false;const auto* F=AWildlineWorld::Find(PC->GetWorld());if(F->bKnobs){const float Y=WildlineKnobs::CreekY(-1800);View(PC,FVector(-2650,Y-500,F->GroundHeight(-2650,Y-500)+250),FVector(-700,Y+350,230),72);}else if(F->bLake)View(PC,FVector(2000,-10500,14000),AWildlineLake::HuntingCentre(),82);else if(F->bResidential)View(PC,FVector(-4600,-6100,3600),FVector(0,-200,450),65);else if(F->bFarm)View(PC,FVector(-3000,-5200,3000),FVector(1500,500,400),65);else View(PC,FVector(-1800,-2300,1350),FVector(600,900,500),75);});
    At(PC,7,[PC](){FString Name=PC->GetWorld()->GetMapName();Name.RemoveFromStart(TEXT("UEDPIE_0_"));FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/TEXT("ArtSource/Lake/Previews/")+Name+TEXT(".png"),false,false);});
    At(PC,9,[](){FPlatformMisc::RequestExit(false);});
}
void WildlineRunLakeSmoke(AWildlineController* PC){
    At(PC,2,[PC](){
        bool Pass=true;auto Check=[&](bool OK,const TCHAR* Name){Pass&=OK;UE_LOG(LogTemp,Display,TEXT("LAKE CHECK %s: %s"),OK?TEXT("PASS"):TEXT("FAIL"),Name);};
        auto* W=PC->GetWorld();auto* F=AWildlineWorld::Find(W);auto* L=AWildlineLake::Find(W);auto* GM=W->GetAuthGameMode<AWildlineGameMode>();auto* S=W->GetGameState<AWildlineGameState>();
        Check(F&&F->bLake&&L&&GM&&S&&S->ExpeditionMode==3,TEXT("Lake loads as a cooked selectable expedition"));if(!F||!L||!GM||!S){FPlatformMisc::RequestExitWithStatus(false,1);return;}
        Check(PC->Menu&&PC->Menu->MapCards.Num()==7&&PC->Menu->Mode->GetOptionCount()==4,TEXT("Map tab contains all seven map cards"));
        PC->Menu->SelectMap(5);PC->Menu->ShowMaps();Check(PC->GetGameInstance<UWildlineGameInstance>()->PreferredMode==3&&PC->Menu->Pages->GetActiveWidgetIndex()==3,TEXT("Lake card selection and Map button work"));
        Check(F->AcornLocations.Num()==30&&F->Trees.Num()>75&&L->Fish.Num()==AWildlineLake::FishCount()+AWildlineLake::FishCount(true),TEXT("Playable forage, woodland and textured bass populations exist"));
        Check(AWildlineLake::Extent().X>=10000&&AWildlineLake::Extent().Y>=7500&&AWildlineLake::RadiusAt(F->DenLocation)>1.1f&&AWildlineLake::RadiusAt(GM->SpawnLocation(false))>1.1f&&AWildlineLake::RadiusAt(GM->SpawnLocation(true))>1.1f,TEXT("Expanded lake retains dry den and both team spawns"));
        bool Dry=true;for(const auto& A:F->AcornLocations)Dry&=AWildlineLake::RadiusAt(A)>1.05f;Check(Dry,TEXT("Every acorn is on dry ground outside the lake"));
        bool Trail=true;for(int32 I=0;I<128;I++){const auto P=AWildlineLake::TrailPoint(I*2*PI/128);Trail&=AWildlineLake::RadiusAt(P)>1.1f&&P.Z>100&&P.Size2D()<WildlineRules::MapRadius-100;}Check(Trail,TEXT("Continuous dry trail surrounds the lake within playable bounds"));
        int32 Boats=0,Wood=0;bool Assets=true;for(auto* C:L->Props){Assets&=C&&C->GetStaticMesh();if(C&&C->GetStaticMesh()){Boats+=C->GetStaticMesh()->GetName()==TEXT("SM_FishingBoat");Wood+=C->GetName().Contains(TEXT("Submerged fallen limb"));}}Check(Assets&&Boats==4&&Wood>=20,TEXT("Both lakes have boats, rocks and submerged limbs"));
        PC->EnterWaitingRoom(false);auto* P=Cast<AWildlineCharacter>(PC->GetPawn());if(!P){FPlatformMisc::RequestExitWithStatus(false,1);return;}
        P->SetActorLocation(AWildlineLake::FishingSpot(0));GM->UpdateWaitingRooms();Check(P->bInLobby&&FVector::Dist2D(P->GetActorLocation(),AWildlineLake::FishingSpot(0))<1&&!S->bRoundStarted,TEXT("Waiting players can use the dock without forced recovery or timer start"));
        auto Advance=[P](){P->FishingUntil=P->ServerTime()-.1f;P->UpdateLake(.016f);};
        P->ServerFishInteract();Check(P->FishingStage==4&&P->FishingHelpAlpha()>.99f,TEXT("E picks up the rod and shows brief instructions"));Advance();Check(P->FishingStage==5,TEXT("Pickup finishes before casting"));
        auto* Other=W->SpawnActor<AWildlineCharacter>();Other->bInLobby=true;Other->SetActorLocation(AWildlineLake::FishingSpot(0));Other->ServerFishInteract();const bool Reserved=Other->FishingStage==0;Other->SetActorLocation(AWildlineLake::FishingSpot(2));Other->ServerFishInteract();Check(Reserved&&Other->FishingStage==4,TEXT("Fishing stations reserve one rod each and allow simultaneous players"));Other->CancelFishing();Other->Destroy();
        P->FishingEquippedAt=P->ServerTime()-6;Check(P->FishingHelpAlpha()==0,TEXT("Instructions fully fade shortly after pickup"));
        P->ServerReel(true);P->FishingChargeAt=P->ServerTime()-1;P->ServerReel(false);Check(P->FishingStage==6&&P->FishingPower>.8f,TEXT("Held cast input sets strength and begins cast animation"));Advance();Check(P->FishingStage==1,TEXT("Cast flight completes before waiting for bite"));Advance();Check(P->FishingStage==2,TEXT("Bite opens the hook window"));P->ServerFishInteract();Check(P->FishingStage==3,TEXT("E hooks the fish"));
        P->ServerReel(true);for(int32 I=0;I<3;I++)P->UpdateLake(.25f);const float Raised=P->FishingTension;P->ServerReel(false);P->UpdateLake(.25f);Check(P->FishingTension<Raised&&P->FishingProgress>0,TEXT("Reeling gains fish; releasing lowers line tension"));
        for(int32 I=0;I<100&&P->FishingStage==3;I++){P->ServerReel(P->FishingTension<.62f);P->UpdateLake(.2f);}Check(P->FishCaught==1&&P->FishingStage==7,TEXT("Controlled fight lands a bass with a catch-and-release animation"));Advance();Check(P->FishingStage==5,TEXT("Landing returns to equipped state"));
        P->ServerFishInteract();Advance();Advance();P->ServerFishInteract();P->FishingTension=.99f;P->ServerReel(true);P->UpdateLake(.2f);Check(P->FishingStage==5&&P->FishCaught==1,TEXT("Over-tension breaks the line without awarding a catch"));
        P->ServerFishInteract();Advance();Advance();Advance();Check(P->FishingStage==5,TEXT("Missed hook times out while retaining the rod"));
        P->ServerCancelFishing();Check(P->FishingStage==8,TEXT("Returning the rod animates before stowing"));Advance();Check(!P->FishingStage,TEXT("Rod return completes"));
        P->ServerFishInteract();P->SetActorLocation(AWildlineLobby::Spawn(false));P->UpdateLake(.016f);Check(!P->FishingStage,TEXT("Walking away cancels fishing"));
        P->bInLobby=false;P->SetActorLocation(AWildlineLake::HuntingCentre()+FVector(0,0,-220));P->GetCharacterMovement()->StopMovementImmediately();P->UpdateLake(.016f);Check(P->bSwimming&&P->IsHeadUnderwater()&&P->GetCharacterMovement()->MovementMode==MOVE_Flying,TEXT("Hunter enters 3D swimming and diving"));
        P->SelectWeapon(0);const int32 Ammo=P->Ammo;P->ServerFire(FVector::ForwardVector);P->ServerAim(true);P->StartFire();Check(P->Ammo==Ammo&&!P->bAiming&&!P->bFiring,TEXT("Underwater rifle fire and aiming are rejected by server and local controls"));
        P->SelectWeapon(1);const int32 Shells=P->Ammo;P->ServerFire(FVector::ForwardVector);Check(P->Ammo==Shells,TEXT("Underwater shotgun fire is rejected"));
        P->SetActorLocation(GM->SpawnLocation(false));P->UpdateLake(.016f);Check(!P->bSwimming,TEXT("Returning to shore restores land movement"));
        const FTransform T(FRotator::ZeroRotator,AWildlineLake::HuntingCentre()-FVector(0,0,10));auto* Animal=W->SpawnActorDeferred<AWildlineCharacter>(AWildlineCharacter::StaticClass(),T,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);Animal->bSquirrel=true;Animal->FinishSpawning(T);Animal->ProtectedUntil=Animal->ServerTime()+50;const int32 Kills=S->HunterKills;Animal->UpdateLake(.016f);Check(Animal->bDead&&Animal->Health==0&&Animal->RespawnAt>Animal->ServerTime()&&S->HunterKills==Kills,TEXT("Water kills squirrels even with spawn protection; no hunter kill credit"));
        auto Result=MakeShared<bool>(Pass);P->bInLobby=true;P->SetActorLocation(AWildlineLobby::Centre(false)+FVector(570,125,105));P->GetCharacterMovement()->StopMovementImmediately();PC->SetControlRotation(FRotator::ZeroRotator);PC->SetMenu(false);
        At(PC,3.5f,[PC,Result](){auto* P=Cast<AWildlineCharacter>(PC->GetPawn());const bool OK=P&&P->bInLobby&&P->GetActorLocation().X>AWildlineLobby::Centre(false).X+1000;*Result&=OK;UE_LOG(LogTemp,Display,TEXT("LAKE WALKABLE DOOR AND DOCK: %s position=%s"),OK?TEXT("PASS"):TEXT("FAIL"),*P->GetActorLocation().ToString());P->bInLobby=false;P->SetActorLocation(AWildlineLake::HuntingCentre()+FVector(0,0,-230));P->GetCharacterMovement()->StopMovementImmediately();});
        At(PC,6,[PC,Result](){auto* P=Cast<AWildlineCharacter>(PC->GetPawn());const bool OK=P&&P->bSwimming&&P->GetActorLocation().Z<AWildlineLake::HuntingCentre().Z-350;*Result&=OK;UE_LOG(LogTemp,Display,TEXT("LAKE CTRL DIVE MOVEMENT: %s z=%.1f"),OK?TEXT("PASS"):TEXT("FAIL"),P->GetActorLocation().Z);P->DoUnCrouch();});
        At(PC,10,[PC,Result,Animal](){auto* P=Cast<AWildlineCharacter>(PC->GetPawn());const bool OK=P&&P->bSwimming&&!P->IsHeadUnderwater()&&P->GetActorLocation().Z>AWildlineLake::HuntingCentre().Z-100;*Result&=OK;UE_LOG(LogTemp,Display,TEXT("LAKE SPACE SURFACE MOVEMENT: %s z=%.1f"),OK?TEXT("PASS"):TEXT("FAIL"),P->GetActorLocation().Z);const bool Respawned=IsValid(Animal)&&!Animal->bDead&&AWildlineLake::RadiusAt(Animal->GetActorLocation())>1;*Result&=Respawned;UE_LOG(LogTemp,Display,TEXT("LAKE SQUIRREL RESPAWN: %s"),Respawned?TEXT("PASS"):TEXT("FAIL"));UE_LOG(LogTemp,Display,TEXT("WILDLINE LAKE SMOKE: %s"),*Result?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Result?0:1);});
    });
}
void WildlineLakeProbeInput(AWildlineCharacter* P){
    if(!P->IsLocallyControlled()||!FParse::Param(FCommandLine::Get(),TEXT("WildlineLakeSmoke")))return;
    const float T=P->GetWorld()->GetTimeSeconds();
    if(T>3&&T<5.2f)P->AddMovementInput(FVector::ForwardVector,1);
    if(T>6&&T<7.7f)P->DoCrouch();
    if(T>8&&T<11.5f)P->DoJump();
    if(T>=11.5f)P->StopSwimRise();
}
void WildlineRunLakeNetwork(AWildlineController* PC){
    if(PC->HasAuthority()){
        At(PC,2,[PC](){auto* GM=PC->GetWorld()->GetAuthGameMode<AWildlineGameMode>();PC->GetWorld()->GetGameState<AWildlineGameState>()->MatchConfig.RespawnSeconds=30;GM->EnterWaitingRoom(PC,false,0);PC->SetMenu(false);auto* P=Cast<AWildlineCharacter>(PC->GetPawn());P->SetActorLocation(AWildlineLake::FishingSpot(0));P->ServerFishInteract();P->FishingUntil=P->ServerTime()-.1f;P->UpdateLake(.016f);P->ServerFishInteract();P->FishingUntil=P->ServerTime()-.1f;P->UpdateLake(.016f);P->FishingUntil=P->ServerTime()+.5f;
            const FTransform T(FRotator::ZeroRotator,AWildlineLake::HuntingCentre()+FVector(0,0,-220));auto* Hunter=PC->GetWorld()->SpawnActor<AWildlineCharacter>(AWildlineCharacter::StaticClass(),T);Hunter->UpdateLake(.016f);Hunter->ForceNetUpdate();
            auto* Animal=PC->GetWorld()->SpawnActorDeferred<AWildlineCharacter>(AWildlineCharacter::StaticClass(),T,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);Animal->bSquirrel=true;Animal->FinishSpawning(T);Animal->UpdateLake(.016f);
        });
        At(PC,3,[PC](){auto* P=Cast<AWildlineCharacter>(PC->GetPawn());P->ServerFishInteract();P->FishingProgress=.4f;P->FishingTension=.5f;P->ForceNetUpdate();});
        auto Moved=MakeShared<TSet<TWeakObjectPtr<AWildlineCharacter>>>(),Fished=MakeShared<TSet<TWeakObjectPtr<AWildlineCharacter>>>(),Dived=MakeShared<TSet<TWeakObjectPtr<AWildlineCharacter>>>();
        for(int32 I=5;I<=25;I++)At(PC,I,[PC,Moved,Fished,Dived](){for(auto It=PC->GetWorld()->GetPlayerControllerIterator();It;++It)if(!It->Get()->IsLocalController())if(auto* P=Cast<AWildlineCharacter>(It->Get()->GetPawn());P){
            if(!Moved->Contains(P)){P->bInLobby=true;P->SetActorLocation(AWildlineLake::FishingSpot(2));P->ForceNetUpdate();Moved->Add(P);}
            if(P->FishingStage)Fished->Add(P);
            if(Fished->Contains(P)&&!P->FishingStage&&!Dived->Contains(P)){P->bInLobby=false;P->SetActorLocation(AWildlineLake::HuntingCentre()+FVector(-200,0,-220));P->UpdateLake(.016f);P->ForceNetUpdate();Dived->Add(P);}
        }});
        At(PC,28,[PC,Moved,Fished,Dived](){bool Pass=Moved->Num()>0&&Fished->Num()>0&&Dived->Num()>0;for(auto Weak:*Moved)if(auto* P=Weak.Get())Pass&=P->Ammo==5&&P->bSwimming&&P->IsHeadUnderwater()&&Fished->Contains(P);UE_LOG(LogTemp,Display,TEXT("WILDLINE LAKE NET HOST: %s"),Pass?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,Pass?0:1);});
    }else{
        At(PC,2,[PC](){PC->ServerEnterWaitingRoom(false,0);PC->SetMenu(false);});
        auto ClientFished=MakeShared<bool>(false);
        At(PC,5,[PC](){if(auto* P=Cast<AWildlineCharacter>(PC->GetPawn()))P->ServerFishInteract();});
        At(PC,6.4f,[PC](){if(auto* P=Cast<AWildlineCharacter>(PC->GetPawn()))P->StartFire();});
        At(PC,6.8f,[PC](){if(auto* P=Cast<AWildlineCharacter>(PC->GetPawn()))P->StopFire();});
        At(PC,7,[PC,ClientFished](){if(auto* P=Cast<AWildlineCharacter>(PC->GetPawn())){*ClientFished=(P->FishingStage==6||P->FishingStage==1)&&P->FishingPower>.2f;UE_LOG(LogTemp,Display,TEXT("WILDLINE LAKE NET CLIENT FISHING: %s stage=%d power=%.2f"),*ClientFished?TEXT("PASS"):TEXT("FAIL"),P->FishingStage,P->FishingPower);PC->SetMenu(true);}});
        At(PC,8,[PC](){PC->SetMenu(false);});
        At(PC,10,[PC,ClientFished](){bool Swimming=false,Fishing=false,Dead=false;for(TActorIterator<AWildlineCharacter> It(PC->GetWorld());It;++It){if(*It==PC->GetPawn())continue;Swimming|=It->bSwimming;Fishing|=It->FishingStage==3&&It->FishingWeight>0;Dead|=It->bSquirrel&&It->bDead;}
            auto* P=Cast<AWildlineCharacter>(PC->GetPawn());const bool Pass=*ClientFished&&Swimming&&Fishing&&Dead&&P&&P->bSwimming&&P->IsHeadUnderwater();if(P)P->ServerFire(FVector::ForwardVector);UE_LOG(LogTemp,Display,TEXT("WILDLINE LAKE NET CLIENT STATE: %s swimming=%d fishing=%d localWater=%d"),Pass?TEXT("PASS"):TEXT("FAIL"),Swimming,Fishing,P?P->bSwimming:0);
            At(PC,2,[PC,Pass](){const auto* P=Cast<AWildlineCharacter>(PC->GetPawn());const bool OK=Pass&&P&&P->Ammo==5;UE_LOG(LogTemp,Display,TEXT("WILDLINE LAKE NET CLIENT: %s"),OK?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,OK?0:1);});
        });
    }
}
void WildlineRunLakeReview(AWildlineController* PC){
    At(PC,2,[PC](){PC->GetGameInstance<UWildlineGameInstance>()->PreferredMode=3;PC->Menu->RefreshMode();PC->Menu->ShowMaps();});At(PC,3,[PC](){PC->Menu->MapScroll->ScrollToEnd();});At(PC,5,[](){Shot(TEXT("MapTab"));});
    At(PC,6,[PC](){PC->EnterWaitingRoom(false);auto* P=Cast<AWildlineCharacter>(PC->GetPawn());P->SetActorLocation(AWildlineLobby::Centre(false)+FVector(410,125,105));PC->SetControlRotation(FRotator(-3,0,0));});At(PC,9,[](){Shot(TEXT("LodgeDoor"));});
    At(PC,10,[PC](){PC->GetHUD()->bShowHUD=false;View(PC,AWildlineLobby::Centre(false)+FVector(3000,-1800,1000),AWildlineLobby::Centre(false)+FVector(1500,140,0),74);});At(PC,14,[](){Shot(TEXT("LodgeDock"));});
    At(PC,15,[PC](){PC->GetHUD()->bShowHUD=true;auto* P=Cast<AWildlineCharacter>(PC->GetPawn());PC->SetViewTarget(P);P->SetActorLocation(AWildlineLake::FishingSpot(0));PC->SetControlRotation(FRotator(-22,0,0));P->ServerFishInteract();});At(PC,15.65f,[](){Shot(TEXT("RodPickup"));});
    At(PC,16.5f,[PC](){Cast<AWildlineCharacter>(PC->GetPawn())->ServerReel(true);});At(PC,17.5f,[PC](){Cast<AWildlineCharacter>(PC->GetPawn())->ServerReel(false);});At(PC,19,[](){Shot(TEXT("FishingCast"));});
    At(PC,18.05f,[](){Shot(TEXT("RodCasting"));});
    At(PC,20.5f,[PC](){auto* P=Cast<AWildlineCharacter>(PC->GetPawn());P->FishingUntil=P->ServerTime()+3;Shot(TEXT("FishingInstructionsFaded"));});
    At(PC,23,[PC](){auto* P=Cast<AWildlineCharacter>(PC->GetPawn());P->FishingStage=3;P->FishingStageAt=P->ServerTime();P->FishingProgress=.46f;P->FishingTension=.55f;P->FishingUntil=P->ServerTime()+35;P->ServerReel(true);});At(PC,25,[](){Shot(TEXT("FishingFight"));});
    for(int32 I=1;I<20;I++)At(PC,23+I*.15f,[PC](){auto* P=Cast<AWildlineCharacter>(PC->GetPawn());if(P->FishingStage==3)P->ServerReel(P->FishingTension<.60f);});
    At(PC,26,[PC](){auto* P=Cast<AWildlineCharacter>(PC->GetPawn());P->FishingStage=3;P->FishingTension=.4f;P->FishingProgress=.99f;P->ServerReel(true);P->UpdateLake(.2f);});At(PC,27.3f,[](){Shot(TEXT("FishingCatch"));});
    At(PC,30,[PC](){auto* P=Cast<AWildlineCharacter>(PC->GetPawn());P->CancelFishing();P->bInLobby=false;P->SetActorLocation(FVector(-11500,0,600));PC->GetHUD()->bShowHUD=false;View(PC,FVector(2000,-10500,14000),AWildlineLake::HuntingCentre(),82);});At(PC,33,[](){Shot(TEXT("LakeOverview"));});
    At(PC,34,[PC](){View(PC,FVector(-9500,-1800,650),FVector(1000,2500,100),75);});At(PC,38,[](){Shot(TEXT("ShoreTrail"));});
    At(PC,39,[PC](){auto* P=Cast<AWildlineCharacter>(PC->GetPawn());P->SetActorLocation(AWildlineLake::ShorePoint(PI,.82f)+FVector(0,0,-230));P->GetCharacterMovement()->StopMovementImmediately();PC->SetViewTarget(P);PC->SetControlRotation(FRotator(-12,25,0));PC->GetHUD()->bShowHUD=true;});At(PC,43,[](){Shot(TEXT("Diving"));});
    At(PC,44,[PC](){auto* P=Cast<AWildlineCharacter>(PC->GetPawn());P->bThirdPerson=true;PC->SetControlRotation(FRotator(-10,120,0));});At(PC,48,[](){Shot(TEXT("Swimming"));});
    At(PC,49,[PC](){PC->GetHUD()->bShowHUD=false;auto* Fish=AWildlineLake::Find(PC->GetWorld())->Fish[0];const FVector Q=Fish->GetComponentLocation();View(PC,Q+Fish->GetRightVector()*110+Fish->GetForwardVector()*20+FVector(0,0,15),Q,50);});At(PC,49.15f,[](){Shot(TEXT("BassUnderwater"));});At(PC,51,[](){FPlatformMisc::RequestExit(false);});
}

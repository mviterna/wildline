#include "WildlineGame.h"
#include "WildlineLobby.h"
#include "WildlineLake.h"
#include "WildlineCharacter.h"
#include "WildlineMenu.h"
#include "WildlineUserSettings.h"
#include "WildlineWorld.h"
#include "WildlineRules.h"
#include "WildlineRoom.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

void AWildlineController::EnterWaitingRoom(bool Animal){
    if(GetNetMode()==NM_Standalone)if(auto* Room=GetGameInstance()->GetSubsystem<UWildlineRoomSubsystem>();Room&&!Room->RoomAddress().IsEmpty()){Join(Room->RoomAddress(),Animal);return;}
    if(auto* GI=GetGameInstance<UWildlineGameInstance>())GI->bPreferredSquirrel=Animal;
    if(Menu){if(Animal)Menu->Squirrel();else Menu->Hunter();}
    const auto* Settings=UWildlineUserSettings::Get();ServerEnterWaitingRoom(Animal,Settings?Settings->SelectedWeapon:0);SetMenu(false);
}
void AWildlineController::ServerEnterWaitingRoom_Implementation(bool Animal,int32 Weapon){if(auto* GM=GetWorld()->GetAuthGameMode<AWildlineGameMode>())GM->EnterWaitingRoom(this,Animal,Weapon);}
void AWildlineGameMode::EnterWaitingRoom(APlayerController* PC,bool Animal,int32 Weapon){
    if(!PC)return;auto* S=GetGameState<AWildlineGameState>();if(!S||S->Winner)return;
    if(auto* PS=PC->GetPlayerState<AWildlinePlayerState>()){PS->bSquirrelTeam=Animal;PS->bWaiting=true;PS->ForceNetUpdate();}
    bWaitingRoomsEnabled=true;
    // Runtime interiors are deterministic, and each client creates its own copy.
    if(!AWildlineLobby::Find(GetWorld()))GetWorld()->SpawnActor<AWildlineLobby>();
    auto* Old=Cast<AWildlineCharacter>(PC->GetPawn());
    if(Old&&Old->bInLobby&&Old->bSquirrel==Animal)return;
    if(Old)Old->ReturnCarriedPeanuts();
    const FTransform T(FRotator::ZeroRotator,AWildlineLobby::Spawn(Animal,GetNumPlayers()-1));
    auto* P=GetWorld()->SpawnActorDeferred<AWildlineCharacter>(AWildlineCharacter::StaticClass(),T,PC,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(!P)return;P->bSquirrel=Animal;P->bInLobby=true;UGameplayStatics::FinishSpawningActor(P,T);
    if(!Animal)P->SelectWeapon(Weapon);PC->Possess(P);if(Old)Old->Destroy();PC->ClientSetRotation(FRotator::ZeroRotator,true);P->ForceNetUpdate();
}
void AWildlineController::ServerLobbyInteract_Implementation(){
    auto* P=Cast<AWildlineCharacter>(GetPawn());if(!P||!P->bInLobby||P->bSquirrel||P->bLobbyReady)return;
    const int32 Weapon=AWildlineLobby::WeaponAt(P->GetActorLocation());if(Weapon==INDEX_NONE)return;
    P->SelectWeapon(Weapon);ClientLobbyWeaponEquipped(P->SelectedWeapon);
}
void AWildlineController::ClientLobbyWeaponEquipped_Implementation(int32 Weapon){if(auto* Settings=UWildlineUserSettings::Get()){Settings->SelectedWeapon=FMath::Clamp(Weapon,0,1);Settings->Persist();}}
void AWildlineGameMode::UpdateWaitingRooms(){
    if(!bWaitingRoomsEnabled)return;auto* S=GetGameState<AWildlineGameState>();if(!S||S->Winner)return;
    TArray<AWildlineCharacter*> Waiting;int32 Humans=0,ReadyCount=0;bool AllReady=true;
    for(auto It=GetWorld()->GetPlayerControllerIterator();It;++It){
        auto* PC=It->Get();if(!PC)continue;Humans++;
        auto* P=Cast<AWildlineCharacter>(PC->GetPawn());if(!P||!P->bInLobby){if(!S->bRoundStarted)AllReady=false;continue;}
        Waiting.Add(P);
        const FVector Offset=P->GetActorLocation()-AWildlineLobby::Centre(P->bSquirrel);
        // Also recover anyone who finds a way over furniture or out of the shell.
        const bool OutsideShell=P->bSquirrel?Offset.SizeSquared2D()>FMath::Square(470.f):FMath::Abs(Offset.X)>730.f||FMath::Abs(Offset.Y)>730.f;
        const auto* Forest=AWildlineWorld::Find(GetWorld());const bool LakeGrounds=Forest&&Forest->bLake&&!P->bSquirrel&&AWildlineLake::InLodgeGrounds(P->GetActorLocation());
        if(!LakeGrounds&&(Offset.Z<-30||Offset.Z>510||OutsideShell)){
            P->SetActorLocation(AWildlineLobby::Spawn(P->bSquirrel,Waiting.Num()-1),false,nullptr,ETeleportType::TeleportPhysics);P->GetCharacterMovement()->StopMovementImmediately();
        }
        const bool Ready=AWildlineLobby::OnPortal(P->bSquirrel,P->GetActorLocation());
        if(P->bLobbyReady!=Ready){P->bLobbyReady=Ready;P->ForceNetUpdate();}
        if(Ready)ReadyCount++;
        AllReady&=Ready;
    }
    const int32 Count=S->bRoundStarted?Waiting.Num():Humans;
    if(S->LobbyPlayers!=Count||S->LobbyReadyPlayers!=ReadyCount){S->LobbyPlayers=Count;S->LobbyReadyPlayers=ReadyCount;S->ForceNetUpdate();}
    if(Waiting.IsEmpty()||Humans==0)return;
    if(!S->bRoundStarted&&!AllReady)return;
    if(!S->bRoundStarted){S->bRoundStarted=true;S->RoundStartedAt=S->GetServerWorldTimeSeconds();RefreshBots();S->ForceNetUpdate();}
    int32 Index=0;
    for(auto* P:Waiting){if(!P->bLobbyReady)continue;P->bInLobby=false;P->bLobbyReady=false;if(auto* PS=P->GetPlayerState<AWildlinePlayerState>()){PS->bWaiting=false;PS->ForceNetUpdate();}P->ProtectedUntil=P->bSquirrel?P->ServerTime()+S->MatchConfig.SpawnProtectionSeconds:0;P->GetCharacterMovement()->StopMovementImmediately();P->SetActorLocation(SpawnLocation(P->bSquirrel,Index++),false,nullptr,ETeleportType::TeleportPhysics);P->ForceNetUpdate();}
}
void AWildlineGameMode::ResetWaitingRooms(){
    auto* S=GetGameState<AWildlineGameState>();if(!S)return;S->bRoundStarted=false;S->RoundStartedAt=0;
    int32 Index=0;for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It){
        auto* P=*It;if(P->bPreviewHunter)continue;
        if(P->bBot){if(P->Controller)P->Controller->Destroy();P->Destroy();continue;}
        if(!Cast<APlayerController>(P->Controller))continue;
        if(auto* PS=P->GetPlayerState<AWildlinePlayerState>()){PS->bWaiting=true;PS->ForceNetUpdate();}
        P->bInLobby=true;P->bLobbyReady=false;P->bFiring=false;P->bAiming=false;P->bSprinting=false;P->GetCharacterMovement()->StopMovementImmediately();P->SetActorLocation(AWildlineLobby::Spawn(P->bSquirrel,Index++),false,nullptr,ETeleportType::TeleportPhysics);P->ForceNetUpdate();
    }
    S->ForceNetUpdate();
}

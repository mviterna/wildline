#include "WildlineGame.h"
#include "WildlineLobby.h"
#include "WildlineIntro.h"
#include "WildlineMenu.h"
#include "WildlineSocial.h"
#include "WildlineRoom.h"
#include "WildlineCharacter.h"
#include "WildlineUserSettings.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWave.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "TimerManager.h"

void AWildlineController::StartIntro(){
    if(Intro||!IsLocalController()||GetNetMode()==NM_DedicatedServer)return;
    PlayerCameraManager->StopCameraFade();MenuRevealAt=-10;if(Menu)Menu->SetRenderOpacity(1);
    if(auto* GI=GetGameInstance<UWildlineGameInstance>())GI->bIntroPlayed=true;
    SetMenu(true);if(Menu)Menu->SetVisibility(ESlateVisibility::Collapsed);
    if(MenuMusic)MenuMusic->Stop();CurrentMusicVolume=0;
    // The production ident is entirely in screen-space UI.

    Intro=GetWorld()->SpawnActorDeferred<AWildlineIntro>(AWildlineIntro::StaticClass(),FTransform::Identity,this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(Intro){Intro->Viewer=this;UGameplayStatics::FinishSpawningActor(Intro,FTransform::Identity);}else SetMenu(true);
}
void AWildlineController::StartMenuMusic(){
    if(!IsLocalController()||FParse::Param(FCommandLine::Get(),TEXT("NoSound")))return;
    if(!MenuMusic){
        auto* Sound=LoadObject<USoundWave>(nullptr,TEXT("/Game/Wildline/Presentation/SW_MenuRap.SW_MenuRap"));
        if(!Sound){UE_LOG(LogTemp,Warning,TEXT("WILDLINE PRESENTATION: menu rap asset missing; run import_presentation.py"));return;}
        MenuMusic=UGameplayStatics::SpawnSound2D(this,Sound,.001f,1,0,nullptr,false,false);
    }else if(!MenuMusic->IsPlaying())MenuMusic->Play();
}
void AWildlineController::Tick(float Delta){
    Super::Tick(Delta);if(!IsLocalController())return;
    if(Social){Social->RefreshAt-=Delta;if(Social->RefreshAt<=0){Social->RefreshAt=.4f;Social->Refresh();}}
    ProfileSyncAt-=Delta;if(ProfileSyncAt<=0&&PlayerState){ProfileSyncAt=.5f;if(const auto* S=UWildlineUserSettings::Get();S&&!S->PlayerDisplayName.IsEmpty()&&PlayerState->GetPlayerName()!=S->PlayerDisplayName)ServerSetProfileName(S->PlayerDisplayName);}
    if(Menu)Menu->RefreshRoom();
    if(LobbyProbeWalkAt>0&&GetWorld()->GetTimeSeconds()>=LobbyProbeWalkAt){if(auto* P=Cast<AWildlineCharacter>(GetPawn());P&&P->bInLobby)P->AddMovementInput((AWildlineLobby::Portal(P->bSquirrel)-P->GetActorLocation()).GetSafeNormal2D(),1);else LobbyProbeWalkAt=0;}
    if(Menu&&!Intro&&MenuRevealAt>=0){const float U=bMenuOpen?FMath::Clamp((GetWorld()->GetTimeSeconds()-MenuRevealAt)/.45f,0.f,1.f):1.f;Menu->SetRenderOpacity(U*U*(3-2*U));if(U>=1){MenuRevealAt=-10;PlayerCameraManager->StopCameraFade();}}
    const auto* Settings=UWildlineUserSettings::Get();const bool Audible=Intro?Intro->bMusic:bMenuOpen;
    // Keep the saved slider unchanged; ease down another 40% as the main menu appears.
    const float Target=Audible?(Settings?Settings->MusicVolume:.65f)*.7f*(Intro?1.f:.6f):0;
    CurrentMusicVolume=FMath::FInterpTo(CurrentMusicVolume,Target,Delta,Audible?2.4f:7.f);
    if(MenuMusic){
        MenuMusic->SetVolumeMultiplier(CurrentMusicVolume);
        if(!Audible&&CurrentMusicVolume<.001f&&MenuMusic->IsPlaying())MenuMusic->Stop();
    }
}
void AWildlineController::EndPlay(const EEndPlayReason::Type Reason){
    if(Social){Social->RemoveFromParent();Social=nullptr;}if(Menu){Menu->RemoveFromParent();Menu=nullptr;}if(IsValid(Intro))Intro->Destroy();Intro=nullptr;if(MenuMusic){MenuMusic->Stop();MenuMusic->DestroyComponent();MenuMusic=nullptr;}Super::EndPlay(Reason);
}

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "WildlineMatchConfig.h"
#include "WildlineGame.generated.h"
class AWildlineCharacter;
class UWildlineMenu;
UCLASS()
class WILDLINE_API UWildlineGameInstance : public UGameInstance {
    GENERATED_BODY()
public:
    bool bPreferredSquirrel=false;
    bool bStartAfterTravel=false;
    int32 PreferredMode=0;
    FString JoinAddress=TEXT("127.0.0.1");
};
UCLASS()
class WILDLINE_API AWildlineGameState : public AGameStateBase {
    GENERATED_BODY()
public:
    UPROPERTY(Replicated) FWildlineMatchConfig MatchConfig;
    UPROPERTY(Replicated) int32 HunterKills=0;
    UPROPERTY(Replicated) int32 StoredAcorns=0;
    UPROPERTY(Replicated) int32 Winner=0;
    UPROPERTY(Replicated) float RoundStartedAt=0;
    UPROPERTY(Replicated) bool bRoundStarted=false;
    UPROPERTY(Replicated) int32 HunterDifficulty=1;
    UPROPERTY(Replicated) int32 ExpeditionMode=0;
    UPROPERTY(Replicated) TArray<int32> TakenPeanuts;
    void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    float RoundRemaining() const;
};
UCLASS()
class WILDLINE_API AWildlineGameMode : public AGameModeBase {
    GENERATED_BODY()
public:
    AWildlineGameMode();
    void InitGame(const FString& MapName,const FString& Options,FString& ErrorMessage) override;
    void BeginPlay() override;
    void Tick(float Delta) override;
    void Logout(AController* Exiting) override;
    void PickRole(APlayerController* PC,bool bSquirrel);
    void RecordKill(AWildlineCharacter* Victim, AController* Hunter);
    void Deposit(int32 Count);
    void RestartRound();
    FVector SpawnLocation(bool bAnimal,int32 Index=0) const;
private:
    void RefreshBots();
    void RespawnAnimal(TWeakObjectPtr<AWildlineCharacter> Victim);
};
UCLASS()
class WILDLINE_API AWildlineController : public APlayerController {
    GENERATED_BODY()
public:
    AWildlineController();
    void BeginPlay() override;
    void SetupInputComponent() override;
    UPROPERTY() UWildlineMenu* Menu;
    bool bMenuOpen=true;
    bool bMapZoomed=false;
    void ToggleMapZoom();
    bool bNetProbePassed=false;
    void SetMenu(bool bOpen);
    void ToggleMenu();
    void ChooseRole(bool bSquirrel);
    void Host(bool bSquirrel);
    void Join(FString Address,bool bSquirrel);
    void MenuClick();
    void RequestRestart();
    void PlaySelected(bool bSquirrel);
    UFUNCTION(Server,Reliable) void ServerSelectWeapon(int32 Weapon);
    UFUNCTION(Server,Reliable) void ServerPickRole(bool bSquirrel);
    UFUNCTION(Server,Reliable) void ServerRestartRound();
    void RunSmokeTest();
    void RunFarmSmokeTest();
    void RunHillSmokeTest();
    void RunHillNetworkProbe();
};

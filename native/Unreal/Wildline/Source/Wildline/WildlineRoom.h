#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "WildlineRoom.generated.h"
class FSocket;
class AWildlineController;
struct FWildlineRoomOffer { FString Id,Address,Name; double Seen=0; bool Active=false; };
// A tiny LAN beacon, deliberately independent of Steam/EOS or a fixed host PC.
UCLASS()
class WILDLINE_API UWildlineRoomSubsystem : public UGameInstanceSubsystem, public FTickableGameObject {
    GENERATED_BODY()
public:
    void Initialize(FSubsystemCollectionBase& Collection) override;
    void Deinitialize() override;
    void Tick(float Delta) override;
    TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UWildlineRoomSubsystem,STATGROUP_Tickables); }
    bool IsTickable() const override { return !IsTemplate()&&Socket!=nullptr; }
    bool Ready() const;
    FString RoomAddress() const;
    FString RoomLabel() const;
    void RequestHost(AWildlineController* Controller,bool Animal);
    bool bAvailable=false;
    bool bClaiming=false;
private:
    void Send(const FString& Message,const FString& Address,int32 Port);
    FString Announcement() const;
    FSocket* Socket=nullptr;
    TArray<FSocket*> SendSockets;
    TMap<FString,FWildlineRoomOffer> Offers;
    FString Id;
    int32 BeaconPort=7778;
    double Started=0,NextSend=0,ClaimAt=0;
    bool bAnimal=false;
    TWeakObjectPtr<AWildlineController> Pending;
};

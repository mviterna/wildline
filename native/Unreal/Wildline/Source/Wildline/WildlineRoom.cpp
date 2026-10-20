#include "WildlineRoom.h"
#include "WildlineGame.h"
#include "WildlineUserSettings.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void UWildlineRoomSubsystem::Initialize(FSubsystemCollectionBase& Collection){
    Super::Initialize(Collection);Started=FPlatformTime::Seconds();Id=FGuid::NewGuid().ToString(EGuidFormats::Digits);
    FParse::Value(FCommandLine::Get(),TEXT("WildlineDiscoveryPort="),BeaconPort);
    auto* SS=ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);if(!SS)return;
    Socket=SS->CreateSocket(NAME_DGram,TEXT("Wildline room discovery"),false);
    auto Bind=SS->CreateInternetAddr();Bind->SetAnyAddress();Bind->SetPort(BeaconPort);
    if(!Socket||!Socket->SetNonBlocking(true)||!Socket->SetBroadcast(true)||!Socket->Bind(*Bind)){
        if(Socket){SS->DestroySocket(Socket);Socket=nullptr;}UE_LOG(LogTemp,Warning,TEXT("WILDLINE ROOM discovery unavailable on UDP %d"),BeaconPort);return;
    }
    // Bind outgoing beacons to each adapter. Otherwise Windows can choose Wi-Fi
    // for a broadcast that needs to traverse the Hamachi virtual adapter.
    TArray<TSharedPtr<FInternetAddr>> Adapters;SS->GetLocalAdapterAddresses(Adapters);
    for(const auto& Adapter:Adapters){
        if(!Adapter.IsValid()||Adapter->GetProtocolType()!=FNetworkProtocolTypes::IPv4)continue;
        auto* Out=SS->CreateSocket(NAME_DGram,TEXT("Wildline adapter beacon"),false);
        auto Local=Adapter->Clone();Local->SetPort(0);
        if(Out&&Out->SetBroadcast(true)&&Out->SetNonBlocking(true)&&Out->Bind(*Local))SendSockets.Add(Out);else if(Out)SS->DestroySocket(Out);
    }
    bAvailable=true;
}
void UWildlineRoomSubsystem::Deinitialize(){auto* SS=ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);if(SS){for(auto* Out:SendSockets){Out->Close();SS->DestroySocket(Out);}SendSockets.Empty();if(Socket){Socket->Close();SS->DestroySocket(Socket);Socket=nullptr;}}Super::Deinitialize();}
bool UWildlineRoomSubsystem::Ready()const{return bAvailable&&FPlatformTime::Seconds()-Started>2.5;}
FString UWildlineRoomSubsystem::RoomAddress()const{
    const FWildlineRoomOffer* Best=nullptr;const double Now=FPlatformTime::Seconds();
    for(const auto& Pair:Offers)if(Now-Pair.Value.Seen<3.5&&Pair.Value.Active&&(!Best||Pair.Key<Best->Id))Best=&Pair.Value;
    return Best?Best->Address:FString();
}
FString UWildlineRoomSubsystem::RoomLabel()const{const FString Address=RoomAddress();for(const auto& Pair:Offers)if(Pair.Value.Address==Address)return Pair.Value.Name;return FString();}
FString UWildlineRoomSubsystem::Announcement()const{
    const auto* W=GetWorld();const bool Active=W&&W->GetNetMode()==NM_ListenServer;
    if(!Active&&!bClaiming)return FString::Printf(TEXT("WILDLINE_ROOM_1|Q|%d"),BeaconPort);
    const auto* S=UWildlineUserSettings::Get();return FString::Printf(TEXT("WILDLINE_ROOM_1|%s|%s|%d|%s"),Active?TEXT("H"):TEXT("C"),*Id,W?W->URL.Port:7777,S?*UWildlineUserSettings::CleanPlayerName(S->PlayerDisplayName):TEXT("Player"));
}
void UWildlineRoomSubsystem::Send(const FString& Message,const FString& Address,int32 Port){
    auto* SS=ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);auto To=SS->CreateInternetAddr();bool Valid=false;To->SetIp(*Address,Valid);To->SetPort(Port);if(!Valid)return;
    FTCHARToUTF8 Bytes(*Message);int32 Sent=0;
    // Replies must originate from the listening port for a direct-IP seed.
    Socket->SendTo(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length(),Sent,*To);
    for(auto* Out:SendSockets)Out->SendTo(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length(),Sent,*To);
}
void UWildlineRoomSubsystem::RequestHost(AWildlineController* PC,bool Animal){if(!Ready()||bClaiming||!PC)return;Pending=PC;bAnimal=Animal;bClaiming=true;ClaimAt=FPlatformTime::Seconds()+2.;NextSend=0;}
void UWildlineRoomSubsystem::Tick(float Delta){
    if(!Socket)return;const double Now=FPlatformTime::Seconds();auto* SS=ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    uint32 Size=0;int32 Packets=0;
    while(Packets++<64&&Socket->HasPendingData(Size)){
        uint8 Data[1025];int32 Read=0;auto From=SS->CreateInternetAddr();if(!Socket->RecvFrom(Data,1024,Read,*From)||Read<=0)continue;Data[Read]=0;
        TArray<FString> Parts;FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Data))).ParseIntoArray(Parts,TEXT("|"),false);
        if(Parts.Num()<2||Parts[0]!=TEXT("WILDLINE_ROOM_1"))continue;
        if(Parts[1]==TEXT("Q")){const FString Reply=Announcement();const int32 ReplyPort=Parts.Num()==3?FCString::Atoi(*Parts[2]):0;if(ReplyPort>=1024&&ReplyPort<=65535&&!Reply.Contains(TEXT("|Q|")))Send(Reply,From->ToString(false),ReplyPort);continue;}
        if(Parts.Num()!=5||Parts[2]==Id||(Parts[1]!=TEXT("H")&&Parts[1]!=TEXT("C")))continue;
        const int32 Port=FCString::Atoi(*Parts[3]);if(Port<1024||Port>65535||Parts[2].Len()!=32)continue;
        FWildlineRoomOffer Offer;Offer.Id=Parts[2];Offer.Address=From->ToString(false)+TEXT(":")+FString::FromInt(Port);Offer.Name=UWildlineUserSettings::CleanPlayerName(Parts[4]);Offer.Seen=Now;Offer.Active=Parts[1]==TEXT("H");Offers.Add(Offer.Id,Offer);
    }
    for(auto It=Offers.CreateIterator();It;++It)if(Now-It.Value().Seen>3.5)It.RemoveCurrent();
    if(Now>=NextSend){NextSend=Now+.5;const FString Message=Announcement();Send(Message,TEXT("255.255.255.255"),BeaconPort);Send(Message,TEXT("25.255.255.255"),BeaconPort);
        // Optional direct-IP fallback; ordinary Hamachi discovery needs no list.
        FString Seed;FParse::Value(FCommandLine::Get(),TEXT("WildlineDiscoverySeed="),Seed);if(!Seed.IsEmpty()){FString IP,Port;if(Seed.Split(TEXT(":"),&IP,&Port))Send(Message,IP,FCString::Atoi(*Port));else Send(Message,Seed,BeaconPort);}
    }
    if(bClaiming&&Pending.IsValid()&&Now>=ClaimAt){
        auto* PC=Pending.Get();const FString Address=RoomAddress();
        if(!Address.IsEmpty()){bClaiming=false;Pending.Reset();PC->Join(Address,bAnimal);return;}
        // Simultaneous clicks elect the lowest claim ID before starting travel.
        for(const auto& Pair:Offers)if(!Pair.Value.Active&&Pair.Key<Id){ClaimAt=Now+.75;return;}
        Pending.Reset();PC->CompleteHost(bAnimal);
    }
    if(GetWorld()&&GetWorld()->GetNetMode()==NM_ListenServer)bClaiming=false;
    if(bClaiming&&!Pending.IsValid()&&Now>ClaimAt+20)bClaiming=false;
}

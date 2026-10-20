#include "WildlineGame.h"
#include "WildlineSocial.h"
#include "WildlineRoom.h"
#include "WildlineMenu.h"
#include "WildlineHunterCreator.h"
#include "WildlineCharacter.h"
#include "WildlineUserSettings.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

void AWildlineController::RunSocialReview(){
    auto Ok=MakeShared<bool>(true);auto Camera=MakeShared<FVector>();
    auto Check=[Ok](bool Value,const TCHAR* Text){*Ok&=Value;UE_LOG(LogTemp,Display,TEXT("SOCIAL CHECK %s: %s"),Value?TEXT("PASS"):TEXT("FAIL"),Text);};
    auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
    auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,true,false);};
    At(1,[this,Check](){Social->Refresh();Check(UWildlineUserSettings::Get()->PlayerDisplayName.IsEmpty()&&Social->bOpen,TEXT("fresh profile prompts for name"));});
    At(1.5f,[Shot](){Shot(TEXT("ProfileFirstLaunch.png"));});
    At(1.75f,[this,Check](){Social->Name->SetText(FText::FromString(TEXT("   ")));Social->SaveName();Check(Social->bOpen&&UWildlineUserSettings::Get()->PlayerDisplayName.IsEmpty(),TEXT("blank name rejected"));});
    At(2,[this,Check](){Social->Name->SetText(FText::FromString(TEXT("  Local Explorer  ")));Social->SaveName();auto* S=UWildlineUserSettings::Get();S->LoadSettings(true);Check(S->PlayerDisplayName==TEXT("Local Explorer")&&!Social->bOpen&&PlayerState->GetPlayerName()==S->PlayerDisplayName,TEXT("trimmed name persists and reaches player state"));Social->Open(true);});
    At(2.5f,[Shot](){Shot(TEXT("ProfileRoom.png"));});
    At(3,[this,Check](){FKeyEvent Key(EKeys::Tab,FModifierKeysState(true,false,false,false,false,false,false,false,false),0,false,0,0);Social->NativeOnPreviewKeyDown(FGeometry(),Key);Check(!Social->bOpen,TEXT("Shift Tab closes overlay"));Menu->NativeOnPreviewKeyDown(FGeometry(),Key);Check(Social->bOpen,TEXT("Shift Tab opens overlay from menu"));Social->Hunter();Check(GetPawn()&&Cast<AWildlineCharacter>(GetPawn())->bInLobby&&!Cast<AWildlineCharacter>(GetPawn())->bSquirrel,TEXT("hunter room entry"));});
    At(4,[this,Check](){Social->Open(true);Social->Squirrel();auto* PS=GetPlayerState<AWildlinePlayerState>();Check(Cast<AWildlineCharacter>(GetPawn())->bSquirrel&&PS&&PS->bSquirrelTeam&&PS->bWaiting,TEXT("team change updates server pawn and roster"));SetMenu(true);Menu->CreateHunter();Menu->Creator->HeightChanged(145);Menu->Creator->SyncControls();});
    At(5,[this,Camera,Shot](){*Camera=Menu->Creator->Capture->GetComponentLocation();Shot(TEXT("HunterHeight145.png"));});
    At(6,[this,Check,Camera](){auto* C=Menu->Creator;C->HeightChanged(205);C->SyncControls();Check(C->Capture->GetComponentLocation().Equals(*Camera,.1f)&&FMath::IsNearlyEqual(C->Preview->GetActorScale3D().Z,205.f/174.5f),TEXT("205 cm grows without camera zoom cancellation"));});
    At(7,[Shot](){Shot(TEXT("HunterHeight205.png"));});
    At(8,[this,Check,Camera](){auto* C=Menu->Creator;C->HeightChanged(230);C->SyncControls();Check(C->Capture->GetComponentLocation().Equals(*Camera,.1f)&&FMath::IsNearlyEqual(C->Preview->GetActorScale3D().Z,230.f/174.5f),TEXT("maximum height continues growth in fixed frame"));});
    At(9,[Shot](){Shot(TEXT("HunterHeight230.png"));});
    At(10,[this,Check](){Menu->Creator->SaveHunter();auto* S=UWildlineUserSettings::Get();S->LoadSettings(true);Check(S->HunterProfile.HeightCm==230&&S->PlayerDisplayName==TEXT("Local Explorer"),TEXT("hunter settings and independent player profile survive reload"));});
    At(11,[Ok](){UE_LOG(LogTemp,Display,TEXT("WILDLINE SOCIAL REVIEW: %s"),*Ok?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Ok?0:1);});
}
void AWildlineController::RunSocialNetworkProbe(){
    const bool HostProbe=FParse::Param(FCommandLine::Get(),TEXT("WildlineSocialHost"));
    if(GetNetMode()==NM_Standalone){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[this](){auto* R=GetGameInstance()->GetSubsystem<UWildlineRoomSubsystem>();const bool Found=R&&!R->RoomAddress().IsEmpty()&&Menu&&!Menu->HostRoomButton->GetIsEnabled();UE_LOG(LogTemp,Display,TEXT("WILDLINE DISCOVERY HOST LOCK: %s"),Found?TEXT("PASS"):TEXT("FAIL"));if(!Found){FPlatformMisc::RequestExitWithStatus(false,1);return;}Host(true);}),4.f,false);return;}
    EnterWaitingRoom(!HostProbe);
    if(HostProbe)if(auto* PS=GetPlayerState<AWildlinePlayerState>()){PS->Kills=2;PS->Shots=4;PS->Hits=3;PS->FoodCollected=5;PS->FoodDeposited=2;PS->FeederScores=1;PS->ForceNetUpdate();}
    if(!HostProbe){FTimerHandle Switch;GetWorldTimerManager().SetTimer(Switch,FTimerDelegate::CreateWeakLambda(this,[this](){EnterWaitingRoom(false);}),6.f,false);}
    const double Started=FPlatformTime::Seconds();auto Done=MakeShared<bool>(false);
    FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[this,HostProbe,Started,Done](){
        if(*Done)return;
        auto* State=GetWorld()->GetGameState<AWildlineGameState>();bool HostName=false,ClientName=false,Stats=false;int32 Hunters=0;for(const auto& Base:State->PlayerArray){auto* PS=Cast<AWildlinePlayerState>(Base.Get());if(!PS)continue;HostName|=PS->GetPlayerName()==TEXT("RoomHost");ClientName|=PS->GetPlayerName()==TEXT("RoomClient");Hunters+=!PS->bSquirrelTeam&&PS->bWaiting;if(PS->GetPlayerName()==TEXT("RoomHost"))Stats=PS->Kills==2&&PS->Shots==4&&PS->Hits==3&&PS->FoodCollected==5&&PS->FoodDeposited==2&&PS->FeederScores==1&&PS->HitPercentage()==75.f;}
        const bool Ok=HostName&&ClientName&&Hunters==2&&Stats;
        if(HostProbe&&!Ok&&FPlatformTime::Seconds()-Started<100)return;
        *Done=true;UE_LOG(LogTemp,Display,TEXT("WILDLINE SOCIAL NETWORK: %s side=%s players=%d hunters=%d hostName=%d clientName=%d"),Ok?TEXT("PASS"):TEXT("FAIL"),HostProbe?TEXT("host"):TEXT("client"),State->PlayerArray.Num(),Hunters,HostName,ClientName);
        if(HostProbe&&Ok){FTimerHandle Exit;GetWorldTimerManager().SetTimer(Exit,[](){FPlatformMisc::RequestExitWithStatus(false,0);},20.f,false);}else FPlatformMisc::RequestExitWithStatus(false,Ok?0:1);
    }),HostProbe?1.f:12.f,HostProbe);
}

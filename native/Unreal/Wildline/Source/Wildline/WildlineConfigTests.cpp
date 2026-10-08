#include "WildlineGame.h"
#include "WildlineCharacter.h"
#include "WildlineMenu.h"
#include "WildlineWorld.h"
#include "WildlineUserSettings.h"
#include "WildlineRules.h"
#include "Components/CheckBox.h"
#include "Components/SpinBox.h"
#include "Components/ScrollBox.h"
#include "Components/WidgetSwitcher.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HighResScreenshot.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWildlineCustomRulesTest,"Wildline.Gameplay.CustomRulesAndBotRoster",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWildlineCustomRulesTest::RunTest(const FString& Parameters){
    // SaveConfig updates class defaults too; keep this fixture isolated from later tests.
    auto* Defaults=GetMutableDefault<UWildlineUserSettings>();const auto OriginalClassic=Defaults->ClassicConfig,OriginalFarm=Defaults->FarmConfig,OriginalHill=Defaults->HillConfig;
    auto C=FWildlineMatchConfig::Preset(1,0);
    TestEqual(TEXT("Farm defaults use shotguns"),C.Defenders[0].Weapon,1);
    TestEqual(TEXT("Hill default preserves ten trips"),FWildlineMatchConfig::Preset(2,0).SquirrelGoal,10);
    C.DefenderBots=99;C.SquirrelBots=-2;C.Defenders.SetNum(1);C.Defenders[0].Weapon=99;C.Defenders[0].Difficulty=-4;
    C.SquirrelHealth=1000;C.RespawnSeconds=0;C.SpawnProtectionSeconds=100;C.CarryCapacity=99;C.MovementScale=10;C.ReloadScale=0;C.ThermalSeconds=0;C.ThermalCooldownSeconds=-2;C.MapIntel=99;C.TimeWinner=99;C.Clamp();
    TestTrue(TEXT("Bot counts and imported partial arrays are bounded"),C.DefenderBots==8&&C.SquirrelBots==0&&C.Defenders.Num()==8&&C.Squirrels.Num()==8);
    TestTrue(TEXT("Invalid loadouts and difficulty are sanitized"),C.Defenders[0].Weapon==1&&C.Defenders[0].Difficulty==0);
    TestTrue(TEXT("Gameplay rule limits are enforced"),C.SquirrelHealth==200&&C.RespawnSeconds==1&&C.SpawnProtectionSeconds==10&&C.CarryCapacity==12&&C.MovementScale==1.5f&&C.ReloadScale==.25f&&C.ThermalSeconds==5&&C.ThermalCooldownSeconds==0&&C.MapIntel==2&&C.TimeWinner==2);
    const auto Standard=FWildlineMatchConfig::Preset(0,0),Casual=FWildlineMatchConfig::Preset(0,1),Hard=FWildlineMatchConfig::Preset(0,2),Play=FWildlineMatchConfig::Preset(0,3);
    TestTrue(TEXT("Casual improves survivability and uses easy defenders"),Casual.SquirrelHealth>Standard.SquirrelHealth&&Casual.Defenders[0].Difficulty==0&&Casual.SpawnProtectionSeconds>0);
    TestTrue(TEXT("Hardcore disables map and thermal and assigns hard bots"),!Hard.bAllowMinimap&&!Hard.bAllowThermal&&Hard.Defenders[0].Difficulty==2);
    TestTrue(TEXT("Playground removes ammunition, stamina and livestock constraints"),Play.bInfiniteAmmo&&Play.bUnlimitedStamina&&!Play.bLivestockDamage);
    auto Changed=Standard;Changed.Defenders[5].Weapon=1;TestTrue(TEXT("Even inactive bot loadout changes require fresh match"),Changed!=Standard);
    Changed=Standard;Changed.bAllowMinimap=false;TestTrue(TEXT("Minimap changes require fresh match"),Changed!=Standard);
    Changed=Standard;Changed.TimeWinner=2;TestTrue(TEXT("Timeout changes require fresh match"),Changed!=Standard);
    const FString Fixture=FPaths::ProjectSavedDir()/TEXT("CustomRulesFixture.ini");
    auto* Saved=NewObject<UWildlineUserSettings>();Saved->ClassicConfig=Casual;Saved->FarmConfig=Play;Saved->HillConfig=Hard;Saved->FarmConfig.Defenders[7].Weapon=1;Saved->FarmConfig.Squirrels[6].Difficulty=0;Saved->SaveConfig(CPF_Config,*Fixture);
    auto* Loaded=NewObject<UWildlineUserSettings>();Loaded->LoadConfig(UWildlineUserSettings::StaticClass(),*Fixture);
    TestTrue(TEXT("Complete rules and every active/inactive bot slot survive persistence per mode"),Loaded->ClassicConfig==Saved->ClassicConfig&&Loaded->FarmConfig==Saved->FarmConfig&&Loaded->HillConfig==Saved->HillConfig);
    Defaults->ClassicConfig=OriginalClassic;Defaults->FarmConfig=OriginalFarm;Defaults->HillConfig=OriginalHill;
    return true;
}
#endif

void AWildlineController::RunConfigSmokeTest(){
    auto Result=MakeShared<bool>(true);auto Check=[Result](bool Ok,const TCHAR* What){*Result&=Ok;UE_LOG(LogTemp,Display,TEXT("CONFIG CHECK %s: %s"),What,Ok?TEXT("PASS"):TEXT("FAIL"));};
    auto* S=GetWorld()->GetGameState<AWildlineGameState>();auto* Settings=UWildlineUserSettings::Get();auto* GM=GetWorld()->GetAuthGameMode<AWildlineGameMode>();
    Check(S&&Settings&&GM&&Menu,TEXT("Config test world and menu exist"));if(!S||!Settings||!GM||!Menu){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    const auto Original=Settings->ConfigForMode(S->ExpeditionMode);
    GetGameInstance<UWildlineGameInstance>()->PreferredMode=S->ExpeditionMode;Menu->Mode->SetSelectedIndex(S->ExpeditionMode);
    auto C=FWildlineMatchConfig::Preset(S->ExpeditionMode,0);C.DefenderBots=2;C.SquirrelBots=3;C.Defenders[0].Weapon=0;C.Defenders[0].Difficulty=0;C.Defenders[1].Weapon=1;C.Defenders[1].Difficulty=2;C.Squirrels[0].Difficulty=2;
    C.SquirrelHealth=80;C.CarryCapacity=1;C.RespawnSeconds=1;C.SpawnProtectionSeconds=2;C.bAllowMinimap=false;C.bAllowThermal=false;C.MovementScale=1.2f;C.bUnlimitedStamina=true;C.ReloadScale=.5f;
    Settings->ConfigForMode(S->ExpeditionMode)=C;ChooseRole(true);S->Winner=1;GM->RestartRound();Menu->RefreshConfig();
    auto* P=Cast<AWildlineCharacter>(GetPawn());Check(P&&P->Health==80,TEXT("Configured health applies on restart"));
    TWeakObjectPtr<AWildlineCharacter> First;int32 Defenders=0,Squirrels=0;bool Loadouts=true;
    for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(It->bBot){It->SetActorTickEnabled(false);if(It->bSquirrel){Squirrels++;Loadouts&=It->AIDifficulty==C.Squirrels[It->BotSlot].Difficulty;}else{Defenders++;Loadouts&=It->AIDifficulty==C.Defenders[It->BotSlot].Difficulty&&It->SelectedWeapon==C.Defenders[It->BotSlot].Weapon;if(It->BotSlot==0)First=*It;}}
    Check(Defenders==2&&Squirrels==3&&Loadouts,TEXT("Exact bot counts, individual weapons and difficulty reach spawned actors"));
    Check(!Menu->Minimap->IsChecked()&&Menu->BotRows[1]->Difficulty->GetSelectedIndex()==2&&Menu->BotRows[2]->GetVisibility()==ESlateVisibility::Collapsed,TEXT("Menu reflects disabled map, individual AI, and hides inactive slots"));
    bMapZoomed=false;bMenuOpen=false;ToggleMapZoom();Check(!bMapZoomed,TEXT("Denied minimap cannot be toggled with M"));bMenuOpen=true;
    if(P){
        UGameplayStatics::ApplyDamage(P,100,nullptr,nullptr,UDamageType::StaticClass());Check(!P->bDead&&P->Health==80&&S->HunterKills==0,TEXT("Spawn protection rejects damage and kills"));P->ProtectedUntil=0;P->Stamina=20;P->Tick(.01f);
        Check(P->Stamina==100&&FMath::IsNearlyEqual(P->GetCharacterMovement()->MaxWalkSpeed,384),TEXT("Unlimited stamina and speed multiplier apply"));
        auto* F=AWildlineWorld::Find(GetWorld());if(F&&!F->bResidential){P->SetActorLocation(F->AcornLocations[0]+FVector(0,0,13));P->Tick(.01f);P->SetActorLocation(F->AcornLocations[1]+FVector(0,0,13));P->Tick(.01f);Check(P->Bag==1,TEXT("Carry capacity is enforced during actual pickups"));}
        UGameplayStatics::ApplyDamage(P,100,nullptr,nullptr,UDamageType::StaticClass());Check(P->bDead&&S->HunterKills==1&&FMath::IsNearlyEqual(P->RespawnAt-P->ServerTime(),1,.1f),TEXT("Damage scores a kill and schedules configured respawn"));
    }
    FTimerHandle RespawnCheck;GetWorldTimerManager().SetTimer(RespawnCheck,FTimerDelegate::CreateWeakLambda(this,[this,Check,P,S,Settings,GM,Original,First,Result](){
        Check(P&&!P->bDead&&P->Health==80&&P->ProtectedUntil>P->ServerTime(),TEXT("Configured respawn restores health and protection"));
        ChooseRole(false);auto* H=Cast<AWildlineCharacter>(GetPawn());Check(First.IsValid(),TEXT("Human role changes retain existing bots"));
        if(H){H->SelectWeapon(0);H->ThermalReadyAt=0;H->ServerThermal();Check(H->ThermalUntil==0,TEXT("Disabled thermal is enforced by server"));S->MatchConfig.bAllowThermal=true;S->MatchConfig.ThermalSeconds=5;S->MatchConfig.ThermalCooldownSeconds=9;H->ServerThermal();Check(FMath::IsNearlyEqual(H->ThermalUntil-H->ServerTime(),5,.1f)&&FMath::IsNearlyEqual(H->ThermalReadyAt-H->ThermalUntil,9,.01f),TEXT("Custom thermal duration and cooldown apply"));H->Ammo=0;H->ServerReload();Check(FMath::IsNearlyEqual(H->ReloadUntil-H->ServerTime(),1.1f,.1f),TEXT("Reload duration multiplier applies"));H->ReloadUntil=0;H->Ammo=5;S->MatchConfig.bInfiniteAmmo=true;H->ServerFire(FVector::UpVector);Check(H->Ammo==5,TEXT("Infinite ammo preserves magazine on accepted shot"));}
        auto Zero=Settings->ConfigForMode(S->ExpeditionMode);Zero.DefenderBots=Zero.SquirrelBots=0;Settings->ConfigForMode(S->ExpeditionMode)=Zero;S->Winner=1;GM->RestartRound();int32 Bots=0;for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(It->bBot)Bots++;Check(Bots==0,TEXT("Zero bot counts remove both teams on restart"));
        S->MatchConfig.TimeWinner=2;S->RoundStartedAt=S->GetServerWorldTimeSeconds()-2000;GM->Tick(.01f);Check(S->Winner==2,TEXT("Custom timeout winner overrides mode default"));
        // Exercise menu callbacks and actual saved settings in the isolated probe profile.
        Menu->RefreshConfig();Menu->DefenderBotCount->SetValue(2);Menu->BotRows[1]->Weapon->SetSelectedIndex(1);Menu->BotRows[1]->Difficulty->SetSelectedIndex(0);Menu->SaveBot(Menu->BotRows[1]);Settings->LoadSettings(true);
        Check(Settings->ConfigForMode(S->ExpeditionMode).DefenderBots==2&&Settings->ConfigForMode(S->ExpeditionMode).Defenders[1].Weapon==1&&Settings->ConfigForMode(S->ExpeditionMode).Defenders[1].Difficulty==0,TEXT("Menu count and per-bot edits survive real save/reload"));
        Settings->ConfigForMode(S->ExpeditionMode)=Original;Settings->Persist();
        FFileHelper::SaveStringToFile(*Result?TEXT("PASS\n"):TEXT("FAIL\n"),*(FPaths::ProjectSavedDir()/TEXT("ConfigSmokeResult.txt")));UE_LOG(LogTemp,Display,TEXT("WILDLINE CONFIG SMOKE: %s"),*Result?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Result?0:1);
    }),1.3f,false);
}

void AWildlineController::RunConfigNetworkProbe(){
    auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
    if(HasAuthority())At(2,[this](){auto* S=GetWorld()->GetGameState<AWildlineGameState>();auto& C=S->MatchConfig;C.DefenderBots=2;C.SquirrelBots=1;C.Defenders[0].Weapon=1;C.Defenders[0].Difficulty=0;C.Defenders[1].Difficulty=2;C.bAllowMinimap=false;C.SquirrelHealth=90;C.RespawnSeconds=3;C.CarryCapacity=7;C.bInfiniteAmmo=true;C.bAllowThermal=false;C.MapIntel=2;C.TimeWinner=2;C.Clamp();UWildlineUserSettings::Get()->ConfigForMode(S->ExpeditionMode)=C;S->Winner=1;GetWorld()->GetAuthGameMode<AWildlineGameMode>()->RestartRound();S->ForceNetUpdate();});
    At(HasAuthority()?45.f:18.f,[this](){const auto* S=GetWorld()->GetGameState<AWildlineGameState>();int32 Defenders=0,Squirrels=0;bool Loadout=false;for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(It->bBot){if(It->bSquirrel)Squirrels++;else{Defenders++;Loadout|=It->BotSlot==0&&It->SelectedWeapon==1&&It->AIDifficulty==0;}}
        if(Menu)Menu->RefreshConfig();const bool Ok=S&&Defenders==2&&Squirrels==1&&Loadout&&!S->MatchConfig.bAllowMinimap&&S->MatchConfig.SquirrelHealth==90&&S->MatchConfig.RespawnSeconds==3&&S->MatchConfig.CarryCapacity==7&&S->MatchConfig.bInfiniteAmmo&&!S->MatchConfig.bAllowThermal&&S->MatchConfig.MapIntel==2&&S->MatchConfig.TimeWinner==2&&Menu&&!Menu->Minimap->IsChecked()&&(HasAuthority()||!Menu->Minimap->GetIsEnabled());
        UE_LOG(LogTemp,Display,TEXT("WILDLINE CONFIG NETWORK %s: %s"),HasAuthority()?TEXT("Host"):TEXT("Client"),Ok?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,Ok?0:1);
    });
}

void AWildlineController::RunConfigReview(){
    auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
    auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows")/Name,true,false);};
    At(2,[this](){SetMenu(true);Menu->ShowRules();});At(5,[Shot](){Shot(TEXT("ConfigRules.png"));});
    At(7,[this](){auto* Scroll=Cast<UScrollBox>(Menu->Pages->GetActiveWidget());if(Scroll)Scroll->ScrollToEnd();});At(9,[Shot](){Shot(TEXT("ConfigAdvanced.png"));});
    At(11,[this](){Menu->ShowBots();});At(13,[Shot](){Shot(TEXT("ConfigBots.png"));});
    At(15,[this](){Menu->ShowSettings();});At(17,[Shot](){Shot(TEXT("ConfigSettings.png"));});
    At(19,[](){FPlatformMisc::RequestExit(false);});
}

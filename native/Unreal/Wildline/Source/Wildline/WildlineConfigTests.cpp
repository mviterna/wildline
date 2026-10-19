#include "WildlineGame.h"
#include "WildlineCharacter.h"
#include "WildlineMenu.h"
#include "WildlineWorld.h"
#include "WildlineUserSettings.h"
#include "WildlineRules.h"
#include "Components/CheckBox.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
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
    Changed=Standard;Changed.SquirrelHitbox=100;TestTrue(TEXT("Hitbox changes require a fresh match"),Changed!=Standard);
    TestEqual(TEXT("Existing profiles retain the original target size"),Standard.SquirrelHitboxScale(),1.f);
    auto Hitbox=Standard;Hitbox.SquirrelHitbox=-10;Hitbox.Clamp();TestEqual(TEXT("Hitbox minimum is 1"),Hitbox.SquirrelHitbox,1);
    TestEqual(TEXT("Minimum hitbox is half size"),Hitbox.SquirrelHitboxScale(),.5f);
    float Previous=Hitbox.SquirrelHitboxScale();bool Monotonic=true;
    for(int32 Value=2;Value<=100;Value++){Hitbox.SquirrelHitbox=Value;const float Scale=Hitbox.SquirrelHitboxScale();Monotonic&=Scale>Previous;Previous=Scale;}
    TestTrue(TEXT("Every hitbox step makes the target larger"),Monotonic);
    Hitbox.SquirrelHitbox=999;Hitbox.Clamp();TestTrue(TEXT("Hitbox maximum is 100 and double size"),Hitbox.SquirrelHitbox==100&&Hitbox.SquirrelHitboxScale()==2.f);
    Changed=Standard;Changed.BotSightScale=.75f;Changed.BotReactionScale=.85f;Changed.BotAccuracyScale=.75f;Changed.HunterAggression=.9f;Changed.SquirrelCaution=.8f;Changed.BotSpacing=260;Changed.BotTreeSeconds=4;Changed.bBotClimbing=false;
    TestTrue(TEXT("AI behavior changes require a fresh match"),Changed!=Standard);
    auto InvalidAI=Changed;InvalidAI.BotSightScale=20;InvalidAI.BotReactionScale=0;InvalidAI.BotAccuracyScale=0;InvalidAI.HunterAggression=20;InvalidAI.SquirrelCaution=0;InvalidAI.BotSpacing=0;InvalidAI.BotTreeSeconds=100;InvalidAI.Clamp();
    TestTrue(TEXT("Imported AI controls are bounded"),InvalidAI.BotSightScale==1&&InvalidAI.BotReactionScale==0&&InvalidAI.BotAccuracyScale==0&&InvalidAI.HunterAggression==1&&InvalidAI.SquirrelCaution==0&&InvalidAI.BotSpacing==60&&InvalidAI.BotTreeSeconds==12);
    auto Maximum=Standard;Maximum.BotSightScale=Maximum.BotReactionScale=Maximum.BotAccuracyScale=Maximum.HunterAggression=Maximum.SquirrelCaution=1;
    auto Minimum=Standard;Minimum.BotSightScale=Minimum.BotReactionScale=Minimum.BotAccuracyScale=Minimum.HunterAggression=Minimum.SquirrelCaution=0;
    TestTrue(TEXT("100 accuracy has zero aim error even for Easy bots"),Maximum.AimError(100)==0&&Standard.AimError(42)==42&&Minimum.AimError(42)>Standard.AimError(42));
    TestTrue(TEXT("Higher reaction speed is faster and 100 uses the weapon cadence"),Maximum.ReactionDelay(1.4f)<Standard.ReactionDelay(1.4f)&&Standard.ReactionDelay(1.4f)<Minimum.ReactionDelay(1.4f)&&Maximum.ShotDelay(3.8f,.85f)==.85f);
    TestTrue(TEXT("100 strengthens sight, pursuit, proximity and search beyond the old maxima"),Maximum.SightMultiplier()==3&&Maximum.PursuitSpeed(320)==580&&Maximum.StopDistance(800)==110&&Maximum.SearchMemory(4)==20&&Maximum.CautionMultiplier()==2.5f);
    const FString Fixture=FPaths::ProjectSavedDir()/TEXT("CustomRulesFixture.ini");
    auto* Saved=NewObject<UWildlineUserSettings>();Saved->ClassicConfig=Changed;Saved->FarmConfig=Play;Saved->HillConfig=Hard;Saved->ClassicConfig.SquirrelHitbox=1;Saved->FarmConfig.SquirrelHitbox=73;Saved->HillConfig.SquirrelHitbox=100;Saved->FarmConfig.Defenders[7].Weapon=1;Saved->FarmConfig.Squirrels[6].Difficulty=0;Saved->SaveConfig(CPF_Config,*Fixture);
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
    C.SquirrelHitbox=73;C.SquirrelHealth=80;C.CarryCapacity=1;C.RespawnSeconds=1;C.SpawnProtectionSeconds=2;C.bAllowMinimap=false;C.bAllowThermal=false;C.MovementScale=1.2f;C.bUnlimitedStamina=true;C.ReloadScale=.5f;
    Settings->ConfigForMode(S->ExpeditionMode)=C;ChooseRole(true);S->Winner=1;GM->RestartRound();Menu->RefreshConfig();
    auto* P=Cast<AWildlineCharacter>(GetPawn());Check(P&&P->Health==80,TEXT("Configured health applies on restart"));
    Check(P&&FMath::IsNearlyEqual(P->SquirrelHitbox->GetUnscaledCapsuleRadius(),7.f*C.SquirrelHitboxScale()),TEXT("Restart applies the configured weapon hitbox"));
    // Exercise the actual weapon query channel away from map geometry and players.
    const auto Rules=S->MatchConfig;const FVector Centre(0,0,20000);
    auto* Target=GetWorld()->SpawnActorDeferred<AWildlineCharacter>(AWildlineCharacter::StaticClass(),FTransform(Centre),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    Target->bSquirrel=true;UGameplayStatics::FinishSpawningActor(Target,FTransform(Centre));Target->SetActorTickEnabled(false);Target->ProtectedUntil=0;
    auto HitAt=[this,Centre,Target](float Offset){FHitResult Hit;return GetWorld()->LineTraceSingleByChannel(Hit,Centre+FVector(-100,Offset,0),Centre+FVector(100,Offset,0),ECC_Visibility)&&Hit.GetActor()==Target&&Hit.GetComponent()==Target->SquirrelHitbox;};
    S->MatchConfig.SquirrelHitbox=1;Target->UpdateSquirrelHitbox();Check(HitAt(0)&&!HitAt(5),TEXT("Small target hits centrally but misses outside its reduced radius"));
    S->MatchConfig.SquirrelHitbox=50;Target->UpdateSquirrelHitbox();Check(HitAt(5)&&!HitAt(10),TEXT("50 preserves the original weapon target"));
    S->MatchConfig.SquirrelHitbox=100;Target->UpdateSquirrelHitbox();Check(HitAt(10),TEXT("Large target accepts shots that miss at 50"));
    Check(Target->GetCapsuleComponent()->GetUnscaledCapsuleRadius()==7&&Target->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()==12&&Target->GetActorScale3D()==FVector::OneVector&&Target->SquirrelHitbox->GetCollisionResponseToChannel(ECC_Pawn)==ECR_Ignore&&Target->SquirrelHitbox->GetCollisionResponseToChannel(ECC_WorldStatic)==ECR_Ignore,TEXT("Hitbox leaves movement capsule, scale and physical world collision unchanged"));
    auto* Cover=GetWorld()->SpawnActor<AActor>();auto* Wall=NewObject<UBoxComponent>(Cover);Cover->SetRootComponent(Wall);Wall->SetBoxExtent(FVector(2,40,40));Wall->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Wall->SetCollisionResponseToAllChannels(ECR_Ignore);Wall->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Wall->RegisterComponent();Cover->SetActorLocation(Centre+FVector(-30,0,0));
    Check(!HitAt(10),TEXT("Cover blocks shots even at maximum hitbox size"));Cover->Destroy();
    Target->bDead=true;Target->OnRepDead();Check(!HitAt(0)&&Target->SquirrelHitbox->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("Dead squirrels no longer block weapon traces"));Target->bDead=false;Target->OnRepDead();Check(HitAt(10),TEXT("Respawn restores the configured hitbox"));
    auto* Shooter=GetWorld()->SpawnActor<AWildlineCharacter>(Centre+FVector(-180,8,-72),FRotator::ZeroRotator);Shooter->SetActorTickEnabled(false);Shooter->bInLobby=false;
    auto ResetTarget=[Target](){Target->bDead=false;Target->Health=200;Target->ProtectedUntil=0;Target->OnRepDead();};
    auto BulletShot=[this,S,Shooter,Target,Centre,ResetTarget](int32 Size){S->MatchConfig.SquirrelHitbox=Size;ResetTarget();auto* Bullet=GetWorld()->SpawnActor<AWildlineBullet>(Centre+FVector(-100,10,0),FRotator::ZeroRotator);Bullet->Shooter=Shooter;Bullet->Velocity=FVector(90000,0,0);Bullet->Tick(.003f);const float Health=Target->Health;Bullet->Destroy();return Health;};
    Check(BulletShot(1)==200&&BulletShot(100)==100,TEXT("Actual ballistic rifle bullet respects smaller and larger targets"));
    Shooter->bBot=true;S->MatchConfig.BotAccuracyScale=1;Shooter->SelectedWeapon=0;
    auto Fire=[Shooter,Target,ResetTarget](){ResetTarget();if(Shooter->SelectedWeapon==1)Target->Health=1000;Shooter->Ammo=5;Shooter->LastShot=-10;Shooter->ServerFire(FVector::ForwardVector);};
    S->MatchConfig.SquirrelHitbox=1;Fire();const float SmallRifle=Target->Health;S->MatchConfig.SquirrelHitbox=100;Fire();
    Check(SmallRifle==200&&Target->Health==100,TEXT("Instant AI rifle uses the same adjustable hitbox"));
    Shooter->SelectedWeapon=1;S->MatchConfig.SquirrelHitbox=1;Fire();const float SmallShotgun=Target->Health;S->MatchConfig.SquirrelHitbox=100;Fire();
    Check(Target->Health<SmallShotgun,TEXT("Shotgun pellets hit more of the larger target"));
    Target->bSquirrel=false;Target->ConfigureRole();Check(Target->SquirrelHitbox->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&Target->GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Visibility)==ECR_Block,TEXT("Switching to hunter disables squirrel hitbox and restores hunter collision"));
    Target->Destroy();Shooter->Destroy();S->MatchConfig=Rules;
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
        Menu->RefreshConfig();Menu->SquirrelHitbox->SetValue(92);Menu->DefenderBotCount->SetValue(2);Menu->BotRows[1]->Weapon->SetSelectedIndex(1);Menu->BotRows[1]->Difficulty->SetSelectedIndex(0);Menu->SaveBot(Menu->BotRows[1]);Settings->LoadSettings(true);
        Check(Settings->ConfigForMode(S->ExpeditionMode).SquirrelHitbox==92&&Settings->ConfigForMode(S->ExpeditionMode).DefenderBots==2&&Settings->ConfigForMode(S->ExpeditionMode).Defenders[1].Weapon==1&&Settings->ConfigForMode(S->ExpeditionMode).Defenders[1].Difficulty==0,TEXT("Menu hitbox, count and per-bot edits survive real save/reload"));
        Settings->ConfigForMode(S->ExpeditionMode)=Original;Settings->Persist();
        FFileHelper::SaveStringToFile(*Result?TEXT("PASS\n"):TEXT("FAIL\n"),*(FPaths::ProjectSavedDir()/TEXT("ConfigSmokeResult.txt")));UE_LOG(LogTemp,Display,TEXT("WILDLINE CONFIG SMOKE: %s"),*Result?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Result?0:1);
    }),1.3f,false);
}

void AWildlineController::RunConfigNetworkProbe(){
    auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
    if(HasAuthority())At(2,[this](){auto* S=GetWorld()->GetGameState<AWildlineGameState>();auto& C=S->MatchConfig;C.DefenderBots=2;C.SquirrelBots=1;C.Defenders[0].Weapon=1;C.Defenders[0].Difficulty=0;C.Defenders[1].Difficulty=2;C.bAllowMinimap=false;C.SquirrelHitbox=100;C.SquirrelHealth=90;C.RespawnSeconds=3;C.CarryCapacity=7;C.bInfiniteAmmo=true;C.bAllowThermal=false;C.MapIntel=2;C.TimeWinner=2;C.BotSightScale=.75f;C.BotReactionScale=.85f;C.BotAccuracyScale=.75f;C.HunterAggression=.9f;C.SquirrelCaution=.8f;C.BotSpacing=260;C.BotTreeSeconds=4;C.bBotClimbing=false;C.Clamp();UWildlineUserSettings::Get()->ConfigForMode(S->ExpeditionMode)=C;S->Winner=1;GetWorld()->GetAuthGameMode<AWildlineGameMode>()->RestartRound();S->ForceNetUpdate();});
    // Unreal shrinks simulated proxies' movement capsules by 0.01 cm.
    At(HasAuthority()?45.f:18.f,[this](){const auto* S=GetWorld()->GetGameState<AWildlineGameState>();int32 Defenders=0,Squirrels=0;bool Loadout=false,Hitboxes=true;for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(It->bBot){if(It->bSquirrel){Squirrels++;Hitboxes&=FMath::IsNearlyEqual(It->SquirrelHitbox->GetUnscaledCapsuleRadius(),14.f)&&FMath::IsNearlyEqual(It->GetCapsuleComponent()->GetUnscaledCapsuleRadius(),7.f,.02f);UE_LOG(LogTemp,Display,TEXT("CONFIG NET HITBOX %s size=%d radius=%.4f movement=%.4f dead=%d"),HasAuthority()?TEXT("Host"):TEXT("Client"),S?S->MatchConfig.SquirrelHitbox:-1,It->SquirrelHitbox->GetUnscaledCapsuleRadius(),It->GetCapsuleComponent()->GetUnscaledCapsuleRadius(),It->bDead);}else{Defenders++;Loadout|=It->BotSlot==0&&It->SelectedWeapon==1&&It->AIDifficulty==0;}}
        if(Menu)Menu->RefreshConfig();const bool Ok=S&&Defenders==2&&Squirrels==1&&Loadout&&Hitboxes&&S->MatchConfig.SquirrelHitbox==100&&!S->MatchConfig.bAllowMinimap&&S->MatchConfig.SquirrelHealth==90&&S->MatchConfig.RespawnSeconds==3&&S->MatchConfig.CarryCapacity==7&&S->MatchConfig.bInfiniteAmmo&&!S->MatchConfig.bAllowThermal&&S->MatchConfig.MapIntel==2&&S->MatchConfig.TimeWinner==2&&S->MatchConfig.BotSightScale==.75f&&S->MatchConfig.BotReactionScale==.85f&&S->MatchConfig.BotAccuracyScale==.75f&&FMath::IsNearlyEqual(S->MatchConfig.HunterAggression,.9f)&&FMath::IsNearlyEqual(S->MatchConfig.SquirrelCaution,.8f)&&S->MatchConfig.BotSpacing==260&&S->MatchConfig.BotTreeSeconds==4&&!S->MatchConfig.bBotClimbing&&Menu&&!Menu->Minimap->IsChecked()&&Menu->SquirrelHitbox->GetValue()==100&&(HasAuthority()||(!Menu->Minimap->GetIsEnabled()&&!Menu->SquirrelHitbox->GetIsEnabled()));
        UE_LOG(LogTemp,Display,TEXT("CONFIG NET SUMMARY defenders=%d squirrels=%d loadout=%d hitboxes=%d menuHitbox=%.0f editable=%d"),Defenders,Squirrels,Loadout,Hitboxes,Menu?Menu->SquirrelHitbox->GetValue():-1,Menu?Menu->SquirrelHitbox->GetIsEnabled():false);
        UE_LOG(LogTemp,Display,TEXT("WILDLINE CONFIG NETWORK %s: %s"),HasAuthority()?TEXT("Host"):TEXT("Client"),Ok?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,Ok?0:1);
    });
}

void AWildlineController::RunConfigReview(){
    auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
    auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows")/Name,true,false);};
    At(2,[this](){SetMenu(true);Menu->ShowRules();});At(5,[Shot](){Shot(TEXT("ConfigRules.png"));});
    At(7,[this](){auto* Scroll=Cast<UScrollBox>(Menu->Pages->GetActiveWidget());if(Scroll)Scroll->ScrollToEnd();});At(9,[Shot](){Shot(TEXT("ConfigAdvanced.png"));});
    At(10,[this](){auto* Scroll=Cast<UScrollBox>(Menu->Pages->GetActiveWidget());if(Scroll)Scroll->ScrollWidgetIntoView(Menu->SquirrelHitbox,false,EDescendantScrollDestination::Center);});At(10.7f,[Shot](){Shot(TEXT("ConfigHitbox.png"));});
    At(11,[this](){Menu->ShowBots();});At(13,[Shot](){Shot(TEXT("ConfigBots.png"));});
    At(15,[this](){Menu->ShowSettings();});At(17,[Shot](){Shot(TEXT("ConfigSettings.png"));});
    At(19,[](){FPlatformMisc::RequestExit(false);});
}

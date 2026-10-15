#include "WildlineGame.h"
#include "WildlineCharacter.h"
#include "WildlineWorld.h"
#include "WildlineMenu.h"
#include "WildlineUserSettings.h"
#include "Components/BoxComponent.h"
#include "Components/SpinBox.h"
#include "Components/CheckBox.h"
#include "Components/ScrollBox.h"
#include "Components/WidgetSwitcher.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UnrealClient.h"

void AWildlineController::RunAIReview(){
    auto Passed=MakeShared<bool>(true);
    auto Check=[Passed](bool Ok,const TCHAR* What){*Passed&=Ok;UE_LOG(LogTemp,Display,TEXT("AI CHECK %s: %s"),Ok?TEXT("PASS"):TEXT("FAIL"),What);};
    auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
    auto Starts=MakeShared<TMap<TWeakObjectPtr<AWildlineCharacter>,FVector>>();
    auto Distance=MakeShared<TMap<TWeakObjectPtr<AWildlineCharacter>,float>>();
    auto Forest=AWildlineWorld::Find(GetWorld());auto* Settings=UWildlineUserSettings::Get();
    const int32 Mode=GetWorld()->GetGameState<AWildlineGameState>()->ExpeditionMode;
    const auto Original=Settings->ConfigForMode(Mode);
    At(2,[this,Check,Starts,Forest,Settings,Mode](){
        auto C=FWildlineMatchConfig::Preset(Mode,0);C.DefenderBots=3;C.SquirrelBots=8;C.DurationSeconds=1800;C.SquirrelGoal=30;C.DefenderGoal=30;C.bLivestockDamage=false;C.bBotClimbing=false;
        for(auto& B:C.Defenders)B.Difficulty=1;
        Settings->ConfigForMode(Mode)=C;GetWorld()->GetGameState<AWildlineGameState>()->MatchConfig=C;ChooseRole(false);SetMenu(false);
        auto* Human=Cast<AWildlineCharacter>(GetPawn());Human->SetActorLocation(FVector(12000,0,3000));Human->GetCharacterMovement()->DisableMovement();
        int32 Hunters=0,Squirrels=0;
        for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(It->bBot){
            Starts->Add(*It,It->GetActorLocation());
            if(It->bSquirrel){Squirrels++;It->SetActorTickEnabled(false);It->SetActorLocation(FVector(12500+It->BotSlot*150,12500,3000));}
            else {Hunters++;It->ResetBotAwareness();}
        }
        Check(Hunters==3&&Squirrels==8,TEXT("full eight-squirrel and three-hunter roster spawned"));
    });
    At(18,[this,Check,Starts,Forest](){
        int32 Moving=0;auto* GM=GetWorld()->GetAuthGameMode<AWildlineGameMode>();
        for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(It->bBot){
            if(!It->bSquirrel){
                const float Travel=FVector::Dist2D(Starts->FindChecked(*It),It->GetActorLocation());
                UE_LOG(LogTemp,Display,TEXT("AI PATROL slot=%d travel=%.0f recoveries=%d"),It->BotSlot,Travel,It->BotRecoveries);
                Moving+=Travel>500;It->SetActorTickEnabled(false);It->GetCharacterMovement()->DisableMovement();It->SetActorLocation(FVector(12000+It->BotSlot*180,-12000,3000));
            }else{
                It->SetActorLocation(GM->SpawnLocation(true,It->BotSlot+1),false,nullptr,ETeleportType::TeleportPhysics);
                It->ResetBotAwareness();It->SetActorTickEnabled(true);Starts->Add(*It,It->GetActorLocation());
            }
        }
        Check(Moving==3,TEXT("every hunter patrols at least five metres with no visible enemies"));
    });
    At(21,[this,Check,Forest](){
        if(Forest->bResidential)return;
        TSet<int32> Goals;for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(It->bBot&&It->bSquirrel&&It->BotFoodIndex>=0)Goals.Add(It->BotFoodIndex);
        UE_LOG(LogTemp,Display,TEXT("AI FOOD distinct=%d"),Goals.Num());Check(Goals.Num()>=6,TEXT("eight squirrels distribute food destinations instead of sharing the nearest nut"));
    });
    for(float Time=23;Time<91;Time+=2)At(Time,[this,Starts,Distance](){
        auto* S=GetWorld()->GetGameState<AWildlineGameState>();S->Winner=0;S->StoredAcorns=FMath::Min(S->StoredAcorns,20);
        for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(It->bBot&&It->bSquirrel){
            const float Travel=FVector::Dist2D(Starts->FindChecked(*It),It->GetActorLocation());Distance->FindOrAdd(*It)=FMath::Max(Distance->FindOrAdd(*It),Travel);
        }
    });
    At(92,[this,Check,Distance,Forest](){
        int32 Moving=0,Foraging=0;for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(It->bBot&&It->bSquirrel){
            const float Travel=Distance->FindRef(*It);Moving+=Travel>400;Foraging+=Forest->bResidential?(It->CourseCheckpoint>1||It->FeederVisits>0):It->Collected.Num()>0;
            UE_LOG(LogTemp,Display,TEXT("AI FORAGE slot=%d travel=%.0f collected=%d checkpoint=%d visits=%d recoveries=%d pos=%s"),It->BotSlot,Travel,It->Collected.Num(),It->CourseCheckpoint,It->FeederVisits,It->BotRecoveries,*It->GetActorLocation().ToString());
            It->SetActorTickEnabled(false);It->GetCharacterMovement()->StopMovementImmediately();
        }
        Check(Moving>=7,TEXT("at least seven of eight squirrels travel four metres during sustained play"));
        Check(Foraging>=(Forest->bResidential?7:4),TEXT("squirrels collect real food or advance real course checkpoints"));
        AWildlineCharacter* Hunter=nullptr;AWildlineCharacter* Animal=nullptr;
        for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(It->bBot){if(!It->bSquirrel&&!Hunter)Hunter=*It;if(It->bSquirrel&&!Animal)Animal=*It;}
        if(!Hunter||!Animal){Check(false,TEXT("perception fixtures exist"));return;}
        const FVector Origin(6500,5500,Forest->GroundHeight(6500,5500)+120);
        Hunter->SetActorLocation(Origin);Animal->SetActorLocation(Origin+FVector(500,0,0));
        Check(Hunter->BotCanSee(Animal,2000),TEXT("hunter perceives an unobstructed opponent"));
        auto* Wall=GetWorld()->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Box);Box->SetBoxExtent(FVector(30,200,250));Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();Wall->SetActorLocation(Origin+FVector(250,0,0));
        Check(!Hunter->BotCanSee(Animal,2000),TEXT("solid cover blocks AI sight"));
        Hunter->ResetBotAwareness();Hunter->BotTarget=Animal;Hunter->BotLastKnown=Origin+FVector(450,0,0);Hunter->BotLastSeenAt=Hunter->ServerTime();const FVector Remembered=Hunter->BotLastKnown;Hunter->PlanBot(Hunter->ServerTime());
        Check(Hunter->BotLastKnown.Equals(Remembered),TEXT("search uses last seen position without tracking through cover"));Wall->Destroy();
        // Force a genuine blocked climb; the production timeout must release it.
        if(!Forest->bResidential&&Forest->Trees.Num()>0){
            auto* S=GetWorld()->GetGameState<AWildlineGameState>();S->MatchConfig.bBotClimbing=true;S->MatchConfig.BotTreeSeconds=2;
            const auto& Tree=Forest->Trees[0];Animal->ResetBotAwareness();Animal->TreeIndex=0;Animal->bWoodClimbing=false;Animal->SetActorLocation(Tree.Base+FVector(Tree.Radius+13,0,100));
            Animal->GetCharacterMovement()->SetMovementMode(MOVE_Flying);Animal->Stamina=100;Animal->SetActorTickEnabled(true);
        }
    });
    At(96,[this,Check,Forest,Settings,Mode](){
        if(!Forest->bResidential){
            AWildlineCharacter* Animal=nullptr;for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(It->bBot&&It->bSquirrel){Animal=*It;break;}
            Check(Animal&&Animal->TreeIndex==INDEX_NONE&&Animal->BotTreeCooldown>Animal->ServerTime(),TEXT("tree deadline releases squirrel and prevents immediate reattachment"));if(Animal)Animal->SetActorTickEnabled(false);
        }
        GetGameInstance<UWildlineGameInstance>()->PreferredMode=Mode;SetMenu(true);Menu->Mode->SetSelectedIndex(Mode);Menu->ShowBots();Menu->RefreshConfig();
        Menu->BotSight->SetValue(75);Menu->BotReaction->SetValue(85);Menu->BotAccuracy->SetValue(75);Menu->HunterAggression->SetValue(90);Menu->SquirrelCaution->SetValue(80);Menu->BotSpacing->SetValue(260);Menu->BotTreeSeconds->SetValue(4);Menu->BotClimbing->SetIsChecked(false);Menu->ConfigChanged(0);Settings->LoadSettings(true);
        const auto& C=Settings->ConfigForMode(Mode);
        Check(FMath::IsNearlyEqual(C.BotSightScale,.75f)&&C.BotReactionScale==.85f&&C.BotAccuracyScale==.75f&&FMath::IsNearlyEqual(C.HunterAggression,.9f)&&FMath::IsNearlyEqual(C.SquirrelCaution,.8f)&&C.BotSpacing==260&&C.BotTreeSeconds==4&&!C.bBotClimbing,TEXT("all eight AI controls save and reload through the real menu"));
    });
    if(!FParse::Param(FCommandLine::Get(),TEXT("NullRHI"))){
        At(98,[](){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/AIBehavior.png"),true,false);});
        At(100,[this](){if(auto* Scroll=Cast<UScrollBox>(Menu->Pages->GetActiveWidget()))Scroll->ScrollToEnd();});
        At(102,[](){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/AIRoster.png"),true,false);});
    }
    At(104,[Passed,Settings,Mode,Original](){Settings->ConfigForMode(Mode)=Original;Settings->Persist();
        FFileHelper::SaveStringToFile(*Passed?TEXT("PASS\n"):TEXT("FAIL\n"),*(FPaths::ProjectSavedDir()/TEXT("AIReviewResult.txt")));
        UE_LOG(LogTemp,Display,TEXT("WILDLINE AI REVIEW: %s"),*Passed?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Passed?0:1);
    });
}

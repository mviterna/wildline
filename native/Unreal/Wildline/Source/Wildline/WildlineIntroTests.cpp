#include "WildlineGame.h"
#include "WildlineIntro.h"
#include "WildlineMenu.h"
#include "WildlineUserSettings.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Blueprint/WidgetTree.h"
#include "Components/MeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/CanvasPanelSlot.h"
#include "Sound/SoundWave.h"
#include "TimerManager.h"

void AWildlineController::RunIntroSmokeTest(){
    auto Ok=MakeShared<bool>(true);auto NutPosition=MakeShared<FVector2D>();auto NutSize=MakeShared<FVector2D>();const TWeakObjectPtr<APawn> Original=GetPawn();const TWeakObjectPtr<AActor> OriginalView=GetViewTarget();
    auto Check=[Ok](bool Value,const TCHAR* Name){*Ok&=Value;UE_LOG(LogTemp,Display,TEXT("INTRO CHECK %s: %s"),Name,Value?TEXT("PASS"):TEXT("FAIL"));};
    auto At=[this](float Delay,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Delay,false);};
    At(1,[this,Check,Original,OriginalView,NutPosition,NutSize](){
        Check(Intro&&Intro->Overlay&&Intro->Overlay->Logo&&FMath::IsNearlyEqual(Intro->Duration,6.572f,.001f),TEXT("shortened 2D studio ident created"));
        Check(GetPawn()==Original.Get()&&GetViewTarget()==OriginalView.Get()&&bMenuOpen&&Menu->GetVisibility()==ESlateVisibility::Collapsed,TEXT("gameplay blocked without possession or camera change"));
        bool NoModels=Intro!=nullptr;if(Intro){TArray<UMeshComponent*> Meshes;Intro->GetComponents(Meshes);NoModels=Meshes.IsEmpty();for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->GetOwner()==Intro)NoModels=false;}
        Check(NoModels,TEXT("screen space intro uses no game models or scene actors"));
        auto* LogoTexture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Wildline/Presentation/T_StudioLogo.T_StudioLogo"));
        Check(LogoTexture&&LogoTexture->GetImportedSize().X>=1024,TEXT("avatar studio logo cooked"));
        Check(Intro&&Intro->Overlay->Atlas&&Intro->Overlay->Squirrel&&Intro->Overlay->Acorn&&Intro->Overlay->CurrentFrame>=0&&Intro->Overlay->CurrentFrame<8,TEXT("2D squirrel run cycle and acorn present"));
        if(Intro&&Intro->Overlay->Acorn){auto* Slot=CastChecked<UCanvasPanelSlot>(Intro->Overlay->Acorn->Slot);*NutPosition=Slot->GetPosition();*NutSize=Slot->GetSize();}
        auto* Poster=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Wildline/Presentation/T_MenuPoster.T_MenuPoster"));
        Check(Poster&&Poster->GetImportedSize()==FIntPoint(1024,1536),TEXT("full supplied poster cooked"));
        auto* Sound=LoadObject<USoundWave>(nullptr,TEXT("/Game/Wildline/Presentation/SW_MenuRap.SW_MenuRap"));
        Check(Sound&&Sound->bLooping&&FMath::IsNearlyEqual(Sound->Duration,140.f,.01f),TEXT("supplied 140-second rap soundtrack cooked"));
        Check(LoadObject<USoundWave>(nullptr,TEXT("/Game/Wildline/Presentation/SW_MenuBluegrass.SW_MenuBluegrass"))!=nullptr,TEXT("previous music asset retained"));
        bool NoReplay=true;TArray<UWidget*> Widgets;Menu->WidgetTree->GetAllWidgets(Widgets);for(auto* W:Widgets)if(auto* Text=Cast<UTextBlock>(W);Text&&Text->GetText().ToString().Contains(TEXT("WATCH INTRO")))NoReplay=false;
        Check(NoReplay,TEXT("menu has no intro replay item"));
    });
    At(FWildlineIntroTiming::ShotAt-.35f,[this,Check](){Check(Intro&&Intro->Overlay->CurrentFrame>=0&&Intro->Overlay->CurrentFrame<8,TEXT("run continues without reaching or pickup pose"));});
    At(FWildlineIntroTiming::ShotAt-.1f,[this,Check](){bool BeforeNut=Intro&&!Intro->bShot&&Intro->Overlay->CurrentFrame<8;if(BeforeNut){auto* S=CastChecked<UCanvasPanelSlot>(Intro->Overlay->Squirrel->Slot);auto* N=CastChecked<UCanvasPanelSlot>(Intro->Overlay->Acorn->Slot);BeforeNut=S->GetPosition().X+S->GetSize().X<N->GetPosition().X;}Check(BeforeNut,TEXT("squirrel is still short of the nut before shot"));});
    At(FWildlineIntroTiming::ShotAt+.15f,[this,Check,NutPosition,NutSize](){bool Untouched=Intro&&Intro->bShot&&Intro->Overlay->CurrentFrame==10;if(Untouched){auto* N=CastChecked<UCanvasPanelSlot>(Intro->Overlay->Acorn->Slot);Untouched=N->GetPosition().Equals(*NutPosition)&&N->GetSize().Equals(*NutSize)&&Intro->Overlay->Acorn->GetRenderTransform().Angle==0;}Check(Untouched,TEXT("shot triggers tumble while nut stays untouched on ground"));});
    At(FWildlineIntroTiming::SceneEnd+.5f,[this,Check](){Check(Intro&&Intro->Overlay->Squirrel->GetRenderOpacity()==0&&Intro->Overlay->Acorn->GetRenderOpacity()==0&&Intro->Overlay->LogoBox->GetRenderOpacity()==0&&!Intro->bMusic,TEXT("one second quiet pause before logo"));});
    At(FWildlineIntroTiming::LogoAt+2.3f,[this,Check](){Check(Intro&&Intro->Overlay->LogoBox->GetRenderOpacity()>=.99f,TEXT("logo remains fully visible beyond previous duration"));Check(FMath::IsNearlyEqual(CurrentMusicVolume,UWildlineUserSettings::Get()->MusicVolume*.7f,.01f),TEXT("studio music reaches intro volume"));});
    At(AWildlineIntro::Duration+.7f,[this,Check,Original,OriginalView](){
        Check(!Intro&&GetPawn()==Original.Get()&&GetViewTarget()==OriginalView.Get()&&bMenuOpen&&Menu->GetVisibility()==ESlateVisibility::Visible&&Menu->GetRenderOpacity()>=.99f,TEXT("completion restores visible menu and camera"));
        int Count=0;for(TActorIterator<AWildlineIntro> It(GetWorld());It;++It)Count++;Check(Count==0,TEXT("studio ident cleaned up"));
        auto* State=GetWorld()->GetGameState<AWildlineGameState>();Check(State&&!State->bRoundStarted&&State->HunterKills==0,TEXT("intro cannot start or score the match"));
        Check(GetGameInstance<UWildlineGameInstance>()->bIntroPlayed,TEXT("once per session survives travel"));
        Menu->ShowSettings();Check(Menu->bWorkshopOpen&&Menu->MusicVolume,TEXT("workshop and music control accessible"));Menu->SetWorkshop(false);
    });
    At(AWildlineIntro::Duration+1.7f,[this,Check](){Check(!Intro&&FMath::IsNearlyEqual(CurrentMusicVolume,UWildlineUserSettings::Get()->MusicVolume*.7f*.6f,.01f),TEXT("main menu music fades forty percent below intro volume"));StartIntro();});
    // Exercise the internal skip path; the menu does not expose a replay action.
    At(AWildlineIntro::Duration+2.2f,[this,Check](){
        Check(Intro!=nullptr,TEXT("skip path ready"));if(!Intro)return;
        for(FKey Key:{EKeys::SpaceBar,EKeys::Enter}){
            FKeyEvent Event(Key,FModifierKeysState(),0,false,0,0);
            Intro->Overlay->NativeOnKeyDown(FGeometry(),Event);
            Check(Intro!=nullptr,TEXT("Space and Enter leave intro playing"));
        }
        bool EscHint=false;TArray<UWidget*> Widgets;Intro->Overlay->WidgetTree->GetAllWidgets(Widgets);
        for(auto* W:Widgets)if(auto* Text=Cast<UTextBlock>(W))EscHint|=Text->GetText().ToString()==TEXT("SKIP  /  ESC");
        Check(EscHint,TEXT("intro advertises Esc to skip"));
        FKeyEvent Escape(EKeys::Escape,FModifierKeysState(),0,false,0,0);
        Intro->Overlay->NativeOnKeyDown(FGeometry(),Escape);
        Check(!Intro,TEXT("Esc key skips intro immediately"));
    });
    At(AWildlineIntro::Duration+2.4f,[this,Check](){StartIntro();Check(Intro&&!PlayerCameraManager->bEnableFading,TEXT("restart clears transition blackout"));if(Intro)Intro->Overlay->Skip();});
    At(AWildlineIntro::Duration+3.f,[this,Check](){
        Check(!Intro&&bMenuOpen&&Menu->GetVisibility()==ESlateVisibility::Visible,TEXT("skip returns immediately to menu"));
        ChooseRole(false);Check(!bMenuOpen&&!Intro,TEXT("play starts after intro and skip"));
    });
    At(AWildlineIntro::Duration+3.2f,[this,Check,Ok](){Check(!PlayerCameraManager->bEnableFading,TEXT("play clears transition blackout"));UE_LOG(LogTemp,Display,TEXT("WILDLINE INTRO SMOKE: %s"),*Ok?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Ok?0:1);});
}

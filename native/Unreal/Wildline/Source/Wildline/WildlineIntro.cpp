#include "WildlineIntro.h"
#include "WildlineGame.h"
#include "WildlineMenu.h"
#include "WildlineAudio.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Engine/Texture2D.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "UnrealClient.h"

namespace {
constexpr float ShotX=580.f;
float Ease(float T){T=FMath::Clamp(T,0.f,1.f);return T*T*(3-2*T);}
void SpriteFrame(UImage* Image,int32 Frame,FVector2D Origin,float Size){
    // Each cel has independent bounds: a few paws/tails cross the atlas grid.
    // Preserve the original cell coordinates while excluding adjacent artwork.
    static const FIntRect Bounds[]={
        {36,75,370,338},{380,82,709,342},{725,71,1058,342},{1070,68,1421,282},
        {34,410,357,690},{383,410,714,688},{733,408,1080,690},{1100,396,1421,656},
        {29,740,325,1024},{388,746,715,1023},{734,745,1073,1024},{1185,795,1349,1010}
    };
    const auto& Rect=Bounds[Frame];const FVector2D Min(Rect.Min),Max(Rect.Max),Cell((Frame%4)*362,(Frame/4)*362);
    FSlateBrush Brush=Image->GetBrush();
    Brush.SetUVRegion(FBox2f(FVector2f((Min+FVector2D(.5f,.5f))/FVector2D(1448,1086)),FVector2f((Max-FVector2D(.5f,.5f))/FVector2D(1448,1086))));Image->SetBrush(Brush);
    auto* Slot=CastChecked<UCanvasPanelSlot>(Image->Slot);Slot->SetPosition(Origin+(Min-Cell)*(Size/362));Slot->SetSize((Max-Min)*(Size/362));
    Image->SetRenderTransformPivot((FVector2D(.55f,.65f)*362-(Min-Cell))/(Max-Min));
}
void ExportReviewShot(){
    const auto PCM=WildlineAudio::Samples(WildlineAudio::Rifle);const uint32 Bytes=PCM.Num()*sizeof(int16),RiffSize=36+Bytes;
    const uint8 Header[]={'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,1,0,1,0,34,86,0,0,68,172,0,0,2,0,16,0,'d','a','t','a',0,0,0,0};
    TArray<uint8> Wave;Wave.Append(Header,UE_ARRAY_COUNT(Header));FMemory::Memcpy(Wave.GetData()+4,&RiffSize,4);FMemory::Memcpy(Wave.GetData()+40,&Bytes,4);Wave.Append(reinterpret_cast<const uint8*>(PCM.GetData()),Bytes);
    FFileHelper::SaveArrayToFile(Wave,*(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/IntroFilm/Shot.wav")));
    const FString Timing=FString::Printf(TEXT("{\"duration\": %.6f, \"shotAt\": %.6f, \"musicAt\": %.6f, \"fps\": %d, \"frames\": %d}"),FWildlineIntroTiming::End,FWildlineIntroTiming::ShotAt,FWildlineIntroTiming::LogoAt,FWildlineIntroTiming::FilmFPS,FWildlineIntroTiming::FilmFrames);
    FFileHelper::SaveStringToFile(Timing,*(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/IntroFilm/Timing.json")));
}
}

TSharedRef<SWidget> UWildlineIntroOverlay::RebuildWidget(){
    if(WidgetTree->RootWidget)return Super::RebuildWidget();SetIsFocusable(true);
    auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Root;
    auto Place=[&](UWidget* W,FAnchors A,FMargin M){auto* S=Root->AddChildToCanvas(W);S->SetAnchors(A);S->SetOffsets(M);return S;};
    auto* Black=WidgetTree->ConstructWidget<UBorder>();Black->SetBrushColor(FLinearColor(.002f,.003f,.004f));Black->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Black,FAnchors(0,0,1,1),FMargin(0));
    Atlas=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Wildline/Presentation/T_IntroSquirrelAtlas.T_IntroSquirrelAtlas"));
    Squirrel=WidgetTree->ConstructWidget<UImage>();Squirrel->SetBrushFromTexture(Atlas);Squirrel->SetVisibility(ESlateVisibility::HitTestInvisible);Squirrel->SetRenderTransformPivot(FVector2D(.55f,.65f));Place(Squirrel,FAnchors(0,0),FMargin(100,274,420,420))->SetZOrder(1);
    Acorn=WidgetTree->ConstructWidget<UImage>();Acorn->SetBrushFromTexture(Atlas);Acorn->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Acorn,FAnchors(0,0),FMargin(1000,457,220,220));
    LogoBox=WidgetTree->ConstructWidget<UScaleBox>();LogoBox->SetStretch(EStretch::ScaleToFit);LogoBox->SetVisibility(ESlateVisibility::HitTestInvisible);LogoBox->SetRenderTransformPivot(FVector2D(.5f,.5f));
    Logo=WidgetTree->ConstructWidget<UImage>();Logo->SetBrushFromTexture(LoadObject<UTexture2D>(nullptr,TEXT("/Game/Wildline/Presentation/T_StudioLogo.T_StudioLogo")),true);LogoBox->AddChild(Logo);Place(LogoBox,FAnchors(.20f,.08f,.80f,.82f),FMargin(0));
    Presents=WidgetTree->ConstructWidget<UTextBlock>();Presents->SetText(FText::FromString(TEXT("P R E S E N T S")));auto Font=Presents->GetFont();Font.Size=11;Presents->SetFont(Font);Presents->SetColorAndOpacity(FLinearColor(.64f,.43f,.18f));Presents->SetJustification(ETextJustify::Center);Presents->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Presents,FAnchors(.25f,.84f,.75f,.89f),FMargin(0));
    auto* SkipButton=WidgetTree->ConstructWidget<UButton>();auto Style=SkipButton->GetStyle();Style.Normal.TintColor=FSlateColor(FLinearColor(0,0,0,0));Style.Hovered.TintColor=FSlateColor(FLinearColor(.04f,.03f,.01f));SkipButton->SetStyle(Style);
    auto* Hint=WidgetTree->ConstructWidget<UTextBlock>();Hint->SetText(FText::FromString(TEXT("SKIP  /  ESC")));auto HintFont=Hint->GetFont();HintFont.Size=10;Hint->SetFont(HintFont);Hint->SetColorAndOpacity(FLinearColor(.42f,.38f,.29f));Hint->SetMargin(FMargin(12,8));SkipButton->AddChild(Hint);SkipButton->OnClicked.AddDynamic(this,&UWildlineIntroOverlay::Skip);Place(SkipButton,FAnchors(1,1),FMargin(-180,-50,150,30));
    Update(0);return Super::RebuildWidget();
}
void UWildlineIntroOverlay::Update(float T){
    AnimationTime=T;if(!LogoBox)return;
    const float In=Ease((T-FWildlineIntroTiming::LogoAt)/FWildlineIntroTiming::FadeIn),Out=1-Ease((T-(FWildlineIntroTiming::End-FWildlineIntroTiming::FadeOut))/FWildlineIntroTiming::FadeOut);
    LogoBox->SetRenderOpacity(In*Out);
    const float SettleAge=T-(FWildlineIntroTiming::LogoAt+FWildlineIntroTiming::FadeIn);
    const float Settle=SettleAge>0?.015f*FMath::Sin(SettleAge*7)*FMath::Exp(-SettleAge*3):0;
    LogoBox->SetRenderScale(FVector2D(FMath::Lerp(.90f,1.f,In)+Settle));
    LogoBox->SetRenderTranslation(FVector2D(0,(1-In)*15));
    Presents->SetRenderOpacity(Ease((T-(FWildlineIntroTiming::LogoAt+.52f))/.25f)*Out);
    const FVector2D Size=GetCachedGeometry().GetLocalSize();
    const float Scale=Size.X>0&&Size.Y>0?FMath::Min(static_cast<float>(Size.X/1600),static_cast<float>(Size.Y/900)):1.f;
    const FVector2D Origin=Size.X>0?(Size-FVector2D(1600,900)*Scale)*.5f:FVector2D::ZeroVector;
    float X=100,Y=319,Angle=0;
    CurrentFrame=8;
    if(T>=FWildlineIntroTiming::RunAt&&T<FWildlineIntroTiming::ShotAt){const float Run=FMath::Clamp((T-FWildlineIntroTiming::RunAt)/(FWildlineIntroTiming::ShotAt-FWildlineIntroTiming::RunAt),0.f,1.f);X=100+(ShotX-100)*Run;CurrentFrame=FMath::FloorToInt((T-FWildlineIntroTiming::RunAt)*18)%8;const float Base[]={.92f,.934f,.925f,.92f,.898f,.892f,.892f,.92f};Y=660-420*Base[CurrentFrame];Y-=FMath::Max(0.f,FMath::Sin((T-FWildlineIntroTiming::RunAt)*18*PI/4))*8;}
    else if(T>=FWildlineIntroTiming::ShotAt){const float U=FMath::Clamp((T-FWildlineIntroTiming::ShotAt)/FWildlineIntroTiming::TumbleDuration,0.f,1.f);X=ShotX-600*U;Y=312-210*FMath::Sin(U*PI)+500*U*U;Angle=-U*540;CurrentFrame=10;}
    SpriteFrame(Squirrel,CurrentFrame,Origin+FVector2D(X,Y)*Scale,420*Scale);Squirrel->SetRenderTransformAngle(Angle);Squirrel->SetRenderOpacity(Ease(T/.2f)*(1-Ease((T-(FWildlineIntroTiming::SceneEnd-.17f))/.17f)));
    // The untouched nut stays on the ground while the shot interrupts the run.
    SpriteFrame(Acorn,11,Origin+FVector2D(1000,488)*Scale,220*Scale);
    Acorn->SetRenderOpacity(Ease(T/.25f)*(1-Ease((T-(FWildlineIntroTiming::SceneEnd-.17f))/.17f)));
}
int32 UWildlineIntroOverlay::NativePaint(const FPaintArgs& Args,const FGeometry& G,const FSlateRect& Culling,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool Enabled) const{
    const int32 Top=Super::NativePaint(Args,G,Culling,Elements,Layer,Style,Enabled);
    const auto Size=G.GetLocalSize();const float Scale=FMath::Min(static_cast<float>(Size.X/1600),static_cast<float>(Size.Y/900));const FVector2D Origin=(Size-FVector2D(1600,900)*Scale)*.5f;
    auto Line=[&](FVector2D A,FVector2D B,FLinearColor Color,float Width){TArray<FVector2D> Points;Points.Add(Origin+A*Scale);Points.Add(Origin+B*Scale);FSlateDrawElement::MakeLines(Elements,Top+1,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,Color,true,Width*Scale);};
    const float StoryAlpha=Ease(AnimationTime/.2f)*(1-Ease((AnimationTime-(FWildlineIntroTiming::SceneEnd-.2f))/.2f));
    Line(FVector2D(110,660),FVector2D(1470,660),FLinearColor(.65f,.39f,.12f,StoryAlpha*.18f),1);
    if(AnimationTime>=FWildlineIntroTiming::RunAt&&AnimationTime<FWildlineIntroTiming::ShotAt){const float X=100+(ShotX-100)*(AnimationTime-FWildlineIntroTiming::RunAt)/(FWildlineIntroTiming::ShotAt-FWildlineIntroTiming::RunAt);for(int I=0;I<3;I++)Line(FVector2D(X-80-I*30,575+I*14),FVector2D(X-18,575+I*14),FLinearColor(.75f,.45f,.15f,.12f),1.5f);}
    const float ShotAge=AnimationTime-FWildlineIntroTiming::ShotAt;
    if(ShotAge>=0&&ShotAge<.22f){const float A=1-ShotAge/.22f;const FVector2D Center(ShotX+247,490);for(int I=0;I<12;I++){const float R=I*2*PI/12;const FVector2D Dir(FMath::Cos(R),FMath::Sin(R));Line(Center+Dir*30,Center+Dir*(75+ShotAge*300),FLinearColor(1,.69f,.16f,A),I%2?2:4);}Line(FVector2D(1520,490),Center+FVector2D(115,0),FLinearColor(1,.84f,.47f,A*.6f),2);}
    // Small 2D embers accompany the flaming studio emblem.
    const float Alpha=Ease((AnimationTime-(FWildlineIntroTiming::LogoAt+.07f))/.3f)*(1-Ease((AnimationTime-(FWildlineIntroTiming::End-FWildlineIntroTiming::FadeOut))/FWildlineIntroTiming::FadeOut));
    for(int I=0;I<10;I++){
        const float Age=FMath::Fmod(AnimationTime*.55f+I*.173f,1.f);
        const FVector2D P(Size.X*(.32f+.36f*I/9.f)+FMath::Sin(I*3.1f+AnimationTime)*7,Size.Y*(.78f-Age*.55f));
        const float A=Alpha*FMath::Sin(Age*PI)*.3f;
        FSlateDrawElement::MakeBox(Elements,Top+1,G.ToPaintGeometry(FVector2D(1.5f,3.f),FSlateLayoutTransform(P)),FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")),ESlateDrawEffect::None,FLinearColor(1,.29f,.025f,A));
    }
    return Top+1;
}
void UWildlineIntroOverlay::Skip(){if(auto* PC=Cast<AWildlineController>(GetOwningPlayer());PC&&PC->Intro)PC->Intro->Finish();}
FReply UWildlineIntroOverlay::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E){if(E.GetKey()==EKeys::Escape){Skip();return FReply::Handled();}return Super::NativeOnKeyDown(G,E);}
AWildlineIntro::AWildlineIntro(){PrimaryActorTick.bCanEverTick=true;bReplicates=false;}
void AWildlineIntro::BeginPlay(){
    Super::BeginPlay();bPreviousScreenMessages=GAreScreenMessagesEnabled;GAreScreenMessagesEnabled=false;bReview=FParse::Param(FCommandLine::Get(),TEXT("WildlineIntroReview"));
    bFilm=FParse::Param(FCommandLine::Get(),TEXT("WildlineIntroFilm"));if(bFilm){bPriorFixedStep=FApp::UseFixedTimeStep();FApp::SetUseFixedTimeStep(true);FApp::SetFixedDeltaTime(1./FWildlineIntroTiming::FilmFPS);IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/IntroFilm")),true);ExportReviewShot();}
    if(!Viewer){Finish();return;}
    Overlay=CreateWidget<UWildlineIntroOverlay>(Viewer);if(!Overlay){Finish();return;}Overlay->AddToViewport(100);
    FInputModeUIOnly Input;Input.SetWidgetToFocus(Overlay->TakeWidget());Viewer->SetInputMode(Input);Viewer->bShowMouseCursor=true;
    UE_LOG(LogTemp,Display,TEXT("WILDLINE INTRO: hot_turd / 2D squirrel short / %.3f seconds / no game models"),Duration);
}
void AWildlineIntro::Tick(float Delta){
    Super::Tick(Delta);if(bFinished)return;Elapsed+=Delta;if(Overlay)Overlay->Update(Elapsed);
    if(!bShot&&Elapsed>=FWildlineIntroTiming::ShotAt){bShot=true;if(!FParse::Param(FCommandLine::Get(),TEXT("NoSound")))WildlineAudio::Play(this,WildlineAudio::Rifle,FVector::ZeroVector,true);UE_LOG(LogTemp,Display,TEXT("WILDLINE INTRO: 2D comic shot and tumble"));}
    if(!bMusic&&Elapsed>=FWildlineIntroTiming::LogoAt){bMusic=true;if(Viewer)Viewer->StartMenuMusic();}
    if(bFilm&&FilmFrame<FWildlineIntroTiming::FilmFrames){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/IntroFilm/")+FString::Printf(TEXT("Frame%04d.png"),FilmFrame++),true,false);}
    else if(bReview&&!bFilm){const float Times[]={.35f,1.10f,FWildlineIntroTiming::ShotAt-.10f,FWildlineIntroTiming::ShotAt+.04f,FWildlineIntroTiming::ShotAt+.29f,FWildlineIntroTiming::SceneEnd+.5f,FWildlineIntroTiming::LogoAt+.9f,FWildlineIntroTiming::End-.2f};const TCHAR* Names[]={TEXT("Intro2DNotice"),TEXT("Intro2DRun"),TEXT("Intro2DBeforeShot"),TEXT("Intro2DShot"),TEXT("Intro2DTumble"),TEXT("Intro2DPause"),TEXT("StudioLogo"),TEXT("StudioFade")};if(ReviewShot<UE_ARRAY_COUNT(Times)&&Elapsed>=Times[ReviewShot]){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Names[ReviewShot]+TEXT(".png"),true,false);ReviewShot++;}}
    if(Elapsed>=Duration)Finish();
}
void AWildlineIntro::Finish(){
    if(bFinished)return;bFinished=true;
    if(Viewer){auto* PC=Viewer;PC->Intro=nullptr;if(Overlay)Overlay->RemoveFromParent();PC->PlayerCameraManager->StartCameraFade(1,1,1,FLinearColor::Black,false,true);PC->SetMenu(true);if(PC->Menu)PC->Menu->SetRenderOpacity(0);PC->MenuRevealAt=GetWorld()->GetTimeSeconds();}
    UE_LOG(LogTemp,Display,TEXT("WILDLINE INTRO: finished / hot_turd studio / menu restored"));Destroy();
}
void AWildlineIntro::EndPlay(const EEndPlayReason::Type Reason){if(bFilm)FApp::SetUseFixedTimeStep(bPriorFixedStep);GAreScreenMessagesEnabled=bPreviousScreenMessages;if(Overlay)Overlay->RemoveFromParent();Super::EndPlay(Reason);}

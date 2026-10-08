#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "WildlineIntro.generated.h"

class AWildlineController;
class UTextBlock;
class UImage;
class UScaleBox;

struct FWildlineIntroTiming {
    static constexpr float RunAt=.55f,ShotAt=1.90f,TumbleDuration=.60f;
    static constexpr float SceneEnd=ShotAt+TumbleDuration,LogoPause=1.f,LogoAt=SceneEnd+LogoPause;
    static constexpr float LogoDuration=1.92f*1.6f,FadeIn=.45f,FadeOut=.45f,End=LogoAt+LogoDuration;
    static constexpr int32 FilmFPS=30,FilmFrames=static_cast<int32>(End*FilmFPS)+1;
};

UCLASS()
class WILDLINE_API UWildlineIntroOverlay : public UUserWidget {
    GENERATED_BODY()
public:
    TSharedRef<SWidget> RebuildWidget() override;
    FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    int32 NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& CullingRect,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool Enabled) const override;
    UFUNCTION() void Skip();
    void Update(float Time);
    UPROPERTY() UImage* Logo;
    UPROPERTY() UScaleBox* LogoBox;
    UPROPERTY() UTextBlock* Presents;
    UPROPERTY() UImage* Squirrel;
    UPROPERTY() UImage* Acorn;
    UPROPERTY() class UTexture2D* Atlas;
    float AnimationTime=0;
    int32 CurrentFrame=8;
};

// A 2D squirrel short and studio ident. Only UI is animated; no game models.
UCLASS()
class WILDLINE_API AWildlineIntro : public AActor {
    GENERATED_BODY()
public:
    AWildlineIntro();
    void BeginPlay() override;
    void Tick(float Delta) override;
    void EndPlay(const EEndPlayReason::Type Reason) override;
    void Finish();
    static constexpr float Duration=FWildlineIntroTiming::End;
    UPROPERTY() AWildlineController* Viewer;
    UPROPERTY() UWildlineIntroOverlay* Overlay;
    float Elapsed=0;
    bool bMusic=false,bFinished=false,bShot=false;
private:
    bool bReview=false,bFilm=false,bPriorFixedStep=false;
    bool bPreviousScreenMessages=true;
    int32 ReviewShot=0,FilmFrame=0;
};

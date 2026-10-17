#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "WildlineHunterProfile.h"
#include "WildlineHunterCreator.generated.h"
class AWildlineCharacter;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UImage;
class UTextBlock;
class USlider;
class UCheckBox;
class USizeBox;
class UScrollBox;
UCLASS()
class WILDLINE_API UWildlineHunterCreator : public UUserWidget {
    GENERATED_BODY()
public:
    TSharedRef<SWidget> RebuildWidget() override;
    void NativeDestruct() override;
    FReply NativeOnKeyDown(const FGeometry&,const FKeyEvent&) override;
    FWildlineHunterProfile Pending;
    FString PhotoData;
    UPROPERTY() AWildlineCharacter* Preview;
    UPROPERTY() USceneCaptureComponent2D* Capture;
    UPROPERTY() UTextureRenderTarget2D* Target;
    UPROPERTY() UImage* PreviewImage;
    UPROPERTY() UImage* PhotoImage;
    UPROPERTY() USizeBox* PhotoFrame;
    UPROPERTY() UScrollBox* AppearanceScroll;
    UPROPERTY() UTextBlock* Message;
    UPROPERTY() UTexture2D* PhotoTexture;
    UPROPERTY() UComboBoxString* PresetCombo;
    UPROPERTY() UComboBoxString* BodyCombo;
    UPROPERTY() UComboBoxString* OutfitCombo;
    UPROPERTY() UComboBoxString* HairCombo;
    UPROPERTY() UCheckBox* CapCheck;
    UPROPERTY() UCheckBox* BeardCheck;
    UPROPERTY() TArray<USlider*> AppearanceSliders;
    bool bSyncing=false;
    bool bFaceView=false;
    float PreviewAngle=-16;
    void RefreshPreview();
    void UpdateCamera();
    bool ImportPortraitPath(const FString& Path);
    void SyncControls();
    void LoadTravisPreset();
    void LoadEthanPreset();
    void LoadGreysonPreset();
    void LoadMarkPreset();
    UFUNCTION() void PresetChanged(FString Item,ESelectInfo::Type Type);
    UFUNCTION() void ResetPreset();
    UFUNCTION() void BeardChanged(bool Value);
    UFUNCTION() void SlimnessChanged(float Value);
    UFUNCTION() void BodyChanged(FString Item,ESelectInfo::Type Type);
    UFUNCTION() void OutfitChanged(FString Item,ESelectInfo::Type Type);
    UFUNCTION() void HairChanged(FString Item,ESelectInfo::Type Type);
    UFUNCTION() void CapChanged(bool Value);
    UFUNCTION() void SkinChanged(float Value);
    UFUNCTION() void WidthChanged(float Value);
    UFUNCTION() void NoseChanged(float Value);
    UFUNCTION() void PhotoXChanged(float Value);
    UFUNCTION() void PhotoYChanged(float Value);
    UFUNCTION() void PhotoZoomChanged(float Value);
    UFUNCTION() void UploadPhoto();
    UFUNCTION() void ClearPhoto();
    UFUNCTION() void FullBody();
    UFUNCTION() void FaceView();
    UFUNCTION() void TurnLeft();
    UFUNCTION() void TurnRight();
    UFUNCTION() void SaveHunter();
    UFUNCTION() void Cancel();
};

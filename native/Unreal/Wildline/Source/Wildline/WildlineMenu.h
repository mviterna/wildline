#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "WildlineMenu.generated.h"
class UEditableTextBox;
class UCheckBox;
class USlider;
class USpinBox;
class UScrollBox;
class UTextBlock;
class UWildlineHunterCreator;
class UComboBoxString;
UCLASS()
class WILDLINE_API UWildlineMenu : public UUserWidget {
    GENERATED_BODY()
public:
    TSharedRef<SWidget> RebuildWidget() override;
    FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    UPROPERTY() UScrollBox* FieldScroll;
    void ScrollToSettings();
    UPROPERTY() UEditableTextBox* Address;
    UPROPERTY() UCheckBox* Inversion;
    UPROPERTY() UCheckBox* Blood;
    UPROPERTY() USlider* Sensitivity;
    UPROPERTY() USlider* ScopeSensitivity;
    UPROPERTY() USlider* SoundVolume;
    UPROPERTY() UComboBoxString* Resolution;
    TArray<FIntPoint> ResolutionOptions;
    UPROPERTY() USpinBox* PlayerLimit;
    UPROPERTY() USpinBox* RoundMinutes;
    UPROPERTY() USpinBox* SquirrelPoints;
    UPROPERTY() USpinBox* DefenderPoints;
    UPROPERTY() UTextBlock* SquirrelPointsLabel;
    bool bRefreshingConfig=false;
    void RefreshConfig();
    UFUNCTION() void ConfigChanged(float Value);
    UFUNCTION() void ScopeSensitivityChanged(float Value);
    UFUNCTION() void SoundVolumeChanged(float Value);
    UPROPERTY() UTextBlock* RoleDescription;
    UPROPERTY() UTextBlock* Status;
    UPROPERTY() UComboBoxString* Mode;
    UPROPERTY() UComboBoxString* Weapon;
    UPROPERTY() UTextBlock* ModeDescription;
    UPROPERTY() UTextBlock* DefenderLabel;
    UFUNCTION() void ModeChanged(FString Value,ESelectInfo::Type SelectionType);
    UFUNCTION() void WeaponChanged(FString Value,ESelectInfo::Type SelectionType);
    void RefreshMode();
    UPROPERTY() UComboBoxString* Difficulty;
    UPROPERTY() UTextBlock* DifficultyDescription;
    UFUNCTION() void DifficultyChanged(FString Value,ESelectInfo::Type SelectionType);
    void RefreshDifficulty();
    UPROPERTY() UWildlineHunterCreator* Creator;
    UFUNCTION() void CreateHunter();
    bool bSquirrelSelected=false;
    UFUNCTION() void Hunter();
    UFUNCTION() void Squirrel();
    UFUNCTION() void Play();
    UFUNCTION() void Host();
    UFUNCTION() void Join();
    UFUNCTION() void InvertChanged(bool Value);
    UFUNCTION() void BloodChanged(bool Value);
    UFUNCTION() void SensitivityChanged(float Value);
    UFUNCTION() void ResolutionChanged(FString Value,ESelectInfo::Type SelectionType);
    UFUNCTION() void Quit();
};

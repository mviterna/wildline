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
class UWidgetSwitcher;
class UButton;
class UBorder;
class UCanvasPanelSlot;
class UScaleBox;
class UWildlineMenu;
UCLASS()
class WILDLINE_API UWildlineChoice : public UComboBoxString {
    GENERATED_BODY()
public:
    UWildlineChoice();
    void SetMenuOpen(bool bOpen);
};
UCLASS()
class WILDLINE_API UWildlineBotRow : public UUserWidget {
    GENERATED_BODY()
public:
    UPROPERTY() UWildlineMenu* Menu;
    UPROPERTY() UComboBoxString* Weapon;
    UPROPERTY() UComboBoxString* Difficulty;
    int32 Index=0;
    bool bSquirrel=false;
    TSharedRef<SWidget> RebuildWidget() override;
    UFUNCTION() void Changed(FString Value,ESelectInfo::Type SelectionType);
};
UCLASS()
class WILDLINE_API UWildlineMenu : public UUserWidget {
    GENERATED_BODY()
public:
    TSharedRef<SWidget> RebuildWidget() override;
    FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    UPROPERTY() UBorder* Workshop;
    UPROPERTY() UCanvasPanelSlot* ExpeditionSlot;
    UPROPERTY() UScaleBox* MenuArtwork;
    UPROPERTY() USlider* MusicVolume;
    UPROPERTY() UButton* HunterChoice;
    UPROPERTY() UButton* SquirrelChoice;
    bool bWorkshopOpen=false;
    void SetWorkshop(bool Open);
    UFUNCTION() void ToggleWorkshop();

    UFUNCTION() void MusicVolumeChanged(float Value);
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
    UPROPERTY() USpinBox* DefenderBotCount;
    UPROPERTY() USpinBox* SquirrelBotCount;
    UPROPERTY() USpinBox* SquirrelHealth;
    UPROPERTY() USpinBox* RespawnSeconds;
    UPROPERTY() USpinBox* SpawnProtection;
    UPROPERTY() USpinBox* CarryCapacity;
    UPROPERTY() USpinBox* MovementPercent;
    UPROPERTY() USpinBox* ReloadPercent;
    UPROPERTY() USpinBox* ThermalSeconds;
    UPROPERTY() USpinBox* ThermalCooldown;
    UPROPERTY() UCheckBox* Minimap;
    UPROPERTY() UCheckBox* UnlimitedStamina;
    UPROPERTY() UCheckBox* InfiniteAmmo;
    UPROPERTY() UCheckBox* AllowThermal;
    UPROPERTY() UCheckBox* LivestockDamage;
    UPROPERTY() UComboBoxString* MapIntel;
    UPROPERTY() UComboBoxString* TimeWinner;
    UPROPERTY() UComboBoxString* Presets;
    UPROPERTY() UWidgetSwitcher* Pages;
    UPROPERTY() TArray<UButton*> TabButtons;
    UPROPERTY() UTextBlock* ConfigSummary;
    UPROPERTY() TArray<UWildlineBotRow*> BotRows;
    void SaveBot(UWildlineBotRow* Row);
    UFUNCTION() void RulesChecked(bool Value);
    UFUNCTION() void RuleSelected(FString Value,ESelectInfo::Type SelectionType);
    UFUNCTION() void PresetSelected(FString Value,ESelectInfo::Type SelectionType);
    UFUNCTION() void ShowRules();
    UFUNCTION() void ShowBots();
    UFUNCTION() void ShowSettings();
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

#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "WildlineGraphics.generated.h"
class USlider;
class UCheckBox;
class UTextBlock;
UCLASS()
class WILDLINE_API UWildlineGraphics : public UUserWidget {
    GENERATED_BODY()
public:
    TSharedRef<SWidget> RebuildWidget() override;
    UPROPERTY() UComboBoxString* Preset;
    UPROPERTY() UComboBoxString* AA;
    UPROPERTY() UComboBoxString* FrameCap;
    UPROPERTY() TArray<UComboBoxString*> Quality;
    UPROPERTY() USlider* RenderScale;
    UPROPERTY() USlider* Sharpness;
    UPROPERTY() UCheckBox* VSync;
    UPROPERTY() UCheckBox* MotionBlur;
    UPROPERTY() UTextBlock* Status;
    UPROPERTY() UTextBlock* ScaleLabel;
    bool bSyncing=false;
    void Sync();
    UFUNCTION() void Apply();
    UFUNCTION() void Defaults();
    UFUNCTION() void PresetChanged(FString Value,ESelectInfo::Type Type);
    UFUNCTION() void QualityChanged(FString Value,ESelectInfo::Type Type);
    UFUNCTION() void ScaleChanged(float Value);
};

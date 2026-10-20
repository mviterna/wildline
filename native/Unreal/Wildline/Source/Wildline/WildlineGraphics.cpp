#include "WildlineGraphics.h"
#include "WildlineMenu.h"
#include "WildlineUserSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"

TSharedRef<SWidget> UWildlineGraphics::RebuildWidget(){
    if(WidgetTree->RootWidget)return Super::RebuildWidget();
    auto* Root=WidgetTree->ConstructWidget<UVerticalBox>();WidgetTree->RootWidget=Root;
    auto Add=[&](UWidget* W){Root->AddChildToVerticalBox(W)->SetPadding(FMargin(0,0,0,10));};
    auto Text=[&](const FString& S,int Size=11){auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(S));T->SetColorAndOpacity(FLinearColor(.94f,.91f,.8f));auto Font=T->GetFont();Font.Size=Size;T->SetFont(Font);T->SetAutoWrapText(true);return T;};
    auto Choice=[&](const FString& Label,const TArray<FString>& Options){Add(Text(Label));auto* C=WidgetTree->ConstructWidget<UWildlineChoice>();for(const auto& O:Options)C->AddOption(O);Add(C);return C;};
    Add(Text(TEXT("GRAPHICS & PERFORMANCE"),18));
    Preset=Choice(TEXT("QUALITY PRESET"),{TEXT("Low"),TEXT("Medium"),TEXT("High"),TEXT("Epic"),TEXT("Cinematic"),TEXT("Custom")});Preset->OnSelectionChanged.AddDynamic(this,&UWildlineGraphics::PresetChanged);
    AA=Choice(TEXT("ANTI-ALIASING METHOD"),{TEXT("Off"),TEXT("FXAA / fast"),TEXT("TAA / temporal"),TEXT("TSR / temporal super resolution")});
    for(const TCHAR* Label:{TEXT("ANTI-ALIASING QUALITY"),TEXT("VIEW DISTANCE"),TEXT("TEXTURES"),TEXT("SHADOWS"),TEXT("GLOBAL ILLUMINATION"),TEXT("REFLECTIONS"),TEXT("EFFECTS"),TEXT("POST PROCESSING"),TEXT("FOLIAGE"),TEXT("SHADING"),TEXT("LANDSCAPE")}){auto* C=Choice(Label,{TEXT("Low"),TEXT("Medium"),TEXT("High"),TEXT("Epic"),TEXT("Cinematic")});Quality.Add(C);C->OnSelectionChanged.AddDynamic(this,&UWildlineGraphics::QualityChanged);}
    ScaleLabel=Text(TEXT("RENDER SCALE"));Add(ScaleLabel);RenderScale=WidgetTree->ConstructWidget<USlider>();RenderScale->SetMinValue(50);RenderScale->SetMaxValue(100);RenderScale->SetStepSize(1);RenderScale->OnValueChanged.AddDynamic(this,&UWildlineGraphics::ScaleChanged);Add(RenderScale);
    Add(Text(TEXT("Output resolution stays at the display setting above. TSR reconstructs lower render scales; 100% renders at native resolution.")));
    Add(Text(TEXT("IMAGE SHARPNESS")));Sharpness=WidgetTree->ConstructWidget<USlider>();Sharpness->SetMinValue(0);Sharpness->SetMaxValue(2);Add(Sharpness);
    FrameCap=Choice(TEXT("FRAME RATE LIMIT"),{TEXT("Unlimited"),TEXT("30 FPS"),TEXT("60 FPS"),TEXT("90 FPS"),TEXT("120 FPS"),TEXT("144 FPS"),TEXT("165 FPS"),TEXT("240 FPS")});
    auto Check=[&](const FString& Label){auto* C=WidgetTree->ConstructWidget<UCheckBox>();C->AddChild(Text(Label,12));Add(C);return C;};VSync=Check(TEXT("  VSync"));MotionBlur=Check(TEXT("  Motion blur"));
    auto Button=[&](const FString& Label){auto* B=WidgetTree->ConstructWidget<UButton>();B->SetBackgroundColor(FLinearColor(.2f,.35f,.23f));auto* T=Text(Label,12);T->SetMargin(FMargin(10));B->AddChild(T);Add(B);return B;};
    Button(TEXT("APPLY GRAPHICS / SAVE ON THIS PC"))->OnClicked.AddDynamic(this,&UWildlineGraphics::Apply);
    Button(TEXT("RESTORE EPIC DEFAULTS"))->OnClicked.AddDynamic(this,&UWildlineGraphics::Defaults);
    Status=Text(TEXT("Changes apply when you press Apply. Graphics are local to this PC."));Add(Status);Sync();return Super::RebuildWidget();
}
void UWildlineGraphics::Sync(){auto* S=UWildlineUserSettings::Get();if(!S||!Preset)return;bSyncing=true;
    const int32 Overall=S->GetOverallScalabilityLevel();Preset->SetSelectedIndex(Overall<0?5:Overall);AA->SetSelectedIndex(S->AntiAliasingMethod==4?3:S->AntiAliasingMethod);
    const int32 Levels[]={S->GetAntiAliasingQuality(),S->GetViewDistanceQuality(),S->GetTextureQuality(),S->GetShadowQuality(),S->GetGlobalIlluminationQuality(),S->GetReflectionQuality(),S->GetVisualEffectQuality(),S->GetPostProcessingQuality(),S->GetFoliageQuality(),S->GetShadingQuality(),S->GetLandscapeQuality()};for(int I=0;I<Quality.Num();I++)Quality[I]->SetSelectedIndex(Levels[I]);
    float Normal,Scale,Min,Max;S->GetResolutionScaleInformationEx(Normal,Scale,Min,Max);RenderScale->SetValue(Scale);ScaleChanged(Scale);Sharpness->SetValue(S->Sharpening);VSync->SetIsChecked(S->IsVSyncEnabled());MotionBlur->SetIsChecked(S->bMotionBlur);
    const float Caps[]={0,30,60,90,120,144,165,240};int Best=0;for(int I=0;I<8;I++)if(FMath::IsNearlyEqual(S->GetFrameRateLimit(),Caps[I]))Best=I;FrameCap->SetSelectedIndex(Best);bSyncing=false;
}
void UWildlineGraphics::PresetChanged(FString Value,ESelectInfo::Type Type){if(bSyncing||Type==ESelectInfo::Direct||Preset->GetSelectedIndex()>4)return;bSyncing=true;for(auto* C:Quality)C->SetSelectedIndex(Preset->GetSelectedIndex());bSyncing=false;}
void UWildlineGraphics::QualityChanged(FString Value,ESelectInfo::Type Type){if(!bSyncing&&Type!=ESelectInfo::Direct)Preset->SetSelectedIndex(5);}
void UWildlineGraphics::ScaleChanged(float Value){if(ScaleLabel)ScaleLabel->SetText(FText::FromString(FString::Printf(TEXT("RENDER SCALE / %.0f%%"),Value)));}
void UWildlineGraphics::Apply(){auto* S=UWildlineUserSettings::Get();if(!S)return;
    using Setter=void(UWildlineUserSettings::*)(int32);const Setter Setters[]={&UWildlineUserSettings::SetAntiAliasingQuality,&UWildlineUserSettings::SetViewDistanceQuality,&UWildlineUserSettings::SetTextureQuality,&UWildlineUserSettings::SetShadowQuality,&UWildlineUserSettings::SetGlobalIlluminationQuality,&UWildlineUserSettings::SetReflectionQuality,&UWildlineUserSettings::SetVisualEffectQuality,&UWildlineUserSettings::SetPostProcessingQuality,&UWildlineUserSettings::SetFoliageQuality,&UWildlineUserSettings::SetShadingQuality,&UWildlineUserSettings::SetLandscapeQuality};for(int I=0;I<Quality.Num();I++)(S->*Setters[I])(Quality[I]->GetSelectedIndex());
    S->AntiAliasingMethod=AA->GetSelectedIndex()==3?4:AA->GetSelectedIndex();S->SetResolutionScaleValueEx(RenderScale->GetValue());S->Sharpening=Sharpness->GetValue();S->SetVSyncEnabled(VSync->IsChecked());S->bMotionBlur=MotionBlur->IsChecked();const float Caps[]={0,30,60,90,120,144,165,240};S->SetFrameRateLimit(Caps[FMath::Clamp(FrameCap->GetSelectedIndex(),0,7)]);S->SetDynamicResolutionEnabled(false);S->ApplyNonResolutionSettings();S->Persist();Sync();Status->SetText(FText::FromString(TEXT("Graphics applied and saved on this PC.")));
}
void UWildlineGraphics::Defaults(){auto* S=UWildlineUserSettings::Get();S->SetOverallScalabilityLevel(3);S->SetResolutionScaleValueEx(100);S->AntiAliasingMethod=4;S->bMotionBlur=false;S->Sharpening=.3f;S->SetVSyncEnabled(false);S->SetFrameRateLimit(90);S->ApplyNonResolutionSettings();S->Persist();Sync();Status->SetText(FText::FromString(TEXT("Epic defaults restored and saved.")));}

#include "WildlineHunterCreator.h"
#include "WildlineCharacter.h"
#include "WildlineGame.h"
#include "WildlineMenu.h"
#include "WildlineUserSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputCoreTypes.h"
#include "Misc/Base64.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
TSharedRef<SWidget> UWildlineHunterCreator::RebuildWidget(){
    if(WidgetTree->RootWidget)return Super::RebuildWidget();SetIsFocusable(true);
    if(auto* S=UWildlineUserSettings::Get())Pending=S->HunterProfile;Pending.Clamp();PhotoData=WildlinePortrait::LoadSavedPhoto();
    const FLinearColor Cream(.89f,.92f,.81f),Sage(.69f,.79f,.48f),Muted(.58f,.67f,.58f);
    auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Root;
    auto* Back=WidgetTree->ConstructWidget<UBorder>();Back->SetBrushColor(FLinearColor(.012f,.022f,.017f,1));auto* BS=Root->AddChildToCanvas(Back);BS->SetAnchors(FAnchors(0,0,1,1));BS->SetOffsets(FMargin(0));
    auto* Panel=WidgetTree->ConstructWidget<UHorizontalBox>();Back->SetContent(Panel);Back->SetPadding(FMargin(40,28));
    auto Text=[&](const FString& Value,int32 Size,FLinearColor Color){auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Value));T->SetColorAndOpacity(FSlateColor(Color));auto F=T->GetFont();F.Size=Size;T->SetFont(F);T->SetAutoWrapText(true);return T;};
    auto Button=[&](const FString& Value){auto* B=WidgetTree->ConstructWidget<UButton>();auto Style=B->GetStyle();Style.Normal.TintColor=FSlateColor(FLinearColor(.13f,.21f,.14f));Style.Hovered.TintColor=FSlateColor(FLinearColor(.25f,.35f,.19f));B->SetStyle(Style);auto* T=Text(Value,12,Cream);T->SetMargin(FMargin(12,9));B->AddChild(T);return B;};
    auto* Left=WidgetTree->ConstructWidget<UVerticalBox>();auto* Fixed=WidgetTree->ConstructWidget<USizeBox>();Fixed->SetWidthOverride(410);Fixed->SetContent(Left);auto* LS=Panel->AddChildToHorizontalBox(Fixed);LS->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));LS->SetPadding(FMargin(0,0,28,0));
    auto* LeftBorder=WidgetTree->ConstructWidget<UBorder>();LeftBorder->SetBrushColor(FLinearColor(.026f,.045f,.033f));LeftBorder->SetPadding(FMargin(20));
    auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();AppearanceScroll=Scroll;LeftBorder->SetContent(Scroll);Left->AddChildToVerticalBox(LeftBorder)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* List=WidgetTree->ConstructWidget<UVerticalBox>();Scroll->AddChild(List);
    auto Add=[&](UWidget* W,float Space=9){auto* S=List->AddChildToVerticalBox(W);S->SetPadding(FMargin(0,0,0,Space));};
    Add(Text(TEXT("W  /  HUNTER CREATION"),12,Sage),12);Add(Text(TEXT("Make the hunt yours."),27,Cream),12);
    Add(Text(TEXT("Choose your look, then preview it in the field."),11,Muted),16);
    auto Combo=[&](const FString& Label,const TArray<FString>& Options,int32 Index){Add(Text(Label,10,Sage),4);auto* C=WidgetTree->ConstructWidget<UWildlineChoice>();for(const auto& O:Options)C->AddOption(O);C->SetSelectedIndex(Index);Add(C,10);return C;};
    PresetCombo=Combo(TEXT("STARTING PRESET"),{TEXT("Custom hunter"),TEXT("Travis"),TEXT("Ethan"),TEXT("Greyson"),TEXT("Mark")},Pending.PresetOptionIndex());PresetCombo->OnSelectionChanged.AddDynamic(this,&UWildlineHunterCreator::PresetChanged);
    Add(Text(TEXT("Names are starting settings. Keep the default size or build your own adult or smaller hunter."),11,Muted),10);
    auto* Reset=Button(TEXT("RESET SELECTED PRESET"));Reset->OnClicked.AddDynamic(this,&UWildlineHunterCreator::ResetPreset);Add(Reset);
    auto* Body=Combo(TEXT("BODY"),{TEXT("Male hunter"),TEXT("Female hunter")},Pending.Body);BodyCombo=Body;Body->OnSelectionChanged.AddDynamic(this,&UWildlineHunterCreator::BodyChanged);
    auto Slider=[&](const FString& Label,float Min,float Max,float Value){Add(Text(Label,10,Muted),4);auto* S=WidgetTree->ConstructWidget<USlider>();S->SetMinValue(Min);S->SetMaxValue(Max);S->SetValue(Value);AppearanceSliders.Add(S);Add(S,11);return S;};
    Add(Text(TEXT("BODY MEASUREMENTS"),14,Sage),10);
    auto Dimension=[&](float Min,float Max,float Value){auto* Label=Text(TEXT(""),10,Muted);DimensionLabels.Add(Label);Add(Label,4);auto* S=WidgetTree->ConstructWidget<USlider>();S->SetMinValue(Min);S->SetMaxValue(Max);S->SetStepSize(1);S->SetValue(Value);DimensionSliders.Add(S);Add(S,12);return S;};
    Dimension(60,230,Pending.HeightCm)->OnValueChanged.AddDynamic(this,&UWildlineHunterCreator::HeightChanged);
    WeightLabel=Text(TEXT(""),10,Muted);Add(WeightLabel,4);
    auto* Weight=Slider(TEXT("Lean  /  Heavy"),0,150,(1-Pending.Slimness)*100);Weight->OnValueChanged.AddDynamic(this,&UWildlineHunterCreator::SlimnessChanged);
    Dimension(70,150,Pending.BodyWidth*100)->OnValueChanged.AddDynamic(this,&UWildlineHunterCreator::BodyWidthChanged);
    Dimension(70,160,Pending.BodyDepth*100)->OnValueChanged.AddDynamic(this,&UWildlineHunterCreator::BodyDepthChanged);
    Dimension(50,150,Pending.HandSize*100)->OnValueChanged.AddDynamic(this,&UWildlineHunterCreator::HandSizeChanged);
    Dimension(50,150,Pending.FootSize*100)->OnValueChanged.AddDynamic(this,&UWildlineHunterCreator::FootSizeChanged);
    Dimension(60,150,Pending.FaceSize*100)->OnValueChanged.AddDynamic(this,&UWildlineHunterCreator::FaceSizeChanged);
    Add(Text(TEXT("Height uses centimetres. Proportions use percentages of the adult rig. Weight changes body fullness. Ethan and Greyson can grow to adult size; reset restores each name's original measurements."),10,Muted),16);
    Add(Text(TEXT("CLOTHING & FACE DETAILS"),14,Sage),10);
    auto* Outfit=Combo(TEXT("FIELD CLOTHING"),{TEXT("Woodland olive"),TEXT("Blaze orange"),TEXT("Earth brown")},Pending.Outfit);OutfitCombo=Outfit;Outfit->OnSelectionChanged.AddDynamic(this,&UWildlineHunterCreator::OutfitChanged);
    auto* Hair=Combo(TEXT("HAIR"),{TEXT("Brown"),TEXT("Black"),TEXT("Silver")},Pending.Hair);HairCombo=Hair;Hair->OnSelectionChanged.AddDynamic(this,&UWildlineHunterCreator::HairChanged);
    auto* Cap=WidgetTree->ConstructWidget<UCheckBox>();Cap->AddChild(Text(TEXT("  Field cap"),12,Cream));Cap->SetIsChecked(Pending.bCap);Cap->OnCheckStateChanged.AddDynamic(this,&UWildlineHunterCreator::CapChanged);Add(Cap);
    CapCheck=Cap;
    BeardCheck=WidgetTree->ConstructWidget<UCheckBox>();BeardCheck->AddChild(Text(TEXT("  Facial hair"),12,Cream));BeardCheck->SetIsChecked(Pending.bBeard);BeardCheck->OnCheckStateChanged.AddDynamic(this,&UWildlineHunterCreator::BeardChanged);Add(BeardCheck);
    Slider(TEXT("SKIN TONE"),0,1,Pending.SkinTone)->OnValueChanged.AddDynamic(this,&UWildlineHunterCreator::SkinChanged);
    Slider(TEXT("FACE WIDTH"),-1,1,Pending.FaceWidth)->OnValueChanged.AddDynamic(this,&UWildlineHunterCreator::WidthChanged);
    Slider(TEXT("NOSE PROFILE"),-1,1,Pending.NoseDepth)->OnValueChanged.AddDynamic(this,&UWildlineHunterCreator::NoseChanged);
    Add(Text(TEXT("FACE FROM PHOTO"),14,Sage),7);
    Add(Text(TEXT("Use a clear, front-facing portrait. Align the eyes and mouth with the live face preview."),11,Muted),9);
    auto* Upload=Button(TEXT("CHOOSE PNG / JPEG"));Upload->OnClicked.AddDynamic(this,&UWildlineHunterCreator::UploadPhoto);Add(Upload);
    PhotoImage=WidgetTree->ConstructWidget<UImage>();PhotoFrame=WidgetTree->ConstructWidget<USizeBox>();PhotoFrame->SetHeightOverride(170);PhotoFrame->SetWidthOverride(170);auto* PhotoFit=WidgetTree->ConstructWidget<UScaleBox>();PhotoFit->SetStretch(EStretch::ScaleToFit);PhotoFit->SetContent(PhotoImage);PhotoFrame->SetContent(PhotoFit);PhotoFrame->SetVisibility(ESlateVisibility::Collapsed);Add(PhotoFrame,8);
    auto* Clear=Button(TEXT("USE DEFAULT FACE"));Clear->OnClicked.AddDynamic(this,&UWildlineHunterCreator::ClearPhoto);Add(Clear);
    Slider(TEXT("PHOTO HORIZONTAL ALIGNMENT"),-.5,.5,Pending.PhotoX)->OnValueChanged.AddDynamic(this,&UWildlineHunterCreator::PhotoXChanged);
    Slider(TEXT("PHOTO VERTICAL ALIGNMENT"),-.5,.5,Pending.PhotoY)->OnValueChanged.AddDynamic(this,&UWildlineHunterCreator::PhotoYChanged);
    Slider(TEXT("PHOTO CROP / ZOOM"),.5,4,Pending.PhotoZoom)->OnValueChanged.AddDynamic(this,&UWildlineHunterCreator::PhotoZoomChanged);
    Add(Text(TEXT("Photo appearance is mapped to the selected 3D head. Face width adjusts its shape; custom hunters also support nose depth. The photo is saved locally and shared with players in your co-op session."),10,Muted),10);
    Message=Text(TEXT("Your saved hunter will be used when you enter an expedition."),11,Sage);Add(Message,12);
    auto* Actions=WidgetTree->ConstructWidget<UHorizontalBox>();auto* Save=Button(TEXT("SAVE HUNTER"));Save->OnClicked.AddDynamic(this,&UWildlineHunterCreator::SaveHunter);auto* Close=Button(TEXT("CANCEL"));Close->OnClicked.AddDynamic(this,&UWildlineHunterCreator::Cancel);Actions->AddChildToHorizontalBox(Save);Actions->AddChildToHorizontalBox(Close);Left->AddChildToVerticalBox(Actions)->SetPadding(FMargin(0,14,0,0));
    auto* Right=WidgetTree->ConstructWidget<UVerticalBox>();Panel->AddChildToHorizontalBox(Right)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Right->AddChildToVerticalBox(Text(TEXT("FIELD PORTRAIT"),12,Sage))->SetPadding(FMargin(12,0,0,12));
    auto* Views=WidgetTree->ConstructWidget<UHorizontalBox>();auto* Whole=Button(TEXT("FULL BODY"));Whole->OnClicked.AddDynamic(this,&UWildlineHunterCreator::FullBody);Views->AddChildToHorizontalBox(Whole);auto* Face=Button(TEXT("FACE CLOSE-UP"));Face->OnClicked.AddDynamic(this,&UWildlineHunterCreator::FaceView);Views->AddChildToHorizontalBox(Face);auto* TurnL=Button(TEXT("TURN LEFT"));TurnL->OnClicked.AddDynamic(this,&UWildlineHunterCreator::TurnLeft);Views->AddChildToHorizontalBox(TurnL);auto* TurnR=Button(TEXT("TURN RIGHT"));TurnR->OnClicked.AddDynamic(this,&UWildlineHunterCreator::TurnRight);Views->AddChildToHorizontalBox(TurnR);Right->AddChildToVerticalBox(Views)->SetPadding(FMargin(0,0,0,8));
    PreviewImage=WidgetTree->ConstructWidget<UImage>();auto* FitPreview=WidgetTree->ConstructWidget<UScaleBox>();FitPreview->SetStretch(EStretch::ScaleToFit);FitPreview->SetContent(PreviewImage);Right->AddChildToVerticalBox(FitPreview)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Right->AddChildToVerticalBox(Text(TEXT("Sporting bolt-action rifle  /  Boots, field clothing and configurable cap"),11,Muted))->SetPadding(FMargin(12,10,0,0));
    // Keep the preview away from the playable forest and capture only its actor.
    const FTransform Transform(FRotator::ZeroRotator,FVector(0,0,200000));Preview=GetWorld()->SpawnActorDeferred<AWildlineCharacter>(AWildlineCharacter::StaticClass(),Transform,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    Preview->SetReplicates(false);Preview->bPreviewHunter=true;Preview->bProfileSubmitted=true;Preview->bThirdPerson=true;Preview->HunterProfile=Pending;Preview->HunterFaceData=PhotoData;Preview->FinishSpawning(Transform);Preview->SetActorEnableCollision(false);Preview->GetCharacterMovement()->DisableMovement();Preview->GetMesh()->SetOwnerNoSee(false);Preview->WorldRifle->SetOwnerNoSee(false);Preview->WorldBolt->SetOwnerNoSee(false);Preview->ViewRifle->SetVisibility(false);Preview->ViewBolt->SetVisibility(false);
    Target=NewObject<UTextureRenderTarget2D>(this);Target->RenderTargetFormat=RTF_RGBA8;Target->ClearColor=FLinearColor(.025f,.04f,.03f);Target->InitAutoFormat(768,896);Target->TargetGamma=2.2f;FSlateBrush PortraitBrush;PortraitBrush.SetResourceObject(Target);PortraitBrush.ImageSize=FVector2D(768,896);PreviewImage->SetBrush(PortraitBrush);
    Capture=NewObject<USceneCaptureComponent2D>(Preview);Capture->SetupAttachment(Preview->GetRootComponent());Capture->TextureTarget=Target;Capture->CaptureSource=ESceneCaptureSource::SCS_FinalColorLDR;Capture->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;Capture->ShowOnlyActors.Add(Preview);Capture->ShowFlags.SetAtmosphere(false);Capture->ShowFlags.SetDeferredAtmospherePass(false);Capture->ShowFlags.SetFog(false);Capture->ShowFlags.SetGlobalIllumination(false);Capture->ShowFlags.SetAmbientOcclusion(false);Capture->ShowFlags.SetReflectionEnvironment(false);Capture->FOVAngle=43;Capture->PostProcessSettings.bOverride_AutoExposureMinBrightness=true;Capture->PostProcessSettings.bOverride_AutoExposureMaxBrightness=true;Capture->PostProcessSettings.AutoExposureMinBrightness=9;Capture->PostProcessSettings.AutoExposureMaxBrightness=9;Capture->RegisterComponent();
    // Isolate portrait lighting from the reserve's much brighter sun.
    Preview->GetMesh()->SetLightingChannels(false,true,false);
    for(int32 I=0;I<2;I++){auto* Light=NewObject<UPointLightComponent>(Preview);Light->SetMobility(EComponentMobility::Movable);Light->SetLightingChannels(false,true,false);Light->SetupAttachment(Preview->GetRootComponent());Light->SetRelativeLocation(I==0?FVector(160,-100,105):FVector(150,100,65));Light->SetIntensityUnits(ELightUnits::Candelas);Light->SetIntensity((I==0?1800:1100)*FMath::Square(Pending.HeightScale()));Light->SetSourceRadius(40);Light->SetAttenuationRadius(800);Light->SetCastShadows(false);Light->RegisterComponent();}
    auto* Background=NewObject<UStaticMeshComponent>(Preview);Background->SetupAttachment(Preview->GetRootComponent());Background->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));Background->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Wildline/Hunters/Materials/M_PreviewBackdrop.M_PreviewBackdrop")));Background->SetRelativeScale3D(FVector(30));Background->SetCollisionEnabled(ECollisionEnabled::NoCollision);Background->SetCastShadow(false);Background->RegisterComponent();
    RefreshPreview();UpdateCamera();return Super::RebuildWidget();
}
void UWildlineHunterCreator::RefreshPreview(){
    Pending.Clamp();RefreshMeasurementLabels();const bool bPhotoPreset=Pending.IsPhotoPreset();BeardCheck->SetIsEnabled(!bPhotoPreset);HairCombo->SetIsEnabled(!bPhotoPreset);
    if(AppearanceSliders.Num()>3)AppearanceSliders[3]->SetIsEnabled(!bPhotoPreset);
    if(Preview){
        const float OldScale=Preview->GetActorScale3D().Z;
        Preview->HunterProfile=Pending;Preview->HunterFaceData=PhotoData;Preview->ApplyHunterProfile();
        // Keep portrait exposure consistent as the lights move with the rig.
        TArray<UPointLightComponent*> Lights;Preview->GetComponents(Lights);
        for(auto* Light:Lights)Light->SetIntensity(Light->Intensity*FMath::Square(Pending.HeightScale()/OldScale));
        Preview->WorldRifle->SetWorldRotation(FRotator(-35,Preview->GetActorRotation().Yaw,0));UpdateCamera();
    }
    PhotoTexture=Pending.bUsePhoto?WildlinePortrait::DecodePhoto(PhotoData):nullptr;
    if(PhotoTexture){PhotoImage->SetBrushFromTexture(PhotoTexture);PhotoFrame->SetVisibility(ESlateVisibility::Visible);}else if(PhotoFrame)PhotoFrame->SetVisibility(ESlateVisibility::Collapsed);
}
void UWildlineHunterCreator::UpdateCamera(){if(!Capture||!Preview)return;const float A=FMath::DegreesToRadians(PreviewAngle);const float Scale=Pending.HeightScale();float BodyHeight=0;if(auto* Mesh=Preview->GetMesh()->GetSkeletalMeshAsset()){const auto B=Mesh->GetImportedBounds();BodyHeight=(Preview->GetMesh()->GetRelativeLocation().Z+B.Origin.Z)*Scale;}const FVector FacePivot=Preview->GetMesh()->GetSocketLocation(TEXT("head"))-Preview->GetActorLocation()+FVector(10,0,-3)*Scale;const FVector Pivot=bFaceView?FacePivot:FVector(0,0,BodyHeight);const float Distance=bFaceView?46*Scale*Pending.FaceSize:FMath::Max(160.f,Pending.HeightCm*1.58f);const FVector Offset(FMath::Cos(A)*Distance,FMath::Sin(A)*Distance,bFaceView?0:28);Capture->SetWorldLocation(Preview->GetActorLocation()+Pivot+Offset);Capture->SetWorldRotation((-Offset).Rotation());}
void UWildlineHunterCreator::SyncControls(){
    bSyncing=true;PresetCombo->SetSelectedIndex(Pending.PresetOptionIndex());BodyCombo->SetSelectedIndex(Pending.Body);OutfitCombo->SetSelectedIndex(Pending.Outfit);HairCombo->SetSelectedIndex(Pending.Hair);CapCheck->SetIsChecked(Pending.bCap);BeardCheck->SetIsChecked(Pending.bBeard);
    const float Values[]={(1-Pending.Slimness)*100,Pending.SkinTone,Pending.FaceWidth,Pending.NoseDepth,Pending.PhotoX,Pending.PhotoY,Pending.PhotoZoom};
    for(int32 I=0;I<AppearanceSliders.Num()&&I<UE_ARRAY_COUNT(Values);I++)AppearanceSliders[I]->SetValue(Values[I]);
    const float Dimensions[]={Pending.HeightCm,Pending.BodyWidth*100,Pending.BodyDepth*100,Pending.HandSize*100,Pending.FootSize*100,Pending.FaceSize*100};
    for(int32 I=0;I<DimensionSliders.Num()&&I<UE_ARRAY_COUNT(Dimensions);I++)DimensionSliders[I]->SetValue(Dimensions[I]);RefreshMeasurementLabels();bSyncing=false;
}
void UWildlineHunterCreator::LoadTravisPreset(){Pending=FWildlineHunterProfile::Travis();SyncControls();RefreshPreview();Message->SetText(FText::FromString(TEXT("Travis loaded. Customize clothing and build, then save your hunter.")));}
void UWildlineHunterCreator::LoadEthanPreset(){Pending=FWildlineHunterProfile::Ethan();SyncControls();RefreshPreview();Message->SetText(FText::FromString(TEXT("Ethan loaded. Customize clothing and build, then save your hunter.")));}
void UWildlineHunterCreator::LoadGreysonPreset(){Pending=FWildlineHunterProfile::Greyson();SyncControls();RefreshPreview();Message->SetText(FText::FromString(TEXT("Greyson loaded. Customize clothing and build, then save your hunter.")));}
void UWildlineHunterCreator::LoadMarkPreset(){Pending=FWildlineHunterProfile::Mark();SyncControls();RefreshPreview();Message->SetText(FText::FromString(TEXT("Mark loaded. Customize clothing and build, then save your hunter.")));}
void UWildlineHunterCreator::PresetChanged(FString I,ESelectInfo::Type){if(bSyncing)return;if(I==TEXT("Travis")){LoadTravisPreset();return;}if(I==TEXT("Ethan")){LoadEthanPreset();return;}if(I==TEXT("Greyson")){LoadGreysonPreset();return;}if(I==TEXT("Mark")){LoadMarkPreset();return;}Pending.Preset=0;Pending.bBeard=false;SyncControls();RefreshPreview();}
void UWildlineHunterCreator::ResetPreset(){if(Pending.Preset==1){LoadTravisPreset();return;}if(Pending.Preset==3){LoadEthanPreset();return;}if(Pending.Preset==4){LoadGreysonPreset();return;}if(Pending.Preset==5){LoadMarkPreset();return;}Pending=FWildlineHunterProfile();SyncControls();RefreshPreview();}
void UWildlineHunterCreator::BeardChanged(bool V){if(bSyncing)return;Pending.bBeard=V;RefreshPreview();}
void UWildlineHunterCreator::SlimnessChanged(float V){if(bSyncing)return;Pending.Slimness=1-V/100.f;RefreshPreview();}
void UWildlineHunterCreator::RefreshMeasurementLabels(){
    const FString Labels[]={FString::Printf(TEXT("HEIGHT / %.1f cm"),Pending.HeightCm),FString::Printf(TEXT("BODY WIDTH / %.0f%%"),Pending.BodyWidth*100),FString::Printf(TEXT("BODY DEPTH / %.0f%%"),Pending.BodyDepth*100),FString::Printf(TEXT("HAND SIZE / %.0f%%"),Pending.HandSize*100),FString::Printf(TEXT("FOOT SIZE / %.0f%%"),Pending.FootSize*100),FString::Printf(TEXT("HEAD / FACE SIZE / %.0f%%"),Pending.FaceSize*100)};
    for(int32 I=0;I<DimensionLabels.Num()&&I<UE_ARRAY_COUNT(Labels);I++)DimensionLabels[I]->SetText(FText::FromString(Labels[I]));
    if(WeightLabel)WeightLabel->SetText(FText::FromString(FString::Printf(TEXT("WEIGHT / BUILD / %.0f%%"),(1-Pending.Slimness)*100)));
}
void UWildlineHunterCreator::HeightChanged(float V){if(bSyncing)return;Pending.HeightCm=V;RefreshPreview();}
void UWildlineHunterCreator::BodyWidthChanged(float V){if(bSyncing)return;Pending.BodyWidth=V/100.f;RefreshPreview();}
void UWildlineHunterCreator::BodyDepthChanged(float V){if(bSyncing)return;Pending.BodyDepth=V/100.f;RefreshPreview();}
void UWildlineHunterCreator::HandSizeChanged(float V){if(bSyncing)return;Pending.HandSize=V/100.f;RefreshPreview();}
void UWildlineHunterCreator::FootSizeChanged(float V){if(bSyncing)return;Pending.FootSize=V/100.f;RefreshPreview();}
void UWildlineHunterCreator::FaceSizeChanged(float V){if(bSyncing)return;Pending.FaceSize=V/100.f;RefreshPreview();}
void UWildlineHunterCreator::BodyChanged(FString I,ESelectInfo::Type){if(bSyncing)return;Pending.Body=I.StartsWith(TEXT("Female"))?1:0;RefreshPreview();UpdateCamera();}
void UWildlineHunterCreator::OutfitChanged(FString I,ESelectInfo::Type){if(bSyncing)return;Pending.Outfit=I.StartsWith(TEXT("Blaze"))?1:I.StartsWith(TEXT("Earth"))?2:0;RefreshPreview();}
void UWildlineHunterCreator::HairChanged(FString I,ESelectInfo::Type){if(bSyncing)return;Pending.Hair=I==TEXT("Black")?1:I==TEXT("Silver")?2:0;RefreshPreview();}
void UWildlineHunterCreator::CapChanged(bool V){if(bSyncing)return;Pending.bCap=V;RefreshPreview();}
void UWildlineHunterCreator::SkinChanged(float V){if(bSyncing)return;Pending.SkinTone=V;RefreshPreview();}
void UWildlineHunterCreator::WidthChanged(float V){if(bSyncing)return;Pending.FaceWidth=V;RefreshPreview();}
void UWildlineHunterCreator::NoseChanged(float V){if(bSyncing)return;Pending.NoseDepth=V;RefreshPreview();}
void UWildlineHunterCreator::PhotoXChanged(float V){if(bSyncing)return;Pending.PhotoX=V;RefreshPreview();}
void UWildlineHunterCreator::PhotoYChanged(float V){if(bSyncing)return;Pending.PhotoY=V;RefreshPreview();}
void UWildlineHunterCreator::PhotoZoomChanged(float V){if(bSyncing)return;Pending.PhotoZoom=V;RefreshPreview();}
bool UWildlineHunterCreator::ImportPortraitPath(const FString& Path){FString Error,Encoded;if(!WildlinePortrait::ImportPhoto(Path,Encoded,Error)){Message->SetText(FText::FromString(Error));return false;}PhotoData=Encoded;Pending.bUsePhoto=true;Message->SetText(FText::FromString(TEXT("Portrait loaded. Use face close-up and the alignment controls.")));bFaceView=true;PreviewAngle=0;RefreshPreview();UpdateCamera();if(AppearanceScroll)AppearanceScroll->ScrollToEnd();return true;}
void UWildlineHunterCreator::UploadPhoto(){const FString Path=WildlinePortrait::ChoosePhotoFile();if(!Path.IsEmpty())ImportPortraitPath(Path);}
void UWildlineHunterCreator::ClearPhoto(){Pending.bUsePhoto=false;RefreshPreview();Message->SetText(FText::FromString(TEXT("Using the default textured face.")));}
void UWildlineHunterCreator::FullBody(){bFaceView=false;UpdateCamera();}
void UWildlineHunterCreator::FaceView(){bFaceView=true;PreviewAngle=0;UpdateCamera();}
void UWildlineHunterCreator::TurnLeft(){PreviewAngle-=20;UpdateCamera();}
void UWildlineHunterCreator::TurnRight(){PreviewAngle+=20;UpdateCamera();}
void UWildlineHunterCreator::SaveHunter(){Pending.Clamp();if(Pending.bUsePhoto){TArray<uint8> Bytes;if(!FBase64::Decode(PhotoData,Bytes))return;IFileManager::Get().MakeDirectory(*FPaths::GetPath(WildlinePortrait::SavedPhotoPath()),true);if(!FFileHelper::SaveArrayToFile(Bytes,*WildlinePortrait::SavedPhotoPath())){Message->SetText(FText::FromString(TEXT("Could not save the portrait. Check your game profile folder.")));return;}}if(auto* S=UWildlineUserSettings::Get()){S->HunterProfile=Pending;S->Persist();}if(auto* PC=GetOwningPlayer())if(auto* P=Cast<AWildlineCharacter>(PC->GetPawn())){P->HunterProfile=Pending;P->HunterFaceData=Pending.bUsePhoto?PhotoData:FString();P->ApplyHunterProfile();P->ServerSetHunterProfile(Pending,P->HunterFaceData);}Cancel();}
void UWildlineHunterCreator::Cancel(){if(auto* PC=Cast<AWildlineController>(GetOwningPlayer())){if(PC->Menu)PC->Menu->SetVisibility(ESlateVisibility::Visible);PC->SetMenu(true);}RemoveFromParent();}
void UWildlineHunterCreator::NativeDestruct(){if(Preview){Preview->Destroy();Preview=nullptr;}Super::NativeDestruct();}
FReply UWildlineHunterCreator::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E){if(E.GetKey()==EKeys::Escape){Cancel();return FReply::Handled();}return Super::NativeOnKeyDown(G,E);}

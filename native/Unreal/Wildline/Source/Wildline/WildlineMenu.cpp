#include "WildlineMenu.h"
#include "WildlineGame.h"
#include "WildlineCharacter.h"
#include "WildlineUserSettings.h"
#include "WildlineRules.h"
#include "WildlineHunterCreator.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/SpinBox.h"
#include "Components/ScrollBox.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

TSharedRef<SWidget> UWildlineMenu::RebuildWidget(){
    if(WidgetTree->RootWidget)return Super::RebuildWidget();SetIsFocusable(true);
    auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Root;
    const FLinearColor Cream(.87f,.9f,.77f),Sage(.68f,.77f,.47f),Muted(.56f,.65f,.52f);
    auto Text=[&](const FString& Value,int32 Size,FLinearColor Color){auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Value));T->SetColorAndOpacity(FSlateColor(Color));auto Font=T->GetFont();Font.Size=Size;T->SetFont(Font);T->SetAutoWrapText(true);return T;};
    auto Place=[&](UWidget* W,FVector2D Position,FVector2D Size){auto* S=Root->AddChildToCanvas(W);S->SetPosition(Position);S->SetSize(Size);return S;};
    auto* Shade=WidgetTree->ConstructWidget<UBorder>();Shade->SetBrushColor(FLinearColor(.018f,.03f,.023f,.68f));auto* SS=Place(Shade,FVector2D::ZeroVector,FVector2D::ZeroVector);SS->SetAnchors(FAnchors(0,0,1,1));SS->SetOffsets(FMargin(0));Shade->SetVisibility(ESlateVisibility::HitTestInvisible);
    Place(Text(TEXT("W  /  WILDLINE"),24,Cream),FVector2D(55,35),FVector2D(450,50));
    Place(Text(TEXT("FOREST / FARM / NEIGHBORHOOD"),10,Sage),FVector2D(60,135),FVector2D(550,25));
    Place(Text(TEXT("The wild has\ntwo sides."),58,Cream),FVector2D(55,180),FVector2D(620,175));
    Place(Text(TEXT("Follow the trail. Raid the pantry. Race to the feeder.\nA shared wilderness. A different kind of hunt."),15,Muted),FVector2D(60,380),FVector2D(560,65));
    Place(Text(TEXT("WASD move  /  Shift sprint  /  V camera\nHunter: LMB fire, RMB scope, T thermal, R reload\nSquirrel: E grab a tree, W/S climb, Space launch\nEsc releases the mouse and returns to camp."),12,Muted),FVector2D(60,535),FVector2D(590,115));
    auto* Panel=WidgetTree->ConstructWidget<UBorder>();Panel->SetBrushColor(FLinearColor(.028f,.05f,.036f,.96f));Panel->SetPadding(FMargin(26));auto* PanelSlot=Root->AddChildToCanvas(Panel);PanelSlot->SetAnchors(FAnchors(1,0,1,1));PanelSlot->SetAlignment(FVector2D(1,0));PanelSlot->SetOffsets(FMargin(-55,40,450,40));
    auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();FieldScroll=Scroll;Panel->SetContent(Scroll);
    auto* List=WidgetTree->ConstructWidget<UVerticalBox>();Scroll->AddChild(List);
    auto Add=[&](UWidget* W,float Space=8.f){auto* S=List->AddChildToVerticalBox(W);S->SetPadding(FMargin(0,0,0,Space));};
    auto Button=[&](const FString& Title){auto* B=WidgetTree->ConstructWidget<UButton>();auto Style=B->GetStyle();Style.Normal.TintColor=FSlateColor(FLinearColor(.12f,.19f,.12f));Style.Hovered.TintColor=FSlateColor(FLinearColor(.24f,.33f,.19f));Style.Pressed.TintColor=FSlateColor(Sage);B->SetStyle(Style);auto* T=Text(Title,13,Cream);T->SetMargin(FMargin(14,10));B->AddChild(T);return B;};
    Add(Text(TEXT("YOUR NEXT EXPEDITION"),10,Sage),12);Add(Text(TEXT("Choose your instincts."),27,Cream),17);
    Add(Text(TEXT("GAME MODE"),10,Sage),5);Mode=WidgetTree->ConstructWidget<UComboBoxString>();Mode->AddOption(TEXT("Classic / Blackwood"));Mode->AddOption(TEXT("Farm Raid / Peanut Heist"));Mode->AddOption(TEXT("King of the Hill / Bird Feeder"));
    if(auto* GI=GetGameInstance<UWildlineGameInstance>())GI->PreferredMode=UWildlineUserSettings::Get()?UWildlineUserSettings::Get()->ExpeditionMode:0;
    Mode->SetSelectedIndex(GetGameInstance<UWildlineGameInstance>()?GetGameInstance<UWildlineGameInstance>()->PreferredMode:0);Mode->OnSelectionChanged.AddDynamic(this,&UWildlineMenu::ModeChanged);Add(Mode,5);ModeDescription=Text(TEXT(""),10,Muted);Add(ModeDescription,10);
    Add(Text(TEXT("MATCH CONFIG / SAVED PER MODE"),10,Sage),5);
    auto Number=[&](USpinBox*& Box,const TCHAR* Label,float Min,float Max){Add(Text(Label,9,Muted),3);Box=WidgetTree->ConstructWidget<USpinBox>();Box->SetMinValue(Min);Box->SetMaxValue(Max);Box->SetMinSliderValue(Min);Box->SetMaxSliderValue(Max);Box->SetForegroundColor(FSlateColor(FLinearColor(.06f,.09f,.07f)));Box->SetDelta(1);Box->SetMinFractionalDigits(0);Box->SetMaxFractionalDigits(0);Box->OnValueChanged.AddDynamic(this,&UWildlineMenu::ConfigChanged);Add(Box,6);};
    Number(PlayerLimit,TEXT("HUMAN PLAYER LIMIT / 1-8"),1,8);Number(RoundMinutes,TEXT("ROUND TIME / MINUTES"),1,30);
    SquirrelPointsLabel=Text(TEXT("SQUIRREL POINTS TO WIN"),9,Muted);Add(SquirrelPointsLabel,3);Number(SquirrelPoints,TEXT(""),1,30);Number(DefenderPoints,TEXT("DEFENDER CAPTURES TO WIN"),1,30);RefreshConfig();
    Add(Text(TEXT("Host sets the match rules. Changed rules start a new match when you enter, or apply with F5 after a result."),9,Muted),10);
    auto* Roles=WidgetTree->ConstructWidget<UHorizontalBox>();auto* H=Button(TEXT("THE HUNTER"));DefenderLabel=Cast<UTextBlock>(H->GetContent());auto* A=Button(TEXT("THE SQUIRREL"));H->OnClicked.AddDynamic(this,&UWildlineMenu::Hunter);A->OnClicked.AddDynamic(this,&UWildlineMenu::Squirrel);Roles->AddChildToHorizontalBox(H)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Roles->AddChildToHorizontalBox(A)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Add(Roles,10);
    RoleDescription=Text(TEXT("HUNTER / First person. Ballistic rifle. Rifle thermal: 20 seconds, then 2 minutes to recharge."),11,Muted);Add(RoleDescription,15);
    Add(Text(TEXT("DEFENDER WEAPON"),10,Sage),5);Weapon=WidgetTree->ConstructWidget<UComboBoxString>();Weapon->AddOption(TEXT("Sporting rifle / precision"));Weapon->AddOption(TEXT("Pump-action shotgun / close range"));Weapon->SetSelectedIndex(UWildlineUserSettings::Get()?UWildlineUserSettings::Get()->SelectedWeapon:0);Weapon->OnSelectionChanged.AddDynamic(this,&UWildlineMenu::WeaponChanged);Add(Weapon,10);RefreshMode();
    Add(Text(TEXT("DEFENDER BOT DIFFICULTY"),10,Sage),6);
    Difficulty=WidgetTree->ConstructWidget<UComboBoxString>();
    for(const TCHAR* Option:{TEXT("Easy"),TEXT("Medium"),TEXT("Hard")})Difficulty->AddOption(Option);
    DifficultyDescription=Text(TEXT(""),10,Muted);
    RefreshDifficulty();Difficulty->OnSelectionChanged.AddDynamic(this,&UWildlineMenu::DifficultyChanged);Add(Difficulty,7);Add(DifficultyDescription,12);
    auto* Customize=Button(TEXT("CREATE / EDIT HUNTER"));Customize->OnClicked.AddDynamic(this,&UWildlineMenu::CreateHunter);Add(Customize,8);
    auto* PlayButton=Button(TEXT("ENTER THE WILD / RESUME"));PlayButton->OnClicked.AddDynamic(this,&UWildlineMenu::Play);Add(PlayButton,8);
    auto* HostButton=Button(TEXT("HOST CO-OP EXPEDITION"));HostButton->OnClicked.AddDynamic(this,&UWildlineMenu::Host);Add(HostButton,8);
    Address=WidgetTree->ConstructWidget<UEditableTextBox>();Address->SetHintText(FText::FromString(TEXT("Host IP address, e.g. 192.168.1.46")));Address->SetText(FText::FromString(TEXT("127.0.0.1")));Add(Address,8);
    auto* JoinButton=Button(TEXT("JOIN EXPEDITION"));JoinButton->OnClicked.AddDynamic(this,&UWildlineMenu::Join);Add(JoinButton,14);
    Add(Text(TEXT("FIELD SETTINGS"),10,Sage),8);
    Inversion=WidgetTree->ConstructWidget<UCheckBox>();Inversion->AddChild(Text(TEXT("  Invert mouse Y"),12,Cream));Inversion->SetIsChecked(true);if(auto* S=UWildlineUserSettings::Get())Inversion->SetIsChecked(S->bInvertMouseY);Inversion->OnCheckStateChanged.AddDynamic(this,&UWildlineMenu::InvertChanged);Add(Inversion,7);
    Blood=WidgetTree->ConstructWidget<UCheckBox>();Blood->AddChild(Text(TEXT("  Blood effects"),12,Cream));Blood->SetIsChecked(true);if(auto* S=UWildlineUserSettings::Get())Blood->SetIsChecked(S->bBloodEnabled);Blood->OnCheckStateChanged.AddDynamic(this,&UWildlineMenu::BloodChanged);Add(Blood,7);
    Add(Text(TEXT("MOUSE SENSITIVITY"),9,Muted),4);Sensitivity=WidgetTree->ConstructWidget<USlider>();Sensitivity->SetMinValue(.15f);Sensitivity->SetMaxValue(2.f);Sensitivity->SetValue(UWildlineUserSettings::Get()?UWildlineUserSettings::Get()->MouseSensitivity:.8f);Sensitivity->OnValueChanged.AddDynamic(this,&UWildlineMenu::SensitivityChanged);Add(Sensitivity,10);
    Add(Text(TEXT("SCOPE MOUSE SENSITIVITY"),9,Muted),4);ScopeSensitivity=WidgetTree->ConstructWidget<USlider>();ScopeSensitivity->SetMinValue(.05f);ScopeSensitivity->SetMaxValue(2.f);ScopeSensitivity->SetValue(UWildlineUserSettings::Get()?UWildlineUserSettings::Get()->ScopeSensitivity:.256f);ScopeSensitivity->OnValueChanged.AddDynamic(this,&UWildlineMenu::ScopeSensitivityChanged);Add(ScopeSensitivity,10);
    Add(Text(TEXT("SOUND EFFECTS VOLUME"),9,Muted),4);SoundVolume=WidgetTree->ConstructWidget<USlider>();SoundVolume->SetMinValue(0);SoundVolume->SetMaxValue(1);SoundVolume->SetValue(UWildlineUserSettings::Get()?UWildlineUserSettings::Get()->SoundVolume:.8f);SoundVolume->OnValueChanged.AddDynamic(this,&UWildlineMenu::SoundVolumeChanged);Add(SoundVolume,10);
    Add(Text(TEXT("DISPLAY RESOLUTION"),9,Muted),4);
    Resolution=WidgetTree->ConstructWidget<UComboBoxString>();
    if(auto* S=UWildlineUserSettings::Get()){
        const FIntPoint Desktop=S->GetDesktopResolution(),Current=S->GetScreenResolution();
        const FIntPoint Common[]={FIntPoint(1280,720),FIntPoint(1366,768),FIntPoint(1600,900),FIntPoint(1920,1080),FIntPoint(2560,1440),FIntPoint(3840,2160)};
        for(const auto& Size:Common)if(Size.X<=Desktop.X&&Size.Y<=Desktop.Y)ResolutionOptions.AddUnique(Size);
        if(Desktop.X>0&&Desktop.Y>0)ResolutionOptions.AddUnique(Desktop);
        ResolutionOptions.AddUnique(Current);
        ResolutionOptions.Sort([](const FIntPoint& A,const FIntPoint& B){return A.X==B.X?A.Y<B.Y:A.X<B.X;});
        for(const auto& Size:ResolutionOptions)Resolution->AddOption(FString::Printf(TEXT("%d x %d"),Size.X,Size.Y));
        Resolution->SetSelectedIndex(ResolutionOptions.IndexOfByKey(Current));
    }
    Resolution->OnSelectionChanged.AddDynamic(this,&UWildlineMenu::ResolutionChanged);Add(Resolution,7);
    Add(Text(TEXT("High quality / 100% render resolution"),10,Muted),10);
    auto* Exit=Button(TEXT("LEAVE THE FOREST"));Exit->OnClicked.AddDynamic(this,&UWildlineMenu::Quit);Add(Exit,8);
    Status=Text(TEXT("Solo AI or up to 8 hunters and squirrels. Co-op uses the host's IP address."),10,Muted);Add(Status,0);
    return Super::RebuildWidget();
}
FReply UWildlineMenu::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E){if(E.GetKey()==EKeys::Escape){if(auto* PC=Cast<AWildlineController>(GetOwningPlayer()))PC->SetMenu(false);return FReply::Handled();}return Super::NativeOnKeyDown(G,E);}
void UWildlineMenu::Hunter(){bSquirrelSelected=false;RefreshMode();}
void UWildlineMenu::CreateHunter(){if(Creator&&Creator->IsInViewport())return;Creator=CreateWidget<UWildlineHunterCreator>(GetOwningPlayer(),UWildlineHunterCreator::StaticClass());if(Creator){Creator->AddToViewport(20);SetVisibility(ESlateVisibility::Collapsed);FInputModeGameAndUI InputMode;InputMode.SetWidgetToFocus(Creator->TakeWidget());InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);GetOwningPlayer()->SetInputMode(InputMode);}}
void UWildlineMenu::Squirrel(){bSquirrelSelected=true;RefreshMode();}
void UWildlineMenu::RefreshDifficulty(){
    RefreshMode();if(!Difficulty||!DifficultyDescription)return;
    const bool Host=GetOwningPlayer()&&GetOwningPlayer()->HasAuthority();
    const auto* State=GetWorld()->GetGameState<AWildlineGameState>();const auto* Settings=UWildlineUserSettings::Get();
    const int32 Level=FMath::Clamp(!Host&&State?State->HunterDifficulty:Settings?Settings->HunterDifficulty:1,0,2);
    Difficulty->SetSelectedIndex(Level);Difficulty->SetIsEnabled(Host);
    const TCHAR* Help=Level==0?TEXT("Relaxed pursuit, slower shots. Map shows all food, the den and every defender."):Level==2?TEXT("Fast pursuit and precise shots. Nearby food only; defenders are hidden on the map."):TEXT("Balanced pursuit and aim. Nearby food and visible nearby defenders appear on the map.");
    DifficultyDescription->SetText(FText::FromString(FString(Help)+(Host?TEXT(""):TEXT(" Host controls difficulty."))));
}
void UWildlineMenu::DifficultyChanged(FString Value,ESelectInfo::Type SelectionType){
    if(SelectionType==ESelectInfo::Direct||!GetOwningPlayer()||!GetOwningPlayer()->HasAuthority())return;
    const int32 Level=Difficulty->FindOptionIndex(Value);if(Level<0)return;
    if(auto* Settings=UWildlineUserSettings::Get()){Settings->HunterDifficulty=Level;Settings->Persist();}
    if(auto* State=GetWorld()->GetGameState<AWildlineGameState>()){State->HunterDifficulty=Level;State->ForceNetUpdate();}
    RefreshDifficulty();
}
void UWildlineMenu::RefreshMode(){
    if(!Mode||!ModeDescription)return;const bool Host=GetOwningPlayer()&&GetOwningPlayer()->HasAuthority();const auto* State=GetWorld()->GetGameState<AWildlineGameState>();auto* GI=GetGameInstance<UWildlineGameInstance>();
    const int32 Selection=!Host&&State?State->ExpeditionMode:GI?GI->PreferredMode:0;Mode->SetIsEnabled(Host);Mode->SetSelectedIndex(Selection);
    const bool Farm=Selection==1,Hill=Selection==2;const auto* Settings=UWildlineUserSettings::Get();
    FWildlineMatchConfig Config;if(!Host&&State)Config=State->MatchConfig;else if(Settings)Config=const_cast<UWildlineUserSettings*>(Settings)->ConfigForMode(Selection);
    const FString Help=Hill?FString::Printf(TEXT("Race either backyard obstacle route to the bird feeder. %d completed trips win; defenders need %d captures or hold for %.0f minutes. Return to a start pad between trips."),Config.SquirrelGoal,Config.DefenderGoal,Config.DurationSeconds/60):Farm?FString::Printf(TEXT("Steal %d peanuts and return them to the den. Farmers need %d captures or defend for %.0f minutes. Beware livestock."),Config.SquirrelGoal,Config.DefenderGoal,Config.DurationSeconds/60):FString::Printf(TEXT("Store %d acorns or survive %.0f minutes; hunters need %d captures."),Config.SquirrelGoal,Config.DurationSeconds/60,Config.DefenderGoal);
    ModeDescription->SetText(FText::FromString(Help));
    if(DefenderLabel)DefenderLabel->SetText(FText::FromString(Farm?TEXT("THE FARMER"):Hill?TEXT("THE DEFENDER"):TEXT("THE HUNTER")));
    const bool Shotgun=Settings&&Settings->SelectedWeapon==1;
    if(RoleDescription)RoleDescription->SetText(FText::FromString(bSquirrelSelected?(Hill?TEXT("SQUIRREL / Land on every checkpoint: bin, bench, fence, pergola, then feeder. Falling to the lawn resets your run. The pole cannot be climbed."):Farm?TEXT("SQUIRREL / Slip through doors or low vents. Carry four peanuts per trip."):TEXT("SQUIRREL / Climb trunks, launch between trees and bring acorns home.")):(Shotgun?TEXT("DEFENDER / Two-shell shotgun. Aim to tighten spread. No thermal sight."):TEXT("DEFENDER / Scoped rifle. T: 20 seconds of thermal, followed by a 2-minute cooldown."))));
    if(Weapon)Weapon->SetIsEnabled(!bSquirrelSelected);RefreshConfig();
}
void UWildlineMenu::ModeChanged(FString Value,ESelectInfo::Type Type){if(Type==ESelectInfo::Direct||!GetOwningPlayer()->HasAuthority())return;const int32 Selection=Mode->FindOptionIndex(Value);if(Selection<0)return;if(auto* GI=GetGameInstance<UWildlineGameInstance>())GI->PreferredMode=Selection;if(auto* Settings=UWildlineUserSettings::Get()){Settings->ExpeditionMode=Selection;Settings->Persist();}RefreshMode();}
void UWildlineMenu::WeaponChanged(FString Value,ESelectInfo::Type Type){if(Type==ESelectInfo::Direct)return;const int32 Selection=Weapon->FindOptionIndex(Value);if(Selection>=0)if(auto* Settings=UWildlineUserSettings::Get()){Settings->SelectedWeapon=Selection;Settings->Persist();}RefreshMode();}
void UWildlineMenu::Play(){if(auto* PC=Cast<AWildlineController>(GetOwningPlayer()))PC->PlaySelected(bSquirrelSelected);}
void UWildlineMenu::Host(){if(auto* PC=Cast<AWildlineController>(GetOwningPlayer()))PC->Host(bSquirrelSelected);}
void UWildlineMenu::Join(){if(auto* PC=Cast<AWildlineController>(GetOwningPlayer()))PC->Join(Address->GetText().ToString(),bSquirrelSelected);}
void UWildlineMenu::InvertChanged(bool V){if(auto* S=UWildlineUserSettings::Get()){S->bInvertMouseY=V;S->Persist();}}
void UWildlineMenu::BloodChanged(bool V){if(auto* S=UWildlineUserSettings::Get()){S->bBloodEnabled=V;S->Persist();}}
void UWildlineMenu::SensitivityChanged(float V){if(auto* S=UWildlineUserSettings::Get()){S->MouseSensitivity=V;S->Persist();}}
void UWildlineMenu::ResolutionChanged(FString Value,ESelectInfo::Type Type){
    if(Type==ESelectInfo::Direct||!Resolution)return;
    const int32 Index=Resolution->FindOptionIndex(Value);if(!ResolutionOptions.IsValidIndex(Index))return;
    if(auto* S=UWildlineUserSettings::Get()){
        S->SetScreenResolution(ResolutionOptions[Index]);S->ApplySettings(false);S->ConfirmVideoMode();S->Persist();
    }
}
void UWildlineMenu::Quit(){UKismetSystemLibrary::QuitGame(this,GetOwningPlayer(),EQuitPreference::Quit,false);}


void UWildlineMenu::RefreshConfig(){
    if(!PlayerLimit||!RoundMinutes||!SquirrelPoints||!DefenderPoints)return;bRefreshingConfig=true;
    const bool Host=GetOwningPlayer()&&GetOwningPlayer()->HasAuthority();auto* Settings=UWildlineUserSettings::Get();const auto* State=GetWorld()->GetGameState<AWildlineGameState>();
    const int32 Selection=Mode?Mode->GetSelectedIndex():0;FWildlineMatchConfig Config=Host&&Settings?Settings->ConfigForMode(Selection):State?State->MatchConfig:FWildlineMatchConfig();
    PlayerLimit->SetValue(Config.MaxPlayers);RoundMinutes->SetValue(Config.DurationSeconds/60);SquirrelPoints->SetValue(Config.SquirrelGoal);DefenderPoints->SetValue(Config.DefenderGoal);
    for(auto* Box:{PlayerLimit,RoundMinutes,SquirrelPoints,DefenderPoints})Box->SetIsEnabled(Host);
    if(SquirrelPointsLabel)SquirrelPointsLabel->SetText(FText::FromString(Selection==2?TEXT("COMPLETED FEEDER TRIPS TO WIN"):Selection==1?TEXT("STORED PEANUTS TO WIN"):TEXT("STORED ACORNS TO WIN")));bRefreshingConfig=false;
}
void UWildlineMenu::ConfigChanged(float Value){
    if(bRefreshingConfig||!PlayerLimit||!RoundMinutes||!SquirrelPoints||!DefenderPoints||!GetOwningPlayer()->HasAuthority())return;
    if(auto* S=UWildlineUserSettings::Get()){auto& C=S->ConfigForMode(Mode->GetSelectedIndex());C.MaxPlayers=FMath::RoundToInt(PlayerLimit->GetValue());C.DurationSeconds=FMath::RoundToInt(RoundMinutes->GetValue())*60.f;C.SquirrelGoal=FMath::RoundToInt(SquirrelPoints->GetValue());C.DefenderGoal=FMath::RoundToInt(DefenderPoints->GetValue());C.Clamp();S->Persist();}RefreshMode();
}
void UWildlineMenu::ScopeSensitivityChanged(float Value){if(auto* S=UWildlineUserSettings::Get()){S->ScopeSensitivity=Value;S->Persist();}}
void UWildlineMenu::SoundVolumeChanged(float Value){if(auto* S=UWildlineUserSettings::Get()){S->SoundVolume=Value;S->Persist();}}

void UWildlineMenu::ScrollToSettings(){if(FieldScroll)FieldScroll->ScrollToEnd();}

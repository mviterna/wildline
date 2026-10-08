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
#include "Components/WidgetSwitcher.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Engine/Texture2D.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Widgets/Input/SComboBox.h"

UWildlineChoice::UWildlineChoice(){
    const FLinearColor Ink(.018f,.032f,.022f),Sage(.55f,.64f,.50f),Hover(.69f,.77f,.62f),Selected(.80f,.87f,.67f);
    auto Style=GetWidgetStyle();auto Brush=[&](FLinearColor Color){FSlateBrush B;B.DrawAs=ESlateBrushDrawType::RoundedBox;B.TintColor=FSlateColor(Color);B.OutlineSettings=FSlateBrushOutlineSettings(4.f);B.OutlineSettings.Color=FSlateColor(FLinearColor(.22f,.33f,.23f));B.OutlineSettings.Width=1;return B;};
    auto& Buttons=Style.ComboButtonStyle.ButtonStyle;Buttons.Normal=Brush(Sage);Buttons.Hovered=Brush(Hover);Buttons.Pressed=Brush(Selected);Buttons.Disabled=Brush(FLinearColor(.38f,.44f,.35f));
    Style.ComboButtonStyle.DownArrowImage.TintColor=FSlateColor(Ink);Style.ComboButtonStyle.MenuBorderBrush=Brush(Sage);SetWidgetStyle(Style);
    auto Row=GetItemStyle();Row.TextColor=FSlateColor(Ink);Row.SelectedTextColor=FSlateColor(Ink);Row.EvenRowBackgroundBrush=Brush(Sage);Row.OddRowBackgroundBrush=Row.EvenRowBackgroundBrush;
    Row.ActiveBrush=Brush(Selected);Row.InactiveBrush=Row.ActiveBrush;Row.ActiveHoveredBrush=Brush(Hover);Row.InactiveHoveredBrush=Row.ActiveHoveredBrush;Row.EvenRowBackgroundHoveredBrush=Brush(Hover);Row.OddRowBackgroundHoveredBrush=Row.EvenRowBackgroundHoveredBrush;Row.SelectorFocusedBrush=Brush(Selected);SetItemStyle(Row);
    InitForegroundColor(FSlateColor(Ink));auto ChoiceFont=GetFont();ChoiceFont.Size=13;InitFont(ChoiceFont);
}
void UWildlineChoice::SetMenuOpen(bool bOpen){if(MyComboBox.IsValid())MyComboBox->SetIsOpen(bOpen);}

TSharedRef<SWidget> UWildlineBotRow::RebuildWidget(){
    if(WidgetTree->RootWidget)return Super::RebuildWidget();
    auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();WidgetTree->RootWidget=Row;
    auto* Name=WidgetTree->ConstructWidget<UTextBlock>();Name->SetText(FText::FromString(FString::Printf(TEXT("BOT %02d"),Index+1)));Name->SetColorAndOpacity(FSlateColor(FLinearColor(.87f,.9f,.77f)));auto Font=Name->GetFont();Font.Size=12;Name->SetFont(Font);
    auto* NS=Row->AddChildToHorizontalBox(Name);NS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));NS->SetVerticalAlignment(VAlign_Center);
    Weapon=WidgetTree->ConstructWidget<UWildlineChoice>();
    if(bSquirrel)Weapon->AddOption(TEXT("Unarmed / forage & climb"));else{Weapon->AddOption(TEXT("Sporting rifle"));Weapon->AddOption(TEXT("Pump-action shotgun"));}
    auto* WS=Row->AddChildToHorizontalBox(Weapon);FSlateChildSize WeaponSize(ESlateSizeRule::Fill);WeaponSize.Value=2;WS->SetSize(WeaponSize);WS->SetPadding(FMargin(8,0));
    Difficulty=WidgetTree->ConstructWidget<UWildlineChoice>();for(const TCHAR* O:{TEXT("Easy"),TEXT("Medium"),TEXT("Hard")})Difficulty->AddOption(O);
    Row->AddChildToHorizontalBox(Difficulty)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Weapon->OnSelectionChanged.AddDynamic(this,&UWildlineBotRow::Changed);Difficulty->OnSelectionChanged.AddDynamic(this,&UWildlineBotRow::Changed);
    return Super::RebuildWidget();
}
void UWildlineBotRow::Changed(FString Value,ESelectInfo::Type Type){if(Type!=ESelectInfo::Direct&&Menu)Menu->SaveBot(this);}

TSharedRef<SWidget> UWildlineMenu::RebuildWidget(){
    if(WidgetTree->RootWidget)return Super::RebuildWidget();SetIsFocusable(true);
    auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Root;
    const FLinearColor Cream(.98f,.91f,.77f),Sage(1.f,.44f,.09f),Muted(.75f,.73f,.66f);
    auto Text=[&](const FString& Value,int32 Size,FLinearColor Color){auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Value));T->SetColorAndOpacity(FSlateColor(Color));auto Font=T->GetFont();Font.Size=Size;T->SetFont(Font);T->SetAutoWrapText(true);return T;};
    auto* Poster=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Wildline/Presentation/T_MenuPoster.T_MenuPoster"));
    auto Art=[&](FAnchors Anchors,EStretch::Type Stretch){auto* Box=WidgetTree->ConstructWidget<UScaleBox>();Box->SetStretch(Stretch);Box->SetClipping(EWidgetClipping::ClipToBounds);auto* Image=WidgetTree->ConstructWidget<UImage>();Image->SetBrushFromTexture(Poster,true);Box->AddChild(Image);Box->SetVisibility(ESlateVisibility::HitTestInvisible);auto* S=Root->AddChildToCanvas(Box);S->SetAnchors(Anchors);S->SetOffsets(FMargin(0));return Box;};
    Art(FAnchors(0,0,1,1),EStretch::ScaleToFill);
    auto* Shade=WidgetTree->ConstructWidget<UBorder>();Shade->SetBrushColor(FLinearColor(.009f,.012f,.008f,.91f));auto* SS=Root->AddChildToCanvas(Shade);SS->SetAnchors(FAnchors(0,0,1,1));SS->SetOffsets(FMargin(0));Shade->SetVisibility(ESlateVisibility::HitTestInvisible);
    MenuArtwork=Art(FAnchors(0,0,.61f,1),EStretch::ScaleToFit);
    auto Panel=[&](FAnchors Anchors,FMargin Offsets){auto* B=WidgetTree->ConstructWidget<UBorder>();B->SetBrushColor(FLinearColor(.018f,.026f,.02f,.95f));B->SetPadding(FMargin(24));auto* S=Root->AddChildToCanvas(B);S->SetAnchors(Anchors);S->SetOffsets(Offsets);return B;};
    auto* Left=Panel(FAnchors(.61f,0,1,1),FMargin(12,24,24,24));ExpeditionSlot=Cast<UCanvasPanelSlot>(Left->Slot);auto* LeftScroll=WidgetTree->ConstructWidget<UScrollBox>();Left->SetContent(LeftScroll);auto* List=WidgetTree->ConstructWidget<UVerticalBox>();LeftScroll->AddChild(List);
    auto Add=[&](UWidget* W,float Space=8.f){auto* Slot=List->AddChildToVerticalBox(W);Slot->SetPadding(FMargin(0,0,0,Space));};
    auto Button=[&](const FString& Title){auto* B=WidgetTree->ConstructWidget<UButton>();auto Style=B->GetStyle();auto Brush=[](FLinearColor Color){FSlateBrush S;S.DrawAs=ESlateBrushDrawType::RoundedBox;S.TintColor=FSlateColor(Color);S.OutlineSettings=FSlateBrushOutlineSettings(4.f);S.OutlineSettings.Color=FSlateColor(FLinearColor(.07f,.09f,.055f));S.OutlineSettings.Width=1;return S;};Style.Normal=Brush(FLinearColor(.016f,.024f,.018f));Style.Hovered=Brush(FLinearColor(.055f,.075f,.034f));Style.Pressed=Brush(FLinearColor(.3f,.10f,.008f));B->SetStyle(Style);auto* T=Text(Title,12,Cream);T->SetAutoWrapText(false);T->SetMargin(FMargin(12,10));B->AddChild(T);return B;};
    Add(Text(TEXT("WELCOME TO THE WOODS"),10,Sage),5);Add(Text(TEXT("Let's get nuts."),30,Cream),10);
    Add(Text(TEXT("GAME MODE"),10,Sage),5);Mode=WidgetTree->ConstructWidget<UWildlineChoice>();for(const TCHAR* O:{TEXT("Classic / Blackwood"),TEXT("Farm Raid / Peanut Heist"),TEXT("King of the Hill / Bird Feeder")})Mode->AddOption(O);
    auto* Settings=UWildlineUserSettings::Get();if(auto* GI=GetGameInstance<UWildlineGameInstance>()){GI->PreferredMode=Settings?Settings->ExpeditionMode:0;Mode->SetSelectedIndex(GI->PreferredMode);}
    Mode->OnSelectionChanged.AddDynamic(this,&UWildlineMenu::ModeChanged);Add(Mode);ModeDescription=Text(TEXT(""),11,Muted);Add(ModeDescription,18);
    auto* Roles=WidgetTree->ConstructWidget<UHorizontalBox>();auto* H=Button(TEXT("THE HUNTER"));HunterChoice=H;DefenderLabel=Cast<UTextBlock>(H->GetContent());auto* A=Button(TEXT("THE SQUIRREL"));SquirrelChoice=A;H->OnClicked.AddDynamic(this,&UWildlineMenu::Hunter);A->OnClicked.AddDynamic(this,&UWildlineMenu::Squirrel);Roles->AddChildToHorizontalBox(H)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Roles->AddChildToHorizontalBox(A)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Add(Roles);
    RoleDescription=Text(TEXT(""),11,Muted);Add(RoleDescription,12);
    Add(Text(TEXT("YOUR WEAPON"),10,Sage),5);Weapon=WidgetTree->ConstructWidget<UWildlineChoice>();Weapon->AddOption(TEXT("Sporting rifle / precision"));Weapon->AddOption(TEXT("Pump-action shotgun / close range"));Weapon->SetSelectedIndex(Settings?Settings->SelectedWeapon:0);Weapon->OnSelectionChanged.AddDynamic(this,&UWildlineMenu::WeaponChanged);Add(Weapon,10);
    auto* Customize=Button(TEXT("CREATE / EDIT HUNTER"));Customize->OnClicked.AddDynamic(this,&UWildlineMenu::CreateHunter);Add(Customize,18);
    auto* WorkshopButton=Button(TEXT("MATCH RULES / BOTS / SETTINGS"));WorkshopButton->OnClicked.AddDynamic(this,&UWildlineMenu::ToggleWorkshop);Add(WorkshopButton,10);
    auto* PlayButton=Button(TEXT("ENTER THE WILD / RESUME"));auto PlayStyle=PlayButton->GetStyle();PlayStyle.Normal.TintColor=FSlateColor(FLinearColor(.9f,.23f,.012f));PlayStyle.Hovered.TintColor=FSlateColor(FLinearColor(1,.36f,.026f));PlayButton->SetStyle(PlayStyle);PlayButton->OnClicked.AddDynamic(this,&UWildlineMenu::Play);Add(PlayButton);
    auto* HostButton=Button(TEXT("HOST CO-OP EXPEDITION"));HostButton->OnClicked.AddDynamic(this,&UWildlineMenu::Host);Add(HostButton,14);
    Address=WidgetTree->ConstructWidget<UEditableTextBox>();auto AddressStyle=Address->GetWidgetStyle();AddressStyle.TextStyle.Font.Size=14;AddressStyle.TextStyle.ColorAndOpacity=FSlateColor(FLinearColor(.06f,.09f,.07f));Address->SetWidgetStyle(AddressStyle);Address->SetForegroundColor(FLinearColor(.06f,.09f,.07f));Address->SetHintText(FText::FromString(TEXT("Host IP address")));Address->SetText(FText::FromString(TEXT("127.0.0.1")));Add(Address);
    auto* JoinButton=Button(TEXT("JOIN EXPEDITION"));JoinButton->OnClicked.AddDynamic(this,&UWildlineMenu::Join);Add(JoinButton);
    Status=Text(TEXT("Host owns the match rules. Up to 8 humans plus the configured bots."),10,Muted);Add(Status,14);
    auto* Exit=Button(TEXT("QUIT GAME"));Exit->OnClicked.AddDynamic(this,&UWildlineMenu::Quit);Add(Exit,0);

    auto* Right=Panel(FAnchors(.32f,0,1,1),FMargin(12,28,28,28));Workshop=Right;auto* Main=WidgetTree->ConstructWidget<UVerticalBox>();Right->SetContent(Main);
    List=Main;Add(Text(TEXT("MATCH WORKSHOP"),24,Cream),5);ConfigSummary=Text(TEXT(""),11,Sage);Add(ConfigSummary,12);
    auto* Back=Button(TEXT("BACK TO MAIN MENU"));Back->OnClicked.AddDynamic(this,&UWildlineMenu::ToggleWorkshop);Add(Back,10);
    auto* Tabs=WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* RulesTab=Button(TEXT("01 / MATCH RULES"));RulesTab->OnClicked.AddDynamic(this,&UWildlineMenu::ShowRules);
    auto* BotsTab=Button(TEXT("02 / BOT ROSTER"));BotsTab->OnClicked.AddDynamic(this,&UWildlineMenu::ShowBots);
    auto* SettingsTab=Button(TEXT("03 / YOUR SETTINGS"));SettingsTab->OnClicked.AddDynamic(this,&UWildlineMenu::ShowSettings);
    for(auto* B:{RulesTab,BotsTab,SettingsTab}){Tabs->AddChildToHorizontalBox(B)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));TabButtons.Add(B);}Add(Tabs,12);
    Pages=WidgetTree->ConstructWidget<UWidgetSwitcher>();Main->AddChildToVerticalBox(Pages)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto Page=[&](){auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();Pages->AddChild(Scroll);List=WidgetTree->ConstructWidget<UVerticalBox>();Scroll->AddChild(List);return Scroll;};
    Page();
    auto Section=[&](const TCHAR* Title){Add(Text(Title,12,Sage),10);};
    auto Number=[&](USpinBox*& Box,const TCHAR* Label,float Min,float Max,float Step=1.f){auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();auto* LabelSlot=Row->AddChildToHorizontalBox(Text(Label,12,Cream));LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));LabelSlot->SetVerticalAlignment(VAlign_Center);Box=WidgetTree->ConstructWidget<USpinBox>();Box->SetMinValue(Min);Box->SetMaxValue(Max);Box->SetMinSliderValue(Min);Box->SetMaxSliderValue(Max);Box->SetForegroundColor(FSlateColor(FLinearColor(.06f,.09f,.07f)));Box->SetDelta(Step);Box->SetMinFractionalDigits(0);Box->SetMaxFractionalDigits(0);Box->SetMinDesiredWidth(130);Box->OnValueChanged.AddDynamic(this,&UWildlineMenu::ConfigChanged);Row->AddChildToHorizontalBox(Box);Add(Row,10);};
    auto Check=[&](UCheckBox*& Box,const TCHAR* Label){Box=WidgetTree->ConstructWidget<UCheckBox>();auto* T=Text(FString(TEXT("  "))+Label,12,Cream);T->SetAutoWrapText(false);Box->AddChild(T);Box->OnCheckStateChanged.AddDynamic(this,&UWildlineMenu::RulesChecked);Add(Box,10);};
    auto Combo=[&](UComboBoxString*& Box,const TCHAR* Label,std::initializer_list<const TCHAR*> Options){Add(Text(Label,11,Muted),4);Box=WidgetTree->ConstructWidget<UWildlineChoice>();for(const TCHAR* O:Options)Box->AddOption(O);Box->OnSelectionChanged.AddDynamic(this,&UWildlineMenu::RuleSelected);Add(Box,12);};
    Section(TEXT("START WITH A SCHEME"));Presets=WidgetTree->ConstructWidget<UWildlineChoice>();for(const TCHAR* O:{TEXT("Custom / choose a preset to replace these rules"),TEXT("Standard / original balance"),TEXT("Casual / forgiving & easy to read"),TEXT("Hardcore / scarce intel & hard bots"),TEXT("Playground / experiment freely")})Presets->AddOption(O);Presets->SetSelectedIndex(0);Presets->OnSelectionChanged.AddDynamic(this,&UWildlineMenu::PresetSelected);Add(Presets,6);
    Add(Text(TEXT("Presets replace this mode's rules and roster. Every setting stays editable. Changes save automatically and apply when entering a fresh match or restarting with F5."),10,Muted),18);
    Section(TEXT("VICTORY & SESSION"));Number(PlayerLimit,TEXT("Human player limit"),1,8);Number(RoundMinutes,TEXT("Round length / minutes"),1,30);
    auto* GoalRow=WidgetTree->ConstructWidget<UHorizontalBox>();SquirrelPointsLabel=Text(TEXT(""),12,Cream);auto* GS=GoalRow->AddChildToHorizontalBox(SquirrelPointsLabel);GS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));GS->SetVerticalAlignment(VAlign_Center);SquirrelPoints=WidgetTree->ConstructWidget<USpinBox>();SquirrelPoints->SetMinValue(1);SquirrelPoints->SetMaxValue(30);SquirrelPoints->SetMinSliderValue(1);SquirrelPoints->SetMaxSliderValue(30);SquirrelPoints->SetDelta(1);SquirrelPoints->SetMinFractionalDigits(0);SquirrelPoints->SetMaxFractionalDigits(0);SquirrelPoints->SetForegroundColor(FSlateColor(FLinearColor(.06f,.09f,.07f)));SquirrelPoints->SetMinDesiredWidth(130);SquirrelPoints->OnValueChanged.AddDynamic(this,&UWildlineMenu::ConfigChanged);GoalRow->AddChildToHorizontalBox(SquirrelPoints);Add(GoalRow,10);
    Number(DefenderPoints,TEXT("Defender kills to win"),1,30);Combo(TimeWinner,TEXT("WHEN TIME EXPIRES"),{TEXT("Mode default / Classic: squirrels; Farm & Hill: defenders"),TEXT("Defenders win"),TEXT("Squirrels win")});
    Section(TEXT("SURVIVAL & MOVEMENT"));Number(SquirrelHealth,TEXT("Squirrel starting health"),10,200,10);Number(RespawnSeconds,TEXT("Respawn delay / seconds"),1,30);Number(SpawnProtection,TEXT("Spawn protection / seconds"),0,10);Number(CarryCapacity,TEXT("Food carried per trip"),1,12);Number(MovementPercent,TEXT("Movement speed / %"),50,150,10);Check(UnlimitedStamina,TEXT("Unlimited stamina"));
    Add(Text(TEXT("Speed affects running and climbing. Course jumps keep their authored launch strength. Carrying capacity applies to acorns and peanuts."),10,Muted),18);
    Section(TEXT("WEAPONS & HAZARDS"));Number(ReloadPercent,TEXT("Reload duration / %"),25,200,25);Check(InfiniteAmmo,TEXT("Infinite ammunition / no magazine consumption"));Check(AllowThermal,TEXT("Allow rifle thermal sights"));Number(ThermalSeconds,TEXT("Thermal duration / seconds"),5,60,5);Number(ThermalCooldown,TEXT("Thermal cooldown / seconds"),0,300,10);Check(LivestockDamage,TEXT("Livestock can damage squirrels / Farm Raid"));
    Section(TEXT("MAP & INTELLIGENCE"));Check(Minimap,TEXT("Allow minimap"));Combo(MapIntel,TEXT("MINIMAP INFORMATION / INDEPENDENT OF BOT DIFFICULTY"),{TEXT("Full / all food and defenders"),TEXT("Nearby / food and visible defenders"),TEXT("Limited / nearby food, defenders hidden")});

    Page();Section(TEXT("BUILD YOUR BOT ROSTER"));Add(Text(TEXT("Exact bot counts, in addition to human players. Set either team to zero to disable its bots. Removed slots keep their loadout if you add them back."),11,Muted),16);
    auto RosterHeader=[&](const TCHAR* Team,const TCHAR* Equipment){auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();auto* NS=Row->AddChildToHorizontalBox(Text(Team,10,Sage));NS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));auto* WS=Row->AddChildToHorizontalBox(Text(Equipment,10,Sage));FSlateChildSize Wide(ESlateSizeRule::Fill);Wide.Value=2;WS->SetSize(Wide);WS->SetPadding(FMargin(8,0));Row->AddChildToHorizontalBox(Text(TEXT("AI DIFFICULTY"),10,Sage))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Add(Row,8);};
    Number(DefenderBotCount,TEXT("Defender bots / 0-8"),0,8);RosterHeader(TEXT("DEFENDER"),TEXT("WEAPON"));
    for(int32 I=0;I<FWildlineMatchConfig::MaxBotsPerTeam;I++){auto* Row=CreateWidget<UWildlineBotRow>(GetOwningPlayer());Row->Menu=this;Row->Index=I;Add(Row,10);Row->TakeWidget();BotRows.Add(Row);}
    Add(Text(TEXT("Easy: slower reactions and wide aim. Medium: balanced pursuit. Hard: longer sight, faster reactions and precise leading shots. All defenders need line of sight."),10,Muted),18);
    Number(SquirrelBotCount,TEXT("Squirrel bots / 0-8"),0,8);RosterHeader(TEXT("SQUIRREL"),TEXT("ABILITIES"));
    for(int32 I=0;I<FWildlineMatchConfig::MaxBotsPerTeam;I++){auto* Row=CreateWidget<UWildlineBotRow>(GetOwningPlayer());Row->Menu=this;Row->Index=I;Row->bSquirrel=true;Add(Row,10);Row->TakeWidget();BotRows.Add(Row);}
    Add(Text(TEXT("Squirrels are unarmed. Difficulty changes how quickly they notice and react to danger; course bots pause between jumps."),10,Muted),12);
    Add(Text(TEXT("A match needs opponents to earn kills, and enough food to reach its goal. Zero bots is useful for co-op or practicing alone."),10,Muted),12);

    FieldScroll=Page();Section(TEXT("YOUR CONTROLS & PRESENTATION"));
    Inversion=WidgetTree->ConstructWidget<UCheckBox>();Inversion->AddChild(Text(TEXT("  Invert mouse Y"),12,Cream));Inversion->SetIsChecked(Settings?Settings->bInvertMouseY:true);Inversion->OnCheckStateChanged.AddDynamic(this,&UWildlineMenu::InvertChanged);Add(Inversion,10);
    Blood=WidgetTree->ConstructWidget<UCheckBox>();Blood->AddChild(Text(TEXT("  Blood effects"),12,Cream));Blood->SetIsChecked(Settings?Settings->bBloodEnabled:true);Blood->OnCheckStateChanged.AddDynamic(this,&UWildlineMenu::BloodChanged);Add(Blood,16);
    Add(Text(TEXT("MOUSE SENSITIVITY"),10,Muted),4);Sensitivity=WidgetTree->ConstructWidget<USlider>();Sensitivity->SetMinValue(.15f);Sensitivity->SetMaxValue(2.f);Sensitivity->SetValue(Settings?Settings->MouseSensitivity:.8f);Sensitivity->OnValueChanged.AddDynamic(this,&UWildlineMenu::SensitivityChanged);Add(Sensitivity,18);
    Add(Text(TEXT("SCOPE MOUSE SENSITIVITY"),10,Muted),4);ScopeSensitivity=WidgetTree->ConstructWidget<USlider>();ScopeSensitivity->SetMinValue(.05f);ScopeSensitivity->SetMaxValue(2.f);ScopeSensitivity->SetValue(Settings?Settings->ScopeSensitivity:.256f);ScopeSensitivity->OnValueChanged.AddDynamic(this,&UWildlineMenu::ScopeSensitivityChanged);Add(ScopeSensitivity,18);
    Add(Text(TEXT("SOUND EFFECTS VOLUME"),10,Muted),4);SoundVolume=WidgetTree->ConstructWidget<USlider>();SoundVolume->SetMinValue(0);SoundVolume->SetMaxValue(1);SoundVolume->SetValue(Settings?Settings->SoundVolume:.8f);SoundVolume->OnValueChanged.AddDynamic(this,&UWildlineMenu::SoundVolumeChanged);Add(SoundVolume,18);
    Add(Text(TEXT("MENU MUSIC / BLUEGRASS"),10,Muted),4);MusicVolume=WidgetTree->ConstructWidget<USlider>();MusicVolume->SetMinValue(0);MusicVolume->SetMaxValue(1);MusicVolume->SetValue(Settings?Settings->MusicVolume:.65f);MusicVolume->OnValueChanged.AddDynamic(this,&UWildlineMenu::MusicVolumeChanged);Add(MusicVolume,18);
    Add(Text(TEXT("DISPLAY RESOLUTION"),10,Muted),4);Resolution=WidgetTree->ConstructWidget<UWildlineChoice>();
    if(Settings){const FIntPoint Desktop=Settings->GetDesktopResolution(),Current=Settings->GetScreenResolution();const FIntPoint Common[]={FIntPoint(1280,720),FIntPoint(1366,768),FIntPoint(1600,900),FIntPoint(1920,1080),FIntPoint(2560,1440),FIntPoint(3840,2160)};for(const auto& Size:Common)if(Size.X<=Desktop.X&&Size.Y<=Desktop.Y)ResolutionOptions.AddUnique(Size);if(Desktop.X>0&&Desktop.Y>0)ResolutionOptions.AddUnique(Desktop);ResolutionOptions.AddUnique(Current);ResolutionOptions.Sort([](const FIntPoint& A,const FIntPoint& B){return A.X==B.X?A.Y<B.Y:A.X<B.X;});for(const auto& Size:ResolutionOptions)Resolution->AddOption(FString::Printf(TEXT("%d x %d"),Size.X,Size.Y));Resolution->SetSelectedIndex(ResolutionOptions.IndexOfByKey(Current));}
    Resolution->OnSelectionChanged.AddDynamic(this,&UWildlineMenu::ResolutionChanged);Add(Resolution,10);Add(Text(TEXT("High quality / 100% render resolution"),10,Muted),20);
    Add(Text(TEXT("WASD move / Shift sprint / V camera\nHunter: LMB fire, RMB scope, T thermal, R reload\nSquirrel: E grab a tree, W/S climb, Space launch\nM: minimap zoom, when allowed / Esc: return to camp"),12,Cream));
    Pages->SetActiveWidgetIndex(0);SetWorkshop(false);RefreshMode();return Super::RebuildWidget();
}
FReply UWildlineMenu::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E){if(E.GetKey()==EKeys::Escape){if(bWorkshopOpen)SetWorkshop(false);else if(auto* PC=Cast<AWildlineController>(GetOwningPlayer()))PC->SetMenu(false);return FReply::Handled();}return Super::NativeOnKeyDown(G,E);}
void UWildlineMenu::Hunter(){bSquirrelSelected=false;RefreshMode();}
void UWildlineMenu::CreateHunter(){if(Creator&&Creator->IsInViewport())return;Creator=CreateWidget<UWildlineHunterCreator>(GetOwningPlayer(),UWildlineHunterCreator::StaticClass());if(Creator){Creator->AddToViewport(20);SetVisibility(ESlateVisibility::Collapsed);FInputModeGameAndUI InputMode;InputMode.SetWidgetToFocus(Creator->TakeWidget());InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);GetOwningPlayer()->SetInputMode(InputMode);}}
void UWildlineMenu::Squirrel(){bSquirrelSelected=true;RefreshMode();}
void UWildlineMenu::RefreshDifficulty(){RefreshMode();}
void UWildlineMenu::RefreshMode(){
    if(!Mode||!ModeDescription)return;const bool Host=GetOwningPlayer()&&GetOwningPlayer()->HasAuthority();const auto* State=GetWorld()->GetGameState<AWildlineGameState>();auto* GI=GetGameInstance<UWildlineGameInstance>();
    const int32 Selection=!Host&&State?State->ExpeditionMode:GI?GI->PreferredMode:0;Mode->SetIsEnabled(Host);Mode->SetSelectedIndex(Selection);
    const bool Farm=Selection==1,Hill=Selection==2;const auto* Settings=UWildlineUserSettings::Get();
    FWildlineMatchConfig Config;if(!Host&&State)Config=State->MatchConfig;else if(Settings)Config=const_cast<UWildlineUserSettings*>(Settings)->ConfigForMode(Selection);
    const int32 Timeout=Config.TimeWinner?Config.TimeWinner:WildlineRules::TimeoutWinner(Selection);
    const FString Objective=Hill?FString::Printf(TEXT("Complete %d feeder trips through either route."),Config.SquirrelGoal):FString::Printf(TEXT("Store %d %s at the den."),Config.SquirrelGoal,Farm?TEXT("peanuts"):TEXT("acorns"));
    const FString Help=Objective+FString::Printf(TEXT(" Defenders need %d kills. After %.0f minutes, %s win."),Config.DefenderGoal,Config.DurationSeconds/60,Timeout==1?TEXT("defenders"):TEXT("squirrels"));
    ModeDescription->SetText(FText::FromString(Help));
    if(DefenderLabel)DefenderLabel->SetText(FText::FromString(Farm?TEXT("THE FARMER"):Hill?TEXT("THE DEFENDER"):TEXT("THE HUNTER")));
    const bool Shotgun=Settings&&Settings->SelectedWeapon==1;
    if(RoleDescription)RoleDescription->SetText(FText::FromString(bSquirrelSelected?(Hill?TEXT("SQUIRREL / Land on every checkpoint: bin, bench, fence, pergola, then feeder. Falling to the lawn resets your run. The pole cannot be climbed."):Farm?TEXT("SQUIRREL / Slip through doors or low vents. Carry peanuts back to the den."):TEXT("SQUIRREL / Climb trunks, launch between trees and bring acorns home.")):(Shotgun?TEXT("DEFENDER / Two-shell shotgun. Aim to tighten spread. No thermal sight."):TEXT("DEFENDER / Scoped rifle. Thermal availability and timing follow the match rules."))));
    if(Weapon)Weapon->SetIsEnabled(!bSquirrelSelected);
    for(auto* Choice:{HunterChoice,SquirrelChoice})if(Choice){auto Style=Choice->GetStyle();const bool Selected=Choice==(bSquirrelSelected?SquirrelChoice:HunterChoice);Style.Normal.OutlineSettings.Color=FSlateColor(Selected?FLinearColor(.9f,.23f,.012f):FLinearColor(.07f,.09f,.055f));Style.Normal.OutlineSettings.Width=Selected?2:1;Choice->SetStyle(Style);}
    RefreshConfig();
}
void UWildlineMenu::ModeChanged(FString Value,ESelectInfo::Type Type){if(Type==ESelectInfo::Direct||!GetOwningPlayer()->HasAuthority())return;const int32 Selection=Mode->FindOptionIndex(Value);if(Selection<0)return;if(auto* GI=GetGameInstance<UWildlineGameInstance>())GI->PreferredMode=Selection;if(auto* Settings=UWildlineUserSettings::Get()){Settings->ExpeditionMode=Selection;Settings->Persist();}if(Presets)Presets->SetSelectedIndex(0);RefreshMode();}
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
    if(!Pages||!PlayerLimit||!Minimap)return;bRefreshingConfig=true;
    const bool Host=GetOwningPlayer()&&GetOwningPlayer()->HasAuthority();auto* Settings=UWildlineUserSettings::Get();const auto* State=GetWorld()->GetGameState<AWildlineGameState>();
    const int32 Selection=Mode?Mode->GetSelectedIndex():0;FWildlineMatchConfig C=Host&&Settings?Settings->ConfigForMode(Selection):State?State->MatchConfig:FWildlineMatchConfig();C.Clamp();
    PlayerLimit->SetValue(C.MaxPlayers);RoundMinutes->SetValue(C.DurationSeconds/60);SquirrelPoints->SetValue(C.SquirrelGoal);DefenderPoints->SetValue(C.DefenderGoal);
    DefenderBotCount->SetValue(C.DefenderBots);SquirrelBotCount->SetValue(C.SquirrelBots);SquirrelHealth->SetValue(C.SquirrelHealth);RespawnSeconds->SetValue(C.RespawnSeconds);SpawnProtection->SetValue(C.SpawnProtectionSeconds);CarryCapacity->SetValue(C.CarryCapacity);MovementPercent->SetValue(C.MovementScale*100);ReloadPercent->SetValue(C.ReloadScale*100);ThermalSeconds->SetValue(C.ThermalSeconds);ThermalCooldown->SetValue(C.ThermalCooldownSeconds);
    for(auto* Box:{PlayerLimit,RoundMinutes,SquirrelPoints,DefenderPoints,DefenderBotCount,SquirrelBotCount,SquirrelHealth,RespawnSeconds,SpawnProtection,CarryCapacity,MovementPercent,ReloadPercent,ThermalSeconds,ThermalCooldown})Box->SetIsEnabled(Host);
    Minimap->SetIsChecked(C.bAllowMinimap);UnlimitedStamina->SetIsChecked(C.bUnlimitedStamina);InfiniteAmmo->SetIsChecked(C.bInfiniteAmmo);AllowThermal->SetIsChecked(C.bAllowThermal);LivestockDamage->SetIsChecked(C.bLivestockDamage);
    for(auto* Box:{Minimap,UnlimitedStamina,InfiniteAmmo,AllowThermal,LivestockDamage})Box->SetIsEnabled(Host);
    LivestockDamage->SetIsEnabled(Host&&Selection==1);CarryCapacity->SetIsEnabled(Host&&Selection!=2);
    MapIntel->SetSelectedIndex(C.MapIntel);MapIntel->SetIsEnabled(Host&&C.bAllowMinimap);TimeWinner->SetSelectedIndex(C.TimeWinner);TimeWinner->SetIsEnabled(Host);Presets->SetIsEnabled(Host);
    ThermalSeconds->SetIsEnabled(Host&&C.bAllowThermal);ThermalCooldown->SetIsEnabled(Host&&C.bAllowThermal);ReloadPercent->SetIsEnabled(Host&&!C.bInfiniteAmmo);
    for(auto* Row:BotRows){const auto& B=Row->bSquirrel?C.Squirrels[Row->Index]:C.Defenders[Row->Index];Row->SetVisibility(Row->Index<(Row->bSquirrel?C.SquirrelBots:C.DefenderBots)?ESlateVisibility::Visible:ESlateVisibility::Collapsed);Row->Weapon->SetSelectedIndex(Row->bSquirrel?0:B.Weapon);Row->Weapon->SetIsEnabled(Host&&!Row->bSquirrel);Row->Difficulty->SetSelectedIndex(B.Difficulty);Row->Difficulty->SetIsEnabled(Host);}
    SquirrelPointsLabel->SetText(FText::FromString(Selection==2?TEXT("Completed feeder trips to win"):Selection==1?TEXT("Stored peanuts to win"):TEXT("Stored acorns to win")));
    ConfigSummary->SetText(FText::FromString(FString::Printf(TEXT("%.0f MIN  /  %d KILLS  /  %d DEFENDER + %d SQUIRREL BOTS  /  MAP %s%s"),C.DurationSeconds/60,C.DefenderGoal,C.DefenderBots,C.SquirrelBots,C.bAllowMinimap?TEXT("ON"):TEXT("OFF"),Host?TEXT("  /  SAVED PER MODE"):TEXT("  /  HOST RULES"))));bRefreshingConfig=false;
}
void UWildlineMenu::ConfigChanged(float Value){
    if(bRefreshingConfig||!Minimap||!GetOwningPlayer()||!GetOwningPlayer()->HasAuthority())return;
    if(auto* S=UWildlineUserSettings::Get()){auto& C=S->ConfigForMode(Mode->GetSelectedIndex());C.MaxPlayers=FMath::RoundToInt(PlayerLimit->GetValue());C.DurationSeconds=FMath::RoundToInt(RoundMinutes->GetValue())*60.f;C.SquirrelGoal=FMath::RoundToInt(SquirrelPoints->GetValue());C.DefenderGoal=FMath::RoundToInt(DefenderPoints->GetValue());
        C.DefenderBots=FMath::RoundToInt(DefenderBotCount->GetValue());C.SquirrelBots=FMath::RoundToInt(SquirrelBotCount->GetValue());C.SquirrelHealth=SquirrelHealth->GetValue();C.RespawnSeconds=RespawnSeconds->GetValue();C.SpawnProtectionSeconds=SpawnProtection->GetValue();C.CarryCapacity=FMath::RoundToInt(CarryCapacity->GetValue());C.MovementScale=MovementPercent->GetValue()/100.f;C.ReloadScale=ReloadPercent->GetValue()/100.f;C.ThermalSeconds=ThermalSeconds->GetValue();C.ThermalCooldownSeconds=ThermalCooldown->GetValue();
        C.bAllowMinimap=Minimap->IsChecked();C.bUnlimitedStamina=UnlimitedStamina->IsChecked();C.bInfiniteAmmo=InfiniteAmmo->IsChecked();C.bAllowThermal=AllowThermal->IsChecked();C.bLivestockDamage=LivestockDamage->IsChecked();C.MapIntel=MapIntel->GetSelectedIndex();C.TimeWinner=TimeWinner->GetSelectedIndex();C.Clamp();S->Persist();Presets->SetSelectedIndex(0);
    }RefreshMode();
}
void UWildlineMenu::RulesChecked(bool Value){ConfigChanged(0);}
void UWildlineMenu::RuleSelected(FString Value,ESelectInfo::Type Type){if(Type!=ESelectInfo::Direct)ConfigChanged(0);}
void UWildlineMenu::PresetSelected(FString Value,ESelectInfo::Type Type){
    if(Type==ESelectInfo::Direct||!GetOwningPlayer()->HasAuthority()||Presets->GetSelectedIndex()<=0)return;
    if(auto* S=UWildlineUserSettings::Get()){S->ConfigForMode(Mode->GetSelectedIndex())=FWildlineMatchConfig::Preset(Mode->GetSelectedIndex(),Presets->GetSelectedIndex()-1);S->Persist();}RefreshMode();
}
void UWildlineMenu::SaveBot(UWildlineBotRow* Row){
    if(bRefreshingConfig||!Row||!GetOwningPlayer()->HasAuthority())return;
    if(auto* S=UWildlineUserSettings::Get()){auto& C=S->ConfigForMode(Mode->GetSelectedIndex());auto& B=Row->bSquirrel?C.Squirrels[Row->Index]:C.Defenders[Row->Index];B.Weapon=Row->bSquirrel?0:Row->Weapon->GetSelectedIndex();B.Difficulty=Row->Difficulty->GetSelectedIndex();C.Clamp();S->Persist();Presets->SetSelectedIndex(0);}RefreshConfig();
}
void UWildlineMenu::ShowRules(){SetWorkshop(true);if(Pages)Pages->SetActiveWidgetIndex(0);for(int32 I=0;I<TabButtons.Num();I++)TabButtons[I]->SetBackgroundColor(I==0?FLinearColor(.7f,.9f,.45f):FLinearColor(.35f,.45f,.35f));}
void UWildlineMenu::ShowBots(){SetWorkshop(true);if(Pages)Pages->SetActiveWidgetIndex(1);for(int32 I=0;I<TabButtons.Num();I++)TabButtons[I]->SetBackgroundColor(I==1?FLinearColor(.7f,.9f,.45f):FLinearColor(.35f,.45f,.35f));}
void UWildlineMenu::ShowSettings(){SetWorkshop(true);if(Pages)Pages->SetActiveWidgetIndex(2);for(int32 I=0;I<TabButtons.Num();I++)TabButtons[I]->SetBackgroundColor(I==2?FLinearColor(.7f,.9f,.45f):FLinearColor(.35f,.45f,.35f));}
void UWildlineMenu::ScopeSensitivityChanged(float Value){if(auto* S=UWildlineUserSettings::Get()){S->ScopeSensitivity=Value;S->Persist();}}
void UWildlineMenu::SoundVolumeChanged(float Value){if(auto* S=UWildlineUserSettings::Get()){S->SoundVolume=Value;S->Persist();}}

void UWildlineMenu::ScrollToSettings(){ShowSettings();if(FieldScroll)FieldScroll->ScrollToStart();}

void UWildlineMenu::SetWorkshop(bool Open){
    bWorkshopOpen=Open;
    if(Workshop)Workshop->SetVisibility(Open?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    if(MenuArtwork)MenuArtwork->SetVisibility(Open?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
    if(ExpeditionSlot){ExpeditionSlot->SetAnchors(Open?FAnchors(0,0,.32f,1):FAnchors(.61f,0,1,1));ExpeditionSlot->SetOffsets(Open?FMargin(28,28,12,28):FMargin(12,24,24,24));}
}
void UWildlineMenu::ToggleWorkshop(){SetWorkshop(!bWorkshopOpen);}
void UWildlineMenu::MusicVolumeChanged(float Value){if(auto* S=UWildlineUserSettings::Get()){S->MusicVolume=Value;S->Persist();}}

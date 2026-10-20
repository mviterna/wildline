#include "WildlineSocial.h"
#include "WildlineGame.h"
#include "WildlineMenu.h"
#include "WildlineRoom.h"
#include "WildlineCharacter.h"
#include "WildlineUserSettings.h"
#include "WildlineHunterCreator.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputKeyEventArgs.h"
#include "Net/UnrealNetwork.h"

TSharedRef<SWidget> UWildlineSocial::RebuildWidget(){
    if(WidgetTree->RootWidget)return Super::RebuildWidget();SetIsFocusable(true);
    auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Root;
    auto Text=[&](const FString& Value,int Size){auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Value));auto Font=T->GetFont();Font.Size=Size;T->SetFont(Font);T->SetColorAndOpacity(FLinearColor(.98f,.91f,.77f));T->SetAutoWrapText(true);return T;};
    auto Button=[&](const FString& Title){auto* B=WidgetTree->ConstructWidget<UButton>();B->SetBackgroundColor(FLinearColor(.07f,.13f,.09f));auto* T=Text(Title,13);T->SetAutoWrapText(false);T->SetMargin(FMargin(12,9));B->AddChild(T);return B;};
    auto* Profile=Button(TEXT(""));Badge=Cast<UTextBlock>(Profile->GetContent());Profile->OnClicked.AddDynamic(this,&UWildlineSocial::Toggle);
    auto* BS=Root->AddChildToCanvas(Profile);BS->SetAnchors(FAnchors(1,0));BS->SetAlignment(FVector2D(1,0));BS->SetPosition(FVector2D(-22,16));BS->SetAutoSize(true);
    Panel=WidgetTree->ConstructWidget<UBorder>();Panel->SetBrushColor(FLinearColor(.012f,.024f,.017f,.99f));Panel->SetPadding(FMargin(28));
    auto* PS=Root->AddChildToCanvas(Panel);PS->SetAnchors(FAnchors(.5f,.5f));PS->SetAlignment(FVector2D(.5f,.5f));PS->SetSize(FVector2D(690,820));PS->SetPosition(FVector2D(0,0));
    auto* List=WidgetTree->ConstructWidget<UVerticalBox>();Panel->SetContent(List);
    auto Add=[&](UWidget* W){List->AddChildToVerticalBox(W)->SetPadding(FMargin(0,0,0,12));};
    Title=Text(TEXT("YOUR PROFILE & ROOM  /  SHIFT + TAB"),20);Add(Title);
    Message=Text(TEXT(""),12);Add(Message);
    ProfileFields=WidgetTree->ConstructWidget<UVerticalBox>();Add(ProfileFields);
    Name=WidgetTree->ConstructWidget<UEditableTextBox>();auto NameStyle=Name->GetWidgetStyle();NameStyle.TextStyle.Font.Size=16;NameStyle.TextStyle.ColorAndOpacity=FSlateColor(FLinearColor(.025f,.055f,.035f));Name->SetWidgetStyle(NameStyle);Name->SetForegroundColor(FLinearColor(.025f,.055f,.035f));Name->SetHintText(FText::FromString(TEXT("Choose your player name")));Name->SetText(FText::FromString(UWildlineUserSettings::Get()->PlayerDisplayName));ProfileFields->AddChildToVerticalBox(Name)->SetPadding(FMargin(0,0,0,12));
    auto* Save=Button(TEXT("SAVE NAME"));Save->OnClicked.AddDynamic(this,&UWildlineSocial::SaveName);ProfileFields->AddChildToVerticalBox(Save);
    RenameButton=Button(TEXT("EDIT MY NAME"));RenameButton->OnClicked.AddDynamic(this,&UWildlineSocial::EditName);Add(RenameButton);
    RoomStatus=Text(TEXT(""),12);Add(RoomStatus);Roster=Text(TEXT(""),12);auto* RosterScroll=WidgetTree->ConstructWidget<UScrollBox>();RosterScroll->AddChild(Roster);RosterBox=WidgetTree->ConstructWidget<USizeBox>();RosterBox->SetHeightOverride(220);RosterBox->SetContent(RosterScroll);Add(RosterBox);
    Board=WidgetTree->ConstructWidget<UVerticalBox>();Add(Board);
    auto* Actions=WidgetTree->ConstructWidget<UHorizontalBox>();Add(Actions);
    auto Action=[&](UButton* B){auto* ActionSlot=Actions->AddChildToHorizontalBox(B);ActionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));ActionSlot->SetPadding(FMargin(2,0));};
    auto* H=Button(TEXT("JOIN HUNTERS"));H->OnClicked.AddDynamic(this,&UWildlineSocial::Hunter);Action(H);
    auto* S=Button(TEXT("JOIN SQUIRRELS"));S->OnClicked.AddDynamic(this,&UWildlineSocial::Squirrel);Action(S);
    JoinButton=Button(TEXT("JOIN NETWORK ROOM"));JoinButton->OnClicked.AddDynamic(this,&UWildlineSocial::JoinRoom);Action(JoinButton);
    auto* Close=Button(TEXT("CLOSE  /  SHIFT + TAB"));Close->OnClicked.AddDynamic(this,&UWildlineSocial::Toggle);Add(Close);
    Panel->SetVisibility(ESlateVisibility::Collapsed);Refresh();return Super::RebuildWidget();
}
void UWildlineSocial::NativeTick(const FGeometry& G,float D){Super::NativeTick(G,D);}
void UWildlineSocial::Refresh(){
    if(!Badge)return;auto* S=UWildlineUserSettings::Get();auto* PC=Cast<AWildlineController>(GetOwningPlayer());
    const bool Missing=!S||S->PlayerDisplayName.IsEmpty();Badge->SetText(FText::FromString(Missing?TEXT("PROFILE  /  SET NAME"):FString(TEXT("●  "))+S->PlayerDisplayName));
    bGameplayBoard=PC&&!PC->bMenuOpen&&!Missing&&!bEditingName;
    Panel->SetBrushColor(bGameplayBoard?FLinearColor(.012f,.02f,.03f,.30f):FLinearColor(.012f,.024f,.017f,.99f));
    if(auto* PanelSlot=Cast<UCanvasPanelSlot>(Panel->Slot))PanelSlot->SetSize(bGameplayBoard?FVector2D(1100,590):FVector2D(690,730));
    ProfileFields->SetVisibility(Missing||bEditingName?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    RenameButton->SetVisibility(!Missing&&!bEditingName&&!bGameplayBoard?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    RosterBox->SetVisibility(bGameplayBoard?ESlateVisibility::Collapsed:ESlateVisibility::Visible);
    Board->SetVisibility(bGameplayBoard?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    Title->SetText(FText::FromString(Missing?TEXT("WELCOME / CHOOSE YOUR NAME"):bEditingName?TEXT("EDIT YOUR NAME"):bGameplayBoard?TEXT("WILDLINE   /   SCOREBOARD"):TEXT("YOUR ROOM  /  SHIFT + TAB")));
    const bool Intro=PC&&PC->Intro;SetVisibility(Intro?ESlateVisibility::Collapsed:ESlateVisibility::SelfHitTestInvisible);
    if(Missing&&!Intro&&!bOpen)Open(true);
    Message->SetText(FText::FromString(Missing?TEXT("Welcome! Choose a name for this PC (1–24 characters). Your profile is saved locally."):TEXT("Live stats: K / D = kills / deaths. Food = banked / picked. Aim = successful shots / fired; one hit per shotgun blast. Stats reset each round.")));
    auto* Room=GetGameInstance()->GetSubsystem<UWildlineRoomSubsystem>();const FString Address=Room?Room->RoomAddress():FString();
    const bool Connected=GetWorld()->GetNetMode()!=NM_Standalone;
    RoomStatus->SetText(FText::FromString(Connected?TEXT("CONNECTED ROOM  /  host controls match rules"):!Address.IsEmpty()?TEXT("ROOM FOUND  /  ")+Room->RoomLabel():TEXT("No active room found. Anyone can host from the menu.")));
    JoinButton->SetIsEnabled(!Connected&&!Missing&&!Address.IsEmpty());
    FString Hunters=TEXT("HUNTERS\n"),Squirrels=TEXT("SQUIRRELS\n");int32 H=0,A=0;
    TArray<AWildlinePlayerState*> Players;
    if(const auto* State=GetWorld()->GetGameState<AWildlineGameState>())for(const auto& Base:State->PlayerArray){auto* P=Cast<AWildlinePlayerState>(Base.Get());if(!P||P->IsInactive())continue;
        Players.Add(P);const FString Display=P==PC->PlayerState&&Missing?TEXT("Choose your name"):P->GetPlayerName();FString Row=TEXT("  ")+Display+(P==PC->PlayerState?TEXT(" (you)"):TEXT(""))+(P->bWaiting?TEXT(" — waiting room\n"):TEXT(" — playing\n"));Row+=FString::Printf(TEXT("  K %d / D %d | Food %d/%d | Aim %.0f%% (%d/%d) | Feeder %d\n\n"),P->Kills,P->Deaths,P->FoodDeposited,P->FoodCollected,P->HitPercentage(),P->Hits,P->Shots,P->FeederScores);if(P->bSquirrelTeam){Squirrels+=Row;A++;}else{Hunters+=Row;H++;}}
    if(!H)Hunters+=TEXT("  No players\n");if(!A)Squirrels+=TEXT("  No players\n");Roster->SetText(FText::FromString(Hunters+TEXT("\n")+Squirrels));
    if(bGameplayBoard&&bOpen){
        Board->ClearChildren();
        auto Row=[&](const TArray<FString>& Values,FLinearColor Background,FLinearColor Ink,int Size){
            auto* Back=WidgetTree->ConstructWidget<UBorder>();Back->SetBrushColor(Background);Back->SetPadding(FMargin(12,9));
            auto* Line=WidgetTree->ConstructWidget<UHorizontalBox>();Back->SetContent(Line);
            const float Widths[]={290,135,55,55,125,80,120,80};
            for(int I=0;I<Values.Num();I++){auto* Cell=WidgetTree->ConstructWidget<USizeBox>();Cell->SetWidthOverride(Widths[I]);auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Values[I]));auto Font=T->GetFont();Font.Size=Size;T->SetFont(Font);T->SetColorAndOpacity(Ink);T->SetJustification(I<2?ETextJustify::Left:ETextJustify::Center);T->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);T->SetShadowOffset(FVector2D(0,1));T->SetShadowColorAndOpacity(FLinearColor(0,0,0,.85f));Cell->SetContent(T);Line->AddChildToHorizontalBox(Cell);}
            Board->AddChildToVerticalBox(Back)->SetPadding(FMargin(0,0,0,2));
        };
        Row({TEXT("PLAYER"),TEXT("LOCATION"),TEXT("K"),TEXT("D"),TEXT("BANK / PICK"),TEXT("HIT %"),TEXT("HITS / SHOTS"),TEXT("FEEDERS")},FLinearColor(0,0,0,.35f),FLinearColor(.75f,.80f,.85f),11);
        Players.Sort([](const AWildlinePlayerState& L,const AWildlinePlayerState& R){return L.Kills!=R.Kills?L.Kills>R.Kills:L.GetPlayerName()<R.GetPlayerName();});
        for(bool Animal:{false,true}){
            const FLinearColor Team=Animal?FLinearColor(.61f,.84f,.66f):FLinearColor(.94f,.75f,.43f);
            Row({FString::Printf(TEXT("%s  /  %d"),Animal?TEXT("SQUIRRELS"):TEXT("HUNTERS"),Animal?A:H)},Animal?FLinearColor(.04f,.17f,.10f,.55f):FLinearColor(.22f,.13f,.035f,.55f),Team,14);
            int Count=0;for(auto* P:Players)if(P->bSquirrelTeam==Animal){Count++;const bool Local=P==PC->PlayerState;
                Row({P->GetPlayerName()+(Local?TEXT("  (you)"):TEXT("")),P->bWaiting?TEXT("Waiting room"):TEXT("In match"),FString::FromInt(P->Kills),FString::FromInt(P->Deaths),FString::Printf(TEXT("%d / %d"),P->FoodDeposited,P->FoodCollected),FString::Printf(TEXT("%.0f%%"),P->HitPercentage()),FString::Printf(TEXT("%d / %d"),P->Hits,P->Shots),FString::FromInt(P->FeederScores)},Local?FLinearColor(.10f,.20f,.29f,.45f):FLinearColor(.01f,.015f,.025f,.24f),FLinearColor(.94f,.96f,.98f),14);
            }if(!Count)Row({TEXT("Open team")},FLinearColor(0,0,0,.15f),FLinearColor(.65f,.70f,.75f),12);
        }
    }
}
void UWildlineSocial::Open(bool Value){if(!Value&&UWildlineUserSettings::Get()->PlayerDisplayName.IsEmpty())Value=true;if(!Value)bEditingName=false;bOpen=Value;if(Panel){Panel->SetVisibility(Value?ESlateVisibility::Visible:ESlateVisibility::Collapsed);Refresh();}if(auto* PC=Cast<AWildlineController>(GetOwningPlayer()))PC->RefreshSocialInput();}
void UWildlineSocial::Toggle(){Open(!bOpen);}
void UWildlineSocial::SaveName(){auto* S=UWildlineUserSettings::Get();const FString Clean=UWildlineUserSettings::CleanPlayerName(Name->GetText().ToString());if(Clean.IsEmpty()){Name->SetError(FText::FromString(TEXT("Enter a player name.")));return;}Name->ClearError();S->PlayerDisplayName=Clean;S->Persist();if(!S->bLastProfileSaveSucceeded){Name->SetError(FText::FromString(TEXT("Could not save your profile. Please check write access to your profile folder.")));return;}bEditingName=false;Name->SetText(FText::FromString(Clean));if(auto* PC=Cast<AWildlineController>(GetOwningPlayer()))PC->ServerSetProfileName(Clean);Open(false);Refresh();}
void UWildlineSocial::Hunter(){if(UWildlineUserSettings::Get()->PlayerDisplayName.IsEmpty())return;if(auto* PC=Cast<AWildlineController>(GetOwningPlayer())){PC->EnterWaitingRoom(false);Open(false);}}
void UWildlineSocial::Squirrel(){if(UWildlineUserSettings::Get()->PlayerDisplayName.IsEmpty())return;if(auto* PC=Cast<AWildlineController>(GetOwningPlayer())){PC->EnterWaitingRoom(true);Open(false);}}
void UWildlineSocial::JoinRoom(){auto* Room=GetGameInstance()->GetSubsystem<UWildlineRoomSubsystem>();if(auto* PC=Cast<AWildlineController>(GetOwningPlayer());PC&&Room&&!Room->RoomAddress().IsEmpty())PC->Join(Room->RoomAddress(),GetGameInstance<UWildlineGameInstance>()->bPreferredSquirrel);}
FReply UWildlineSocial::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E){if((E.GetKey()==EKeys::Tab&&E.IsShiftDown())||E.GetKey()==EKeys::Escape){Toggle();return FReply::Handled();}return Super::NativeOnPreviewKeyDown(G,E);}

void AWildlinePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AWildlinePlayerState,bSquirrelTeam);DOREPLIFETIME(AWildlinePlayerState,bWaiting);DOREPLIFETIME(AWildlinePlayerState,Kills);DOREPLIFETIME(AWildlinePlayerState,Deaths);DOREPLIFETIME(AWildlinePlayerState,Shots);DOREPLIFETIME(AWildlinePlayerState,Hits);DOREPLIFETIME(AWildlinePlayerState,FoodCollected);DOREPLIFETIME(AWildlinePlayerState,FoodDeposited);DOREPLIFETIME(AWildlinePlayerState,FeederScores);}
void AWildlinePlayerState::ResetMatchStats(){Kills=Deaths=Shots=Hits=FoodCollected=FoodDeposited=FeederScores=0;ForceNetUpdate();}
void AWildlineController::ServerSetProfileName_Implementation(const FString& Name){const FString Clean=UWildlineUserSettings::CleanPlayerName(Name);if(!Clean.IsEmpty()&&PlayerState){PlayerState->SetPlayerName(Clean);PlayerState->ForceNetUpdate();}}
void AWildlineController::ToggleSocial(){if(!Intro&&Social)Social->Toggle();}
void AWildlineController::RefreshSocialInput(){
    const bool Open=Social&&Social->bOpen;bShowMouseCursor=bMenuOpen||Open;
    if(Open){if(auto* P=Cast<AWildlineCharacter>(GetPawn())){P->bFiring=false;P->StopAim();P->StopSprint();P->GetCharacterMovement()->StopMovementImmediately();}FInputModeUIOnly Mode;Mode.SetWidgetToFocus(Social->TakeWidget());Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);}
    else if(bMenuOpen){FInputModeGameAndUI Mode;if(Menu){if(Menu->Creator&&Menu->Creator->IsInViewport())Mode.SetWidgetToFocus(Menu->Creator->TakeWidget());else Mode.SetWidgetToFocus(Menu->TakeWidget());}Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);}else{FInputModeGameOnly Mode;SetInputMode(Mode);}
}
bool AWildlineController::InputKey(const FInputKeyEventArgs& P){if(P.Key==EKeys::Tab&&P.Event==IE_Pressed&&(IsInputKeyDown(EKeys::LeftShift)||IsInputKeyDown(EKeys::RightShift))){ToggleSocial();return true;}return Super::InputKey(P);}

void UWildlineSocial::EditName(){bEditingName=true;Name->SetText(FText::FromString(UWildlineUserSettings::Get()->PlayerDisplayName));Open(true);Name->SetKeyboardFocus();}

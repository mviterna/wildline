#include "WildlineMenu.h"
#include "WildlineGame.h"
#include "WildlineUserSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "Components/Image.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Texture2D.h"

namespace {
const TCHAR* MapNames[]={TEXT("Blackwood"),TEXT("Frostwood"),TEXT("Farmstead"),TEXT("Maple Crescent"),TEXT("Cedar Backyards"),TEXT("Taylorsville Lake")};
const TCHAR* MapAssets[]={TEXT("Blackwood"),TEXT("Frostwood"),TEXT("Farmstead"),TEXT("Neighborhood"),TEXT("NeighborhoodCedar"),TEXT("TaylorsvilleLake")};
const TCHAR* Details[]={
    TEXT("CLASSIC HUNT / Autumn woodland\nA rolling reserve of oak and pine, leaf-covered clearings, fallen timber and a hollow den. Squirrels climb trunks and leap between branches to gather acorns; hunters track them through cover. Store acorns to win, or stop the squirrels before time expires."),
    TEXT("CLASSIC HUNT / Winter woodland\nSnow-covered trails and bare oak branches open long sightlines through the reserve. Squirrels must use trunks, rocks and terrain for cover while carrying acorns to their den. The winter layout retains the classic team objectives and your woodland controls."),
    TEXT("FARM RAID / Peanut heist\nRaid a furnished farmhouse through its doors and low vents, cross crop rows and bring stolen peanuts back to the den. Farmers defend the pantry with rifle or shotgun. Cows and pigs can strike nearby squirrels, so use crop cover and give livestock room."),
    TEXT("KING OF THE HILL / Bird feeder\nA suburban course through Maple Crescent. Climb the utility pole, cross the telephone line and oak branches, or take the garden route. Land on the fence and trampoline before reaching the feeder. Defenders watch both approaches; each successful feeder trip scores for the squirrels."),
    TEXT("KING OF THE HILL / Bird feeder\nStaggered gardens, backyard fences and household obstacles change the route to the feeder. Choose the overhead utility line or the ground course and land on each checkpoint. Return to the lawn to begin another run. Defenders must protect the feeder from both approaches."),
    TEXT("LAKESIDE HUNT / Fishing lodge\nA 200-meter wooded lake reserve with coves and a continuous shore trail, fishing piers and rocky coves. Gather acorns on dry land or hunt along the banks. Hunters can swim and dive among fish, submerged limestone and fallen limbs; weapons cannot fire underwater. Squirrels cannot swim and drown in the lake. While waiting, visit the lodge dock with its moored boats, rods and tackle: E picks up a rod. Hold LMB and release to cast, E hooks a bite, hold LMB to reel, release to ease a surge, and R returns the rod. Space rises; Ctrl dives.")};
}
TSharedRef<SWidget> UWildlineMapCard::RebuildWidget(){
    if(WidgetTree->RootWidget)return Super::RebuildWidget();
    Button=WidgetTree->ConstructWidget<UButton>();WidgetTree->RootWidget=Button;Button->OnClicked.AddDynamic(this,&UWildlineMapCard::Select);
    auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();Button->AddChild(Row);
    auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(230);Size->SetHeightOverride(150);
    auto* Scale=WidgetTree->ConstructWidget<UScaleBox>();Scale->SetStretch(EStretch::ScaleToFill);Scale->SetClipping(EWidgetClipping::ClipToBounds);Size->AddChild(Scale);
    auto* Image=WidgetTree->ConstructWidget<UImage>();const FString Path=FString::Printf(TEXT("/Game/Wildline/Lake/Previews/T_Map_%s.T_Map_%s"),MapAssets[MapIndex],MapAssets[MapIndex]);Image->SetBrushFromTexture(LoadObject<UTexture2D>(nullptr,*Path),true);Scale->AddChild(Image);
    auto* PreviewSlot=Row->AddChildToHorizontalBox(Size);PreviewSlot->SetPadding(FMargin(10));PreviewSlot->SetVerticalAlignment(VAlign_Center);
    auto* Copy=WidgetTree->ConstructWidget<UVerticalBox>();auto* CopySlot=Row->AddChildToHorizontalBox(Copy);CopySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));CopySlot->SetPadding(FMargin(8,15,15,15));
    auto Text=[&](const TCHAR* Value,int32 FontSize,FLinearColor Color){auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Value));auto Font=T->GetFont();Font.Size=FontSize;T->SetFont(Font);T->SetColorAndOpacity(Color);T->SetAutoWrapText(true);return T;};
    Copy->AddChildToVerticalBox(Text(MapNames[MapIndex],20,FLinearColor(1,.77f,.42f)))->SetPadding(FMargin(0,0,0,8));Copy->AddChildToVerticalBox(Text(Details[MapIndex],12,FLinearColor(.92f,.92f,.84f)));
    return Super::RebuildWidget();
}
void UWildlineMapCard::Select(){if(Menu)Menu->SelectMap(MapIndex);}
void UWildlineMenu::SelectMap(int32 Index){
    if(Index<0||Index>5||!GetOwningPlayer()->HasAuthority())return;
    const int32 Selection=Index==5?3:Index==2?1:Index>=3?2:0;
    if(auto* GI=GetGameInstance<UWildlineGameInstance>())GI->PreferredMode=Selection;
    if(auto* S=UWildlineUserSettings::Get()){S->ExpeditionMode=Selection;if(Selection==0)S->ClassicConfig.ClassicLayout=Index==1?1:0;if(Selection==2)S->HillConfig.NeighborhoodLayout=Index==4?1:0;S->Persist();}
    if(Presets)Presets->SetSelectedIndex(0);RefreshMode();
}
void UWildlineMenu::ShowMaps(){SetWorkshop(true);if(Pages)Pages->SetActiveWidgetIndex(3);for(int32 I=0;I<TabButtons.Num();I++)TabButtons[I]->SetBackgroundColor(I==3?FLinearColor(.7f,.9f,.45f):FLinearColor(.35f,.45f,.35f));}

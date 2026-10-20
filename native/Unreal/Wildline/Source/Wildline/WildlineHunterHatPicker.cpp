#include "WildlineHunterCreator.h"
#include "WildlineHeadwear.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UWildlineHatButton::Choose(){if(Creator){if(HatId>=0)Creator->SelectHat(HatId);else Creator->SetHatFilter(-100-HatId);}}
void UWildlineHunterCreator::BuildHeadwearPicker(UBorder* Parent){
    auto Text=[this](FString Value,int32 Size,FLinearColor Color){auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Value));auto Font=T->GetFont();Font.Size=Size;T->SetFont(Font);T->SetColorAndOpacity(Color);T->SetAutoWrapText(true);return T;};
    AppearanceScroll->RemoveFromParent();auto* Layers=WidgetTree->ConstructWidget<UOverlay>();Parent->SetContent(Layers);Layers->AddChildToOverlay(AppearanceScroll);
    HatPicker=WidgetTree->ConstructWidget<UBorder>();HatPicker->SetBrushColor(FLinearColor(.026f,.045f,.033f));HatPicker->SetPadding(FMargin(0));Layers->AddChildToOverlay(HatPicker);
    auto* List=WidgetTree->ConstructWidget<UVerticalBox>();HatPicker->SetContent(List);
    List->AddChildToVerticalBox(Text(TEXT("HEADWEAR / 36 STYLES"),13,FLinearColor(.72f,.82f,.5f)))->SetPadding(FMargin(0,0,0,8));
    List->AddChildToVerticalBox(Text(TEXT("Find your field look."),25,FLinearColor(.92f,.94f,.86f)))->SetPadding(FMargin(0,0,0,8));
    List->AddChildToVerticalBox(Text(TEXT("Pick a thumbnail to try it on. Scroll for more, and turn the portrait to inspect the fit."),11,FLinearColor(.61f,.72f,.64f)))->SetPadding(FMargin(0,0,0,12));
    auto* Filters=WidgetTree->ConstructWidget<UHorizontalBox>();List->AddChildToVerticalBox(Filters)->SetPadding(FMargin(0,0,0,10));
    const TCHAR* Names[]={TEXT("ALL"),TEXT("FIELD / WORK"),TEXT("FUNNY")};
    for(int32 I=0;I<3;I++){auto* B=WidgetTree->ConstructWidget<UWildlineHatButton>();B->Creator=this;B->HatId=-100-I;B->OnClicked.AddDynamic(B,&UWildlineHatButton::Choose);auto* T=Text(Names[I],10,FLinearColor(.88f,.92f,.8f));T->SetMargin(FMargin(8,7));B->SetContent(T);B->SetBackgroundColor(FLinearColor(.15f,.24f,.17f));Filters->AddChildToHorizontalBox(B)->SetPadding(FMargin(0,0,5,0));HatFilters.Add(B);}
    HatSelection=Text(TEXT(""),12,FLinearColor(.95f,.77f,.35f));List->AddChildToVerticalBox(HatSelection)->SetPadding(FMargin(0,0,0,10));
    HatScroll=WidgetTree->ConstructWidget<UScrollBox>();HatScroll->SetScrollBarVisibility(ESlateVisibility::Visible);HatGrid=WidgetTree->ConstructWidget<UUniformGridPanel>();HatGrid->SetSlotPadding(FMargin(3));HatScroll->AddChild(HatGrid);List->AddChildToVerticalBox(HatScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Back=WidgetTree->ConstructWidget<UButton>();Back->SetBackgroundColor(FLinearColor(.19f,.28f,.17f));auto* BT=Text(TEXT("BACK TO APPEARANCE"),11,FLinearColor(.9f,.93f,.83f));BT->SetMargin(FMargin(10,10));Back->SetContent(BT);Back->OnClicked.AddDynamic(this,&UWildlineHunterCreator::CloseHeadwear);List->AddChildToVerticalBox(Back)->SetPadding(FMargin(0,12,0,0));
    HatPicker->SetVisibility(ESlateVisibility::Collapsed);RefreshHatCards();
}
void UWildlineHunterCreator::RefreshHatCards(){
    if(HatButtonLabel)HatButtonLabel->SetText(FText::FromString(FString(TEXT("BROWSE HATS / "))+WildlineHeadwear::Styles[WildlineHeadwear::Clamp(Pending.Headwear)].Name));
    for(int32 I=0;I<HatFilters.Num();I++){auto S=HatFilters[I]->GetStyle();S.Normal.TintColor=FSlateColor(I==HatFilter?FLinearColor(.32f,.37f,.18f):FLinearColor(.09f,.16f,.12f));HatFilters[I]->SetStyle(S);}
    if(!HatGrid)return;HatGrid->ClearChildren();HatCards.Empty();int32 CellIndex=0;
    if(HatSelection)HatSelection->SetText(FText::FromString(FString(TEXT("WEARING / "))+WildlineHeadwear::Styles[WildlineHeadwear::Clamp(Pending.Headwear)].Name));
    for(int32 Id=0;Id<WildlineHeadwear::Count;Id++){
        const auto& Style=WildlineHeadwear::Styles[Id];if(Id>0&&((HatFilter==1&&Style.Funny)||(HatFilter==2&&!Style.Funny)))continue;
        auto* Card=WidgetTree->ConstructWidget<UWildlineHatButton>();Card->HatId=Id;Card->Creator=this;Card->OnClicked.AddDynamic(Card,&UWildlineHatButton::Choose);Card->SetToolTipText(FText::FromString(Style.Name));
        auto ButtonStyle=Card->GetStyle();ButtonStyle.Normal.TintColor=FSlateColor(Id==Pending.Headwear?FLinearColor(.43f,.33f,.10f):FLinearColor(.07f,.12f,.095f));ButtonStyle.Hovered.TintColor=FSlateColor(FLinearColor(.3f,.4f,.22f));ButtonStyle.Pressed.TintColor=FSlateColor(FLinearColor(.49f,.39f,.14f));Card->SetStyle(ButtonStyle);
        auto* Content=WidgetTree->ConstructWidget<UVerticalBox>();Card->SetContent(Content);auto* Image=WidgetTree->ConstructWidget<UImage>();Image->SetBrushFromTexture(LoadObject<UTexture2D>(nullptr,*WildlineHeadwear::Asset(Id,true)));
        auto* ImageSize=WidgetTree->ConstructWidget<USizeBox>();ImageSize->SetHeightOverride(88);ImageSize->SetContent(Image);Content->AddChildToVerticalBox(ImageSize);
        auto* Name=WidgetTree->ConstructWidget<UTextBlock>();Name->SetText(FText::FromString(Style.Name));auto Font=Name->GetFont();Font.Size=10;Name->SetFont(Font);Name->SetColorAndOpacity(FLinearColor(.92f,.94f,.84f));Name->SetJustification(ETextJustify::Center);Name->SetAutoWrapText(true);Name->SetMargin(FMargin(3,5));Content->AddChildToVerticalBox(Name);
        auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(104);Size->SetHeightOverride(151);Size->SetContent(Card);HatGrid->AddChildToUniformGrid(Size,CellIndex/3,CellIndex%3);HatCards.Add(Card);CellIndex++;
    }
}
void UWildlineHunterCreator::OpenHeadwear(){bHatPicker=true;bFaceView=true;PreviewAngle=-16;HatPicker->SetVisibility(ESlateVisibility::Visible);AppearanceScroll->SetVisibility(ESlateVisibility::Collapsed);RefreshHatCards();UpdateCamera();}
void UWildlineHunterCreator::CloseHeadwear(){bHatPicker=false;HatPicker->SetVisibility(ESlateVisibility::Collapsed);AppearanceScroll->SetVisibility(ESlateVisibility::Visible);UpdateCamera();}
void UWildlineHunterCreator::SetHatFilter(int32 Filter){HatFilter=FMath::Clamp(Filter,0,2);RefreshHatCards();HatScroll->ScrollToStart();}
void UWildlineHunterCreator::SelectHat(int32 Id){Pending.Headwear=WildlineHeadwear::Clamp(Id);Pending.bCap=Pending.Headwear==1;RefreshPreview();RefreshHatCards();}

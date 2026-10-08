#include "WildlineHUD.h"
#include "WildlineLobby.h"
#include "WildlineCharacter.h"
#include "WildlineGame.h"
#include "WildlineWorld.h"
#include "WildlineRules.h"
#include "WildlineLivestock.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "CanvasItem.h"
#include "EngineUtils.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
static FString ClockText(float Seconds){const int32 T=FMath::Max(0,FMath::CeilToInt(Seconds));return FString::Printf(TEXT("%02d:%02d"),T/60,T%60);}
void AWildlineHUD::Label(const FString& Text,float X,float Y,float Size,FLinearColor Color){FCanvasTextItem T(FVector2D(X,Y),FText::FromString(Text),GEngine->GetSmallFont(),Color);T.Scale=FVector2D(Size/12.f);T.EnableShadow(FLinearColor(0,0,0,.8f));Canvas->DrawItem(T);}
void AWildlineHUD::DrawHUD(){
    Super::DrawHUD();auto* PC=Cast<AWildlineController>(GetOwningPlayerController());auto* P=PC?Cast<AWildlineCharacter>(PC->GetPawn()):nullptr;auto* S=GetWorld()->GetGameState<AWildlineGameState>();if(!Canvas||!P||!S||PC->bMenuOpen)return;
    const bool Hill=S->ExpeditionMode==WildlineRules::HillMode,Farm=S->ExpeditionMode==WildlineRules::FarmMode;const float W=Canvas->SizeX,H=Canvas->SizeY,CX=W*.5f,CY=H*.5f;const FLinearColor Cream(.88f,.91f,.78f),Sage(.70f,.79f,.51f),Dim(.63f,.70f,.60f),Panel(.025f,.04f,.032f,.65f);
    if(P->bInLobby){
        DrawRect(Panel,20,18,W-40,111);
        Label(P->bSquirrel?TEXT("THE ACORN HOLLOW"):TEXT("BLACKWOOD HUNTING LODGE"),38,30,23,Cream);
        Label(P->bSquirrel?TEXT("SQUIRREL / Forage, climb and leap"):P->SelectedWeapon==1?TEXT("EQUIPPED / Pump-action shotgun"):TEXT("EQUIPPED / Sporting rifle"),38,65,14,Sage);
        Label(S->Winner?TEXT("ROUND FINISHED / Press F5 for the next round"):S->bRoundStarted?TEXT("ROUND IN PROGRESS / Step onto your portal to join"):FString::Printf(TEXT("%d / %d READY / The round starts when everyone is on a portal"),S->LobbyReadyPlayers,S->LobbyPlayers),38,96,12,Dim);
        DrawRect(Panel,20,H-110,W-40,90);
        Label(S->Winner?TEXT("F5 / RETURN EVERYONE TO THE WAITING ROOMS"):P->bLobbyReady?TEXT("READY / Waiting for the other players. Step off to cancel."):TEXT("WHEN READY / Walk onto the glowing portal"),38,H-91,17,Sage);
        Label(P->bSquirrel?TEXT("WASD Move   SPACE Leap   V Camera   ESC Menu"):TEXT("WASD Move   E Equip at weapon rack   SPACE Jump   V Camera   ESC Menu"),38,H-55,12,Cream);
        if(!P->bSquirrel){const int32 Weapon=AWildlineLobby::WeaponAt(P->GetActorLocation());if(Weapon!=INDEX_NONE)Label(Weapon==1?TEXT("E / EQUIP SHOTGUN"):TEXT("E / EQUIP RIFLE"),CX-100,CY+65,18,Cream);}
        DrawLine(CX-4,CY,CX+4,CY,Cream,1);DrawLine(CX,CY-4,CX,CY+4,Cream,1);return;
    }
    if(P->IsScoped()){
        const float Radius=H*.31f;DrawRect(FLinearColor::Black,0,0,CX-Radius,H);DrawRect(FLinearColor::Black,CX+Radius,0,W-CX-Radius,H);DrawRect(FLinearColor::Black,CX-Radius,0,Radius*2,CY-Radius);DrawRect(FLinearColor::Black,CX-Radius,CY+Radius,Radius*2,H-CY-Radius);
        for(float Y=-Radius;Y<Radius;Y+=2){const float Span=FMath::Sqrt(FMath::Max(0.f,Radius*Radius-Y*Y));DrawRect(FLinearColor::Black,CX-Radius,CY+Y,Radius-Span,2);DrawRect(FLinearColor::Black,CX+Span,CY+Y,Radius-Span,2);}
        for(int32 I=0;I<120;I++){const float A=I*2*PI/120,B=(I+1)*2*PI/120;DrawLine(CX+FMath::Cos(A)*Radius,CY+FMath::Sin(A)*Radius,CX+FMath::Cos(B)*Radius,CY+FMath::Sin(B)*Radius,Cream,2);}
        // An outlined reticle remains readable against sky, foliage and thermal highlights.
        DrawLine(CX-Radius+6,CY,CX+Radius-6,CY,FLinearColor::Black,4);
        DrawLine(CX,CY-Radius+6,CX,CY+Radius-6,FLinearColor::Black,4);
        DrawLine(CX-Radius+6,CY,CX+Radius-6,CY,FLinearColor::White,1.5f);
        DrawLine(CX,CY-Radius+6,CX,CY+Radius-6,FLinearColor::White,1.5f);
        for(int32 I=-3;I<=3;I++)if(I){DrawLine(CX+I*Radius*.14f,CY-5,CX+I*Radius*.14f,CY+5,FLinearColor::Black,3);DrawLine(CX-5,CY+I*Radius*.14f,CX+5,CY+I*Radius*.14f,FLinearColor::Black,3);}
        DrawRect(FLinearColor::Black,CX-4,CY-4,8,8);DrawRect(FLinearColor(1.f,.2f,.06f),CX-2,CY-2,4,4);
        Label(P->IsThermalActive()?TEXT("THERMAL IMAGING"):TEXT("BLACKWOOD OPTICS"),CX-85,CY-Radius+60,12,Cream);if(P->IsThermalActive())Label(FString::Printf(TEXT("%.1fs"),FMath::Max(0.f,P->ThermalUntil-P->ServerTime())),CX-20,CY-Radius+85,15,Sage);
    }else {DrawLine(CX-5,CY,CX+5,CY,Cream,1);DrawLine(CX,CY-5,CX,CY+5,Cream,1);}
    Label(Hill?TEXT("W  /  KING OF THE HILL"):Farm?TEXT("W  /  FARM RAID"):TEXT("W  /  BLACKWOOD RESERVE"),30,25,18,Cream);Label(P->bSquirrel?TEXT("SQUIRREL"):Farm?TEXT("FARMER"):Hill?TEXT("DEFENDER"):TEXT("HUNTER"),30,50,11,Sage);
    Label(FString::Printf(TEXT("%d / %d KILLS    %s    %s %d / %d"),S->HunterKills,S->MatchConfig.DefenderGoal,*ClockText(S->RoundRemaining()),Hill?TEXT("FEEDER TRIPS"):TEXT("STORED"),S->StoredAcorns,S->MatchConfig.SquirrelGoal),CX-260,30,14,Cream);
    if(!P->IsScoped()){
        Label(TEXT("FIELD OBJECTIVE"),30,120,11,Sage);
        const FString Goal=P->bSquirrel?(Hill?FString::Printf(TEXT("Complete %d trips through the course to the feeder."),S->MatchConfig.SquirrelGoal):FString::Printf(TEXT("Bring %d %ss back to the den."),S->MatchConfig.SquirrelGoal,Farm?TEXT("peanut"):TEXT("acorn"))):FString::Printf(TEXT("Kill %d squirrels%s."),S->MatchConfig.DefenderGoal,(S->MatchConfig.TimeWinner?S->MatchConfig.TimeWinner:WildlineRules::TimeoutWinner(S->ExpeditionMode))==1?TEXT(" or defend until time expires"):TEXT(" with your team"));
        Label(Goal,30,143,15,Cream);
        Label(Hill?TEXT("Two routes / land on every checkpoint / return to start after scoring"):P->bSquirrel?(Farm?TEXT("Use crop cover and low wall vents. Give livestock space."):TEXT("E: climb   W/S: ascend/descend   Space: long leap")):TEXT("Trees and solid scenery stop bullets."),30,167,11,Dim);
    }
    DrawRect(Panel,30,H-63,170,4);DrawRect(Sage,30,H-63,170*P->Stamina/100.f,4);Label(TEXT("STAMINA"),30,H-85,10,Cream);
    Label(TEXT("WASD Move     SHIFT Sprint     V First / third person     ESC Camp"),CX-250,H-35,11,Dim);
    if(P->GetActorLocation().Size2D()>WildlineRules::MapRadius-650.f)Label(TEXT("RESERVE BOUNDARY  /  RETURN TO THE WOODS"),CX-195,H-110,13,Sage);
    if(P->bSquirrel){Label(Hill?FString::Printf(TEXT("%d FEEDER TRIPS"),P->FeederVisits):FString::Printf(TEXT("%02d / %02d   %sS"),P->Bag,S->MatchConfig.CarryCapacity,WildlineRules::FoodName(S->ExpeditionMode)),W-230,H-72,24,Cream);if(P->TreeIndex!=INDEX_NONE)Label(TEXT("CLIMBING  /  E RELEASE  /  SPACE LAUNCH"),CX-185,CY+90,14,Sage);else if(auto* F=AWildlineWorld::Find(GetWorld());F&&F->NearestTree(P->GetActorLocation(),30)!=INDEX_NONE)Label(TEXT("E  /  CLIMB TREE"),CX-70,CY+90,14,Sage);}
    else {Label(FString::Printf(TEXT("%02d / %02d"),P->Ammo,WildlineRules::WeaponCapacity(P->SelectedWeapon)),W-140,H-84,30,Cream);Label(P->SelectedWeapon==1?TEXT("PUMP-ACTION SHOTGUN"):TEXT("BOLT-ACTION RIFLE"),W-180,H-48,10,Dim);const float CD=FMath::Max(0.f,P->ThermalReadyAt-P->ServerTime());if(P->SelectedWeapon==WildlineRules::RifleWeapon)Label(P->IsThermalActive()?TEXT("THERMAL ACTIVE  ")+ClockText(P->ThermalUntil-P->ServerTime()):CD>0?TEXT("THERMAL RECHARGING  ")+ClockText(CD):TEXT("T  THERMAL READY"),W-450,H-65,12,Sage);if(P->ReloadUntil>0)Label(TEXT("RELOADING"),CX-55,CY+90,14,Cream);}
    if(P->bSquirrel&&!P->bDead){DrawRect(Panel,220,H-63,140,4);DrawRect(FLinearColor(.85f,.38f,.24f),220,H-63,140*P->Health/S->MatchConfig.SquirrelHealth,4);Label(FString::Printf(TEXT("HEALTH  %.0f / %.0f"),P->Health,S->MatchConfig.SquirrelHealth),220,H-85,10,Cream);}
    if(Farm&&S->MatchConfig.bLivestockDamage&&!P->bDead){for(TActorIterator<AWildlineLivestock> It(GetWorld());It;++It)if(It->StrikeAt>P->ServerTime()&&It->StrikeAt-P->ServerTime()<1&&FVector::Dist2D(P->GetActorLocation(),It->GetActorLocation())<300){Label(TEXT("LIVESTOCK STARTLED  /  MOVE AWAY!"),CX-160,CY+65,16,FLinearColor(1,.5f,.22f));break;}}
    if(P->bDead){DrawRect(FLinearColor(0,0,0,.55f),0,0,W,H);Label(TEXT("THE HUNT ENDS HERE."),CX-180,CY-30,30,Cream);Label(FString::Printf(TEXT("Returning to the den in %d seconds"),FMath::Max(0,FMath::CeilToInt(P->RespawnAt-P->ServerTime()))),CX-140,CY+20,15,Dim);}
    if(S->Winner){DrawRect(FLinearColor(.025f,.05f,.03f,.8f),0,0,W,H);Label(S->Winner==1?(Hill?TEXT("THE FEEDER IS SAFE."):Farm?TEXT("THE FARM IS SAFE."):TEXT("THE HUNTERS PREVAIL.")):(Hill?TEXT("THE SQUIRRELS REACH THE FEEDER!"):Farm?TEXT("THE PEANUT HEIST SUCCEEDS."):TEXT("THE CANOPY WINS.")),CX-200,CY-40,34,Cream);Label(TEXT("F5  /  NEW ROUND       ESC  /  CAMP"),CX-180,CY+30,15,Sage);}
    if(P->bSquirrel&&!P->bDead&&P->ProtectedUntil>P->ServerTime())Label(FString::Printf(TEXT("SPAWN PROTECTION  %.1fs"),P->ProtectedUntil-P->ServerTime()),CX-115,CY+115,12,Sage);
    if(!P->IsScoped()&&!P->bDead&&!S->Winner){if(auto* F=AWildlineWorld::Find(GetWorld())){if(S->MatchConfig.bAllowMinimap)DrawReserveMap(P,S,F);if(P->bSquirrel)DrawForageHints(P,F);}}
}

void AWildlineHUD::DrawReserveMap(AWildlineCharacter* P,AWildlineGameState* S,AWildlineWorld* F){
    const float H=Canvas->SizeY;const FVector2D Centre(154,H-227);const float Half=108.f;
    const auto* PC=Cast<AWildlineController>(GetOwningPlayerController());const bool Zoom=PC&&PC->bMapZoomed;
    const float Range=Zoom?3500.f:F->bResidential?5500.f:F->bFarm?7000.f:WildlineRules::MapRadius+400;const FVector Origin=Zoom?P->GetActorLocation():FVector::ZeroVector;
    const FLinearColor Cream(.94f,.96f,.84f),Gold(1.f,.72f,.25f),Home(.56f,.9f,.7f),Red(1.f,.28f,.21f),Muted(.53f,.64f,.56f);
    DrawRect(FLinearColor(.013f,.026f,.022f,.94f),30,H-367,248,267);
    DrawLine(30,H-367,278,H-367,Home,.8f);Label(FString::Printf(TEXT("%s  /  %s"),F->bResidential?TEXT("NEIGHBORHOOD"):F->bFarm?TEXT("FARMSTEAD"):TEXT("BLACKWOOD"),S->MatchConfig.MapIntel==0?TEXT("FULL INTEL"):S->MatchConfig.MapIntel==2?TEXT("LIMITED INTEL"):TEXT("NEARBY INTEL")),42,H-356,10,Cream);
    Label(Zoom?TEXT("M: RESERVE MAP  /  70 m"):F->bFarm?TEXT("M: ZOOM IN  /  140 m"):TEXT("M: ZOOM IN  /  290 m"),42,H-344,8,Muted);
    auto Map=[&](FVector World){const FVector D=(World-Origin)*(Half/Range);return Centre+FVector2D(D.Y,-D.X);};
    auto Inside=[&](FVector2D Point,float Margin=3.f){return FMath::Abs(Point.X-Centre.X)<Half-Margin&&FMath::Abs(Point.Y-Centre.Y)<Half-Margin;};
    auto Ring=[&](FVector2D Point,float Radius,FLinearColor Color,float Thickness=1.f){for(int32 I=0;I<20;I++){const float A=I*2*PI/20,B=(I+1)*2*PI/20;DrawLine(Point.X+FMath::Cos(A)*Radius,Point.Y+FMath::Sin(A)*Radius,Point.X+FMath::Cos(B)*Radius,Point.Y+FMath::Sin(B)*Radius,Color,Thickness);}};
    auto Segment=[&](FVector A,FVector B,FLinearColor Color,float Thickness=1.f){const auto MA=Map(A),MB=Map(B);if(Inside(MA,0)&&Inside(MB,0))DrawLine(MA.X,MA.Y,MB.X,MB.Y,Color,Thickness);};
    // Low contrast terrain relief, trail and trees give markers a spatial reference.
    for(int32 Y=0;Y<24;Y++)for(int32 X=0;X<24;X++){
        const float WX=Origin.X+(1-(Y+.5f)/12.f)*Range,WY=Origin.Y+((X+.5f)/12.f-1)*Range;
        if(FVector2D(WX,WY).Size()>WildlineRules::MapRadius)continue;
        const float Shade=FMath::Clamp((F->GroundHeight(WX,WY)+550)/1100.f,0.f,1.f);
        DrawRect(FLinearColor(.035f+Shade*.025f,.085f+Shade*.045f,.059f+Shade*.025f),Centre.X-Half+X*9,Centre.Y-Half+Y*9,9,9);
    }
    for(int32 I=1;I<6;I++){const float Offset=-Half+I*Half/3;DrawLine(Centre.X+Offset,Centre.Y-Half,Centre.X+Offset,Centre.Y+Half,FLinearColor(.06f,.12f,.09f,.25f));DrawLine(Centre.X-Half,Centre.Y+Offset,Centre.X+Half,Centre.Y+Offset,FLinearColor(.06f,.12f,.09f,.25f));}
    for(int32 I=0;I<96;I++){const float A=I*2*PI/96,B=(I+1)*2*PI/96;Segment(FVector(FMath::Cos(A),FMath::Sin(A),0)*WildlineRules::MapRadius,FVector(FMath::Cos(B),FMath::Sin(B),0)*WildlineRules::MapRadius,FLinearColor(.48f,.64f,.46f,.7f));}
    if(!F->bFarm&&!F->bResidential)for(float X=-14000;X<14000;X+=250){auto Trail=[](float D){return FVector(D,700+FMath::Sin(D*.0007f)*210,0);};Segment(Trail(X),Trail(X+250),FLinearColor(.54f,.48f,.3f,.5f),Zoom?2.f:1.f);}
    for(const auto& T:F->Trees){const auto Dot=Map(T.Base);if(Inside(Dot))DrawRect(FLinearColor(.18f,.33f,.24f,.7f),Dot.X-1,Dot.Y-1,2,2);}
    if(F->bFarm){
        auto Building=[&](FVector C,FVector Extent,const TCHAR* Name){Segment(C+FVector(-Extent.X,-Extent.Y,0),C+FVector(Extent.X,-Extent.Y,0),Gold);Segment(C+FVector(Extent.X,-Extent.Y,0),C+FVector(Extent.X,Extent.Y,0),Gold);Segment(C+FVector(Extent.X,Extent.Y,0),C+FVector(-Extent.X,Extent.Y,0),Gold);Segment(C+FVector(-Extent.X,Extent.Y,0),C+FVector(-Extent.X,-Extent.Y,0),Gold);const auto M=Map(C);if(Inside(M,20))Label(Name,M.X-16,M.Y-4,8,Gold);};
        Building(F->FarmhouseLocation,FVector(600,480,0),TEXT("HOUSE"));
        Building(F->FarmhouseLocation+FVector(-768,0,0),FVector(168,460,0),TEXT(""));
        Building(F->FarmhouseLocation+FVector(-161,-1037,0),FVector(489,283,0),TEXT(""));
        Building(FVector(3500,2550,0),FVector(500,400,0),TEXT("BARN"));
        for(int32 Side:{-1,1})for(float X=-2400;X<700;X+=280)Segment(FVector(X,Side*850,0),FVector(X,Side*2250,0),FLinearColor(.35f,.46f,.17f,.7f));
        for(TActorIterator<AWildlineLivestock> It(GetWorld());It;++It){const auto M=Map(It->GetActorLocation());if(Inside(M)){const bool Alert=It->StrikeAt>P->ServerTime();Ring(M,Alert?5:2.5f,Alert?Red:FLinearColor(.75f,.62f,.44f));}}
    }
    if(F->bResidential){
        for(int32 Lane=0;Lane<2;Lane++)for(int32 Step=0;Step<=5;Step++){const FVector A=F->CourseTarget(Lane,Step);const auto M=Map(A);const FLinearColor Color=Lane==0?Home:Gold;if(Inside(M))Ring(M,Step==5?5:3,Color,1.5f);if(Step<5)Segment(A,F->CourseTarget(Lane,Step+1),Color,1.5f);}
        const auto Feeder=Map(F->FeederLocation);if(Inside(Feeder,20))Label(TEXT("FEEDER"),Feeder.X+7,Feeder.Y-4,8,Gold);
        for(int32 Row=0;Row<2;Row++)for(int32 Lot=-1;Lot<=1;Lot++){const FVector C(Lot*3000,Row==0?-2550:3000,0);Segment(C+FVector(-800,-475,0),C+FVector(800,-475,0),Muted);Segment(C+FVector(800,-475,0),C+FVector(800,475,0),Muted);Segment(C+FVector(800,475,0),C+FVector(-800,475,0),Muted);Segment(C+FVector(-800,475,0),C+FVector(-800,-475,0),Muted);}
    }
    Label(TEXT("N"),Centre.X-4,Centre.Y-Half+3,9,Cream);Label(TEXT("E"),Centre.X+Half-11,Centre.Y-4,9,Muted);
    Label(TEXT("S"),Centre.X-4,Centre.Y+Half-12,9,Muted);Label(TEXT("W"),Centre.X-Half+3,Centre.Y-4,9,Muted);
    auto Edge=[&](FVector2D Point){const FVector2D D=Point-Centre;const float Scale=FMath::Max(FMath::Abs(D.X),FMath::Abs(D.Y))/(Half-10);return Scale>1?Centre+D/Scale:Point;};
    if(P->bSquirrel){
        for(int32 I=0;I<F->AcornLocations.Num();I++){
            if((F->bFarm?S->TakenPeanuts.Contains(I):P->Collected.Contains(I))||!WildlineRules::RevealAcorn(S->MatchConfig.MapIntel,FVector::Dist2D(P->GetActorLocation(),F->AcornLocations[I])))continue;
            const auto A=Map(F->AcornLocations[I]);if(!Inside(A))continue;
            DrawRect(FLinearColor::Black,A.X-3,A.Y-3,6,6);DrawRect(Gold,A.X-2,A.Y-2,4,4);
        }
        const auto Den=Edge(Map(F->DenLocation));Ring(Den,7,FLinearColor(.01f,.02f,.01f),4);Ring(Den,6,Home,2);
        DrawLine(Den.X-3,Den.Y+2,Den.X-3,Den.Y-2,Home,2);DrawLine(Den.X+3,Den.Y+2,Den.X+3,Den.Y-2,Home,2);DrawLine(Den.X-3,Den.Y-2,Den.X,Den.Y-5,Home,2);DrawLine(Den.X,Den.Y-5,Den.X+3,Den.Y-2,Home,2);
        Label(F->bResidential?TEXT("START"):TEXT("DEN"),Den.X+8,Den.Y-5,8,Home);
    }
    for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It){
        if(*It==P||It->bDead||It->bPreviewHunter)continue;
        const bool Enemy=It->bSquirrel!=P->bSquirrel;
        if(Enemy){if(!P->bSquirrel)continue;const float Distance=FVector::Dist2D(P->GetActorLocation(),It->GetActorLocation());bool Visible=false;
            if(S->MatchConfig.MapIntel==1&&Distance<=2500){FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(MapHunterSight),false,P);Visible=!GetWorld()->LineTraceSingleByChannel(Hit,P->GetActorLocation()+FVector(0,0,5),It->GetActorLocation(),ECC_Visibility,Query)||Hit.GetActor()==*It;}
            if(!WildlineRules::RevealHunter(S->MatchConfig.MapIntel,Distance,Visible))continue;
        }
        auto Marker=Map(It->GetActorLocation());if(!Inside(Marker)){if(!Enemy)continue;Marker=Edge(Marker);}
        if(Enemy){Ring(Marker,6+FMath::Sin(P->ServerTime()*3)*1.2f,FLinearColor(1.f,.25f,.15f,.4f));DrawRect(FLinearColor::Black,Marker.X-4,Marker.Y-4,8,8);DrawRect(Red,Marker.X-2.5f,Marker.Y-2.5f,5,5);}
        else Ring(Marker,3,Home,1.5f);
    }
    const auto Player=Map(P->GetActorLocation());const FVector Heading=P->GetControlRotation().Vector();const FVector2D Facing=FVector2D(Heading.Y,-Heading.X).GetSafeNormal(),Side(-Facing.Y,Facing.X);
    const auto Tip=Player+Facing*9,A=Player-Facing*5+Side*5,B=Player-Facing*5-Side*5;
    Ring(Player,6,FLinearColor::Black,3);DrawLine(Tip.X,Tip.Y,A.X,A.Y,Cream,2);DrawLine(Tip.X,Tip.Y,B.X,B.Y,Cream,2);DrawLine(A.X,A.Y,B.X,B.Y,Cream,2);
    if(P->bSquirrel){Label(F->bResidential?TEXT("FEEDER"):F->bFarm?TEXT("PEANUT"):TEXT("ACORN"),48,H-113,8,Gold);Label(F->bResidential?TEXT("START"):TEXT("DEN"),116,H-113,8,Home);Label(F->bFarm?TEXT("FARMER"):TEXT("HUNTER"),168,H-113,8,Red);DrawRect(Gold,40,H-109,4,4);DrawRect(Home,108,H-109,4,4);DrawRect(Red,160,H-109,4,4);}
    else Label(TEXT("YOU / HEADING     TEAM"),48,H-113,8,Muted);
}

void AWildlineHUD::DrawForageHints(AWildlineCharacter* P,AWildlineWorld* F){
    const auto* State=GetWorld()->GetGameState<AWildlineGameState>();if(!State)return;
    const float W=Canvas->SizeX,H=Canvas->SizeY,Now=P->ServerTime();const FLinearColor Gold(1.f,.75f,.32f),Home(.6f,.92f,.72f);
    if(F->bResidential){
        const TCHAR* Names[]={TEXT("START PAD"),TEXT("BIN"),TEXT("GARDEN BENCH"),TEXT("FENCE LEDGE"),TEXT("PERGOLA"),TEXT("BIRD FEEDER")};
        const int32 Step=FMath::Clamp(P->CourseCheckpoint,0,5);
        Label(FString::Printf(TEXT("NEXT: %s / %d OF 5 CHECKPOINTS / YOUR TRIPS: %d"),Names[Step],FMath::Max(0,Step-1),P->FeederVisits),30,191,11,Home);
        for(int32 Lane=0;Lane<2;Lane++){if(P->CourseLane>=0&&Lane!=P->CourseLane)continue;FVector2D Screen;const FVector Target=F->CourseTarget(Lane,Step)+FVector(0,0,28);if(GetOwningPlayerController()->ProjectWorldLocationToScreen(Target,Screen)&&Screen.X>285&&Screen.X<W-120&&Screen.Y>220&&Screen.Y<H-110)Label(FString::Printf(TEXT("%s / %.1f m"),Names[Step],FVector::Dist(P->GetActorLocation(),Target)/100),Screen.X,Screen.Y,12,Lane==0?Home:Gold);}
        return;
    }
    if(FeedbackPlayer.Get()!=P){FeedbackPlayer=P;PreviousBag=P->Bag;PreviousCollected=P->Collected.Num();PreviousStored=State->StoredAcorns;FeedbackUntil=0;}
    if(P->Collected.Num()>PreviousCollected){FeedbackText=FString::Printf(TEXT("+%d %s%s  /  %d OF %d CARRIED"),P->Collected.Num()-PreviousCollected,WildlineRules::FoodName(State->ExpeditionMode),P->Collected.Num()-PreviousCollected==1?TEXT(""):TEXT("S"),P->Bag,State->MatchConfig.CarryCapacity);FeedbackUntil=Now+2.3f;}
    if(PreviousBag>P->Bag&&State->StoredAcorns>PreviousStored){FeedbackText=FString::Printf(TEXT("%sS STORED  /  %d OF %d"),WildlineRules::FoodName(State->ExpeditionMode),State->StoredAcorns,State->MatchConfig.SquirrelGoal);FeedbackUntil=Now+3.f;}
    PreviousBag=P->Bag;PreviousCollected=P->Collected.Num();PreviousStored=State->StoredAcorns;
    if(Now<FeedbackUntil){const float Fade=FMath::Clamp(FeedbackUntil-Now,0.f,1.f);DrawRect(FLinearColor(.02f,.035f,.02f,.8f*Fade),W*.5f-185,H*.5f+130,370,36);Label(FeedbackText,W*.5f-164,H*.5f+140,14,FLinearColor(Home.R,Home.G,Home.B,Fade));}
    const float DenDistance=FVector::Dist2D(P->GetActorLocation(),F->DenLocation)/100;
    Label(FString::Printf(TEXT("DEN  %.0f m   /   %s"),DenDistance,P->Bag>=State->MatchConfig.CarryCapacity?TEXT("BAG FULL - RETURN HOME"):P->Bag>0?F->bFarm?TEXT("CARRYING PEANUTS"):TEXT("CARRYING ACORNS"):TEXT("FOLLOW THE GOLD MARKERS")),30,191,11,P->Bag>=State->MatchConfig.CarryCapacity?Gold:Home);
    if(P->Bag>=State->MatchConfig.CarryCapacity)return;
    int32 Shown=0;const FVector Eye=P->GetActorLocation()+FVector(0,0,5);
    for(int32 I=0;I<F->AcornLocations.Num()&&Shown<3;I++){
        const auto& A=F->AcornLocations[I];if((F->bFarm?State->TakenPeanuts.Contains(I):P->Collected.Contains(I))||FVector::Dist2D(P->GetActorLocation(),A)>1800)continue;
        const FVector Hint=A+FVector(0,0,18);FVector2D Screen;
        if(!GetOwningPlayerController()->ProjectWorldLocationToScreen(Hint,Screen)||Screen.X<300||Screen.X>W-100||Screen.Y<220||Screen.Y>H-125)continue;
        FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(AcornHint),false,P);
        if(GetWorld()->LineTraceSingleByChannel(Hit,Eye,Hint,ECC_Visibility,Query))continue;
        const float Radius=5+FMath::Sin(Now*2.4f+I)*1.2f;
        for(int32 J=0;J<16;J++){const float U=J*2*PI/16,V=(J+1)*2*PI/16;DrawLine(Screen.X+FMath::Cos(U)*Radius,Screen.Y+FMath::Sin(U)*Radius,Screen.X+FMath::Cos(V)*Radius,Screen.Y+FMath::Sin(V)*Radius,Gold,1.5f);}
        Label(FString::Printf(TEXT("%s %.0f m"),WildlineRules::FoodName(State->ExpeditionMode),FVector::Dist2D(P->GetActorLocation(),A)/100),Screen.X+10,Screen.Y-5,10,Gold);Shown++;
    }
}

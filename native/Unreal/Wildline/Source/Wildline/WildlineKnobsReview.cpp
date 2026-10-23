#include "WildlineGame.h"
#include "WildlineCharacter.h"
#include "WildlineWorld.h"
#include "WildlineKnobs.h"
#include "WildlineLake.h"
#include "WildlineMenu.h"
#include "WildlineUserSettings.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/ComboBoxString.h"
#include "Components/WidgetSwitcher.h"
#include "Components/ScrollBox.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HUD.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace {
void At(AWildlineController* PC,float Time,TFunction<void()> Work){FTimerHandle H;PC->GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(PC,[Work](){Work();}),Time,false);}
void View(AWildlineController* PC,FVector Eye,FVector Target,float FOV=70){auto* C=PC->GetWorld()->SpawnActor<ACameraActor>(Eye,(Target-Eye).Rotation());C->GetCameraComponent()->SetFieldOfView(FOV);PC->SetViewTarget(C);}
void Shot(const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/Knobs/")+Name+TEXT(".png"),true,false);}
}
void WildlineRunKnobsReview(AWildlineController* PC){
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineKnobsTravel"))){const auto* F=AWildlineWorld::Find(PC->GetWorld());if(F&&!F->bKnobs){At(PC,2,[PC](){PC->Menu->SelectMap(6);PC->PlaySelected(false);});return;}}
    const bool Network=FParse::Param(FCommandLine::Get(),TEXT("WildlineKnobsNetProbe"));
    const bool Capture=FParse::Param(FCommandLine::Get(),TEXT("WildlineKnobsReview"));
    auto Result=MakeShared<bool>(true);
    auto Check=[Result](bool OK,const TCHAR* What){*Result&=OK;UE_LOG(LogTemp,Display,TEXT("KNOBS CHECK %s: %s"),OK?TEXT("PASS"):TEXT("FAIL"),What);};
    At(PC,2,[PC,Network,Check](){
        auto* W=PC->GetWorld();auto* F=AWildlineWorld::Find(W);auto* S=W->GetGameState<AWildlineGameState>();
        Check(F&&F->bKnobs&&!F->bLake&&!F->bFarm&&!F->bWinter&&S&&S->ExpeditionMode==0&&S->MatchConfig.ClassicLayout==2,TEXT("Knobs loads with the replicated Classic map selection"));
        if(!F||!S)return;
        Check(!AWildlineLake::Find(W)&&F->Terrain->GetNumSections()>10&&F->KnobsLabels.Num()>=8,TEXT("Creek, trails, three trailheads and kiosk exist without lake gameplay"));
        Check(PC->Menu&&PC->Menu->MapCards.Num()==7&&PC->Menu->ClassicLayout->GetOptionCount()==3,TEXT("Map gallery and Classic layout picker include Knobs"));
        Check(F->Trees.Num()>180&&F->AcornLocations.Num()==30&&F->ClimbWood.Num()==4&&F->ReserveFence->GetInstanceCount()==0,TEXT("Playable woodland, forage and natural marked boundary generated"));
        bool Paths=true,Support=true;int32 Hits=0;
        for(int32 R=0;R<3;R++)for(const auto& XY:WildlineKnobs::Trail(R)){
            Paths&=XY.Size()<13700;
            const float X=XY.X,Y=XY.Y;
            const FVector N(F->GroundHeight(X-10,Y)-F->GroundHeight(X+10,Y),F->GroundHeight(X,Y-10)-F->GroundHeight(X,Y+10),20);
            Paths&=N.GetSafeNormal().Z>.72f;
        }
        for(int32 I=0;I<3;I++){const auto XY=WildlineKnobs::Parking(I);const float Z=F->GroundHeight(XY.X,XY.Y);FHitResult Hit;Support&=W->LineTraceSingleByChannel(Hit,FVector(XY.X,XY.Y,Z+100),FVector(XY.X,XY.Y,Z-100),ECC_Pawn)&&FMath::Abs(Hit.ImpactPoint.Z-Z)<12;}
        for(const auto& Tree:F->Trees){FHitResult Hit;int32 Index;const FVector P=Tree.Base+FVector(0,0,180);if(F->TraceWood(P+FVector(180,0,0),P-FVector(180,0,0),Hit,Index))Hits++;if(Hits>=5)break;}
        Check(Paths&&Support,TEXT("All trail loops stay inside play bounds with walkable slopes and solid trailheads"));
        Check(Hits>=5&&F->GroundHeight(4600,3200)-F->GroundHeight(4600,WildlineKnobs::CreekY(4600))>1200,TEXT("Climbable trees and a distinct elevated Hay Ridge are physically present"));
        if(!PC->HasAuthority())return;
        PC->Menu->SelectMap(6);auto* Settings=UWildlineUserSettings::Get();
        Check(Settings&&Settings->ClassicConfig.ClassicLayout==2&&PC->GetGameInstance<UWildlineGameInstance>()->PreferredMode==0,TEXT("Selecting map card preserves Knobs in Classic settings"));
        auto C=Settings->ClassicConfig;C.Clamp();Check(C.ClassicLayout==2,TEXT("Knobs selection survives configuration validation"));
        S->MatchConfig.DefenderBots=0;S->MatchConfig.SquirrelBots=0;Settings->ClassicConfig=S->MatchConfig;PC->ChooseRole(true);auto* P=Cast<AWildlineCharacter>(PC->GetPawn());Check(P&&P->bSquirrel,TEXT("Squirrel can join the new map"));if(!P)return;
        S->bRoundStarted=true;P->bInLobby=false;P->ProtectedUntil=0;
        P->SetActorLocation(F->AcornLocations[0]+FVector(0,0,13));P->GetCharacterMovement()->StopMovementImmediately();
        if(Network){S->MatchConfig.SquirrelGoal=19;Settings->ClassicConfig.SquirrelGoal=19;S->ForceNetUpdate();}
    });
    At(PC,4,[PC,Network,Check](){
        if(!PC->HasAuthority())return;auto* F=AWildlineWorld::Find(PC->GetWorld());auto* P=Cast<AWildlineCharacter>(PC->GetPawn());auto* S=PC->GetWorld()->GetGameState<AWildlineGameState>();if(!P||!F||!S)return;
        Check(P->Collected.Num()>0&&P->Bag>0,TEXT("Squirrel actually collects a ground acorn"));
        P->SetActorLocation(F->DenLocation+FVector(90,0,13));P->GetCharacterMovement()->StopMovementImmediately();S->bRoundStarted=true;
    });
    At(PC,6,[PC,Network,Check](){
        auto* S=PC->GetWorld()->GetGameState<AWildlineGameState>();if(!S)return;
        if(!PC->HasAuthority()){Check(Network&&S->MatchConfig.SquirrelGoal==19,TEXT("Client receives host rules on the Knobs map"));return;}
        Check(S->StoredAcorns>0,TEXT("Collected acorn deposits into the map's hollow den"));
        S->bRoundStarted=false;PC->ChooseRole(false);auto* P=Cast<AWildlineCharacter>(PC->GetPawn());auto* F=AWildlineWorld::Find(PC->GetWorld());
        Check(P&&!P->bSquirrel,TEXT("Hunter can join the same map"));if(P){P->bInLobby=false;P->SetActorLocation(FVector(-7800,-5350,F->GroundHeight(-7800,-5350)+110));P->GetCharacterMovement()->StopMovementImmediately();PC->SetMenu(false);}
    });
    At(PC,9,[PC,Check](){if(PC->HasAuthority()){auto* P=Cast<AWildlineCharacter>(PC->GetPawn());auto* F=AWildlineWorld::Find(PC->GetWorld());Check(P&&F&&P->GetActorLocation().X>-7700&&FMath::Abs(P->GetActorLocation().Z-F->GroundHeight(P->GetActorLocation().X,P->GetActorLocation().Y)-110)<45,TEXT("Hunter walks on the new terrain under live character physics"));auto* S=PC->GetWorld()->GetGameState<AWildlineGameState>();S->Winner=1;PC->GetWorld()->GetAuthGameMode<AWildlineGameMode>()->RestartRound();Check(S->MatchConfig.ClassicLayout==2,TEXT("Restart round retains the active Knobs map"));}});
    if(Capture){
        At(PC,10,[PC](){PC->SetMenu(false);PC->GetHUD()->bShowHUD=false;View(PC,FVector(-10300,-11500,5800),FVector(1000,1700,1100),70);});At(PC,15,[](){Shot(TEXT("Overview"));});
        At(PC,16,[PC](){auto* F=AWildlineWorld::Find(PC->GetWorld());auto P=WildlineKnobs::Parking(0);View(PC,FVector(P.X-650,P.Y-1000,F->GroundHeight(P.X-650,P.Y-1000)+300),FVector(P.X-90,P.Y+400,F->GroundHeight(P.X,P.Y)+175),68);});At(PC,21,[](){Shot(TEXT("Trailhead"));});
        At(PC,22,[PC](){auto* F=AWildlineWorld::Find(PC->GetWorld());const float X=-1800,Y=WildlineKnobs::CreekY(X);View(PC,FVector(X-850,Y-500,F->GroundHeight(X-850,Y-500)+250),FVector(X+1100,Y+350,230),72);});At(PC,27,[](){Shot(TEXT("CrookedCreek"));});
        At(PC,28,[PC](){auto* F=AWildlineWorld::Find(PC->GetWorld());View(PC,FVector(3800,1900,F->GroundHeight(3800,1900)+220),FVector(-3400,-2000,350),75);});At(PC,33,[](){Shot(TEXT("HayRidge"));});
        At(PC,34,[PC](){PC->SetMenu(true);PC->Menu->ShowMaps();PC->Menu->MapScroll->ScrollToEnd();});At(PC,38,[](){Shot(TEXT("MapCard"));});
    }
    At(PC,Capture?40:Network?(PC->HasAuthority()?45:10):10,[PC,Result,Network](){UE_LOG(LogTemp,Display,TEXT("WILDLINE KNOBS %s: %s"),Network?(PC->HasAuthority()?TEXT("NET HOST"):TEXT("NET CLIENT")):TEXT("SMOKE"),*Result?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Result?0:1);});
}
void WildlineKnobsProbeInput(AWildlineCharacter* P){
    if(!P->IsLocallyControlled()||!AWildlineWorld::Find(P->GetWorld())||!AWildlineWorld::Find(P->GetWorld())->bKnobs)return;
    if(!FParse::Param(FCommandLine::Get(),TEXT("WildlineKnobsTravel"))&&!FParse::Param(FCommandLine::Get(),TEXT("WildlineKnobsSmoke"))&&!FParse::Param(FCommandLine::Get(),TEXT("WildlineKnobsReview"))&&!FParse::Param(FCommandLine::Get(),TEXT("WildlineKnobsNetProbe")))return;
    const float T=P->GetWorld()->GetTimeSeconds();if(T>6.2f&&T<8.5f&&!P->bSquirrel&&!P->bInLobby)P->AddMovementInput(FVector::ForwardVector,1);
}

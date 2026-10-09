#include "WildlineGame.h"
#include "WildlineWorld.h"
#include "WildlineCharacter.h"
#include "WildlineLobby.h"
#include "WildlineUserSettings.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/HUD.h"
#include "TimerManager.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "InputKeyEventArgs.h"

namespace {
bool FindLimb(AWildlineWorld* F,int32 Tree,FHitResult& Hit){
    const auto& T=F->Trees[Tree];
    // Sample bark beyond the stem. No leaf card can satisfy this wood trace.
    for(float Radius:{140.f,220.f,320.f,430.f,560.f,700.f})for(int32 A=0;A<32;A++){
        const FVector XY=T.Base+FVector(FMath::Cos(A*PI/16)*Radius,FMath::Sin(A*PI/16)*Radius,0);
        int32 Index;
        if(F->TraceWood(FVector(XY.X,XY.Y,T.Base.Z+T.Height+800),FVector(XY.X,XY.Y,T.Base.Z+120),Hit,Index)&&Index==Tree&&Hit.ImpactNormal.Z>.55f){
            float StemRadius;const FVector Stem=F->TrunkCentre(Tree,Hit.ImpactPoint.Z,StemRadius);
            FCollisionQueryParams Query(SCENE_QUERY_STAT(AccessibleLimb));for(TActorIterator<AWildlineCharacter> It(F->GetWorld());It;++It)Query.AddIgnoredActor(*It);
            const bool Clear=!F->GetWorld()->OverlapBlockingTestByChannel(Hit.ImpactPoint+Hit.ImpactNormal*16,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(12),Query);
            bool Run=true;const FVector Along=FVector::VectorPlaneProject((Hit.ImpactPoint-T.Base).GetSafeNormal2D(),Hit.ImpactNormal).GetSafeNormal();
            for(float Step:{25.f,50.f}){const FVector Probe=Hit.ImpactPoint+Along*Step;FHitResult Ahead;int32 AheadTree;Run&=F->TraceWood(Probe+Hit.ImpactNormal*35,Probe-Hit.ImpactNormal*40,Ahead,AheadTree,6)&&AheadTree==Tree&&FVector::Dist(Ahead.ImpactPoint,Probe)<18;}
            if(Clear&&Run&&FVector::Dist2D(Stem,Hit.ImpactPoint)>StemRadius+45)return true;
        }
    }
    return false;
}
}

void AWildlineController::RunTraversalSmokeTest(){
    if(!HasAuthority())return;
    auto* Current=GetWorld()->GetGameState<AWildlineGameState>();
    GetGameInstance<UWildlineGameInstance>()->PreferredMode=Current->ExpeditionMode;
    UWildlineUserSettings::Get()->ConfigForMode(Current->ExpeditionMode)=Current->MatchConfig;
    ChooseRole(true);
    auto* P=Cast<AWildlineCharacter>(GetPawn());auto* F=AWildlineWorld::Find(GetWorld());auto* S=GetWorld()->GetGameState<AWildlineGameState>();
    bool Ok=P&&F&&S;auto Check=[&](bool Passed,const TCHAR* Text){Ok&=Passed;UE_LOG(LogTemp,Display,TEXT("WILDLINE TRAVERSAL %s: %s"),Passed?TEXT("PASS"):TEXT("FAIL"),Text);};
    if(!Ok){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    S->bRoundStarted=false;P->bInLobby=false;P->bDead=false;S->Winner=0;
    Check(F->ClimbWood.Num()==4,TEXT("four species have wood-only collision"));
    int32 SpeciesTested=0;
    for(int32 Species=0;Species<4;Species++){
        int32 Tree=INDEX_NONE;FHitResult Hit;
        for(int32 I=0;I<F->Trees.Num();I++)if(F->Trees[I].Species==Species&&!F->Trees[I].bStraightTrunk&&FindLimb(F,I,Hit)){Tree=I;break;}
        Check(Tree!=INDEX_NONE,TEXT("actual connected limb has an upward bark surface"));if(Tree==INDEX_NONE)continue;
        FHitResult Physical;FCollisionQueryParams Query(SCENE_QUERY_STAT(BranchLanding),false,P);
        Check(GetWorld()->SweepSingleByChannel(Physical,Hit.ImpactPoint+FVector(0,0,65),Hit.ImpactPoint-FVector(0,0,5),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(12,14),Query)&&F->ClimbWood.Contains(Cast<UHierarchicalInstancedStaticMeshComponent>(Physical.Component.Get())),TEXT("real branch blocks a landing squirrel capsule"));
        P->TreeIndex=INDEX_NONE;P->SetActorLocation(Hit.ImpactPoint+Hit.ImpactNormal*16,false);SetControlRotation(FRotator(0,(Hit.ImpactPoint-F->Trees[Tree].Base).Rotation().Yaw,0));
        P->ServerClimb();Check(P->TreeIndex==Tree&&P->bWoodClimbing,TEXT("E catches a branch beyond the trunk"));
        const FVector Start=P->GetActorLocation();P->ServerClimbInput(1,0);
        for(int32 Step=0;Step<20;Step++)P->Tick(.025f);
        const float Travel=FVector::Dist(Start,P->GetActorLocation());
        UE_LOG(LogTemp,Display,TEXT("WILDLINE LIMB species=%d travel=%.1f normal=%s"),Species,Travel,*FVector(P->ClimbNormal).ToString());
        Check(Travel>20&&Travel<160,TEXT("squirrel walks along bark without a teleport"));
        FHitResult Support;int32 SupportTree;
        Check(F->TraceWood(P->GetActorLocation()+P->ClimbNormal*5,P->GetActorLocation()-P->ClimbNormal*35,Support,SupportTree),TEXT("moving paws retain branch support"));
        P->ServerJump();Check(P->TreeIndex==INDEX_NONE&&P->GetCharacterMovement()->PendingLaunchVelocity.Size2D()>850,TEXT("branch jump releases grip with canopy launch"));
        // Re-grab the actual trunk, then climb towards its connected forks.
        const auto& T=F->Trees[Tree];float R;const FVector C=F->TrunkCentre(Tree,T.Base.Z+100,R);
        P->SetActorLocation(C+FVector(R+20,0,0));P->ServerClimb();const float Z=P->GetActorLocation().Z;P->ServerClimbInput(1,0);P->Tick(.5f);
        Check(P->TreeIndex==Tree&&P->GetActorLocation().Z>Z+60,TEXT("trunk ascent remains supported"));P->ServerClimb();SpeciesTested++;
    }
    Check(SpeciesTested==4,TEXT("all oak variants and pine support limbs and trunks"));
    if(F->bResidential){
        bool YardLimb=false;
        for(int32 I=0;I<F->Trees.Num();I++)if(!F->Trees[I].bStraightTrunk&&F->Trees[I].Base.SizeSquared2D()<FMath::Square(5000.f)){
            FHitResult Hit;if(FindLimb(F,I,Hit)){P->TreeIndex=INDEX_NONE;P->SetActorLocation(Hit.ImpactPoint+Hit.ImpactNormal*16);P->ServerClimb();YardLimb=P->TreeIndex==I&&P->bWoodClimbing;P->ServerClimb();if(YardLimb)break;}
        }
        Check(YardLimb,TEXT("a playable neighborhood yard oak supports branch grip"));
    }
    if(F->bWinter){
        Check(S->ExpeditionMode==0&&S->MatchConfig.ClassicLayout==1,TEXT("Frostwood uses Classic rules and the winter selection"));
        bool Leafless=true;for(int32 I=4;I<17;I++)Leafless&=F->ForestLayers[I]->GetInstanceCount()==0;
        for(int32 I=0;I<4;I++)Leafless&=F->ForestLayers[I]->GetStaticMesh()->GetName().EndsWith(TEXT("_Bare"));
        Check(Leafless,TEXT("winter has no leaf cards, needles, ferns or fallen leaves"));Check(F->AcornLocations.Num()==30,TEXT("winter preserves Classic forage"));
    }
    UE_LOG(LogTemp,Display,TEXT("WILDLINE TRAVERSAL: %s"),Ok?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,Ok?0:1);
}

void AWildlineController::RunWorldUpgradeReview(){
    auto* Current=GetWorld()->GetGameState<AWildlineGameState>();
    GetGameInstance<UWildlineGameInstance>()->PreferredMode=Current->ExpeditionMode;
    UWildlineUserSettings::Get()->ConfigForMode(Current->ExpeditionMode)=Current->MatchConfig;
    auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
    auto Camera=[this](FVector Eye,FVector Target,float Fov){auto* C=GetWorld()->SpawnActor<ACameraActor>(Eye,(Target-Eye).Rotation());C->GetCameraComponent()->SetFieldOfView(Fov);SetViewTarget(C);GetHUD()->bShowHUD=false;};
    auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,false,false);};
    ChooseRole(true);GetWorld()->GetGameState<AWildlineGameState>()->bRoundStarted=false;
    auto* F=AWildlineWorld::Find(GetWorld());
    if(F->bResidential){
        At(1,[Camera](){Camera(FVector(-2500,-4850,470),FVector(-2100,-4650,320),58);});At(5,[Shot](){Shot(TEXT("UpgradeCars.png"));});
        At(6,[Camera](){Camera(FVector(-4400,-5300,2650),FVector(-500,-600,450),65);});At(11,[Shot](){Shot(TEXT("UpgradeNeighborhood.png"));});
    }else{
        At(1,[Camera,F](){Camera(FVector(-1300,-1600,F->GroundHeight(-1300,-1600)+160),FVector(600,200,800),68);});At(5,[Shot](){Shot(TEXT("UpgradeReserve.png"));});
        At(6,[this,Camera,F](){FHitResult Hit;if(FindLimb(F,0,Hit)){auto* P=Cast<AWildlineCharacter>(GetPawn());P->bInLobby=false;P->TreeIndex=INDEX_NONE;P->SetActorLocation(Hit.ImpactPoint+Hit.ImpactNormal*16);P->ServerClimb();P->ServerClimbInput(0,0);const FVector N=P->ClimbNormal;const FVector Side=FVector::CrossProduct(N,P->ClimbHeading).GetSafeNormal();Camera(P->GetActorLocation()+N*80+Side*135-P->ClimbHeading*40,P->GetActorLocation(),58);}});At(11,[Shot](){Shot(TEXT("UpgradeBranch.png"));});
    }
    At(12,[this,Camera](){auto* P=Cast<AWildlineCharacter>(GetPawn());P->TreeIndex=INDEX_NONE;P->bInLobby=true;P->GetCharacterMovement()->StopMovementImmediately();P->GetCharacterMovement()->SetMovementMode(MOVE_Flying);P->SetActorLocation(AWildlineLobby::Centre(true));Camera(AWildlineLobby::Centre(true)+FVector(30,-155,155),AWildlineLobby::Centre(true)+FVector(-414,0,205),65);});
    At(17,[Shot](){Shot(TEXT("UpgradeHunterTrophies.png"));});
    At(18,[Camera](){Camera(AWildlineLobby::Centre(true)+FVector(260,-220,210),AWildlineLobby::Centre(true)+FVector(-140,150,90),75);});At(23,[Shot](){Shot(TEXT("UpgradeSquirrelDen.png"));});
    At(24,[Camera](){Camera(AWildlineLobby::Centre(false)+FVector(-210,-320,182),AWildlineLobby::Centre(false)+FVector(-405,-1600,190),63);});At(30,[Shot](){Shot(TEXT("UpgradeLodgeWoods.png"));});
    At(32,[](){FPlatformMisc::RequestExit(false);});
}

void AWildlineController::RunTraversalNetProbe(){
    auto At=[this](float Time,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),Time,false);};
    auto Finish=[this](bool Passed){UE_LOG(LogTemp,Display,TEXT("WILDLINE TRAVERSAL NET %s: %s"),HasAuthority()?TEXT("host"):TEXT("client"),Passed?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,Passed?0:1);};
    auto* S=GetWorld()->GetGameState<AWildlineGameState>();GetGameInstance<UWildlineGameInstance>()->PreferredMode=S->ExpeditionMode;
    UWildlineUserSettings::Get()->ConfigForMode(S->ExpeditionMode)=S->MatchConfig;
    EnterWaitingRoom(!HasAuthority());SetMenu(false);
    auto Phase=MakeShared<int32>(0);auto Start=MakeShared<FVector>(FVector::ZeroVector);auto Changed=MakeShared<float>(0);
    auto Moved=MakeShared<bool>(false);auto Poll=MakeShared<FTimerHandle>();
    GetWorldTimerManager().SetTimer(*Poll,FTimerDelegate::CreateWeakLambda(this,[this,At,Finish,Phase,Start,Changed,Moved,Poll](){
        auto* F=AWildlineWorld::Find(GetWorld());if(!F)return;
        AWildlineCharacter* P=nullptr;
        if(HasAuthority()){for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(It->bSquirrel&&!It->bBot){P=*It;break;}}
        else P=Cast<AWildlineCharacter>(GetPawn());
        if(!P)return;
        const float Now=GetWorld()->GetTimeSeconds();
        if(HasAuthority()){
            if(*Phase==0&&P->bInLobby){
                FHitResult Hit;if(!FindLimb(F,0,Hit)){Finish(false);return;}
                P->bInLobby=false;P->TreeIndex=INDEX_NONE;P->GetCharacterMovement()->StopMovementImmediately();P->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
                P->SetActorLocation(Hit.ImpactPoint+Hit.ImpactNormal*16);P->ForceNetUpdate();*Start=P->GetActorLocation();*Phase=1;
            }else if(*Phase==1&&P->TreeIndex!=INDEX_NONE&&P->bWoodClimbing){*Phase=2;}
            else if(*Phase==2){
                *Moved|=FVector::Dist(*Start,P->GetActorLocation())>20;
                if(P->TreeIndex==INDEX_NONE){const bool Ok=*Moved;At(3,[Finish,Ok](){Finish(Ok);});FTimerHandle Handle=*Poll;GetWorldTimerManager().ClearTimer(Handle);}
            }
        }else{
            if(*Phase==0&&!P->bInLobby&&P->bSquirrel&&P->GetActorLocation().Z<4500){P->Climb();*Phase=1;}
            else if(*Phase==1&&P->TreeIndex!=INDEX_NONE&&P->bWoodClimbing){
                *Start=P->GetActorLocation();SetControlRotation(FRotator(0,(P->GetActorLocation()-F->Trees[P->TreeIndex].Base).Rotation().Yaw,0));InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,EKeys::W,IE_Pressed,FPlatformTime::Cycles64()));*Changed=Now;*Phase=2;
            }else if(*Phase==2&&Now-*Changed>1){
                UE_LOG(LogTemp,Display,TEXT("WILDLINE TRAVERSAL NET client travel=%.1f normal=%s"),FVector::Dist(*Start,P->GetActorLocation()),*FVector(P->ClimbNormal).ToString());*Moved=FVector::Dist(*Start,P->GetActorLocation())>20&&FMath::IsNearlyEqual(FVector(P->ClimbNormal).Size(),1.f,.05f);
                InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,EKeys::W,IE_Released,FPlatformTime::Cycles64()));P->MoveForward(0);P->DoJump();*Phase=3;
            }else if(*Phase==3&&P->TreeIndex==INDEX_NONE&&P->GetCharacterMovement()->IsFalling()){
                const bool Ok=*Moved;At(1,[Finish,Ok](){Finish(Ok);});FTimerHandle Handle=*Poll;GetWorldTimerManager().ClearTimer(Handle);
            }
        }
    }),.1f,true);
    At(140,[Finish](){Finish(false);});
}

#include "WildlineGame.h"
#include "WildlineCharacter.h"
#include "WildlineWorld.h"
#include "WildlineUserSettings.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HUD.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void AWildlineController::RunThermalReview(){
    auto* F=AWildlineWorld::Find(GetWorld());auto* S=GetWorld()->GetGameState<AWildlineGameState>();
    if(!F||!S){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    GetGameInstance<UWildlineGameInstance>()->PreferredMode=S->ExpeditionMode;
    UWildlineUserSettings::Get()->ConfigForMode(S->ExpeditionMode)=S->MatchConfig;
    UWildlineUserSettings::Get()->SelectedWeapon=0;
    ChooseRole(false);auto* P=Cast<AWildlineCharacter>(GetPawn());SetMenu(false);
    S->bRoundStarted=false;S->Winner=0;S->MatchConfig.bAllowThermal=true;
    P->bInLobby=false;P->SelectWeapon(0);P->ThermalUntil=P->ThermalReadyAt=0;
    for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)if(*It!=P){It->SetActorHiddenInGame(true);It->SetActorTickEnabled(false);}
    P->SetActorLocation(FVector(0,-600,F->GroundHeight(0,-600)+96));
    P->GetCharacterMovement()->SetMovementMode(MOVE_None);P->ServerAim(true);GetHUD()->bShowHUD=false;
    auto* Target=GetWorld()->SpawnActor<AWildlineCharacter>();Target->bSquirrel=true;Target->bInLobby=false;Target->ConfigureRole();
    const FVector Location(550,-600,F->GroundHeight(550,-600)+13);
    Target->SetActorLocation(Location);Target->SetActorRotation(FRotator(0,90,0));Target->GetCharacterMovement()->SetMovementMode(MOVE_None);
    SetControlRotation((Location-P->FirstCamera->GetComponentLocation()).Rotation());
    auto* Cover=GetWorld()->SpawnActor<AStaticMeshActor>();
    Cover->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
    Cover->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Cover->SetActorLocation((P->FirstCamera->GetComponentLocation()+Location)*.5f);
    Cover->SetActorScale3D(FVector(.3f,.9f,1.f));Cover->SetActorHiddenInGame(true);
    auto Passed=MakeShared<bool>(true);auto Materials=MakeShared<TArray<UMaterialInterface*>>();
    for(int32 I=0;I<Target->GetMesh()->GetNumMaterials();I++)Materials->Add(Target->GetMesh()->GetMaterial(I));
    auto Check=[Passed](bool Ok,const TCHAR* Message){*Passed&=Ok;UE_LOG(LogTemp,Display,TEXT("WILDLINE THERMAL %s: %s"),Ok?TEXT("PASS"):TEXT("FAIL"),Message);};
    auto At=[this](float T,TFunction<void()> Work){FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[Work](){Work();}),T,false);};
    auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,false,false);};
    At(2,[P,Target,Check,Shot](){
        Check(P->ThermalMaterial!=nullptr,TEXT("cooked thermal camera material present"));
        Check(IConsoleManager::Get().FindConsoleVariable(TEXT("r.CustomDepth"))->GetInt()==3,TEXT("custom depth stencil enabled"));
        Check(Target->GetMesh()->bRenderCustomDepth&&Target->GetMesh()->CustomDepthStencilValue==1,TEXT("living squirrel body tagged"));
        bool Fur=Target->FurSections.Num()>0;for(auto* Part:Target->FurSections)Fur&=Part->bRenderCustomDepth&&Part->CustomDepthStencilValue==1;
        Check(Fur,TEXT("squirrel fur tagged without losing silhouette"));Shot(TEXT("ThermalNormal.png"));
    });
    At(3,[P,Check](){P->StopAim();P->Thermal();Check(P->IsThermalActive()&&!P->IsScoped(),TEXT("thermal activates without forcing zoom"));});
    At(3.2f,[P,Check](){P->StartAim();Check(P->IsThermalActive()&&P->IsScoped(),TEXT("zoom can engage during thermal"));});
    At(3.4f,[P,Check](){P->StopAim();Check(P->IsThermalActive()&&!P->IsScoped(),TEXT("zoom can disengage without cancelling thermal"));});
    At(3.9f,[P,Check,Shot](){Check(P->FirstCamera->FieldOfView>74,TEXT("thermal unscoped view returns to wide field of view"));Shot(TEXT("ThermalUnscoped.png"));});
    At(4.f,[P](){P->bThirdPerson=true;});
    At(4.3f,[P,Check](){
        bool Filter=false;for(const auto& Blend:P->ThirdCamera->PostProcessSettings.WeightedBlendables.Array)Filter|=Blend.Object==P->ThermalMaterial&&Blend.Weight==1;
        Check(P->ThirdCamera->IsActive()&&Filter,TEXT("unscoped third-person camera keeps thermal"));
        P->StartAim();Check(P->IsScoped()&&P->IsThermalActive(),TEXT("thermal can re-enter scope"));P->bThirdPerson=false;
    });
    At(5,[P,Target,Materials,Check,Shot](){
        bool Unchanged=true;for(int32 I=0;I<Materials->Num();I++)Unchanged&=Target->GetMesh()->GetMaterial(I)==(*Materials)[I];
        Check(Unchanged,TEXT("thermal preserves original squirrel materials"));
        bool Filter=false;for(const auto& Blend:P->FirstCamera->PostProcessSettings.WeightedBlendables.Array)Filter|=Blend.Object==P->ThermalMaterial&&Blend.Weight==1;
        Check(Filter,TEXT("local hunter camera applies full thermal filter"));Shot(TEXT("ThermalHot.png"));
    });
    At(6,[Cover](){Cover->SetActorHiddenInGame(false);});
    At(8,[Shot](){Shot(TEXT("ThermalCover.png"));});
    At(9,[Cover,Target](){Cover->SetActorHiddenInGame(true);Target->bDead=true;});
    At(11,[Target,Check,Shot](){Check(!Target->GetMesh()->bRenderCustomDepth,TEXT("dead squirrel loses heat signature"));Shot(TEXT("ThermalDead.png"));});
    At(12,[P,Target](){Target->bDead=false;P->ThermalUntil=0;});
    At(14,[P,Check,Shot](){
        bool Off=!P->IsThermalActive();for(const auto& Blend:P->FirstCamera->PostProcessSettings.WeightedBlendables.Array)if(Blend.Object==P->ThermalMaterial)Off&=Blend.Weight==0;
        Check(Off,TEXT("expiration removes camera filter"));Shot(TEXT("ThermalExpired.png"));
        const float Ready=P->ThermalReadyAt;P->Thermal();Check(P->ThermalUntil==0&&P->ThermalReadyAt==Ready,TEXT("cooldown still rejects early reuse"));
        P->ThermalReadyAt=0;P->Thermal();P->SelectWeapon(1);
        Check(!P->IsThermalActive()&&P->ThermalUntil==0,TEXT("shotgun selection clears thermal"));
        P->SelectWeapon(0);P->ThermalReadyAt=0;
    });
    At(15,[P,S,Check](){S->MatchConfig.bAllowThermal=false;P->Thermal();Check(!P->IsThermalActive(),TEXT("match rules still disable thermal"));});
    At(16,[Passed](){UE_LOG(LogTemp,Display,TEXT("WILDLINE THERMAL REVIEW: %s"),*Passed?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Passed?0:1);});
}

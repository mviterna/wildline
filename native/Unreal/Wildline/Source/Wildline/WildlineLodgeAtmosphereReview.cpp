#include "WildlineGame.h"
#include "WildlineLobby.h"
#include "WildlineWorld.h"
#include "WildlineUserSettings.h"
#include "WildlineSocial.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "ProceduralMeshComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/HUD.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void WildlineRunLodgeAtmosphereReview(AWildlineController* PC){
    UWildlineUserSettings::Get()->PlayerDisplayName=TEXT("Lodge Review");PC->ServerSetProfileName(TEXT("Lodge Review"));PC->Social->Open(false);
    auto At=[PC](float T,TFunction<void()> Work){FTimerHandle H;PC->GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(PC,[Work](){Work();}),T,false);};
    auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,true,false);};
    auto View=[PC](FVector Position,FVector Target){auto* Cam=PC->GetWorld()->SpawnActor<ACameraActor>(AWildlineLobby::Centre(false)+Position,FRotator::ZeroRotator);Cam->SetActorRotation((AWildlineLobby::Centre(false)+Target-Cam->GetActorLocation()).Rotation());Cam->GetCameraComponent()->FieldOfView=83;PC->SetViewTarget(Cam);};
    auto Passed=MakeShared<bool>(true);auto Check=[Passed](bool OK,const TCHAR* Label){*Passed&=OK;UE_LOG(LogTemp,Display,TEXT("LODGE ATMOSPHERE %s: %s"),OK?TEXT("PASS"):TEXT("FAIL"),Label);};
    At(2,[PC](){PC->EnterWaitingRoom(false);PC->GetHUD()->bShowHUD=false;});
    At(3,[PC,Check](){auto* Room=AWildlineLobby::Find(PC->GetWorld());auto* Map=AWildlineWorld::Find(PC->GetWorld());Check(Room&&Map,TEXT("map and waiting room exist"));if(!Room||!Map)return;
        Check(PC->GetWorld()->GetMapName().EndsWith(Room->ExteriorTheme),TEXT("exterior follows loaded map"));
        int Paintings=0,Bare=0,Buildings=0;bool Ground=false,Barn=false,Glow=false;
        for(auto* C:Room->Props)if(C&&C->GetStaticMesh()){Paintings+=C->GetStaticMesh()->GetName().StartsWith(TEXT("SM_Painting"))?1:0;Glow|=C->GetMaterial(0)&&C->GetMaterial(0)->GetName()==TEXT("M_LodgeFireGlow");}
        for(auto* Base:Room->LodgeScenery)if(auto* C=Cast<UStaticMeshComponent>(Base);C&&C->GetStaticMesh()){
            const FString Name=C->GetStaticMesh()->GetName();Ground|=C->GetMaterial(0)==Map->Terrain->GetMaterial(0);Bare+=Name.EndsWith(TEXT("_Bare"))?1:0;
            Buildings+=(Name.Contains(TEXT("House"))||Name.Contains(TEXT("Cottage")))?1:0;
            if(Name==TEXT("SM_Barn"))if(auto* Instances=Cast<UHierarchicalInstancedStaticMeshComponent>(C))Barn|=Instances->GetInstanceCount()>0;
        }
        Check(Paintings==2,TEXT("both museum paintings are mounted"));Check(Ground,TEXT("exterior uses selected map ground material"));
        if(Map->bWinter)Check(Bare>=4,TEXT("snow map has bare winter scenery"));
        if(Map->bFarm)Check(Barn,TEXT("farm map has its actual barn outside"));
        if(Map->bResidential)Check(Buildings>0,TEXT("neighborhood has its actual houses outside"));
        Check(Glow&&Room->HearthLight&&Room->HearthLight->Intensity>3000,TEXT("soft flame halo and warm hearth light exist"));
    });
    At(4,[View](){View(FVector(-200,330,235),FVector(455,687,255));});At(8,[Shot](){Shot(TEXT("LodgeExterior.png"));});
    At(9,[View](){View(FVector(-340,-290,285),FVector(680,35,320));});At(13,[Shot](){Shot(TEXT("LodgePaintings.png"));});
    At(14,[View](){View(FVector(-150,60,170),FVector(95,640,68));});At(18,[Shot](){Shot(TEXT("LodgeFireGlow.png"));});
    At(19,[PC](){auto* Cam=PC->GetWorld()->SpawnActor<ACameraActor>(FVector(0,0,1000),FRotator(-40,0,0));PC->SetViewTarget(Cam);});
    At(20,[PC,Check](){bool Hidden=true;if(auto* Room=AWildlineLobby::Find(PC->GetWorld()))for(auto* C:Room->LodgeScenery)Hidden&=!C->IsVisible();Check(Hidden,TEXT("lodge scenery stays hidden during outdoor gameplay"));});
    At(21,[Passed](){UE_LOG(LogTemp,Display,TEXT("WILDLINE LODGE ATMOSPHERE: %s"),*Passed?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Passed?0:1);});
}

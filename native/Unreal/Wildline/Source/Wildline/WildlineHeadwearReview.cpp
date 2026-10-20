#include "WildlineGame.h"
#include "WildlineCharacter.h"
#include "WildlineHeadwear.h"
#include "WildlineHunterCreator.h"
#include "WildlineMenu.h"
#include "WildlineSocial.h"
#include "WildlineUserSettings.h"
#include "Components/EditableTextBox.h"
#include "Components/VerticalBox.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/HUD.h"
#include "TimerManager.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

void WildlineRunHeadwearReview(AWildlineController* PC){
    auto At=[PC](float T,TFunction<void()> Work){FTimerHandle H;PC->GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(PC,[Work](){Work();}),T,false);};
    auto Passed=MakeShared<bool>(true);auto Check=[Passed](bool OK,const TCHAR* Label){*Passed&=OK;UE_LOG(LogTemp,Display,TEXT("HEADWEAR %s: %s"),OK?TEXT("PASS"):TEXT("FAIL"),Label);};
    auto Shot=[](const TCHAR* Name){if(!FParse::Param(FCommandLine::Get(),TEXT("NullRHI")))FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Windows/")+Name,true,false);};
    auto Exit=[Passed](){UE_LOG(LogTemp,Display,TEXT("WILDLINE HEADWEAR REVIEW: %s"),*Passed?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,*Passed?0:1);};
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineExistingProfileProbe"))){
        At(1,[PC,Check](){auto* S=UWildlineUserSettings::Get();Check(S->PlayerDisplayName==TEXT("Mark")&&S->bLastProfileSaveSucceeded,TEXT("existing Mark profile migrates into shared storage"));PC->Social->Refresh();Check(!PC->Social->bOpen&&PC->Social->ProfileFields->GetVisibility()==ESlateVisibility::Collapsed,TEXT("actual returning player is not shown name setup"));});At(2,Exit);return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineProfileWriteProbe"))){
        At(1,[PC,Check](){auto* S=UWildlineUserSettings::Get();PC->Social->Refresh();Check(S->PlayerDisplayName.IsEmpty()&&PC->Social->bOpen,TEXT("fresh launch asks for a name"));PC->Social->Name->SetText(FText::FromString(TEXT("  Hat Tester  ")));PC->Social->SaveName();Check(S->PlayerDisplayName==TEXT("Hat Tester")&&!PC->Social->bOpen&&S->bLastProfileSaveSucceeded,TEXT("first setup saves trimmed name to shared profile"));S->HunterProfile=FWildlineHunterProfile::Travis();S->HunterProfile.Headwear=18;S->Persist();});At(2,Exit);return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("WildlineProfileReadProbe"))){
        At(1,[PC,Check](){auto* S=UWildlineUserSettings::Get();Check(S->PlayerDisplayName==TEXT("Hat Tester")&&S->HunterProfile.Headwear==18,TEXT("new process recovers name and hat without a build-local settings file"));PC->Social->Open(false);Check(!PC->Social->bOpen,TEXT("returning player is not prompted again"));PC->Social->Open(true);Check(PC->Social->ProfileFields->GetVisibility()==ESlateVisibility::Collapsed,TEXT("room panel hides name setup for an existing profile"));PC->Social->EditName();Check(PC->Social->bEditingName&&PC->Social->ProfileFields->GetVisibility()==ESlateVisibility::Visible,TEXT("explicit rename opens prefilled editor"));PC->Social->Name->SetText(FText::FromString(TEXT("Hat Tester Renamed")));PC->Social->SaveName();S->PlayerDisplayName.Empty();S->LoadSettings(true);Check(S->PlayerDisplayName==TEXT("Hat Tester Renamed"),TEXT("renamed profile survives settings reload"));});At(2,Exit);return;
    }
    At(1,[PC](){PC->SetMenu(true);PC->Social->Open(false);PC->Menu->CreateHunter();PC->Menu->Creator->LoadTravisPreset();PC->Menu->Creator->OpenHeadwear();});
    At(2,[PC,Check](){auto* C=PC->Menu->Creator;Check(C->bHatPicker&&C->bFaceView&&C->HatCards.Num()==37,TEXT("button opens close-up gallery with 36 hats and no-hat option"));
        bool Assets=true;for(int32 Id=0;Id<WildlineHeadwear::Count;Id++){Assets&=LoadObject<UTexture2D>(nullptr,*WildlineHeadwear::Asset(Id,true))!=nullptr;if(Id>0){auto* M=LoadObject<UStaticMesh>(nullptr,*WildlineHeadwear::Asset(Id));Assets&=M&&M->GetNumTriangles(0)>100;}}
        Check(Assets,TEXT("every gallery thumbnail and modeled hat loads"));C->HatCards[18]->OnClicked.Broadcast();Check(C->Pending.Headwear==18&&C->Preview->HunterHat->GetStaticMesh(),TEXT("thumbnail click equips chicken hat in live portrait"));});
    At(5,[Shot](){Shot(TEXT("HeadwearChicken.png"));});
    At(6,[PC](){PC->Menu->Creator->SelectHat(19);});At(8,[Shot](){Shot(TEXT("HeadwearDinosaur.png"));});
    At(9,[PC](){PC->Menu->Creator->SelectHat(14);PC->Menu->Creator->SetHatFilter(1);});At(11,[PC,Check,Shot](){Check(PC->Menu->Creator->HatCards.Num()==17,TEXT("field/work filter has 16 hats plus no hat"));Shot(TEXT("HeadwearConstruction.png"));});
    At(12,[PC](){PC->Menu->Creator->SetHatFilter(2);PC->Menu->Creator->SelectHat(27);PC->Menu->Creator->HatScroll->ScrollToEnd();});At(15,[PC,Check,Shot](){Check(PC->Menu->Creator->HatCards.Num()==21&&PC->Menu->Creator->HatScroll->GetScrollOffset()>100,TEXT("funny gallery scrolls to its last choices"));Shot(TEXT("HeadwearWizard.png"));});
    At(16,[PC,Check](){auto* C=PC->Menu->Creator;bool Fits=true;
        for(int32 Body=0;Body<2;Body++)for(float Height:{60.f,174.5f,230.f})for(float Face:{.6f,1.5f}){
            C->Pending.Body=Body;C->Pending.HeightCm=Height;C->Pending.FaceSize=Face;C->RefreshPreview();C->Preview->GetMesh()->TickAnimation(.1f,false);C->Preview->GetMesh()->RefreshBoneTransforms();
            const FTransform T=C->Preview->HunterHat->GetComponentTransform();const float Scale=C->Pending.HeightScale()*Face;
            Fits&=T.IsValid()&&FMath::IsNearlyEqual(static_cast<float>(T.GetScale3D().Z),Scale,.01f)&&FVector::Dist(T.GetLocation(),C->Preview->GetMesh()->GetSocketLocation(TEXT("head")))<25*Scale;
        }Check(Fits,TEXT("headwear follows head scaling on both bodies across supported heights and face sizes"));C->LoadTravisPreset();C->SelectHat(18);C->SaveHunter();PC->ChooseRole(false);auto* P=Cast<AWildlineCharacter>(PC->GetPawn());P->bThirdPerson=true;P->GetMesh()->SetOwnerNoSee(false);P->HunterHat->SetOwnerNoSee(false);PC->GetWorld()->GetGameState<AWildlineGameState>()->bRoundStarted=false;if(PC->GetHUD())PC->GetHUD()->bShowHUD=false;
        auto* Cam=PC->GetWorld()->SpawnActor<ACameraActor>();Cam->AttachToComponent(P->GetRootComponent(),FAttachmentTransformRules::KeepRelativeTransform);Cam->SetActorRelativeLocation(FVector(200,240,105));Cam->SetActorRelativeRotation((FVector(0,0,50)-FVector(200,240,105)).Rotation());Cam->GetCameraComponent()->FieldOfView=40;PC->SetViewTarget(Cam);
    });
    At(19,[PC,Check,Shot](){auto* P=Cast<AWildlineCharacter>(PC->GetPawn());Check(P->HunterProfile.Headwear==18&&P->HunterHat->IsVisible()&&P->HunterHat->GetStaticMesh()&&P->HunterHat->GetAttachSocketName()==TEXT("head"),TEXT("saved hat is worn on the gameplay hunter"));Shot(TEXT("HeadwearInGame.png"));});
    At(21,Exit);
}

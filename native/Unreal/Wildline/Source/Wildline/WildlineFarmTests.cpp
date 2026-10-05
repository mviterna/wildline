#include "WildlineGame.h"
#include "WildlineWorld.h"
#include "WildlineCharacter.h"
#include "WildlineLivestock.h"
#include "WildlineRules.h"
#include "EngineUtils.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void AWildlineController::RunFarmSmokeTest(){
    auto Result=MakeShared<bool>(true);
    auto Check=[Result](bool Ok,const TCHAR* What){*Result&=Ok;UE_LOG(LogTemp,Display,TEXT("FARM CHECK %s: %s"),What,Ok?TEXT("PASS"):TEXT("FAIL"));};
    auto* F=AWildlineWorld::Find(GetWorld());auto* S=GetWorld()->GetGameState<AWildlineGameState>();
    Check(F&&F->bFarm&&S&&S->ExpeditionMode==1,TEXT("Farm Raid map and replicated mode"));
    if(!F||!S){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    ChooseRole(true);auto* P=Cast<AWildlineCharacter>(GetPawn());
    for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)It->bBot=false;
    Check(F->Farmhouse->GetStaticMesh()&&F->Farmhouse->GetInstanceCount()==1&&F->AcornLocations.Num()==30,TEXT("Farmhouse and 30 peanut supplies"));
    int32 Animals=0;AWildlineLivestock* Cow=nullptr;
    for(TActorIterator<AWildlineLivestock> It(GetWorld());It;++It){Animals++;Check(It->Mesh->GetSkeletalMeshAsset()!=nullptr,TEXT("Textured livestock imported"));if(It->Species==0)Cow=*It;}
    Check(Animals==6&&Cow,TEXT("Four cows and two horses"));
    FCollisionQueryParams Q(SCENE_QUERY_STAT(FarmTest),false,P);FHitResult H;
    const FVector House=F->FarmhouseLocation;
    Check(GetWorld()->LineTraceSingleByChannel(H,House+FVector(-700,250,140),House+FVector(-500,250,140),ECC_Visibility,Q),TEXT("Farmhouse walls block bullets"));
    Check(!GetWorld()->SweepSingleByObjectType(H,House+FVector(-730,0,F->FarmFloor+105),House+FVector(-470,0,F->FarmFloor+105),FQuat::Identity,FCollisionObjectQueryParams(ECC_WorldStatic),FCollisionShape::MakeCapsule(42,96),Q),TEXT("Farmer fits through front door"));
    Check(!GetWorld()->SweepSingleByObjectType(H,House+FVector(730,0,F->FarmFloor+105),House+FVector(470,0,F->FarmFloor+105),FQuat::Identity,FCollisionObjectQueryParams(ECC_WorldStatic),FCollisionShape::MakeCapsule(42,96),Q),TEXT("Rear service door is walkable"));
    Check(GetWorld()->LineTraceSingleByChannel(H,House+FVector(0,0,F->FarmFloor+150),House+FVector(0,0,F->FarmFloor-50),ECC_Visibility,Q)&&FMath::Abs(H.ImpactPoint.Z-(House.Z+F->FarmFloor))<2,TEXT("Pantry floor supports characters at the raised porch height"));
    const bool VentBlocked=GetWorld()->SweepSingleByObjectType(H,House+FVector(40,550,F->FarmFloor+21),House+FVector(40,430,F->FarmFloor+21),FQuat::Identity,FCollisionObjectQueryParams(ECC_WorldStatic),FCollisionShape::MakeCapsule(10,13),Q);
    if(VentBlocked)UE_LOG(LogTemp,Display,TEXT("FARM VENT obstruction: %s"),*H.ToString());
    Check(!VentBlocked,TEXT("Squirrel fits through low vent"));
    Check(GetWorld()->SweepSingleByObjectType(H,House+FVector(40,580,F->FarmFloor+105),House+FVector(40,420,F->FarmFloor+105),FQuat::Identity,FCollisionObjectQueryParams(ECC_WorldStatic),FCollisionShape::MakeCapsule(42,96),Q),TEXT("Low vent blocks farmer"));
    P->SetActorLocation(House+FVector(700,0,13));P->Tick(.01f);Check(P->Bag==0,TEXT("Peanuts cannot be collected outside house"));
    P->SetActorLocation(F->AcornLocations[0]+FVector(0,0,13));P->Tick(.01f);Check(P->Bag>0&&P->Bag<=4&&S->TakenPeanuts.Num()==P->Bag,TEXT("Inside pantry pickups are shared and capped"));
    auto* Second=GetWorld()->SpawnActorDeferred<AWildlineCharacter>(AWildlineCharacter::StaticClass(),FTransform(P->GetActorLocation()),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);Second->bSquirrel=true;UGameplayStatics::FinishSpawningActor(Second,FTransform(P->GetActorLocation()+FVector(0,0,400)));
    const auto Taken=S->TakenPeanuts;for(int32 I=0;I<30;I++)S->TakenPeanuts.AddUnique(I);Second->SetActorLocation(P->GetActorLocation());Second->Tick(.01f);Check(Second->Bag==0,TEXT("Second squirrel cannot duplicate shared loot"));S->TakenPeanuts=Taken;Second->Destroy();
    const int32 Carried=P->Bag;P->SetActorLocation(F->DenLocation+FVector(200,0,13));P->Tick(.01f);Check(P->Bag==0&&S->StoredAcorns==Carried&&S->TakenPeanuts.Num()==Carried,TEXT("Den stores peanuts and keeps deposited supplies removed"));
    P->SetActorLocation(F->AcornLocations[29]+FVector(0,0,13));P->Tick(.01f);Check(P->Bag>0,TEXT("More supplies remain stealable"));P->ReturnCarriedPeanuts();P->Bag=0;Check(S->TakenPeanuts.Num()==Carried,TEXT("Lost carried peanuts return to pantry"));
    // Real damage after the warning interval, followed by a dodge check.
    P->SetActorLocation(Cow->GetActorLocation()+FVector(155,0,-72));P->GetCharacterMovement()->DisableMovement();Cow->Tick(.01f);Check(Cow->StrikeAt>P->ServerTime()&&P->Health==40,TEXT("Livestock warns before damage"));
    FTimerHandle Kick;GetWorldTimerManager().SetTimer(Kick,FTimerDelegate::CreateWeakLambda(this,[this,Check,P,Cow](){
        Check(P->Health==22,TEXT("Cow kick applies 18 squirrel damage"));
        P->SetActorLocation(Cow->GetActorLocation()+FVector(600,0,-72));
        Check(!P->bDead,TEXT("One cow kick is survivable"));
    }),1.15f,false);
    FTimerHandle Gun;GetWorldTimerManager().SetTimer(Gun,FTimerDelegate::CreateWeakLambda(this,[this,Check,F,S](){
        ChooseRole(false);auto* Farmer=Cast<AWildlineCharacter>(GetPawn());for(TActorIterator<AWildlineCharacter> It(GetWorld());It;++It)It->bBot=false;
        Farmer->SetActorLocation(FVector(-3300,-3400,320));Farmer->SelectWeapon(1);Check(Farmer->SelectedWeapon==1&&Farmer->Ammo==2&&Farmer->ViewRifle->GetStaticMesh(),TEXT("Selectable shotgun has two shells"));
        const FVector Eye=Farmer->GetActorLocation()+FVector(0,0,72);const FVector Direction=FVector(1,0,-.12f).GetSafeNormal();FVector Target=Eye+Direction*230;
        auto* Victim=GetWorld()->SpawnActorDeferred<AWildlineCharacter>(AWildlineCharacter::StaticClass(),FTransform(Target),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);Victim->bSquirrel=true;UGameplayStatics::FinishSpawningActor(Victim,FTransform(Target));Victim->GetCharacterMovement()->DisableMovement();
        Farmer->ServerFire(Direction);Check(Farmer->Ammo==1&&Victim->bDead,TEXT("Shotgun pellet spread hits and consumes one shell"));Victim->Destroy();
        Farmer->ServerReload();Check(Farmer->ReloadUntil>Farmer->ServerTime(),TEXT("Shotgun starts pump-action reload"));
    }),1.5f,false);
    FTimerHandle Done;GetWorldTimerManager().SetTimer(Done,FTimerDelegate::CreateWeakLambda(this,[this,Check,Result,S](){
        auto* P=Cast<AWildlineCharacter>(GetPawn());Check(P&&P->Ammo==2&&P->ReloadUntil==0,TEXT("Shotgun reload restores two shells"));
        P->SetActorLocation(FVector(1250,250,320));P->GetCharacterMovement()->DisableMovement();
        const int32 BeforeShots=P->CosmeticShots,BeforeImpacts=P->CosmeticImpacts;int32 BeforeMarks=0;for(TActorIterator<AWildlineImpactMark> It(GetWorld());It;++It)BeforeMarks++;
        P->ServerFire(FVector::ForwardVector);
        Check(P->CosmeticShots==BeforeShots+1&&P->CosmeticImpacts==BeforeImpacts+12,TEXT("Accepted wall shot produces one muzzle event and twelve pellet impacts"));
        int32 Marks=0,Effects=0;for(TActorIterator<AWildlineImpactMark> It(GetWorld());It;++It)Marks++;
        for(TActorIterator<AWildlineWeaponFX> It(GetWorld());It;++It){Effects++;Check(!It->GetActorEnableCollision()&&!It->Particles.IsEmpty(),TEXT("FX cannot interfere with gameplay collision"));for(const auto& Particle:It->Particles)Check(Particle.Material!=nullptr&&Particle.Mesh->GetStaticMesh()!=nullptr,TEXT("Cooked effect materials and meshes resolve"));}
        Check(Marks==BeforeMarks+12&&Effects>1,TEXT("Pellet marks and grouped dust debris spawned"));
        S->RoundStartedAt=S->GetServerWorldTimeSeconds()-361;GetWorld()->GetAuthGameMode<AWildlineGameMode>()->Tick(.01f);Check(S->Winner==1,TEXT("Farmers win when time runs out"));
        UE_LOG(LogTemp,Display,TEXT("WILDLINE FARM SMOKE: %s"),*Result?TEXT("PASS"):TEXT("FAIL"));FFileHelper::SaveStringToFile(*Result?TEXT("PASS\n"):TEXT("FAIL\n"),*(FPaths::ProjectSavedDir()/TEXT("FarmSmokeResult.txt")));FPlatformMisc::RequestExitWithStatus(false,*Result?0:1);
    }),4.6f,false);
}

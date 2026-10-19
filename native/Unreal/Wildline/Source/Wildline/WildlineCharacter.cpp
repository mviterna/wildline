#include "WildlineCharacter.h"
#include "WildlineWorld.h"
#include "WildlineRules.h"
#include "WildlineSquirrelAnim.h"
#include "WildlineHunterAnim.h"
#include "WildlineGame.h"
#include "WildlineUserSettings.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InputComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "AIController.h"
#include "WildlineAudio.h"
#include "Animation/AnimInstance.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "ProceduralMeshComponent.h"

AWildlineCharacter::AWildlineCharacter(){
    PrimaryActorTick.bCanEverTick=true;bReplicates=true;SetReplicateMovement(true);
    GetCapsuleComponent()->InitCapsuleSize(38,96);GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);bUseControllerRotationYaw=true;
    GetCharacterMovement()->bOrientRotationToMovement=false;GetCharacterMovement()->MaxWalkSpeed=320;GetCharacterMovement()->JumpZVelocity=420;GetCharacterMovement()->AirControl=.3f;
    GetCharacterMovement()->NavAgentProps.bCanCrouch=true;GetCharacterMovement()->MaxWalkSpeedCrouched=160;
    FirstCamera=CreateDefaultSubobject<UCameraComponent>(TEXT("First person"));FirstCamera->SetupAttachment(RootComponent);FirstCamera->SetRelativeLocation(FVector(12,0,72));FirstCamera->bUsePawnControlRotation=true;FirstCamera->FieldOfView=78;FirstCamera->bAutoActivate=true;
    Boom=CreateDefaultSubobject<USpringArmComponent>(TEXT("Chase arm"));Boom->SetupAttachment(RootComponent);Boom->TargetArmLength=360;Boom->SocketOffset=FVector(0,55,55);Boom->bUsePawnControlRotation=true;Boom->bEnableCameraLag=true;Boom->CameraLagSpeed=12;
    ThirdCamera=CreateDefaultSubobject<UCameraComponent>(TEXT("Third person"));ThirdCamera->SetupAttachment(Boom);ThirdCamera->bUsePawnControlRotation=false;ThirdCamera->bAutoActivate=false;ThirdCamera->SetActive(false);
    ViewRifle=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("First person rifle"));ViewRifle->SetupAttachment(FirstCamera);ViewRifle->SetOnlyOwnerSee(true);ViewRifle->SetCollisionEnabled(ECollisionEnabled::NoCollision);ViewRifle->CastShadow=false;
    WorldRifle=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("World rifle"));WorldRifle->SetupAttachment(GetMesh());WorldRifle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ViewBolt=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("First person bolt"));ViewBolt->SetupAttachment(ViewRifle);ViewBolt->SetOnlyOwnerSee(true);ViewBolt->SetCollisionEnabled(ECollisionEnabled::NoCollision);ViewBolt->SetCastShadow(false);
    WorldBolt=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("World bolt"));WorldBolt->SetupAttachment(WorldRifle);WorldBolt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AnimalRoot=CreateDefaultSubobject<USceneComponent>(TEXT("Squirrel body"));AnimalRoot->SetupAttachment(RootComponent);
    SquirrelHitbox=CreateDefaultSubobject<UCapsuleComponent>(TEXT("Squirrel weapon hitbox"));SquirrelHitbox->SetupAttachment(RootComponent);
    SquirrelHitbox->InitCapsuleSize(7,12);SquirrelHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SquirrelHitbox->SetCollisionResponseToAllChannels(ECR_Ignore);SquirrelHitbox->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    SquirrelHitbox->SetGenerateOverlapEvents(false);SquirrelHitbox->SetCanEverAffectNavigation(false);
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Human(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
    HumanMeshAsset=Human.Object;
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Squirrel(TEXT("/Game/Wildline/Squirrel/SK_Squirrel.SK_Squirrel"));SquirrelMeshAsset=Squirrel.Object;
    GetMesh()->SetupAttachment(AnimalRoot);GetMesh()->SetSkeletalMesh(HumanMeshAsset);GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;GetMesh()->SetRelativeLocation(FVector(0,0,-96));GetMesh()->SetRelativeRotation(FRotator(0,-90,0));GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Rifle(TEXT("/Game/Wildline/Hunters/Meshes/SM_BoltRifle.SM_BoltRifle"));ViewRifle->SetStaticMesh(Rifle.Object);WorldRifle->SetStaticMesh(Rifle.Object);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Bolt(TEXT("/Game/Wildline/Hunters/Meshes/SM_RifleBolt.SM_RifleBolt"));ViewBolt->SetStaticMesh(Bolt.Object);WorldBolt->SetStaticMesh(Bolt.Object);
    ViewRifle->SetRelativeLocation(FVector(30,14,-19));ViewRifle->SetRelativeRotation(FRotator::ZeroRotator);WorldRifle->SetRelativeLocation(FVector(0,30,130));WorldRifle->SetRelativeRotation(FRotator::ZeroRotator);
    AIControllerClass=AAIController::StaticClass();AutoPossessAI=EAutoPossessAI::Disabled;
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ThermalFilter(TEXT("/Game/Wildline/Materials/M_ThermalVision.M_ThermalVision"));ThermalMaterial=ThermalFilter.Object;
}
void AWildlineCharacter::BeginPlay(){Super::BeginPlay();Forest=AWildlineWorld::Find(GetWorld());ConfigureRole();AWildlineWeaponFX::Warmup(GetWorld());if(bBot&&HasAuthority()){ResetBotAwareness();SpawnDefaultController();}}
void AWildlineCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AWildlineCharacter,bInLobby);DOREPLIFETIME(AWildlineCharacter,bLobbyReady);DOREPLIFETIME(AWildlineCharacter,CourseCheckpoint);DOREPLIFETIME(AWildlineCharacter,CourseLane);DOREPLIFETIME(AWildlineCharacter,FeederVisits);DOREPLIFETIME(AWildlineCharacter,bSquirrel);DOREPLIFETIME(AWildlineCharacter,bDead);DOREPLIFETIME(AWildlineCharacter,bBot);DOREPLIFETIME(AWildlineCharacter,BotSlot);DOREPLIFETIME(AWildlineCharacter,AIDifficulty);DOREPLIFETIME(AWildlineCharacter,ProtectedUntil);DOREPLIFETIME(AWildlineCharacter,bSprinting);DOREPLIFETIME(AWildlineCharacter,Stamina);DOREPLIFETIME(AWildlineCharacter,Health);DOREPLIFETIME(AWildlineCharacter,Ammo);DOREPLIFETIME(AWildlineCharacter,Bag);DOREPLIFETIME(AWildlineCharacter,ThermalUntil);DOREPLIFETIME(AWildlineCharacter,ThermalReadyAt);DOREPLIFETIME(AWildlineCharacter,ReloadUntil);DOREPLIFETIME(AWildlineCharacter,RespawnAt);DOREPLIFETIME(AWildlineCharacter,TreeIndex);DOREPLIFETIME(AWildlineCharacter,bWoodClimbing);DOREPLIFETIME(AWildlineCharacter,bBranchClimbing);DOREPLIFETIME(AWildlineCharacter,ClimbNormal);DOREPLIFETIME(AWildlineCharacter,ClimbHeading);DOREPLIFETIME(AWildlineCharacter,ClimbPosition);DOREPLIFETIME(AWildlineCharacter,Collected);DOREPLIFETIME(AWildlineCharacter,BagPeanuts);DOREPLIFETIME(AWildlineCharacter,SelectedWeapon);
    DOREPLIFETIME(AWildlineCharacter,HunterProfile);DOREPLIFETIME(AWildlineCharacter,HunterFaceData);
    DOREPLIFETIME_CONDITION(AWildlineCharacter,bAiming,COND_SkipOwner);
}
float AWildlineCharacter::ServerTime()const{const auto* S=GetWorld()->GetGameState();return S?S->GetServerWorldTimeSeconds():GetWorld()->GetTimeSeconds();}
bool AWildlineCharacter::CanControl()const{const auto* PC=Cast<AWildlineController>(Controller);const auto* S=GetWorld()->GetGameState<AWildlineGameState>();return !bDead&&(!PC||!PC->bMenuOpen)&&(!S||S->Winner==0);}
const FWildlineMatchConfig& AWildlineCharacter::MatchRules()const{const auto* S=GetWorld()->GetGameState<AWildlineGameState>();static const FWildlineMatchConfig Default;return S?S->MatchConfig:Default;}
bool AWildlineCharacter::IsThermalActive()const{return !bSquirrel&&!bDead&&SelectedWeapon==WildlineRules::RifleWeapon&&ServerTime()<ThermalUntil;}
bool AWildlineCharacter::IsScoped()const{return !bSquirrel&&!bDead&&SelectedWeapon==WildlineRules::RifleWeapon&&bAiming;}
void AWildlineCharacter::ConfigureRole(){
    if(bSquirrel)SetActorScale3D(FVector::OneVector);
    // The authored animal is 42.5 cm nose to tail; 1.15 gives 48.9 cm overall.
    GetCapsuleComponent()->SetCapsuleSize(bSquirrel?7.f:38.f,bSquirrel?12.f:96.f);
    UpdateSquirrelHitbox();
    AnimalRoot->SetRelativeTransform(FTransform::Identity);
    GetMesh()->SetSkeletalMesh(bSquirrel?SquirrelMeshAsset:HumanMeshAsset);
    GetMesh()->EmptyOverrideMaterials();
    GetMesh()->SetRelativeLocation(FVector(0,0,bSquirrel?-12:-96));
    GetMesh()->SetRelativeRotation(FRotator(0,-90,0));GetMesh()->SetRelativeScale3D(FVector(bSquirrel?1.15f:1.f));
    GetMesh()->SetVisibility(true);GetMesh()->SetOwnerNoSee(!bThirdPerson);
    if(bSquirrel)GetMesh()->SetAnimInstanceClass(UWildlineSquirrelAnim::StaticClass());
    else GetMesh()->SetAnimInstanceClass(UWildlineHunterAnim::StaticClass());
    ViewRifle->SetVisibility(!bSquirrel);WorldRifle->SetVisibility(!bSquirrel);WorldRifle->SetOwnerNoSee(!bThirdPerson);
    ViewBolt->SetVisibility(!bSquirrel);WorldBolt->SetVisibility(!bSquirrel);
    FirstCamera->SetRelativeLocation(bSquirrel?FVector(13,0,1):FVector(12,0,72));Boom->TargetArmLength=bSquirrel?110:360;Boom->SocketOffset=bSquirrel?FVector(0,12,22):FVector(0,55,55);
    bUseControllerRotationYaw=!bSquirrel;auto* Movement=GetCharacterMovement();Movement->bOrientRotationToMovement=bSquirrel;Movement->RotationRate=FRotator(0,720,0);
    Movement->GravityScale=bSquirrel?1.2f:1.f;Movement->AirControl=bSquirrel?.18f:.3f;
    Movement->MaxStepHeight=bSquirrel?7:45;Movement->JumpZVelocity=bSquirrel?360:420;
    Movement->MaxAcceleration=bSquirrel?6500:2048;Movement->BrakingDecelerationWalking=bSquirrel?4200:2048;Movement->GroundFriction=bSquirrel?10:8;
    if(HasAuthority()){Health=bSquirrel?MatchRules().SquirrelHealth:100.f;ProtectedUntil=bSquirrel?ServerTime()+MatchRules().SpawnProtectionSeconds:0;}
    BuildFur();
    UpdateHeatSignature();
    if(!bSquirrel){ApplyHunterProfile();OnRepWeapon();}
}
void AWildlineCharacter::OnRepWeapon(){
    const bool Shotgun=SelectedWeapon==WildlineRules::ShotgunWeapon;
    auto* Body=LoadObject<UStaticMesh>(nullptr,Shotgun?TEXT("/Game/Wildline/Farm/Meshes/SM_Shotgun.SM_Shotgun"):TEXT("/Game/Wildline/Hunters/Meshes/SM_BoltRifle.SM_BoltRifle"));
    auto* Action=LoadObject<UStaticMesh>(nullptr,Shotgun?TEXT("/Game/Wildline/Farm/Meshes/SM_ShotgunAction.SM_ShotgunAction"):TEXT("/Game/Wildline/Hunters/Meshes/SM_RifleBolt.SM_RifleBolt"));
    ViewRifle->SetStaticMesh(Body);WorldRifle->SetStaticMesh(Body);ViewBolt->SetStaticMesh(Action);WorldBolt->SetStaticMesh(Action);
    // Weapon lengths differ. Re-evaluate the new stock and both wrist targets
    // in the switch frame, so one frame never shows the old shoulder offset.
    if(!bSquirrel&&!bPreviewHunter&&GetMesh()->GetAnimInstance()){
        GetMesh()->TickAnimation(0.f,false);GetMesh()->RefreshBoneTransforms();
    }
}
void AWildlineCharacter::SelectWeapon(int32 Weapon){
    if(!HasAuthority()||bSquirrel||bDead||ReloadUntil>0||ServerTime()-LastShot<1.1f)return;
    Weapon=FMath::Clamp(Weapon,0,1);if(Weapon==SelectedWeapon)return;
    if(SelectedWeapon==0)RifleAmmo=Ammo;else ShotgunAmmo=Ammo;
    SelectedWeapon=Weapon;if(Weapon==WildlineRules::ShotgunWeapon)ThermalUntil=0;Ammo=Weapon==0?RifleAmmo:ShotgunAmmo;OnRepWeapon();ForceNetUpdate();
}
void AWildlineCharacter::ReturnCarriedPeanuts(){
    if(!HasAuthority())return;
    if(auto* State=GetWorld()->GetGameState<AWildlineGameState>();State&&State->ExpeditionMode==WildlineRules::FarmMode){for(int32 Id:BagPeanuts){State->TakenPeanuts.Remove(Id);Collected.Remove(Id);}State->ForceNetUpdate();}
    BagPeanuts.Empty();
}
void AWildlineCharacter::ApplyHunterProfile(){
    if(bSquirrel)return;HunterProfile.Clamp();
    const bool bTravis=HunterProfile.Preset==1&&HunterProfile.Body==0;
    const bool bEthan=HunterProfile.Preset==3&&HunterProfile.Body==0;
    const bool bGreyson=HunterProfile.Preset==4&&HunterProfile.Body==0;
    const bool bMark=HunterProfile.Preset==5&&HunterProfile.Body==0;
    auto* Asset=LoadObject<USkeletalMesh>(nullptr,HunterProfile.Body==1?TEXT("/Game/Wildline/Hunters/Meshes/SK_HunterFemale.SK_HunterFemale"):bTravis?TEXT("/Game/Wildline/Hunters/Meshes/SK_HunterTravis.SK_HunterTravis"):bEthan?TEXT("/Game/Wildline/Hunters/Meshes/SK_HunterEthan.SK_HunterEthan"):bGreyson?TEXT("/Game/Wildline/Hunters/Meshes/SK_HunterGreyson.SK_HunterGreyson"):bMark?TEXT("/Game/Wildline/Hunters/Meshes/SK_HunterMark.SK_HunterMark"):TEXT("/Game/Wildline/Hunters/Meshes/SK_HunterMale.SK_HunterMale"));
    if(Asset&&GetMesh()->GetSkeletalMeshAsset()!=Asset){HumanMeshAsset=Asset;GetMesh()->SetSkeletalMesh(Asset);GetMesh()->SetAnimInstanceClass(UWildlineHunterAnim::StaticClass());}
    // Scale the complete rig, collision and attached gear together. Deriving
    // this from the saved/replicated preset also upgrades existing profiles.
    const float Scale=HunterProfile.HeightScale();
    const float OldScale=GetActorScale3D().Z;
    const float FeetZ=GetActorLocation().Z-GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    SetActorScale3D(FVector(Scale));
    if(!FMath::IsNearlyEqual(OldScale,Scale)&&(HasAuthority()||IsLocallyControlled()||bPreviewHunter)){
        auto Position=GetActorLocation();Position.Z=FeetZ+GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        SetActorLocation(Position,false,nullptr,ETeleportType::TeleportPhysics);
    }
    BaseEyeHeight=72.f*Scale;
    Boom->TargetArmLength=360.f*Scale;Boom->SocketOffset=FVector(0,55,55)*Scale;
    GetCharacterMovement()->MaxStepHeight=45.f*Scale;
    HunterFaceTexture=HunterProfile.bUsePhoto?WildlinePortrait::DecodePhoto(HunterFaceData):nullptr;
    // Named hunters carry a cooked photo on a featureless head.
    // An optional custom portrait overrides the selected built-in photo.
    HunterMaterials.Empty();const FLinearColor Outfits[]={FLinearColor(.20f,.28f,.10f),FLinearColor(1.f,.12f,.008f),FLinearColor(.32f,.18f,.065f)};
    const FLinearColor Hair[]={FLinearColor(.35f,.23f,.13f),FLinearColor(.07f,.06f,.05f),FLinearColor(.62f,.61f,.58f)};
    GetMesh()->EmptyOverrideMaterials();
    for(int32 I=0;I<GetMesh()->GetNumMaterials();I++)if(auto* M=GetMesh()->CreateDynamicMaterialInstance(I)){
        M->SetVectorParameterValue(TEXT("OutfitTint"),Outfits[HunterProfile.Outfit]);M->SetVectorParameterValue(TEXT("HairTint"),Hair[HunterProfile.Hair]);
        const float T=HunterProfile.SkinTone;M->SetVectorParameterValue(TEXT("SkinTint"),FLinearColor(1-T*.55f,1-T*.68f,1-T*.73f));M->SetScalarParameterValue(TEXT("CapVisible"),HunterProfile.bCap?1:0);
        M->SetScalarParameterValue(TEXT("BeardVisible"),HunterProfile.bBeard?1:0);
        M->SetScalarParameterValue(TEXT("CloseCrop"),0);
        M->SetScalarParameterValue(TEXT("FaceFill"),bPreviewHunter?.025f:.18f);
        M->SetScalarParameterValue(TEXT("HasPhoto"),HunterFaceTexture?1:0);if(HunterFaceTexture)M->SetTextureParameterValue(TEXT("FacePhoto"),HunterFaceTexture);
        M->SetScalarParameterValue(TEXT("PhotoX"),HunterProfile.PhotoX);M->SetScalarParameterValue(TEXT("PhotoY"),HunterProfile.PhotoY);M->SetScalarParameterValue(TEXT("PhotoZoom"),HunterProfile.PhotoZoom);HunterMaterials.Add(M);
    }
    GetMesh()->SetMorphTarget(TEXT("FaceWidth"),HunterProfile.FaceWidth);GetMesh()->SetMorphTarget(TEXT("NoseDepth"),HunterProfile.NoseDepth);
    GetMesh()->SetMorphTarget(TEXT("SlimBuild"),HunterProfile.Slimness);
    // Stay in the mesh's space through crouch offsets and network smoothing.
    // The animation publishes the final shoulder transform after evaluation.
    WorldRifle->AttachToComponent(GetMesh(),FAttachmentTransformRules::KeepWorldTransform);
    if(bPreviewHunter){
        WorldRifle->SetVisibility(false);WorldBolt->SetVisibility(false);
        GetMesh()->SetAnimInstanceClass(UWildlineHunterAnim::StaticClass());
    }
}
void AWildlineCharacter::OnRepHunterProfile(){ApplyHunterProfile();}
void AWildlineCharacter::SubmitSavedHunterProfile(){
    if(auto* S=UWildlineUserSettings::Get()){bProfileSubmitted=true;HunterProfile=S->HunterProfile;HunterFaceData=HunterProfile.bUsePhoto?WildlinePortrait::LoadSavedPhoto():FString();ApplyHunterProfile();ServerSetHunterProfile(HunterProfile,HunterFaceData);}
}
void AWildlineCharacter::ServerSetHunterProfile_Implementation(FWildlineHunterProfile Profile,const FString& FaceData){
    Profile.Clamp();if(!FaceData.IsEmpty()&&!WildlinePortrait::ValidatePhoto(FaceData))return;
    HunterProfile=Profile;HunterFaceData=FaceData;ApplyHunterProfile();ForceNetUpdate();
}
void AWildlineCharacter::OnRepRole(){ConfigureRole();}
void AWildlineCharacter::UpdateSquirrelHitbox(){
    // Only weapon/visibility queries use this shape. Movement keeps its original capsule.
    const float Scale=MatchRules().SquirrelHitboxScale();
    if(!FMath::IsNearlyEqual(SquirrelHitbox->GetUnscaledCapsuleRadius(),7.f*Scale))SquirrelHitbox->SetCapsuleSize(7.f*Scale,12.f*Scale,false);
    const auto Collision=bSquirrel&&!bDead?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision;
    if(SquirrelHitbox->GetCollisionEnabled()!=Collision)SquirrelHitbox->SetCollisionEnabled(Collision);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,bSquirrel||bDead?ECR_Ignore:ECR_Block);
}
void AWildlineCharacter::OnRepDead(){
    if(bDead){GetCharacterMovement()->StopMovementImmediately();GetCharacterMovement()->SetMovementMode(MOVE_Falling);GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);TreeIndex=INDEX_NONE;AnimalRoot->SetRelativeLocation(FVector(0,0,-7));AnimalRoot->SetRelativeRotation(FRotator(0,0,82));}
    else {GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);GetCharacterMovement()->SetMovementMode(MOVE_Walking);AnimalRoot->SetRelativeTransform(FTransform::Identity);GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);}
    UpdateSquirrelHitbox();
}
void AWildlineCharacter::SetupPlayerInputComponent(UInputComponent* I){
    Super::SetupPlayerInputComponent(I);
    I->BindAxis(TEXT("MoveForward"),this,&AWildlineCharacter::MoveForward);I->BindAxis(TEXT("MoveRight"),this,&AWildlineCharacter::MoveRight);I->BindAxis(TEXT("Turn"),this,&AWildlineCharacter::Turn);I->BindAxis(TEXT("LookUp"),this,&AWildlineCharacter::LookUp);
    I->BindAction(TEXT("Jump"),IE_Pressed,this,&AWildlineCharacter::DoJump);I->BindAction(TEXT("Sprint"),IE_Pressed,this,&AWildlineCharacter::StartSprint);I->BindAction(TEXT("Sprint"),IE_Released,this,&AWildlineCharacter::StopSprint);
    I->BindAction(TEXT("Climb"),IE_Pressed,this,&AWildlineCharacter::Climb);I->BindAction(TEXT("Thermal"),IE_Pressed,this,&AWildlineCharacter::Thermal);I->BindAction(TEXT("Reload"),IE_Pressed,this,&AWildlineCharacter::Reload);
    I->BindAction(TEXT("Camera"),IE_Pressed,this,&AWildlineCharacter::ToggleCamera);I->BindAction(TEXT("Aim"),IE_Pressed,this,&AWildlineCharacter::StartAim);I->BindAction(TEXT("Aim"),IE_Released,this,&AWildlineCharacter::StopAim);
    I->BindAction(TEXT("Fire"),IE_Pressed,this,&AWildlineCharacter::StartFire);I->BindAction(TEXT("Fire"),IE_Released,this,&AWildlineCharacter::StopFire);I->BindAction(TEXT("Crouch"),IE_Pressed,this,&AWildlineCharacter::DoCrouch);I->BindAction(TEXT("Crouch"),IE_Released,this,&AWildlineCharacter::DoUnCrouch);
}
void AWildlineCharacter::MoveForward(float V){ForwardInput=CanControl()?V:0;if(CanControl()&&TreeIndex==INDEX_NONE)AddMovementInput(FRotationMatrix(FRotator(0,GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::X),V);}
void AWildlineCharacter::MoveRight(float V){SideInput=CanControl()?V:0;if(CanControl()&&TreeIndex==INDEX_NONE)AddMovementInput(FRotationMatrix(FRotator(0,GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::Y),V);}
void AWildlineCharacter::Turn(float V){if(CanControl()){const auto* S=UWildlineUserSettings::Get();AddControllerYawInput(V*(IsScoped()?(S?S->ScopeSensitivity:.256f):(S?S->MouseSensitivity:.8f)));}}
void AWildlineCharacter::LookUp(float V){if(CanControl()){const auto* S=UWildlineUserSettings::Get();AddControllerPitchInput(WildlineRules::PitchInput(V,S?S->bInvertMouseY:true,IsScoped()?(S?S->ScopeSensitivity:.256f):(S?S->MouseSensitivity:.8f)));}}
void AWildlineCharacter::ToggleCamera(){if(!CanControl())return;bThirdPerson=!bThirdPerson;}
void AWildlineCharacter::StartAim(){if(CanControl()&&!bSquirrel){bAiming=true;ServerAim(true);}}void AWildlineCharacter::StopAim(){bAiming=false;ServerAim(false);}
void AWildlineCharacter::ServerAim_Implementation(bool V){bAiming=V&&!bSquirrel&&!bDead;}
void AWildlineCharacter::StartFire(){if(CanControl()&&!bSquirrel){bFiring=true;LastFireRequest=-10;}}void AWildlineCharacter::StopFire(){bFiring=false;}
void AWildlineCharacter::StartSprint(){if(CanControl()){bSprinting=true;ServerSprint(true);}}void AWildlineCharacter::StopSprint(){bSprinting=false;ServerSprint(false);}
void AWildlineCharacter::ServerSprint_Implementation(bool V){bSprinting=V&&!bDead&&Stamina>4;}
void AWildlineCharacter::DoCrouch(){if(CanControl()&&!bSquirrel)Crouch();}void AWildlineCharacter::DoUnCrouch(){UnCrouch();}
void AWildlineCharacter::DoJump(){if(CanControl()){bClimbTransfer=false;ServerJump();}}void AWildlineCharacter::Climb(){if(!CanControl())return;bClimbTransfer=false;if(bInLobby){if(auto* PC=Cast<AWildlineController>(Controller))PC->ServerLobbyInteract();}else ServerClimb();}void AWildlineCharacter::Thermal(){if(CanControl())ServerThermal();}void AWildlineCharacter::Reload(){if(CanControl())ServerReload();}
void AWildlineCharacter::ServerJump_Implementation(){
    if(bDead||Stamina<15)return;
    bClimbTransfer=false;
    if(bSquirrel){
        auto* Movement=GetCharacterMovement();const bool FromTree=TreeIndex!=INDEX_NONE;
        // Running over uneven ground can briefly lose floor contact. Allow a
        // short grace period, but consume it once so it cannot become a double jump.
        if(!FromTree&&(bGroundJumpConsumed||(!Movement->IsMovingOnGround()&&ServerTime()-LastGroundedAt>.12f)))return;
        bGroundJumpConsumed=true;
        const float LaunchZ=FromTree?450:Forest&&Forest->bResidential?Forest->CourseLaunchZ(GetActorLocation()):360;
        TreeIndex=INDEX_NONE;Movement->SetMovementMode(MOVE_Falling);FRotator R=GetControlRotation();R.Pitch=0;R.Roll=0;
        const FVector Horizontal=FromTree?R.Vector()*900:(Movement->Velocity.Size2D()>10?Movement->Velocity.GetSafeNormal2D()*FMath::Max(600.f,static_cast<float>(Movement->Velocity.Size2D())):R.Vector()*600);
        LaunchCharacter(Horizontal+FVector(0,0,LaunchZ),true,true);if(!MatchRules().bUnlimitedStamina)Stamina-=18;
    }
    else Jump();
    MulticastSound(WildlineAudio::Jump);
}
void AWildlineCharacter::ServerClimb_Implementation(){
    if(bInLobby||!bSquirrel||bDead)return;
    bClimbTransfer=false;
    if(TreeIndex!=INDEX_NONE){TreeIndex=INDEX_NONE;GetCharacterMovement()->SetMovementMode(MOVE_Falling);return;}
    if(!Forest)Forest=AWildlineWorld::Find(GetWorld());if(!Forest)return;
    FHitResult Best;int32 WoodTree=INDEX_NONE;float Distance=70;
    const FVector P=GetActorLocation();
    // A falling squirrel can catch a limb; leaf cards are excluded from traces.
    // Prefer the limb under the paws; at forks the nearest lateral triangle may
    // belong to an adjacent twig or the underside of the same branch.
    FHitResult Foot;int32 FootTree;
    if(Forest->TraceWood(P,P-FVector(0,0,45),Foot,FootTree,12)&&Foot.ImpactNormal.Z>.4f){Best=Foot;WoodTree=FootTree;Distance=-1;}
    for(int32 D=0;D<50&&Distance>=0;D++){
        const float Z=D<16?0.f:D<32?.707f:-.707f;
        const FVector Direction=D==48?FVector::UpVector:D==49?-FVector::UpVector:FVector(FMath::Cos((D%16)*PI/8),FMath::Sin((D%16)*PI/8),Z).GetSafeNormal();
        FHitResult Hit;int32 Index;
        if(Forest->TraceWood(P,P+Direction*70,Hit,Index,12)&&Hit.Distance<Distance){Best=Hit;WoodTree=Index;Distance=Hit.Distance;}
    }
    if(WoodTree!=INDEX_NONE){
        TreeIndex=WoodTree;bWoodClimbing=true;ClimbNormal=Best.ImpactNormal;bBranchClimbing=Forest->IsLimbSurface(TreeIndex,Best.ImpactPoint,ClimbNormal);
        ClimbHeading=FVector::VectorPlaneProject(bBranchClimbing?GetControlRotation().Vector():FVector::UpVector,ClimbNormal).GetSafeNormal();
        SetActorLocation(Best.ImpactPoint+ClimbNormal*16,false);GetCharacterMovement()->StopMovementImmediately();GetCharacterMovement()->SetMovementMode(MOVE_Flying);MulticastSound(WildlineAudio::Climb);return;
    }
    bWoodClimbing=false;
    const int32 I=Forest->NearestTree(GetActorLocation(),30);if(I==INDEX_NONE)return;
    TreeIndex=I;MulticastSound(WildlineAudio::Climb);float Radius;const FVector Centre=Forest->TrunkCentre(I,GetActorLocation().Z,Radius);TreeAngle=FMath::Atan2(GetActorLocation().Y-Centre.Y,GetActorLocation().X-Centre.X);GetCharacterMovement()->StopMovementImmediately();GetCharacterMovement()->SetMovementMode(MOVE_Flying);
}
void AWildlineCharacter::OnRepClimbPosition(){if(TreeIndex!=INDEX_NONE&&IsLocallyControlled()){SetActorLocation(ClimbPosition,false);GetCharacterMovement()->SetMovementMode(MOVE_Flying);GetCharacterMovement()->StopMovementImmediately();}}
void AWildlineCharacter::ServerClimbInput_Implementation(float F,float S){ForwardInput=FMath::Clamp(F,-1.f,1.f);SideInput=FMath::Clamp(S,-1.f,1.f);}
void AWildlineCharacter::UpdateClimb(float Delta){
    if(Delta>.026f){const int32 Steps=FMath::CeilToInt(Delta/.025f);for(int32 I=0;I<Steps;I++)UpdateClimb(Delta/Steps);return;}
    if(!Forest||!Forest->Trees.IsValidIndex(TreeIndex))return;
    if(bWoodClimbing){
        FVector N=ClimbNormal;
        const FVector Look=FRotator(0,GetControlRotation().Yaw,0).Vector();
        float StemRadius;const FVector Stem=Forest->TrunkCentre(TreeIndex,GetActorLocation().Z,StemRadius);
        if(!bClimbTransfer&&FMath::Abs(ForwardInput)>.1f){
            FHitResult Landing;int32 Index;
            if(!bBranchClimbing&&N.Z<.5f&&ForwardInput>0&&FVector::DotProduct(Look,N)>.15f){
                // Find a top surface within a short reach of the fork.
                // Mantle over the junction at normal climb speed, never teleport.
                for(float Reach:{20.f,40.f,65.f,95.f,125.f}){
                    const FVector Probe=GetActorLocation()+Look*Reach;
                    if(Forest->TraceWood(Probe+FVector(0,0,95),Probe-FVector(0,0,25),Landing,Index,8)&&Index==TreeIndex&&Landing.ImpactNormal.Z>.5f&&Forest->IsLimbSurface(Index,Landing.ImpactPoint,Landing.ImpactNormal)&&FVector::Dist(GetActorLocation(),Landing.ImpactPoint)<150){
                        ClimbTransferTarget=Landing.ImpactPoint+Landing.ImpactNormal*16;ClimbTransferNormal=Landing.ImpactNormal;bClimbTransfer=true;bTransferToBranch=true;break;
                    }
                }
            }else if(bBranchClimbing&&ForwardInput<0&&FVector::Dist2D(GetActorLocation(),Stem)<StemRadius+150){
                const FVector Out=(GetActorLocation()-Stem).GetSafeNormal2D();const FVector Centre=Stem-FVector(0,0,55);
                if(Forest->TraceWood(Centre+Out*(StemRadius+120),Centre-Out*20,Landing,Index)&&Index==TreeIndex&&FMath::Abs(Landing.ImpactNormal.Z)<.45f&&FVector::Dist(GetActorLocation(),Landing.ImpactPoint)<180){
                    ClimbTransferTarget=Landing.ImpactPoint+Landing.ImpactNormal*16;ClimbTransferNormal=Landing.ImpactNormal;bClimbTransfer=true;bTransferToBranch=false;
                }
            }
        }
        if(bClimbTransfer){
            if(FMath::Abs(ForwardInput)<.1f)return;
            if((bTransferToBranch?ForwardInput:-ForwardInput)<0){bClimbTransfer=false;return;}
            const FVector Previous=GetActorLocation();const float Speed=200.f*Delta*MatchRules().MovementScale;
            const FVector Target=FMath::VInterpConstantTo(Previous,ClimbTransferTarget,Delta,200.f*MatchRules().MovementScale);
            SetActorLocation(Target,false);ClimbNormal=FMath::VInterpNormalRotationTo(N,ClimbTransferNormal,Delta,180.f);
            ClimbHeading=FVector::VectorPlaneProject(bTransferToBranch?Look:FVector::UpVector,ClimbNormal).GetSafeNormal();
            if(FVector::Dist(Target,ClimbTransferTarget)<=Speed){SetActorLocation(ClimbTransferTarget,false);ClimbNormal=ClimbTransferNormal;bBranchClimbing=bTransferToBranch;bClimbTransfer=false;}
            return;
        }
        // Blend towards the direction the player looks as bark slopes into a
        // fork. A hard trunk/limb switch can reverse W or lose its tangent.
        const float BranchBlend=FMath::Clamp((static_cast<float>(N.Z)-.15f)/.5f,0.f,1.f);
        FVector Forward=FVector::VectorPlaneProject(FMath::Lerp(FVector::UpVector,Look,BranchBlend),N).GetSafeNormal();
        if(Forward.IsNearlyZero())Forward=FVector::VectorPlaneProject(FVector(ClimbHeading),N).GetSafeNormal();
        FVector Right=FVector::CrossProduct(N,Forward).GetSafeNormal();
        const FVector Direction=(Forward*ForwardInput+Right*SideInput).GetClampedToMaxSize(1);
        const float Travel=200.f*FMath::Min(Delta,.05f)*MatchRules().MovementScale;
        const FVector Surface=GetActorLocation()-N*16;
        const FVector Candidate=Surface+Direction*Travel;
        FHitResult Best;int32 Next=INDEX_NONE;
        bool Found=false;float BestScore=MAX_flt;
        auto Probe=[&](FVector Axis,float Sweep){
            FHitResult Hit;int32 Index;
            if(!Forest->TraceWood(Candidate+Axis*40,Candidate-Axis*55,Hit,Index,Sweep)||Index!=TreeIndex||FVector::Dist(Hit.ImpactPoint,Surface)>Travel+18)return;
            if(FVector::Dist(Hit.ImpactPoint+Hit.ImpactNormal*16,GetActorLocation())>Travel+16)return;
            const float Progress=FVector::DotProduct(Hit.ImpactPoint-Surface,Direction);
            if(Direction.SizeSquared()>.1f&&Progress<-.5f)return;
            const float Score=FVector::DistSquared(Hit.ImpactPoint,Candidate)-Progress*8;
            if(Score<BestScore){BestScore=Score;Best=Hit;Next=Index;Found=true;}
        };
        Probe(N,0);
        if(Direction.SizeSquared()>.1f){
            // Look around the corner instead of repeatedly selecting the trunk
            // triangle behind the paws. The short step limit prevents snapping.
            for(const FVector Axis:{(N+Direction).GetSafeNormal(),(N-Direction).GetSafeNormal(),FVector::UpVector,Look.GetSafeNormal()})Probe(Axis,0);
        }
        if(!Found)Probe(N,5);
        if(Found&&FVector::Dist(Best.ImpactPoint,Surface)<Travel+24){
            TreeIndex=Next;ClimbNormal=Best.ImpactNormal;bBranchClimbing=Forest->IsLimbSurface(TreeIndex,Best.ImpactPoint,ClimbNormal);
            if(Direction.SizeSquared()>.1f)ClimbHeading=FVector::VectorPlaneProject(Direction,ClimbNormal).GetSafeNormal();
            SetActorLocation(Best.ImpactPoint+ClimbNormal*16,false);
        }
        return;
    }
    const auto& T=Forest->Trees[TreeIndex];TreeAngle+=SideInput*Delta*1.7f*MatchRules().MovementScale;const float H=FMath::Clamp(GetActorLocation().Z+ForwardInput*200.f*Delta*MatchRules().MovementScale,T.Base.Z+18,T.ClimbTop-12);float Radius;const FVector Centre=Forest->TrunkCentre(TreeIndex,H,Radius);const FVector Target=Centre+FVector(FMath::Cos(TreeAngle)*(Radius+13),FMath::Sin(TreeAngle)*(Radius+13),0);SetActorLocation(Target,false);
    // A grip caught by the trunk profile must also join the real bark once it
    // comes within reach; otherwise that grip can never enter any branch.
    const FVector Out=(Target-Centre).GetSafeNormal2D();FHitResult Bark;int32 Index;
    if(Forest->TraceWood(Target+Out*35,Target-Out*50,Bark,Index)&&Index==TreeIndex&&FVector::Dist(Bark.ImpactPoint+Bark.ImpactNormal*16,Target)<24){
        bWoodClimbing=true;ClimbNormal=Bark.ImpactNormal;bBranchClimbing=Forest->IsLimbSurface(Index,Bark.ImpactPoint,ClimbNormal);
        ClimbHeading=FVector::VectorPlaneProject(FVector::UpVector,ClimbNormal).GetSafeNormal();SetActorLocation(Bark.ImpactPoint+ClimbNormal*16,false);
    }
}
void AWildlineCharacter::ServerThermal_Implementation(){if(bInLobby)return;const float Now=ServerTime();if(!WildlineRules::CanThermal(!bSquirrel&&SelectedWeapon==WildlineRules::RifleWeapon,!bDead,Now,ThermalReadyAt))return;if(!MatchRules().bAllowThermal)return;ThermalUntil=Now+MatchRules().ThermalSeconds;ThermalReadyAt=ThermalUntil+MatchRules().ThermalCooldownSeconds;MulticastSound(WildlineAudio::Thermal);}
void AWildlineCharacter::ServerReload_Implementation(){if(!bSquirrel&&!bDead&&Ammo<WildlineRules::WeaponCapacity(SelectedWeapon)&&ReloadUntil==0){ReloadUntil=ServerTime()+WildlineRules::WeaponReload(SelectedWeapon)*MatchRules().ReloadScale;MulticastSound(WildlineAudio::Reload);}}
void AWildlineCharacter::ServerFire_Implementation(FVector_NetQuantizeNormal Direction){
    using namespace WildlineRules;const float Now=ServerTime();const auto* State=GetWorld()->GetGameState<AWildlineGameState>();
    if(bInLobby||bSquirrel||bDead||Ammo<=0||ReloadUntil>0||Now-LastShot<WeaponInterval(SelectedWeapon)||(State&&State->Winner))return;
    const FVector Aim=FVector(Direction).GetSafeNormal();if(Aim.IsNearlyZero())return;
    // The server owns the muzzle, ammo, ballistic flight, hit detection and death.
    const float Scale=HunterProfile.HeightScale();
    const FVector Eye=GetActorLocation()+FVector(0,0,72*Scale);const FVector Muzzle=Eye+(Aim*65+GetActorRightVector()*2)*Scale;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(RifleMuzzle),false,this);FHitResult Block;
    if(GetWorld()->LineTraceSingleByChannel(Block,Eye,Muzzle,ECC_Visibility,Q))return;
    LastShot=Now;if(!MatchRules().bInfiniteAmmo)Ammo--;
    if(SelectedWeapon==ShotgunWeapon){
        TMap<AWildlineCharacter*,float> Damage;
        TArray<FWildlineImpact> Impacts;
        const FVector Right=FVector::CrossProduct(Aim,FVector::UpVector).GetSafeNormal();const FVector Up=FVector::CrossProduct(Right,Aim).GetSafeNormal();
        const float Spread=FMath::Tan(FMath::DegreesToRadians(ShotgunSpreadDegrees))*(bAiming?.78f:1.f);
        for(int32 I=0;I<ShotgunPellets;I++){
            const float Angle=I*2.39996323f,Radius=FMath::Sqrt(static_cast<float>(I)/ShotgunPellets)*Spread;
            const FVector Pellet=(Aim+Right*(FMath::Cos(Angle)*Radius)+Up*(FMath::Sin(Angle)*Radius)).GetSafeNormal();FHitResult Hit;
            if(GetWorld()->LineTraceSingleByChannel(Hit,Muzzle,Muzzle+Pellet*ShotgunRange,ECC_Visibility,Q)){
                if(auto* Animal=Cast<AWildlineCharacter>(Hit.GetActor());Animal&&Animal->bSquirrel&&!Animal->bDead)Damage.FindOrAdd(Animal)+=PelletDamage(Hit.Distance);
                else if(!Cast<AWildlineCharacter>(Hit.GetActor()))Impacts.Add(AWildlineWeaponFX::DescribeImpact(Hit));
            }
        }
        for(const auto& Hit:Damage){Hit.Key->MulticastBlood(Hit.Key->GetActorLocation(),Aim);UGameplayStatics::ApplyDamage(Hit.Key,Hit.Value,Controller,this,UDamageType::StaticClass());}
        MulticastShot(Aim);if(!Impacts.IsEmpty())MulticastImpacts(Impacts);return;
    }
    if(bBot&&MatchRules().BotAccuracyScale>=1.f){
        // Maximum AI accuracy is a true pinpoint shot. Trace the physical world
        // so cover and intervening pawns still block it; no target-forced damage.
        FHitResult Hit;
        if(GetWorld()->LineTraceSingleByChannel(Hit,Muzzle,Muzzle+Aim*180000.f,ECC_Visibility,Q)){
            if(auto* Animal=Cast<AWildlineCharacter>(Hit.GetActor());Animal&&Animal->bSquirrel&&!Animal->bDead){Animal->MulticastBlood(Hit.ImpactPoint,Aim);UGameplayStatics::ApplyDamage(Animal,100,Controller,this,UDamageType::StaticClass());}
            else if(!Cast<AWildlineCharacter>(Hit.GetActor()))MulticastImpacts({AWildlineWeaponFX::DescribeImpact(Hit)});
        }
        MulticastShot(Aim);return;
    }
    auto* Bullet=GetWorld()->SpawnActor<AWildlineBullet>(AWildlineBullet::StaticClass(),Muzzle,Aim.Rotation());if(Bullet){Bullet->Shooter=this;Bullet->Velocity=Aim*90000;Bullet->SetOwner(this);}
    MulticastShot(Aim);
}
void AWildlineCharacter::MulticastShot_Implementation(FVector_NetQuantizeNormal Direction){
    BoltCycleAt=ServerTime();
    VisualShotAt=BoltCycleAt;
    // A sharp generated gun report, served by Unreal's native audio mixer.
    if(GetNetMode()==NM_DedicatedServer)return;
    CosmeticShots++;
    // Use the visible weapon muzzle, including camera/reload/recoil transforms.
    const auto* Weapon=IsLocallyControlled()&&!bThirdPerson?ViewRifle:WorldRifle;
    const FVector Muzzle=Weapon->GetComponentTransform().TransformPosition(FVector(SelectedWeapon==1?57.4f:70.f,0,2));
    if(auto* FX=GetWorld()->SpawnActor<AWildlineWeaponFX>(Muzzle,FRotator::ZeroRotator))FX->Muzzle(Direction,SelectedWeapon==1);
    if(SelectedWeapon==1){FTimerHandle Eject;GetWorldTimerManager().SetTimer(Eject,FTimerDelegate::CreateWeakLambda(this,[this](){
        const auto* Gun=IsLocallyControlled()&&!bThirdPerson?ViewRifle:WorldRifle;
        const FTransform Pose=Gun->GetComponentTransform();
        if(auto* FX=GetWorld()->SpawnActor<AWildlineWeaponFX>(Pose.TransformPosition(FVector(-2,2,2)),FRotator::ZeroRotator))FX->Shell(Pose.GetRotation().GetRightVector());
    }),.34f,false);}
    WildlineAudio::Play(this,SelectedWeapon==1?WildlineAudio::Shotgun:WildlineAudio::Rifle,GetActorLocation());
    FTimerHandle Mechanism;GetWorldTimerManager().SetTimer(Mechanism,FTimerDelegate::CreateWeakLambda(this,[this](){WildlineAudio::Play(this,SelectedWeapon==1?WildlineAudio::Pump:WildlineAudio::Bolt,GetActorLocation());}),SelectedWeapon==1?.26f:.15f,false);
    if(IsLocallyControlled()&&Controller)AddControllerPitchInput(SelectedWeapon==1?.8f:.45f);
}
void AWildlineCharacter::MulticastImpacts_Implementation(const TArray<FWildlineImpact>& Impacts){
    if(GetNetMode()==NM_DedicatedServer)return;
    TArray<FVector> Clouds;
    for(int32 I=0;I<FMath::Min(Impacts.Num(),WildlineRules::ShotgunPellets);I++){
        const auto& Hit=Impacts[I];CosmeticImpacts++;
        const FVector P=Hit.Position,N=FVector(Hit.Normal).GetSafeNormal();
        const bool Debris=!Clouds.ContainsByPredicate([&](const FVector& Other){return FVector::DistSquared(Other,P)<FMath::Square(24.f);});
        if(Debris){Clouds.Add(P);if(auto* FX=GetWorld()->SpawnActor<AWildlineWeaponFX>(P+N,FRotator::ZeroRotator))FX->Impact(N,Hit.Surface,true);}
        if(auto* Mark=GetWorld()->SpawnActor<AWildlineImpactMark>(P+N*.4f,(-N).Rotation()))Mark->SetActorScale3D(FVector(SelectedWeapon==1?.7f:1.f));
    }
}
float AWildlineCharacter::TakeDamage(float Damage,const FDamageEvent& Event,AController* DamageInstigator,AActor* Causer){
    if(bInLobby||!HasAuthority()||!bSquirrel||bDead||ServerTime()<ProtectedUntil)return 0;
    Health=FMath::Max(0,Health-Damage);MulticastSound(Health<=0?WildlineAudio::Death:WildlineAudio::Hit);
    if(Health<=0){ReturnCarriedPeanuts();bDead=true;RespawnAt=ServerTime()+MatchRules().RespawnSeconds;Bag=0;bSprinting=false;TreeIndex=INDEX_NONE;OnRepDead();if(auto* GM=GetWorld()->GetAuthGameMode<AWildlineGameMode>())GM->RecordKill(this,DamageInstigator);}
    return Damage;
}
void AWildlineCharacter::MulticastBlood_Implementation(FVector_NetQuantize Impact,FVector_NetQuantizeNormal Dir){
    if(GetNetMode()==NM_DedicatedServer)return;const auto* S=UWildlineUserSettings::Get();if(S&&!S->bBloodEnabled)return;
    auto* Effect=GetWorld()->SpawnActor<AWildlineBlood>(AWildlineBlood::StaticClass(),Impact,FRotator::ZeroRotator);if(Effect)Effect->Direction=Dir;
}
void AWildlineCharacter::Forage(){
    if(bInLobby||!Forest||!bSquirrel||bDead)return;
    if(Forest->bResidential){UpdateCourse();return;}
    const auto* State=GetWorld()->GetGameState<AWildlineGameState>();if(!State||!State->bRoundStarted||State->Winner)return;
    auto* Shared=GetWorld()->GetGameState<AWildlineGameState>();
    if(TreeIndex==INDEX_NONE)for(int32 I=0;I<Forest->AcornLocations.Num();I++){
        const auto& Food=Forest->AcornLocations[I];if(Bag>=MatchRules().CarryCapacity||(Forest->bFarm?Shared->TakenPeanuts.Contains(I):Collected.Contains(I))||!WildlineRules::CanPickAcorn(GetActorLocation(),Food))continue;
        if(Forest->bFarm){
            if(!Forest->InsideFarmhouse(GetActorLocation())||FVector::Dist2D(GetActorLocation(),Food)>95)continue;
            FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(PantryPickup),false,this);
            if(GetWorld()->LineTraceSingleByChannel(Hit,GetActorLocation()+FVector(0,0,5),Food+FVector(0,0,5),ECC_Visibility,Query))continue;
            Shared->TakenPeanuts.Add(I);BagPeanuts.Add(I);Shared->ForceNetUpdate();
        }
        Collected.Add(I);Bag++;MulticastSound(WildlineAudio::Pickup);
    }
    if(Bag>0&&TreeIndex==INDEX_NONE&&FMath::Abs(GetActorLocation().Z-Forest->DenLocation.Z)<100&&FVector::Dist2D(GetActorLocation(),Forest->DenLocation)<280){if(auto* GM=GetWorld()->GetAuthGameMode<AWildlineGameMode>())GM->Deposit(Bag);MulticastSound(WildlineAudio::Deposit);Bag=0;BagPeanuts.Empty();}
}
void AWildlineCharacter::UpdateHeatSignature(){
    // Tag geometry on every client. The viewing camera decides whether to display heat.
    // Keeping the original body/fur materials preserves their silhouettes and masks.
    const bool Hot=bSquirrel&&!bDead&&!bInLobby;
    GetMesh()->SetCustomDepthStencilValue(1);GetMesh()->SetRenderCustomDepth(Hot);
    for(auto* Fur:FurSections){Fur->SetCustomDepthStencilValue(1);Fur->SetRenderCustomDepth(Hot);}
}
void AWildlineCharacter::UpdateCamera(float Delta){
    if(Forest){const auto* State=GetWorld()->GetGameState<AWildlineGameState>();Forest->ShowLocalAcorns(Forest->bFarm&&State?State->TakenPeanuts:Collected);}
    const bool Third=bDead||(bThirdPerson&&!IsScoped());FirstCamera->SetActive(!Third);ThirdCamera->SetActive(Third);GetMesh()->SetOwnerNoSee(!Third);WorldRifle->SetOwnerNoSee(!Third);ViewRifle->SetVisibility(!bSquirrel&&!Third&&!IsScoped());
    ViewBolt->SetVisibility(!bSquirrel&&!Third&&!IsScoped());WorldBolt->SetOwnerNoSee(!Third);
    for(auto* Fur:FurSections)Fur->SetOwnerNoSee(!Third);
    // On a trunk, keep the chase camera on the animal's side of the bark.
    // Free yaw used to orbit it through the trunk and hide the tiny silhouette.
    if(bSquirrel&&Third&&Forest&&Forest->Trees.IsValidIndex(TreeIndex)){
        if(ThirdCamera->GetAttachParent())ThirdCamera->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        float Radius;const FVector Centre=Forest->TrunkCentre(TreeIndex,GetActorLocation().Z,Radius);
        const FVector Out=bWoodClimbing?FVector(ClimbNormal):(GetActorLocation()-Centre).GetSafeNormal2D();
        const FVector Side=FVector::CrossProduct(bWoodClimbing?FVector(ClimbHeading):FVector::UpVector,Out).GetSafeNormal();
        const FVector Focus=GetActorLocation()+(bWoodClimbing?Out*8:FVector(0,0,8));
        const float Orbit=FMath::Clamp(FMath::FindDeltaAngleDegrees(Out.Rotation().Yaw,GetControlRotation().Yaw),-55.f,55.f);
        FVector Desired=Focus;float Clearance=0;
        FCollisionQueryParams Q(SCENE_QUERY_STAT(ClimbCamera),false,this);
        for(float Offset:{0.f,55.f,-55.f,100.f,-100.f,180.f,-180.f}){
          for(float Lift:{32.f,-65.f,100.f}){
            FVector Candidate=Focus+(Out*125+Side*(Orbit*.7f+Offset)+FVector(0,0,Lift)).GetClampedToMaxSize(170);FHitResult Hit;
            if(GetWorld()->SweepSingleByChannel(Hit,Focus,Candidate,FQuat::Identity,ECC_Camera,FCollisionShape::MakeSphere(5.f),Q))Candidate=Hit.Location;
            const float Distance=FVector::DistSquared(Focus,Candidate);
            if(Distance>Clearance){Clearance=Distance;Desired=Candidate;}
            if(Offset==0&&Lift==32&&Distance>FMath::Square(90.f))break;
          }
          if(Offset==0&&Clearance>FMath::Square(90.f))break;
        }
        ThirdCamera->SetWorldLocation(Desired);
        ThirdCamera->SetWorldRotation((Focus-Desired).Rotation());
    }else{
        if(!ThirdCamera->GetAttachParent())ThirdCamera->AttachToComponent(Boom,FAttachmentTransformRules::SnapToTargetNotIncludingScale,USpringArmComponent::SocketName);
        ThirdCamera->SetRelativeLocation(FVector::ZeroVector);
        ThirdCamera->SetRelativeRotation(FRotator::ZeroRotator);
    }
    FirstCamera->SetFieldOfView(FMath::FInterpTo(FirstCamera->FieldOfView,IsScoped()?24.f:SelectedWeapon==1&&bAiming?55.f:78.f,Delta,12));
    FirstCamera->PostProcessBlendWeight=1;
    ThirdCamera->PostProcessBlendWeight=1;
    if(ThermalMaterial){FirstCamera->AddOrUpdateBlendable(ThermalMaterial,IsThermalActive()?1.f:0.f);ThirdCamera->AddOrUpdateBlendable(ThermalMaterial,IsThermalActive()?1.f:0.f);}
}
void AWildlineCharacter::Tick(float Delta){
    if(bPreviewHunter)return;
    UpdateSquirrelHitbox();
    UpdateHeatSignature();
    if(!bSquirrel&&IsLocallyControlled()&&!bProfileSubmitted)SubmitSavedHunterProfile();
    if(!bSquirrel){
        const float T=ServerTime()-BoltCycleAt;
        const float Lift=T<.44f?FMath::Clamp((T-.12f)/.13f,0.f,1.f):1-FMath::Clamp((T-.64f)/.13f,0.f,1.f);
        const float Back=T<.44f?FMath::Clamp((T-.25f)/.19f,0.f,1.f):1-FMath::Clamp((T-.44f)/.20f,0.f,1.f);
        const FTransform BoltPose(FRotator(0,0,Lift*-60.f),FVector(-8.f*Back,0,0));ViewBolt->SetRelativeTransform(BoltPose);WorldBolt->SetRelativeTransform(BoltPose);
        if(SelectedWeapon==1){
            const float Remaining=FMath::Max(0.f,ReloadUntil-ServerTime());
            const float ReloadPump=ReloadUntil>0?FMath::Clamp(FMath::Min((2.8f-Remaining)*4,Remaining*4),0.f,1.f):0.f;
            const FTransform Action(FRotator::ZeroRotator,FVector(-10.f*FMath::Max(Back,ReloadPump),0,0));ViewBolt->SetRelativeTransform(Action);WorldBolt->SetRelativeTransform(Action);
        }
        // The animation publishes the shouldered world weapon after solving
        // both wrists; moving it here would invalidate that frame's contacts.
        const float Kick=T>=0&&T<.4f?FMath::Exp(-T*17):0;
        const float Bob=GetCharacterMovement()->IsMovingOnGround()?FMath::Clamp(GetVelocity().Size2D()/400.f,0.f,1.f)*FMath::Sin(ServerTime()*11.f)*.35f:0;
        if(SelectedWeapon==1){
            const FVector Carry=bAiming?FVector(58,2,-5):FVector(52,11,-13);
            ViewRifle->SetRelativeLocation(FMath::VInterpTo(ViewRifle->GetRelativeLocation(),Carry+FVector(-3*Kick,0,Bob),Delta,16));
        }else ViewRifle->SetRelativeLocation(FVector(30-3*Kick,14,-19+Bob));
        ViewRifle->SetRelativeRotation(FRotator(2*Kick,0,0));
    }
    Super::Tick(Delta);
    if(HasAuthority()&&bSquirrel&&!bDead&&TreeIndex==INDEX_NONE&&GetCharacterMovement()->IsMovingOnGround()&&GetCharacterMovement()->PendingLaunchVelocity.IsNearlyZero()){
        LastGroundedAt=ServerTime();bGroundJumpConsumed=false;
    }
    if(!bPreviewHunter&&!bDead&&GetNetMode()!=NM_DedicatedServer&&bSquirrel){const float Now=ServerTime();if(Now>=NextStepSound&&GetVelocity().Size2D()>25&&(GetCharacterMovement()->IsMovingOnGround()||TreeIndex!=INDEX_NONE)){const float Rate=bSprinting?WildlineAudio::RunningFootstepRate:1.f;NextStepSound=Now+.38f/Rate;WildlineAudio::Play(this,TreeIndex!=INDEX_NONE?WildlineAudio::Climb:WildlineAudio::Pawstep,GetActorLocation(),false,TreeIndex!=INDEX_NONE?1.f:Rate);}}
    if(IsLocallyControlled()){const float Now=ServerTime();if(Now>NextBirdSound){NextBirdSound=Now+FMath::FRandRange(6.f,12.f);WildlineAudio::Play(this,WildlineAudio::Bird,GetActorLocation()+FVector(FMath::FRandRange(-1200.f,1200.f),FMath::FRandRange(-1200.f,1200.f),500));}const auto* State=GetWorld()->GetGameState<AWildlineGameState>();if(State&&State->Winner!=HeardWinner){HeardWinner=State->Winner;if(HeardWinner)WildlineAudio::Play(this,WildlineAudio::Win,GetActorLocation(),true);}}
    if(!Forest)Forest=AWildlineWorld::Find(GetWorld());
    if(HasAuthority()&&Forest&&bSquirrel&&!bDead&&!bInLobby&&TreeIndex!=INDEX_NONE&&bWoodClimbing&&bBranchClimbing&&ServerTime()>=NextBranchLeavesAt){
        NextBranchLeavesAt=ServerTime()+1.4f;
        const FTransform Transform(FRotator::ZeroRotator,GetActorLocation());
        if(auto* Leaves=GetWorld()->SpawnActorDeferred<AWildlineBranchLeaves>(AWildlineBranchLeaves::StaticClass(),Transform)){
            Leaves->bSnow=Forest->bWinter;Leaves->FinishSpawning(Transform);
        }
    }
    if(IsLocallyControlled()&&bSquirrel&&FParse::Param(FCommandLine::Get(),TEXT("WildlineSquirrelReview"))){const float T=GetWorld()->GetTimeSeconds();if(T>7&&T<11){bSprinting=T>9;AddMovementInput(FVector::ForwardVector,1);}}
    if(IsLocallyControlled()&&!bSquirrel&&(FParse::Param(FCommandLine::Get(),TEXT("WildlineHunterLocomotionReview"))||FParse::Param(FCommandLine::Get(),TEXT("WildlineHunterAnimationProbe")))){
        const float T=GetWorld()->GetTimeSeconds();bSprinting=T>=8&&T<10;
        if(T>=16&&T<21)Crouch();else UnCrouch();
        if((T>=4&&T<7)||(T>=8&&T<10)||(T>=17&&T<20)||(T>=21.4f&&T<23.5f))AddMovementInput(GetActorForwardVector(),1);
        if(T>=11&&T<13)AddMovementInput(GetActorRightVector(),1);
    }
    GetCharacterMovement()->MaxWalkSpeed=bSquirrel?(bSprinting&&Stamina>4?650:320):(bSprinting&&Stamina>4?580:320);
    if(bBot&&!bSquirrel)GetCharacterMovement()->MaxWalkSpeed=MatchRules().PursuitSpeed(WildlineRules::HunterDifficulty(AIDifficulty).WalkSpeed);
    GetCharacterMovement()->MaxWalkSpeed*=MatchRules().MovementScale;GetCharacterMovement()->MaxWalkSpeedCrouched=160*MatchRules().MovementScale;
    if(HasAuthority()&&!bDead){const auto* S=GetWorld()->GetGameState<AWildlineGameState>();if(!S||S->Winner==0){if(bBot&&S&&S->bRoundStarted)BotThink(Delta);if(TreeIndex!=INDEX_NONE){UpdateClimb(Delta);ClimbPosition=GetActorLocation();}Stamina=MatchRules().bUnlimitedStamina?100.f:FMath::Clamp(Stamina+(bSprinting&&GetVelocity().Size2D()>10?-12:9)*Delta,0.f,100.f);if(Stamina<=4)bSprinting=false;if(ReloadUntil>0&&ServerTime()>=ReloadUntil){Ammo=WildlineRules::WeaponCapacity(SelectedWeapon);ReloadUntil=0;}Forage();const FVector P=GetActorLocation();if(P.SizeSquared2D()>FMath::Square(WildlineRules::MapRadius)){FVector Q=P.GetSafeNormal2D()*WildlineRules::MapRadius;Q.Z=P.Z;SetActorLocation(Q);const FVector Out=P.GetSafeNormal2D();auto* M=GetCharacterMovement();M->Velocity-=Out*FMath::Max(0.f,static_cast<float>(FVector::DotProduct(M->Velocity,Out)));}}}
    GetCharacterMovement()->bIgnoreClientMovementErrorChecksAndCorrection=TreeIndex!=INDEX_NONE;
    if(IsLocallyControlled()){if(TreeIndex!=INDEX_NONE&&bWoodClimbing&&!HasAuthority())UpdateClimb(Delta);UpdateCamera(Delta);if(TreeIndex!=INDEX_NONE&&!HasAuthority())ServerClimbInput(ForwardInput,SideInput);if(bFiring&&CanControl()&&ServerTime()-LastFireRequest>WildlineRules::WeaponInterval(SelectedWeapon)){LastFireRequest=ServerTime();FVector P;FRotator R;Controller->GetPlayerViewPoint(P,R);ServerFire(R.Vector());}}
    if(bSquirrel&&!bDead){
        FRotator Pose=FRotator::ZeroRotator;
        const bool Grounded=GetCharacterMovement()->IsMovingOnGround();
        if(Grounded){const FVector Normal=GetCharacterMovement()->CurrentFloor.HitResult.ImpactNormal;const FRotator GroundPose=FRotationMatrix::MakeFromXZ(GetActorForwardVector(),Normal).Rotator();Pose=FRotator(GroundPose.Pitch,0,GroundPose.Roll);}
        if(TreeIndex!=INDEX_NONE&&Forest&&Forest->Trees.IsValidIndex(TreeIndex)){
            float Radius;const FVector Toward=Forest->TrunkCentre(TreeIndex,GetActorLocation().Z,Radius)-GetActorLocation();
            Pose=bWoodClimbing?(GetActorQuat().Inverse()*FRotationMatrix::MakeFromXZ(ClimbHeading,ClimbNormal).ToQuat()).Rotator():FRotator(90,FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw,Toward.Rotation().Yaw),0);
        }
        AnimalRoot->SetRelativeRotation(FMath::RInterpTo(AnimalRoot->GetRelativeRotation(),Pose,Delta,14));
        // Keep the grip tangent to the bark rather than rotating paws inside it.
        GetMesh()->SetRelativeLocation(FVector(0,0,TreeIndex!=INDEX_NONE?(bWoodClimbing?-18.15f:-8.f):Grounded?-14.15f:-12.f));
        if(IsLocallyControlled()){
            const FVector Eye=TreeIndex!=INDEX_NONE?GetMesh()->GetSocketLocation(TEXT("FACIAL_Nose"))+GetControlRotation().Vector()*3:GetActorTransform().TransformPosition(FVector(13,0,Grounded?-1.15f:1.f));
            FirstCamera->SetWorldLocation(FMath::VInterpTo(FirstCamera->GetComponentLocation(),Eye,Delta,18));
        }
    }
}

AWildlineBullet::AWildlineBullet(){PrimaryActorTick.bCanEverTick=true;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Ballistic root"));SetLifeSpan(2);}
void AWildlineBullet::Tick(float Delta){Super::Tick(Delta);if(!HasAuthority()||!Shooter.IsValid()){Destroy();return;}const FVector Start=GetActorLocation();Velocity.Z-=980.f*Delta;const FVector End=Start+Velocity*Delta;FCollisionQueryParams Q(SCENE_QUERY_STAT(BallisticBullet),false,Shooter.Get());FHitResult Hit;if(GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Q)){if(auto* Animal=Cast<AWildlineCharacter>(Hit.GetActor());Animal&&Animal->bSquirrel&&!Animal->bDead){Animal->MulticastBlood(Hit.ImpactPoint,Velocity.GetSafeNormal());UGameplayStatics::ApplyDamage(Animal,100,Shooter->GetController(),this,UDamageType::StaticClass());}else if(!Cast<AWildlineCharacter>(Hit.GetActor()))Shooter->MulticastImpacts({AWildlineWeaponFX::DescribeImpact(Hit)});Destroy();}else SetActorLocation(End);}
AWildlineBlood::AWildlineBlood(){PrimaryActorTick.bCanEverTick=true;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Blood"));SetLifeSpan(18);}
void AWildlineBlood::BeginPlay(){Super::BeginPlay();auto* Sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));auto* Red=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Wildline/Materials/M_Blood.M_Blood"));for(int32 I=0;I<18;I++){auto* Drop=NewObject<UStaticMeshComponent>(this);Drop->SetupAttachment(RootComponent);Drop->SetStaticMesh(Sphere);Drop->SetMaterial(0,Red);Drop->SetCollisionEnabled(ECollisionEnabled::NoCollision);Drop->SetRelativeScale3D(FVector(.025f,.025f,.045f));Drop->RegisterComponent();Drops.Add(Drop);Velocities.Add(Direction*FMath::FRandRange(100.f,280.f)+FMath::VRand()*180+FVector(0,0,110));}}
void AWildlineBlood::Tick(float Delta){Super::Tick(Delta);for(int32 I=0;I<Drops.Num();I++){if(Velocities[I].IsNearlyZero())continue;Velocities[I].Z-=650*Delta;auto* Drop=Drops[I];FVector P=Drop->GetComponentLocation()+Velocities[I]*Delta;const auto* Forest=AWildlineWorld::Find(GetWorld());const float G=Forest?Forest->GroundHeight(P.X,P.Y):WildlineRules::Ground(P.X,P.Y);if(P.Z<G+1){P.Z=G+1;Velocities[I]=FVector::ZeroVector;Drop->SetWorldScale3D(FVector(.11f,.08f,.006f));}Drop->SetWorldLocation(P);}}


void AWildlineCharacter::MulticastSound_Implementation(uint8 Event){CosmeticSounds++;if(Event<=WildlineAudio::Win)WildlineAudio::Play(this,static_cast<WildlineAudio::Event>(Event),GetActorLocation(),IsLocallyControlled()&&(Event==WildlineAudio::Pickup||Event==WildlineAudio::Deposit||Event==WildlineAudio::Checkpoint||Event==WildlineAudio::Feeder||Event==WildlineAudio::Thermal));}
void AWildlineCharacter::Landed(const FHitResult& Hit){Super::Landed(Hit);if(HasAuthority()){LastGroundedAt=ServerTime();bGroundJumpConsumed=false;MulticastSound(WildlineAudio::Land);}}

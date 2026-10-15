#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WildlineHunterProfile.h"
#include "WildlineMatchConfig.h"
#include "WildlineWeaponFX.h"
#include "WildlineCharacter.generated.h"
class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class AWildlineWorld;
UCLASS()
class WILDLINE_API AWildlineCharacter : public ACharacter {
    GENERATED_BODY()
public:
    AWildlineCharacter();
    void BeginPlay() override;
    void Tick(float Delta) override;
    void SetupPlayerInputComponent(UInputComponent* Input) override;
    void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    float TakeDamage(float Damage,const FDamageEvent& Event,AController* Instigator,AActor* Causer) override;
    UPROPERTY(ReplicatedUsing=OnRepRole) bool bSquirrel=false;
    UPROPERTY(ReplicatedUsing=OnRepDead) bool bDead=false;
    UPROPERTY(Replicated) bool bBot=false;
    UPROPERTY(Replicated) bool bInLobby=false;
    UPROPERTY(Replicated) bool bLobbyReady=false;
    UPROPERTY(Replicated) int32 BotSlot=-1;
    UPROPERTY(Replicated) int32 AIDifficulty=1;
    UPROPERTY(Replicated) float ProtectedUntil=0;
    UPROPERTY(Replicated) bool bSprinting=false;
    UPROPERTY(Replicated) float Stamina=100.f;
    UPROPERTY(Replicated) float Health=100.f;
    UPROPERTY(Replicated) int32 Ammo=5;
    UPROPERTY(Replicated) int32 Bag=0;
    UPROPERTY(Replicated) float ThermalUntil=0;
    UPROPERTY(Replicated) float ThermalReadyAt=0;
    UPROPERTY(Replicated) float ReloadUntil=0;
    UPROPERTY(Replicated) float RespawnAt=0;
    UPROPERTY(Replicated) int32 TreeIndex=INDEX_NONE;
    UPROPERTY(Replicated) bool bWoodClimbing=false;
    UPROPERTY(Replicated) bool bBranchClimbing=false;
    UPROPERTY(Replicated) FVector_NetQuantizeNormal ClimbNormal=FVector::UpVector;
    UPROPERTY(Replicated) FVector_NetQuantizeNormal ClimbHeading=FVector::UpVector;
    UPROPERTY(ReplicatedUsing=OnRepClimbPosition) FVector_NetQuantize10 ClimbPosition=FVector::ZeroVector;
    UFUNCTION() void OnRepClimbPosition();
    UPROPERTY(Replicated) TArray<int32> Collected;
    UPROPERTY(Replicated) TArray<int32> BagPeanuts;
    UPROPERTY(ReplicatedUsing=OnRepWeapon) int32 SelectedWeapon=0;
    UPROPERTY(Replicated) int32 CourseCheckpoint=0;
    UPROPERTY(Replicated) int32 CourseLane=-1;
    UPROPERTY(Replicated) int32 FeederVisits=0;
    int32 RifleAmmo=5,ShotgunAmmo=2;
    UFUNCTION() void OnRepWeapon();
    void SelectWeapon(int32 Weapon);
    void ReturnCarriedPeanuts();
    UPROPERTY() UCameraComponent* FirstCamera;
    UPROPERTY() UCameraComponent* ThirdCamera;
    UPROPERTY() USpringArmComponent* Boom;
    UPROPERTY() UStaticMeshComponent* ViewRifle;
    UPROPERTY() UStaticMeshComponent* WorldRifle;
    UPROPERTY() UStaticMeshComponent* ViewBolt;
    UPROPERTY() UStaticMeshComponent* WorldBolt;
    UPROPERTY(ReplicatedUsing=OnRepHunterProfile) FWildlineHunterProfile HunterProfile;
    UPROPERTY(ReplicatedUsing=OnRepHunterProfile) FString HunterFaceData;
    UPROPERTY() UTexture2D* HunterFaceTexture;
    UPROPERTY() TArray<UMaterialInstanceDynamic*> HunterMaterials;
    bool bProfileSubmitted=false;
    bool bPreviewHunter=false;
    void ApplyHunterProfile();
    void SubmitSavedHunterProfile();
    UFUNCTION() void OnRepHunterProfile();
    UFUNCTION(Server,Reliable) void ServerSetHunterProfile(FWildlineHunterProfile Profile,const FString& FaceData);
    UPROPERTY() USceneComponent* AnimalRoot;
    UPROPERTY() USkeletalMesh* HumanMeshAsset;
    UPROPERTY() USkeletalMesh* SquirrelMeshAsset;
    UPROPERTY() TArray<UProceduralMeshComponent*> FurSections;
    bool bThirdPerson=false;
    UPROPERTY(Replicated) bool bAiming=false;
    bool bFiring=false;
    bool CanControl() const;
    bool IsThermalActive() const;
    bool IsScoped() const;
    float ServerTime() const;
    const FWildlineMatchConfig& MatchRules() const;
    void ResetBotAwareness();
    void ConfigureRole();
    void ToggleCamera();
    void StartSprint();
    void StopSprint();
    void StartAim();
    void StopAim();
    void StartFire();
    void StopFire();
    void DoJump();
    void Climb();
    void Thermal();
    void Reload();
    void DoCrouch();
    void DoUnCrouch();
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void UpdateHeatSignature();
    UPROPERTY() UMaterialInterface* ThermalMaterial;
    UFUNCTION() void OnRepRole();
    UFUNCTION() void OnRepDead();
    UFUNCTION(Server,Reliable) void ServerSprint(bool bValue);
    UFUNCTION(Server,Reliable) void ServerAim(bool bValue);
    UFUNCTION(Server,Reliable) void ServerJump();
    UFUNCTION(Server,Reliable) void ServerClimb();
    UFUNCTION(Server,Unreliable) void ServerClimbInput(float Forward,float Side);
    UFUNCTION(Server,Reliable) void ServerThermal();
    UFUNCTION(Server,Reliable) void ServerReload();
    UFUNCTION(Server,Reliable) void ServerFire(FVector_NetQuantizeNormal Direction);
    UFUNCTION(NetMulticast,Unreliable) void MulticastShot(FVector_NetQuantizeNormal Direction);
    UFUNCTION(NetMulticast,Unreliable) void MulticastSound(uint8 Event);
    void Landed(const FHitResult& Hit) override;
    float NextStepSound=0,NextBirdSound=4;
    int32 HeardWinner=0;
    UFUNCTION(NetMulticast,Unreliable) void MulticastImpacts(const TArray<FWildlineImpact>& Impacts);
    int32 CosmeticShots=0,CosmeticImpacts=0,CosmeticSounds=0;
    float VisualShotAt=-10.f;
    UFUNCTION(NetMulticast,Reliable) void MulticastBlood(FVector_NetQuantize Impact,FVector_NetQuantizeNormal Direction);
private:
    friend class AWildlineController;
    float NextBranchLeavesAt=0;
    FVector ClimbTransferTarget=FVector::ZeroVector;
    FVector ClimbTransferNormal=FVector::UpVector;
    bool bClimbTransfer=false;
    bool bTransferToBranch=false;
    float LastGroundedAt=-100.f;
    bool bGroundJumpConsumed=false;
    float LastShot=-10.f;
    float BoltCycleAt=-10.f;
    float LastFireRequest=-10.f;
    float ForwardInput=0,SideInput=0;
    float TreeAngle=0;
    float BotDecisionAt=0;
    float BotShotAt=0;
    TWeakObjectPtr<AWildlineCharacter> BotTarget;
    FVector BotLastKnown=FVector::ZeroVector;
    float BotLastSeenAt=-100.f;
    int32 BotDifficultyLevel=-1;
    int32 BotCourseStep=-1;
    int32 BotFoodIndex=INDEX_NONE,BotEscapeTree=INDEX_NONE,BotPatrolStep=0;
    int32 BotTrackedTree=INDEX_NONE,BotRecoveries=0;
    FVector BotGoal=FVector::ZeroVector,BotMove=FVector::ZeroVector,BotLookAt=FVector::ZeroVector;
    FVector BotProgressPosition=FVector::ZeroVector,BotEscapeHeading=FVector::ZeroVector;
    float BotPlanAt=0,BotGoalUntil=0,BotProgressAt=0,BotEscapeUntil=0,BotTreeAt=0,BotTreeCooldown=0;
    float BotSteerSide=1;
    FVector BotProgressGoal=FVector::ZeroVector;
    float BotClosestGoalDistance=MAX_flt,BotGoalProgressAt=0;
    TMap<int32,float> BotBlockedFood;
    AWildlineWorld* Forest=nullptr;
    void UpdateCamera(float Delta);
    void BotThink(float Delta);
    void PlanBot(float Now);
    void MoveBot(FVector Target,float Now,bool bSeparate=true);
    FVector BotSteering(FVector Target,bool bSeparate) const;
    bool BotCanSee(const AWildlineCharacter* Other,float Range) const;
    void LeaveBotTree(float Now,FVector Heading);
    void Forage();
    void UpdateCourse();
    void CourseBotThink(float Delta);
    void UpdateClimb(float Delta);
    void BuildFur();

};
UCLASS()
class WILDLINE_API AWildlineBullet : public AActor {
    GENERATED_BODY()
public:
    AWildlineBullet();
    void Tick(float Delta) override;
    FVector Velocity=FVector::ZeroVector;
    TWeakObjectPtr<AWildlineCharacter> Shooter;
};
UCLASS()
class WILDLINE_API AWildlineBlood : public AActor {
    GENERATED_BODY()
public:
    AWildlineBlood();
    void BeginPlay() override;
    void Tick(float Delta) override;
    FVector Direction=FVector::ForwardVector;
private:
    UPROPERTY() TArray<UStaticMeshComponent*> Drops;
    TArray<FVector> Velocities;
};

// Replicated cosmetic disturbance, shared by nearby hunters and squirrels.
UCLASS()
class WILDLINE_API AWildlineBranchLeaves : public AActor {
    GENERATED_BODY()
public:
    AWildlineBranchLeaves();
    void BeginPlay() override;
    void Tick(float Delta) override;
    UPROPERTY(Replicated) bool bSnow=false;
    void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
private:
    UPROPERTY() TArray<UStaticMeshComponent*> Leaves;
    float Age=0;
};

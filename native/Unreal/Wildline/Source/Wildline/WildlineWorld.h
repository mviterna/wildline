#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WildlineWorld.generated.h"
class UHierarchicalInstancedStaticMeshComponent;
class UProceduralMeshComponent;
class UStaticMeshComponent;
class UWildlineNeighborhoodConfig;
class UTextRenderComponent;
USTRUCT()
struct FWildlineTree {
    GENERATED_BODY()
    UPROPERTY() FVector Base=FVector::ZeroVector;
    UPROPERTY() float Radius=50.f;
    UPROPERTY() float Height=2300.f;
    UPROPERTY() int32 Species=0;
    UPROPERTY() FTransform MeshTransform;
    UPROPERTY() float ClimbTop=1500.f;
    UPROPERTY() bool bStraightTrunk=false;
};
UCLASS()
class WILDLINE_API AWildlineWorld : public AActor {
    GENERATED_BODY()
public:
    AWildlineWorld();
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Expedition") bool bFarm=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Expedition") bool bWinter=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Expedition") bool bLake=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Expedition") bool bKnobs=false;
    UPROPERTY() TArray<UTextRenderComponent*> KnobsLabels;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Expedition") bool bResidential=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Neighborhood") UWildlineNeighborhoodConfig* NeighborhoodConfig=nullptr;
    UPROPERTY() FVector CourseStart=FVector(-950,0,210);
    UPROPERTY() FVector FeederLocation=FVector(350,0,450);
    UPROPERTY() TArray<FVector> CoursePlatforms;
    UPROPERTY() TArray<UStaticMeshComponent*> ResidentialProps;
    UPROPERTY() TArray<FBox> ResidentialPoolBasins;
    UPROPERTY() TArray<UProceduralMeshComponent*> ResidentialGables;
    FVector CourseTarget(int32 Lane,int32 Checkpoint)const;
    bool OnCourseTarget(FVector Position,int32 Lane,int32 Checkpoint)const;
    FString CourseLabel(int32 Lane,int32 Checkpoint)const;
    float CourseLaunchZ(FVector Position)const;
    int32 CourseEntryTree(int32 Lane)const;
    UPROPERTY() TArray<int32> CourseEntryTrees;
    void BeginPlay() override;
    void ApplyTreeLeafDensity(float Density);
    UPROPERTY() UProceduralMeshComponent* Terrain;
    UPROPERTY() UHierarchicalInstancedStaticMeshComponent* Trunks;
    UPROPERTY() UHierarchicalInstancedStaticMeshComponent* Foliage;
    UPROPERTY() UHierarchicalInstancedStaticMeshComponent* Branches;
    UPROPERTY() UHierarchicalInstancedStaticMeshComponent* Rocks;
    UPROPERTY() UHierarchicalInstancedStaticMeshComponent* Grass;
    UPROPERTY() UHierarchicalInstancedStaticMeshComponent* Acorns;
    UPROPERTY() UHierarchicalInstancedStaticMeshComponent* AcornCaps;
    UPROPERTY() TArray<UHierarchicalInstancedStaticMeshComponent*> ForestLayers;
    UPROPERTY() TArray<UHierarchicalInstancedStaticMeshComponent*> ClimbWood;
    UPROPERTY() UStaticMeshComponent* DenMesh;
    UPROPERTY() UHierarchicalInstancedStaticMeshComponent* ReserveFence;
    UPROPERTY() UHierarchicalInstancedStaticMeshComponent* Farmhouse;
    UPROPERTY() TArray<UHierarchicalInstancedStaticMeshComponent*> FarmProps;
    UPROPERTY() FVector FarmhouseLocation=FVector(2200,0,210);
    static constexpr float FarmFloor=61.f;
    float GroundHeight(float X,float Y) const;
    bool InsideFarmhouse(FVector Point) const;
    bool InCropCover(FVector Point) const;
    FVector FarmRoute(FVector From,FVector Target) const;
    void ShowLocalAcorns(const TArray<int32>& Collected);
    UPROPERTY() TArray<FWildlineTree> Trees;
    UPROPERTY() TArray<FVector> AcornLocations;
    UPROPERTY() FVector DenLocation;
    UFUNCTION(BlueprintCallable,CallInEditor,Category="Blackwood") void Rebuild();
    int32 NearestTree(FVector Point, float MaxDistance) const;
    FVector TrunkCentre(int32 Index,float WorldZ,float& Radius) const;
    bool TraceWood(FVector Start,FVector End,FHitResult& Hit,int32& Tree,float SweepRadius=0) const;
    void AddClimbWood(int32 Tree);
    bool IsLimbSurface(int32 Tree,FVector Point,FVector Normal) const;
    static AWildlineWorld* Find(UWorld* World);
private:
    void Generate();
    void GenerateKnobsScenery();
    void GenerateFarm();
    void GenerateResidential();
    bool GenerateConfiguredNeighborhood();
};

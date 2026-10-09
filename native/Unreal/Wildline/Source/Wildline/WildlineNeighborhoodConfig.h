#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WildlineNeighborhoodConfig.generated.h"

USTRUCT(BlueprintType)
struct FWildlineNeighborhoodProp {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FString Mesh;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FTransform Transform;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bCollision=true;
};
USTRUCT(BlueprintType)
struct FWildlineNeighborhoodRoute {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FString Name;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector Start=FVector::ZeroVector;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<FVector> Targets;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<FString> Labels;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector EntryTrunk=FVector::ZeroVector;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float EntryClimbTop=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float EntryRadius=15;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float TrampolineLaunchZ=700;
};
/** Authored in centimetres. Cooked data drives art, collision, bots and HUD together. */
UCLASS(BlueprintType)
class WILDLINE_API UWildlineNeighborhoodConfig : public UDataAsset {
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FString DisplayName;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector Feeder=FVector(120,0,450);
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<FWildlineNeighborhoodRoute> Routes;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<FWildlineNeighborhoodProp> Props;
};

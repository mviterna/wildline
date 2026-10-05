#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "WildlineSquirrelAnim.generated.h"

// The skin uses its authored rest skeleton; locomotion is evaluated natively
// from replicated displacement so the same articulated animal runs on clients.
UCLASS(Transient)
class WILDLINE_API UWildlineSquirrelAnim : public UAnimInstance {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="Squirrel",CallInEditor) static void PrepareMesh(USkeletalMesh* Mesh);
protected:
    FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};

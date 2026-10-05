#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "WildlineHUD.generated.h"
class AWildlineCharacter;
class AWildlineWorld;
class AWildlineGameState;
UCLASS()
class WILDLINE_API AWildlineHUD : public AHUD {
    GENERATED_BODY()
public:
    void DrawHUD() override;
private:
    void Label(const FString& Text,float X,float Y,float Size,FLinearColor Color);
    void DrawReserveMap(AWildlineCharacter* Player,AWildlineGameState* State,AWildlineWorld* Forest);
    void DrawForageHints(AWildlineCharacter* Player,AWildlineWorld* Forest);
    TWeakObjectPtr<AWildlineCharacter> FeedbackPlayer;
    int32 PreviousBag=0,PreviousCollected=0,PreviousStored=0;
    float FeedbackUntil=0;
    FString FeedbackText;
};

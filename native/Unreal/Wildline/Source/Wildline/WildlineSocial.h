#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WildlineSocial.generated.h"
class UTextBlock;
class UBorder;
class UEditableTextBox;
class UButton;
class UVerticalBox;
class USizeBox;
UCLASS()
class WILDLINE_API UWildlineSocial : public UUserWidget {
    GENERATED_BODY()
public:
    TSharedRef<SWidget> RebuildWidget() override;
    void NativeTick(const FGeometry& Geometry,float Delta) override;
    FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
    void Refresh();
    void Open(bool Value);
    bool bOpen=false;
    bool bEditingName=false;
    UPROPERTY() UTextBlock* Badge;
    UPROPERTY() UTextBlock* Roster;
    UPROPERTY() UTextBlock* RoomStatus;
    UPROPERTY() UEditableTextBox* Name;
    UPROPERTY() UBorder* Panel;
    UPROPERTY() UButton* JoinButton;
    UPROPERTY() UButton* RenameButton;
    UPROPERTY() UTextBlock* Message;
    UPROPERTY() UTextBlock* Title;
    UPROPERTY() UVerticalBox* ProfileFields;
    UPROPERTY() UVerticalBox* Board;
    UPROPERTY() USizeBox* RosterBox;
    bool bGameplayBoard=false;
    UFUNCTION() void Toggle();
    UFUNCTION() void SaveName();
    UFUNCTION() void EditName();
    UFUNCTION() void Hunter();
    UFUNCTION() void Squirrel();
    UFUNCTION() void JoinRoom();
    float RefreshAt=0;
};

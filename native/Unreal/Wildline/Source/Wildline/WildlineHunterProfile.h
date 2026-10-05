#pragma once
#include "CoreMinimal.h"
#include "WildlineHunterProfile.generated.h"
USTRUCT()
struct FWildlineHunterProfile {
    GENERATED_BODY()
    UPROPERTY() int32 Preset=0;
    UPROPERTY() int32 Body=0;
    UPROPERTY() int32 Outfit=0;
    UPROPERTY() int32 Hair=0;
    UPROPERTY() bool bCap=true;
    UPROPERTY() bool bUsePhoto=false;
    UPROPERTY() bool bBeard=false;
    UPROPERTY() float Slimness=0;
    UPROPERTY() float SkinTone=0;
    UPROPERTY() float FaceWidth=0;
    UPROPERTY() float NoseDepth=0;
    UPROPERTY() float PhotoX=0;
    UPROPERTY() float PhotoY=0;
    UPROPERTY() float PhotoZoom=1.6f;
    static FWildlineHunterProfile Travis(){FWildlineHunterProfile P;P.Preset=1;P.Outfit=1;P.Hair=2;P.bCap=false;P.bBeard=true;P.Slimness=.85f;P.FaceWidth=.2f;P.NoseDepth=-.1f;P.PhotoZoom=1;return P;}
    void Clamp(){Preset=FMath::Clamp(Preset,0,1);Body=FMath::Clamp(Body,0,1);Outfit=FMath::Clamp(Outfit,0,2);Hair=FMath::Clamp(Hair,0,2);Slimness=FMath::Clamp(Slimness,0.f,1.f);SkinTone=FMath::Clamp(SkinTone,0.f,1.f);FaceWidth=FMath::Clamp(FaceWidth,-1.f,1.f);NoseDepth=FMath::Clamp(NoseDepth,-1.f,1.f);PhotoX=FMath::Clamp(PhotoX,-.5f,.5f);PhotoY=FMath::Clamp(PhotoY,-.5f,.5f);PhotoZoom=FMath::Clamp(PhotoZoom,.5f,4.f);}
};
class UTexture2D;
namespace WildlinePortrait {
    FString SavedPhotoPath();
    bool ImportPhoto(const FString& Path,FString& Encoded,FString& Error);
    FString LoadSavedPhoto();
    UTexture2D* DecodePhoto(const FString& Encoded);
    bool ValidatePhoto(const FString& Encoded);
    FString ChoosePhotoFile();
}

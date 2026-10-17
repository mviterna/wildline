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
    static FWildlineHunterProfile Travis(){FWildlineHunterProfile P;P.Preset=1;P.Outfit=1;P.Hair=0;P.bCap=false;P.bBeard=false;P.Slimness=.85f;P.FaceWidth=0;P.NoseDepth=0;P.PhotoZoom=1;return P;}
    // ID 2 belonged to Travis 2. Keep it reserved for saved-profile migration.
    static FWildlineHunterProfile Ethan(){auto P=Travis();P.Preset=3;P.Hair=1;return P;}
    static FWildlineHunterProfile Greyson(){auto P=Travis();P.Preset=4;return P;}
    static FWildlineHunterProfile Mark(){auto P=Travis();P.Preset=5;P.Outfit=0;P.Slimness=.25f;return P;}
    bool IsPhotoPreset() const {return Body==0&&(Preset==1||Preset==3||Preset==4||Preset==5);}
    // Uncapped photo heads are 174.5 cm in the shared source rig.
    float HeightScale() const {return Body==0?(Preset==3?71.4f/174.5f:Preset==4?110.4f/174.5f:1.f):1.f;}
    // Part scales are relative to the body, and fingers/toes inherit them.
    float HandScale() const {return Body==0?(Preset==3?.72f:Preset==4?.82f:1.f):1.f;}
    float FootScale() const {return Body==0?(Preset==3?.75f:Preset==4?.84f:1.f):1.f;}
    int32 PresetOptionIndex() const {return Preset==5?4:Preset==4?3:Preset==3?2:Preset==1?1:0;}
    void Clamp(){Preset=FMath::Clamp(Preset,0,5);if(Preset==2)Preset=1;Body=FMath::Clamp(Body,0,1);if(IsPhotoPreset())bBeard=false;Outfit=FMath::Clamp(Outfit,0,2);Hair=FMath::Clamp(Hair,0,2);Slimness=FMath::Clamp(Slimness,0.f,1.f);SkinTone=FMath::Clamp(SkinTone,0.f,1.f);FaceWidth=FMath::Clamp(FaceWidth,-1.f,1.f);NoseDepth=FMath::Clamp(NoseDepth,-1.f,1.f);PhotoX=FMath::Clamp(PhotoX,-.5f,.5f);PhotoY=FMath::Clamp(PhotoY,-.5f,.5f);PhotoZoom=FMath::Clamp(PhotoZoom,.5f,4.f);}
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

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
    UPROPERTY() int32 Headwear=-1;
    UPROPERTY() bool bUsePhoto=false;
    UPROPERTY() bool bBeard=false;
    UPROPERTY() float Slimness=0;
    // Version 0 is an older save: resolve its name-based sizes once in Clamp.
    UPROPERTY() int32 DimensionsVersion=0;
    UPROPERTY() float HeightCm=174.5f;
    UPROPERTY() float BodyWidth=1.f;
    UPROPERTY() float BodyDepth=1.f;
    UPROPERTY() float HandSize=1.f;
    UPROPERTY() float FootSize=1.f;
    UPROPERTY() float FaceSize=1.f;
    UPROPERTY() float SkinTone=0;
    UPROPERTY() float FaceWidth=0;
    UPROPERTY() float NoseDepth=0;
    UPROPERTY() float PhotoX=0;
    UPROPERTY() float PhotoY=0;
    UPROPERTY() float PhotoZoom=1.6f;
    static FWildlineHunterProfile Travis(){FWildlineHunterProfile P;P.DimensionsVersion=1;P.Preset=1;P.Outfit=1;P.Hair=0;P.bCap=false;P.bBeard=false;P.Slimness=.85f;P.FaceWidth=0;P.NoseDepth=0;P.PhotoZoom=1;return P;}
    // ID 2 belonged to Travis 2. Keep it reserved for saved-profile migration.
    static FWildlineHunterProfile Ethan(){auto P=Travis();P.Preset=3;P.Hair=1;P.HeightCm=71.4f;P.HandSize=.72f;P.FootSize=.75f;return P;}
    static FWildlineHunterProfile Greyson(){auto P=Travis();P.Preset=4;P.HeightCm=110.4f;P.HandSize=.82f;P.FootSize=.84f;return P;}
    static FWildlineHunterProfile Mark(){auto P=Travis();P.Preset=5;P.Outfit=0;P.Slimness=.25f;return P;}
    bool IsPhotoPreset() const {return Body==0&&(Preset==1||Preset==3||Preset==4||Preset==5);}
    // Uncapped photo heads are 174.5 cm in the shared source rig.
    float LegacyHeight() const {return Body==0?(Preset==3?71.4f:Preset==4?110.4f:174.5f):174.5f;}
    float HeightScale() const {return (DimensionsVersion>0?HeightCm:LegacyHeight())/174.5f;}
    // Part scales are relative to the body, and fingers/toes inherit them.
    float HandScale() const {return DimensionsVersion>0?HandSize:Body==0?(Preset==3?.72f:Preset==4?.82f:1.f):1.f;}
    float FootScale() const {return DimensionsVersion>0?FootSize:Body==0?(Preset==3?.75f:Preset==4?.84f:1.f):1.f;}
    static float Bounded(float Value,float Min,float Max,float Default){return FMath::IsFinite(Value)?FMath::Clamp(Value,Min,Max):Default;}
    int32 PresetOptionIndex() const {return Preset==5?4:Preset==4?3:Preset==3?2:Preset==1?1:0;}
    void Clamp(){
        if(Headwear<0)Headwear=bCap?1:0;
        Headwear=FMath::Clamp(Headwear,0,36);
        if(DimensionsVersion<=0){HeightCm=LegacyHeight();HandSize=HandScale();FootSize=FootScale();DimensionsVersion=1;}
        DimensionsVersion=1;HeightCm=Bounded(HeightCm,60.f,230.f,174.5f);
        BodyWidth=Bounded(BodyWidth,.7f,1.5f,1.f);BodyDepth=Bounded(BodyDepth,.7f,1.6f,1.f);
        HandSize=Bounded(HandSize,.5f,1.5f,1.f);FootSize=Bounded(FootSize,.5f,1.5f,1.f);FaceSize=Bounded(FaceSize,.6f,1.5f,1.f);
        Preset=FMath::Clamp(Preset,0,5);if(Preset==2)Preset=1;Body=FMath::Clamp(Body,0,1);if(IsPhotoPreset())bBeard=false;Outfit=FMath::Clamp(Outfit,0,2);Hair=FMath::Clamp(Hair,0,2);Slimness=Bounded(Slimness,-.5f,1.f,0.f);SkinTone=FMath::Clamp(SkinTone,0.f,1.f);FaceWidth=FMath::Clamp(FaceWidth,-1.f,1.f);NoseDepth=FMath::Clamp(NoseDepth,-1.f,1.f);PhotoX=FMath::Clamp(PhotoX,-.5f,.5f);PhotoY=FMath::Clamp(PhotoY,-.5f,.5f);PhotoZoom=FMath::Clamp(PhotoZoom,.5f,4.f);}
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

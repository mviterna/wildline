#pragma once
#include "CoreMinimal.h"
namespace WildlineHeadwear {
    struct FStyle {const TCHAR* Slug;const TCHAR* Name;bool Funny;float Height;};
    inline constexpr FStyle Styles[]={
        {TEXT("None"),TEXT("No hat"),false,0},
        {TEXT("FieldCap"),TEXT("Field cap"),false,12},
        {TEXT("BlazeCap"),TEXT("Blaze orange cap"),false,12},
        {TEXT("CamoCap"),TEXT("Woodland cap"),false,12},
        {TEXT("Trucker"),TEXT("Trucker cap"),false,13},
        {TEXT("Beanie"),TEXT("Ribbed beanie"),false,16},
        {TEXT("WatchCap"),TEXT("Navy watch cap"),false,14},
        {TEXT("Trapper"),TEXT("Winter trapper"),false,14},
        {TEXT("Boonie"),TEXT("Boonie hat"),false,12},
        {TEXT("Cowboy"),TEXT("Leather cowboy"),false,17},
        {TEXT("Ranger"),TEXT("Ranger campaign"),false,18},
        {TEXT("FlatCap"),TEXT("Tweed flat cap"),false,10},
        {TEXT("Fedora"),TEXT("Felt fedora"),false,16},
        {TEXT("Pith"),TEXT("Safari pith helmet"),false,16},
        {TEXT("HardHat"),TEXT("Yellow hard hat"),false,15},
        {TEXT("Foreman"),TEXT("White foreman helmet"),false,15},
        {TEXT("Firefighter"),TEXT("Firefighter helmet"),false,17},
        {TEXT("Astronaut"),TEXT("Space explorer"),true,20},
        {TEXT("Chicken"),TEXT("Chicken hat"),true,25},
        {TEXT("TRex"),TEXT("T. rex hat"),true,22},
        {TEXT("Triceratops"),TEXT("Triceratops hat"),true,24},
        {TEXT("Shark"),TEXT("Shark attack"),true,23},
        {TEXT("Frog"),TEXT("Frog bucket"),true,18},
        {TEXT("Duck"),TEXT("Rubber duck"),true,22},
        {TEXT("Viking"),TEXT("Viking horns"),true,24},
        {TEXT("Pirate"),TEXT("Pirate captain"),true,22},
        {TEXT("TopHat"),TEXT("Dapper top hat"),true,27},
        {TEXT("Wizard"),TEXT("Midnight wizard"),true,39},
        {TEXT("Crown"),TEXT("Acorn royalty"),true,20},
        {TEXT("Propeller"),TEXT("Propeller cap"),true,24},
        {TEXT("Chef"),TEXT("Chef's toque"),true,33},
        {TEXT("TrafficCone"),TEXT("Traffic cone"),true,37},
        {TEXT("UFO"),TEXT("UFO landing"),true,24},
        {TEXT("Pumpkin"),TEXT("Pumpkin patch"),true,23},
        {TEXT("Acorn"),TEXT("Giant acorn"),true,29},
        {TEXT("Mushroom"),TEXT("Forest mushroom"),true,22},
        {TEXT("Antlers"),TEXT("Buck antlers"),true,36}
    };
    inline constexpr int32 Count=UE_ARRAY_COUNT(Styles);
    inline int32 Clamp(int32 Id){return FMath::Clamp(Id,0,Count-1);}
    inline FString Asset(int32 Id,bool Thumbnail=false){const auto& S=Styles[Clamp(Id)];return FString::Printf(TEXT("/Game/Wildline/Hunters/Headwear/%s/%s_Hat_%s.%s_Hat_%s"),Thumbnail?TEXT("Thumbnails"):TEXT("Meshes"),Thumbnail?TEXT("T"):TEXT("SM"),S.Slug,Thumbnail?TEXT("T"):TEXT("SM"),S.Slug);}
}

#include "WildlineHUD.h"
#include "WildlineCharacter.h"
#include "WildlineLake.h"
#include "WildlineWorld.h"
#include "Engine/Canvas.h"

void AWildlineHUD::DrawLakeActivity(AWildlineCharacter* P){
    const auto* F=AWildlineWorld::Find(GetWorld());if(!F||!F->bLake)return;
    const float W=Canvas->SizeX,H=Canvas->SizeY,CX=W*.5f,CY=H*.5f;
    const FLinearColor Cream(.96f,.94f,.8f),Green(.5f,.95f,.65f),Amber(1,.72f,.25f);
    if(P->bSwimming&&!P->bDead){
        Label(P->IsHeadUnderwater()?TEXT("UNDERWATER / WEAPON DISABLED"):TEXT("SWIMMING"),CX-155,H-172,18,Green);
        Label(TEXT("WASD / SWIM    LOOK + W / DIVE    SPACE / RISE    CTRL / DIVE"),CX-270,H-143,12,Cream);
    }
    if(P->bSquirrel&&!P->bDead){if(AWildlineLake::RadiusAt(P->GetActorLocation(),P->bInLobby)<1.23f)Label(TEXT("DEEP WATER / SQUIRRELS CANNOT SWIM"),CX-190,CY+140,16,Amber);}
    if(P->bInLobby&&!P->bSquirrel&&!P->bSwimming&&!P->bDead){
        if(P->FishingStage){
            const float Y=H-185,Help=P->FishingHelpAlpha();
            if(Help>0){
                DrawRect(FLinearColor(.015f,.035f,.04f,.82f*Help),CX-270,H-295,540,90);
                Label(TEXT("DOCK FISHING"),CX-245,H-280,14,FLinearColor(.5f,.95f,.65f,Help));
                Label(TEXT("Hold LMB, release to cast  /  E to hook a bite"),CX-245,H-255,13,FLinearColor(.96f,.94f,.8f,Help));
                Label(TEXT("Hold LMB to reel, release to ease  /  R to return rod"),CX-245,H-233,12,FLinearColor(.96f,.94f,.8f,Help));
            }
            const TCHAR* Status=P->FishingStage==4?TEXT("PICKING UP ROD"):P->FishingStage==5?TEXT("ROD READY"):P->FishingStage==6?TEXT("CASTING"):P->FishingStage==1?TEXT("WAITING FOR A BITE"):P->FishingStage==2?TEXT("BITE!  E / SET THE HOOK"):P->FishingStage==7?TEXT("LANDED / RELEASING"):P->FishingStage==8?TEXT("RETURNING ROD"):P->FishingTension>.78f?TEXT("HIGH TENSION / EASE OFF"):P->FishingPull()>.78f?TEXT("FISH SURGING"):TEXT("REELING IN");
            DrawRect(FLinearColor(.015f,.035f,.04f,.85f),CX-210,Y-8,420,P->FishingStage==3?112:44);
            Label(Status,CX-185,Y+2,15,P->FishingStage==2||P->FishingTension>.78f?Amber:Cream);
            if(P->FishingChargeAt>0){const float Charge=FMath::Clamp((P->ServerTime()-P->FishingChargeAt)/1.1f,0.f,1.f);DrawRect(FLinearColor(.07f,.12f,.12f),CX-185,Y+29,370,6);DrawRect(Green,CX-185,Y+29,370*Charge,6);}
            if(P->FishingStage==3){
                Label(TEXT("LANDING"),CX-185,Y+38,10,Green);DrawRect(FLinearColor(.08f,.13f,.12f),CX-85,Y+40,270,8);DrawRect(Green,CX-85,Y+40,270*P->FishingProgress,8);
                Label(TEXT("TENSION"),CX-185,Y+71,10,Amber);DrawRect(FLinearColor(.08f,.13f,.12f),CX-85,Y+73,270,8);
                DrawRect(FLinearColor(.17f,.36f,.25f),CX-85+270*.22f,Y+70,270*.56f,14);
                DrawRect(P->FishingTension>.78f?FLinearColor(1,.2f,.1f):Amber,CX-85,Y+73,270*P->FishingTension,8);
                DrawLine(CX-85+270*P->FishingTension,Y+68,CX-85+270*P->FishingTension,Y+87,Cream,2);
            }else if(P->FishingStage==2){DrawRect(Amber,CX-185,Y+30,370*FMath::Clamp((P->FishingUntil-P->ServerTime())/1.8f,0.f,1.f),3);}
        }else if(AWildlineLake::FishingSpotAt(P->GetActorLocation())!=INDEX_NONE)Label(TEXT("E / PICK UP FISHING ROD"),CX-135,CY+70,18,Green);
        else Label(TEXT("FISH WHILE YOU WAIT / THROUGH THE LODGE DOOR TO THE DOCK"),CX-275,H-143,13,Green);
        if(P->FishingResultUntil>P->ServerTime())Label(P->FishingResult,CX-250,H-58,14,Amber);
        Label(FString::Printf(TEXT("FISH LANDED / %d"),P->FishCaught),W-220,155,14,Green);
    }
    if(P->bInLobby&&P->bDead)Label(TEXT("SQUIRRELS CANNOT SWIM / RETURNING TO THE HOLLOW"),CX-250,CY+70,15,Amber);
}

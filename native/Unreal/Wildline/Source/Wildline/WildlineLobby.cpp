#include "WildlineLobby.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

namespace {
UMaterialInterface* LobbyMat(const TCHAR* Name){return LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Wildline/Lobby/%s.%s"),Name,Name));}
UMaterialInterface* Bark(){return LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Wildline/Forest/Materials/M_TreeBark.M_TreeBark"));}
}
AWildlineLobby::AWildlineLobby(){PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=.25f;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Waiting rooms"));RootComponent->SetMobility(EComponentMobility::Static);}
AWildlineLobby* AWildlineLobby::Find(UWorld* W){for(TActorIterator<AWildlineLobby> It(W);It;++It)return *It;return nullptr;}
void AWildlineLobby::Tick(float DeltaSeconds){
    Super::Tick(DeltaSeconds);
    if(HearthLight){const float T=GetWorld()->GetTimeSeconds();HearthLight->SetIntensity(4200.f*(.94f+.04f*FMath::Sin(T*4.7f)+.02f*FMath::Sin(T*10.3f)));}
    auto* PC=GetWorld()->GetFirstPlayerController();
    const FVector Eye=PC&&PC->PlayerCameraManager?PC->PlayerCameraManager->GetCameraLocation()-Centre(false):FVector(0,0,-10000);
    const bool Visible=FMath::Abs(Eye.X)<760&&FMath::Abs(Eye.Y)<760&&Eye.Z>-50&&Eye.Z<510;
    const FVector TreeEye=PC&&PC->PlayerCameraManager?PC->PlayerCameraManager->GetCameraLocation()-Centre(true):FVector(0,0,-10000);
    const bool TreeVisible=FMath::Abs(TreeEye.X)<520&&FMath::Abs(TreeEye.Y)<520&&TreeEye.Z>-50&&TreeEye.Z<510;
    // Waiting rooms live above the reserve; only the peer looking from inside
    // a room should render their architecture, furniture, signs and portals.
    SetActorHiddenInGame(!Visible&&!TreeVisible);
    for(auto* Prop:Props)if(Prop&&Prop->bCastHiddenShadow)Prop->SetCastShadow(Visible||TreeVisible);
    if(Visible!=bLodgeSceneryVisible){bLodgeSceneryVisible=Visible;for(auto* C:LodgeScenery)if(C)C->SetVisibility(Visible,true);}
}
FVector AWildlineLobby::Centre(bool S){return FVector(S?2400:-2400,-6500,5500);}
FVector AWildlineLobby::Spawn(bool S,int32 I){return Centre(S)+FVector(S?-180:-420,(I%4)*55-82,S?17:110);}
FVector AWildlineLobby::Portal(bool S){return Centre(S)+FVector(S?210:440,0,0);}
bool AWildlineLobby::OnPortal(bool S,FVector P){const FVector Q=P-Portal(S);return Q.SizeSquared2D()<FMath::Square(S?65.f:115.f)&&Q.Z>=5&&Q.Z<(S?50:210);}
FVector AWildlineLobby::Rack(int32 Weapon){return Centre(false)+FVector(Weapon==0?-210.f:90.f,-545,0);}
int32 AWildlineLobby::WeaponAt(FVector P){if(FMath::Abs(P.Z-Centre(false).Z)>210)return INDEX_NONE;int32 Best=INDEX_NONE;float Distance=170;for(int32 I=0;I<2;I++){const float D=FVector::Dist2D(P,Rack(I));if(D<Distance){Distance=D;Best=I;}}return Best;}
void AWildlineLobby::BeginPlay(){
    Super::BeginPlay();BuildLodge();BuildTree();BuildPortal(false);BuildPortal(true);
    SetActorHiddenInGame(true);
    // One invisible, low-poly envelope per room blocks outdoor sunlight instead
    // of asking every log and bark segment to cast overlapping virtual shadows.
    for(bool S:{false,true}){
        auto* Shell=Piece(TEXT("Interior daylight shadow envelope"),S?TEXT("Cylinder"):TEXT("Cube"),Centre(S)+FVector(0,0,S?215:245),S?FVector(950,950,440):FVector(1440,1440,510),nullptr);
        Shell->SetCastShadow(true);Shell->SetCastHiddenShadow(true);Shell->SetVisibility(false);
    }
    // The outdoor maps lock exposure for daylight. Interior-only bounds let eyes
    // adapt to lantern light without changing the reserve's photographic grade.
    for(bool S:{false,true}){
        auto* Bounds=NewObject<UBoxComponent>(this);AddInstanceComponent(Bounds);Bounds->SetupAttachment(RootComponent);Bounds->SetRelativeLocation(Centre(S)+FVector(0,0,225));Bounds->SetBoxExtent(FVector(S?490:730,S?490:730,280));Bounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Bounds->SetCollisionResponseToAllChannels(ECR_Ignore);Bounds->SetGenerateOverlapEvents(false);Bounds->RegisterComponent();
        auto* Grade=NewObject<UPostProcessComponent>(this);AddInstanceComponent(Grade);Grade->SetupAttachment(Bounds);Grade->bUnbound=false;Grade->Priority=200;Grade->BlendRadius=20;
        auto& Settings=Grade->Settings;
        Settings.bOverride_AutoExposureMethod=true;Settings.AutoExposureMethod=AEM_Histogram;
        Settings.bOverride_AutoExposureMinBrightness=true;Settings.bOverride_AutoExposureMaxBrightness=true;Settings.AutoExposureMinBrightness=S?5.7f:7.5f;Settings.AutoExposureMaxBrightness=S?5.7f:7.5f;
        // Reserve grades override the engine's exposure compensation; room light
        // must not inherit a different compensation from each outdoor map.
        Settings.bOverride_AutoExposureBias=true;Settings.AutoExposureBias=S?-1.f:-.15f;
        Settings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;Settings.AutoExposureApplyPhysicalCameraExposure=false;
        Settings.bOverride_BloomIntensity=true;Settings.BloomIntensity=S?.12f:.70f;
        if(!S){Settings.bOverride_BloomThreshold=true;Settings.BloomThreshold=.8f;}Grade->RegisterComponent();
    }
    // Broad cool window light gives furniture shape and soft contact shadows.
    // Outdoor sunlight is occluded by the interior shell, so these sources are
    // placed just inside the five actual apertures, pointing into the room.
    auto Window=[this](FVector P,FRotator R){auto* L=NewObject<URectLightComponent>(this);AddInstanceComponent(L);L->SetupAttachment(RootComponent);L->SetRelativeLocation(Centre(false)+P);L->SetRelativeRotation(R);L->SetLightColor(FLinearColor(.72f,.84f,1));L->SetIntensity(1800);L->SetAttenuationRadius(1050);L->SetSourceWidth(180);L->SetSourceHeight(165);L->SetCastShadows(true);L->RegisterComponent();};
    for(float X:{-405.f,455.f})for(float Side:{-1.f,1.f})Window(FVector(X,Side*664,255),FRotator(0,Side>0?-90:90,0));
    Window(FVector(664,125,255),FRotator(0,180,0));
}
UStaticMeshComponent* AWildlineLobby::Piece(const TCHAR* Name,const TCHAR* Shape,FVector P,FVector Size,UMaterialInterface* M,FRotator R,bool Collision){
    auto* C=NewObject<UStaticMeshComponent>(this,MakeUniqueObjectName(this,UStaticMeshComponent::StaticClass(),FName(Name)));AddInstanceComponent(C);C->SetupAttachment(RootComponent);
    C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"),Shape,Shape)));C->SetRelativeLocation(P);C->SetRelativeScale3D(Size/100.f);C->SetRelativeRotation(R);if(M)C->SetMaterial(0,M);
    C->SetCollisionProfileName(Collision?TEXT("BlockAll"):TEXT("NoCollision"));
    // Roofs shade the interiors; hundreds of overlapping decorative wall segments
    // do not each need their own pages in the reserve's directional shadow map.
    C->SetCastShadow(Collision&&(FCString::Strstr(Name,TEXT("floor"))||FCString::Strstr(Name,TEXT("ceiling"))));C->RegisterComponent();Props.Add(C);return C;
}
void AWildlineLobby::Sign(const FString& Text,FVector P,float Size,FRotator R){
    auto* C=NewObject<UTextRenderComponent>(this);AddInstanceComponent(C);C->SetupAttachment(RootComponent);C->SetRelativeLocation(P);C->SetRelativeRotation(R);C->SetText(FText::FromString(Text));C->SetWorldSize(Size);C->SetHorizontalAlignment(EHTA_Center);C->SetTextRenderColor(FColor(245,215,154));if(auto* M=LobbyMat(TEXT("M_Sign")))C->SetTextMaterial(M);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();
}
UStaticMeshComponent* AWildlineLobby::Model(const TCHAR* Asset,FVector P,FVector Size,FRotator R,bool Collision){
    const FString Path=FString::Printf(TEXT("/Game/Wildline/Lobby/Photoreal/Meshes/%s.%s"),Asset,Asset);
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);if(!Mesh){UE_LOG(LogTemp,Error,TEXT("Missing waiting room model: %s"),*Path);return nullptr;}
    auto* C=NewObject<UStaticMeshComponent>(this,MakeUniqueObjectName(this,UStaticMeshComponent::StaticClass(),FName(Asset)));AddInstanceComponent(C);C->SetupAttachment(RootComponent);C->SetMobility(EComponentMobility::Static);C->SetStaticMesh(Mesh);
    const FVector Bounds=Mesh->GetBoundingBox().GetSize();float Uniform=1;
    for(int32 I=0;I<3;I++)if(Size[I]>0&&Bounds[I]>KINDA_SMALL_NUMBER){Uniform=Size[I]/Bounds[I];break;}
    FVector Scale(Uniform);for(int32 I=0;I<3;I++)if(Size[I]>0&&Bounds[I]>KINDA_SMALL_NUMBER)Scale[I]=Size[I]/Bounds[I];
    C->SetRelativeLocation(P);C->SetRelativeRotation(R);C->SetRelativeScale3D(Scale);C->SetCollisionProfileName(Collision?TEXT("BlockAll"):TEXT("NoCollision"));C->SetCastShadow(true);C->RegisterComponent();Props.Add(C);return C;
}
UPointLightComponent* AWildlineLobby::Light(FVector P,FLinearColor Color,float Intensity,float Radius,bool Shadows){auto* L=NewObject<UPointLightComponent>(this);AddInstanceComponent(L);L->SetupAttachment(RootComponent);L->SetRelativeLocation(P);L->SetLightColor(Color);L->SetIntensity(Intensity);L->SetAttenuationRadius(Radius);L->SetCastShadows(Shadows);L->SetSourceRadius(Shadows?35:12);L->RegisterComponent();return L;}
void AWildlineLobby::BuildPortal(bool S){
    const FVector P=Portal(S);const float R=S?70:125,H=S?165:285;auto* Glow=LobbyMat(S?TEXT("M_PortalGreen"):TEXT("M_PortalAmber"));
    Piece(TEXT("Portal stone plinth"),TEXT("Cylinder"),P+FVector(0,0,2),FVector(R*2+18,R*2+18,4),LobbyMat(TEXT("M_Stone")),FRotator::ZeroRotator,true);
    Piece(TEXT("Ready portal illuminated floor"),TEXT("Cylinder"),P+FVector(0,0,5),FVector(R*2,R*2,1),Glow);
    // A single bevelled, PBR stone arch replaces the old string of gold spheres.
    Model(TEXT("SM_ReadyArch"),P+FVector(32,0,0),FVector(0,0,H));
    Sign(TEXT("READY"),P+FVector(-25,0,H+30),S?18:29);
    Sign(TEXT("STEP ON THE PORTAL"),P+FVector(-25,0,H+5),S?8:12);
    Light(P+FVector(0,0,H*.6f),S?FLinearColor(.3f,1,.6f):FLinearColor(1,.55f,.12f),S?1000:2600,R*4);
}

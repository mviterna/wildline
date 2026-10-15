#include "WildlineLobby.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace {
UMaterialInterface* DetailMaterial(const TCHAR* Name){return LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Wildline/Lobby/Photoreal/Materials/%s.%s"),Name,Name));}
UMaterialInterface* RoomMaterial(const TCHAR* Name){return LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Wildline/Lobby/%s.%s"),Name,Name));}
}

void AWildlineLobby::BuildLodge(){
    const FVector O=Centre(false);
    Model(TEXT("SM_LodgeArchitecture"),O);
    // Collision is independent of the visual meshes, so art revisions do not
    // alter portal/rack geometry or cause expensive per-log movement queries.
    auto Hidden=[this](const TCHAR* Name,FVector P,FVector Size){auto* C=Piece(Name,TEXT("Cube"),P,Size,nullptr,FRotator::ZeroRotator,true);C->SetVisibility(false);C->SetCastShadow(false);};
    Hidden(TEXT("Lodge floor collision"),O-FVector(0,0,10),FVector(1420,1420,20));
    for(float Side:{-1.f,1.f}){Hidden(TEXT("Lodge side wall collision"),O+FVector(Side*710,0,245),FVector(35,1460,490));Hidden(TEXT("Lodge end wall collision"),O+FVector(0,Side*710,245),FVector(1460,35,490));}
    Hidden(TEXT("Lodge ceiling collision"),O+FVector(0,0,490),FVector(1460,1460,20));
    Model(TEXT("SM_LodgeRug"),O+FVector(45,45,1),FVector::ZeroVector,FRotator(0,90,0));
    Model(TEXT("SM_wooden_picnic_table"),O+FVector(-270,365,0),FVector(0,0,79),FRotator(0,90,0),true);
    Model(TEXT("SM_Rockingchair_01"),O+FVector(420,385,0),FVector(0,0,108),FRotator(0,-100,0),true);
    Model(TEXT("SM_Rockingchair_01"),O+FVector(540,130,0),FVector(0,0,104),FRotator(0,-65,0),true);
    Model(TEXT("SM_wooden_bookshelf_worn"),O+FVector(585,-320,0),FVector(0,0,213),FRotator(0,-90,0),true);
    for(int32 I=0;I<4;I++)Model(TEXT("SM_book_encyclopedia_set_01"),O+FVector(580,-365+I*29,40+(I%2)*57),FVector(0,0,23),FRotator(0,-90,0));
    Model(TEXT("SM_wooden_bucket_01"),O+FVector(405,600,0),FVector(0,0,55),FRotator(0,15,0),true);
    Model(TEXT("SM_wooden_axe_03"),O+FVector(375,606,8),FVector(0,0,83),FRotator(0,-30,12));
    Model(TEXT("SM_wicker_basket_01"),O+FVector(-490,548,0),FVector(0,0,26),FRotator(0,30,0),true);
    // Keep the authored table surface clear of floating props and preserve room
    // circulation around the benches and the entrance-to-portal route.
    Model(TEXT("SM_pot_enamel_01"),O+FVector(-266,357,79),FVector(0,0,21),FRotator(0,40,0));
    Model(TEXT("SM_brass_goblets"),O+FVector(-330,340,79),FVector(0,0,23),FRotator(0,30,0));
    Model(TEXT("SM_wooden_bowl_01"),O+FVector(-222,407,79),FVector(0,0,11));
    Model(TEXT("SM_vintage_oil_lamp"),O+FVector(-345,407,79),FVector(0,0,50));
    Model(TEXT("SM_lantern_chandelier_01"),O+FVector(-40,130,350),FVector(0,0,105));
    Light(O+FVector(-40,130,375),FLinearColor(1,.81f,.58f),6500,1400,true);
    Light(O+FVector(-320,-365,335),FLinearColor(.79f,.85f,1),4500,1400,true);
    Light(O+FVector(480,-100,300),FLinearColor(.91f,.87f,.79f),2300,1000);
    Light(O+FVector(-345,407,115),FLinearColor(1,.57f,.26f),700,280);
    for(float X:{-460.f,340.f}){Model(TEXT("SM_Lantern_01"),O+FVector(X,-638,235),FVector(0,0,44),FRotator(0,180,0));Light(O+FVector(X,-600,259),FLinearColor(1,.70f,.42f),1100,450);}
    // Detailed scanned deadwood replaces the old smooth cylinders in the hearth.
    for(int32 I=0;I<5;I++)Model(TEXT("SM_dead_tree_trunk"),O+FVector(94,643-I*9,13+I%2*9),FVector(92,13,15),FRotator(0,I%2?12:-12,0));
    for(int32 I=0;I<8;I++)Model(TEXT("SM_dead_tree_trunk"),O+FVector(478,586+(I%2)*25,4+(I/2)*20),FVector(100,18,18),FRotator(0,90,0));
    auto* Black=RoomMaterial(TEXT("M_Iron"));Piece(TEXT("Recessed hearth back"),TEXT("Cube"),O+FVector(95,686,75),FVector(191,5,148),Black);
    Light(O+FVector(95,610,63),FLinearColor(1,.39f,.105f),1800,630,true);
    for(int32 I=0;I<3;I++)Piece(TEXT("Hearth embers"),TEXT("Sphere"),O+FVector(55+I*37,631,20),FVector(15,8,3),RoomMaterial(TEXT("M_PortalAmber")));
    for(int32 I=0;I<2;I++){
        const FVector P=Rack(I);
        Model(TEXT("SM_katana_stand_01"),P+FVector(0,-45,0),FVector(168,33,120),FRotator::ZeroRotator,true);
        auto Display=[this,P](const TCHAR* Path){auto* Mesh=LoadObject<UStaticMesh>(nullptr,Path);if(!Mesh)return;auto* C=Piece(TEXT("Weapon rack model"),TEXT("Cube"),P+FVector(0,-27,128),FVector(100),nullptr,FRotator(0,0,35));C->SetStaticMesh(Mesh);C->SetCastShadow(true);};
        Display(I?TEXT("/Game/Wildline/Farm/Meshes/SM_Shotgun.SM_Shotgun"):TEXT("/Game/Wildline/Hunters/Meshes/SM_BoltRifle.SM_BoltRifle"));
        Display(I?TEXT("/Game/Wildline/Farm/Meshes/SM_ShotgunAction.SM_ShotgunAction"):TEXT("/Game/Wildline/Hunters/Meshes/SM_RifleBolt.SM_RifleBolt"));
        Sign(I?TEXT("SHOTGUN\nE / EQUIP"):TEXT("RIFLE\nE / EQUIP"),P+FVector(0,-7,205),14,FRotator(0,90,0));
    }
    // Species differ in coat, cheek/snout proportions, size and mounted pose.
    const TCHAR* TrophyNames[]={TEXT("SquirrelTrophy"),TEXT("SquirrelTrophyFox"),TEXT("SquirrelTrophyRed"),TEXT("SquirrelTrophyBlack")};
    const TCHAR* TrophyLabels[]={TEXT("EASTERN GREY SQUIRREL"),TEXT("FOX SQUIRREL"),TEXT("AMERICAN RED SQUIRREL"),TEXT("BLACK EASTERN GREY")};
    const float TrophyScales[]={6.7f,7.3f,5.8f,6.6f};
    const float TrophyTurns[]={-98.f,-83.f,-94.f,-78.f};
    for(int32 I=0;I<4;I++){
        const FVector P=O+FVector(-679,-456+I*225,245);
        Model(TEXT("SM_TrophyPlaque"),P,FVector(0,0,92));
        const FString Asset=FString::Printf(TEXT("/Game/Wildline/Lobby/SM_%s.SM_%s"),TrophyNames[I],TrophyNames[I]);
        if(auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Asset)){
            auto* C=Piece(TEXT("Mounted squirrel taxidermy"),TEXT("Sphere"),P+FVector(23,0,53),FVector(100),nullptr,FRotator(I==2?4:0,TrophyTurns[I],I==1?-4:0));
            C->SetStaticMesh(Mesh);C->EmptyOverrideMaterials();C->SetRelativeScale3D(FVector(TrophyScales[I]));C->SetCastShadow(true);
        }else UE_LOG(LogTemp,Error,TEXT("Missing waiting room model: %s"),*Asset);
        Sign(TrophyLabels[I],P+FVector(12,0,-10),7,FRotator::ZeroRotator);
    }
    Model(TEXT("SM_hanging_picture_frame_01"),O+FVector(683,440,242),FVector(0,0,102),FRotator(0,90,0));
    Sign(TEXT("BLACKWOOD HUNTING LODGE"),O+FVector(674,0,411),22);
    BuildLodgeDetails();
}

void AWildlineLobby::BuildTree(){
    const FVector O=Centre(true);Model(TEXT("SM_HollowArchitecture"),O);
    auto* Floor=Piece(TEXT("Hollow floor collision"),TEXT("Cylinder"),O-FVector(0,0,10),FVector(945,945,20),nullptr,FRotator::ZeroRotator,true);Floor->SetVisibility(false);Floor->SetCastShadow(false);
    for(int32 I=0;I<48;I++){const float A=2*PI*I/48;auto* C=Piece(TEXT("Hollow perimeter collision"),TEXT("Cube"),O+FVector(FMath::Cos(A)*465,FMath::Sin(A)*465,215),FVector(40,67,430),nullptr,FRotator(0,FMath::RadiansToDegrees(A),0),true);C->SetVisibility(false);C->SetCastShadow(false);}
    auto* Roof=Piece(TEXT("Hollow ceiling collision"),TEXT("Cylinder"),O+FVector(0,0,435),FVector(970,970,20),nullptr,FRotator::ZeroRotator,true);Roof->SetVisibility(false);Roof->SetCastShadow(false);
    // Real bark/root photoscans keep the organic forms and fine crevices visible
    // at squirrel eye level; none of these is a stretched basic sphere.
    for(int32 I=0;I<6;I++){
        const float A=2*PI*I/6;const FVector P=O+FVector(FMath::Cos(A)*373,FMath::Sin(A)*373,-3);
        Model(TEXT("SM_root_cluster_01"),P,FVector(170,115,70),FRotator(0,I*60,0),true);
        Model(TEXT("SM_pine_roots"),P+FVector(0,0,1),FVector(175,145,12),FRotator(0,I*61,0));
    }
    Model(TEXT("SM_tree_stump_02"),O+FVector(-267,150,0),FVector(0,0,66),FRotator(0,23,0),true);
    Model(TEXT("SM_dead_tree_trunk"),O+FVector(-265,248,4),FVector(236,38,41),FRotator(0,34,0),true);
    Model(TEXT("SM_wicker_basket_01"),O+FVector(-215,-259,0),FVector(0,0,22),FRotator(0,15,0));
    Model(TEXT("SM_wooden_bowl_01"),O+FVector(-325,-146,2),FVector(0,0,18),FRotator(0,18,0));
    for(int32 I=0;I<24;I++){
        const float A=I*2.399f,R=5+(I%9)*1.2f;const FVector P=O+FVector(-215+FMath::Cos(A)*R,-259+FMath::Sin(A)*R,6+(I%3)*4);
        Model(TEXT("SM_AcornDetailed"),P,FVector(0,0,12),FRotator(0,I*31,18));
    }
    for(int32 I=0;I<16;I++){
        const float A=I*2.399f;const FVector P=O+FVector(FMath::Cos(A)*(340+I%3*25),FMath::Sin(A)*(345+I%4*13),-1);
        Model(TEXT("SM_bark_debris_01"),P,FVector(108,62,10),FRotator(0,I*27,0));
        Model(TEXT("SM_moss_01"),P+FVector(0,0,6),FVector(60,35,13),FRotator(0,I*47,0));
    }
    for(int32 I=0;I<12;I++)Model(TEXT("SM_moss_01"),O+FVector(-305+(I%4)*22,172+(I/4)*25,67),FVector(41,28,8),FRotator(0,I*32,0));
    // A proper antique lantern provides a warm focal light, with cool fill and
    // contact shadows making the bark and scanned root details read clearly.
    Model(TEXT("SM_Lantern_01"),O+FVector(-266,167,68),FVector(0,0,46),FRotator(0,30,0));
    Model(TEXT("SM_vintage_oil_lamp"),O+FVector(-330,-135,19),FVector(0,0,56),FRotator(0,15,0));
    Light(O+FVector(-266,167,108),FLinearColor(1,.70f,.39f),2500,850,true);
    Light(O+FVector(90,-50,335),FLinearColor(.73f,.86f,1),3100,1100,true);
    Light(O+FVector(-190,-195,155),FLinearColor(1,.79f,.52f),1700,640);
    Light(O+FVector(-290,-5,215),FLinearColor(1,.79f,.55f),2200,650,true);
    Light(O+FVector(80,170,230),FLinearColor(1,.85f,.67f),1700,850,true);
    Sign(TEXT("THE ACORN HOLLOW"),O+FVector(417,0,335),23);
    Sign(TEXT("ACORN PANTRY"),O+FVector(-238,-307,105),12,FRotator(0,90,0));
    // The squirrels turn the lodge's specimen wall into a playful rivalry.
    const TCHAR* Hunters[]={TEXT("Travis"),TEXT("Ethan"),TEXT("Greyson")};
    const TCHAR* Captions[]={TEXT("TRAVIS / OUTRUN"),TEXT("ETHAN / OUTSMART"),TEXT("GREYSON / OUTCLIMB")};
    auto* Oak=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Wildline/Lobby/Photoreal/Materials/M_WindowOak.M_WindowOak"));
    auto* Brass=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Wildline/Lobby/Photoreal/Materials/M_LodgeDetailBrass.M_LodgeDetailBrass"));
    Sign(TEXT("THE ONES THAT GOT AWAY"),O+FVector(-397,0,299),13,FRotator(0,0,0));
    for(int32 I=0;I<3;I++){
        const FVector P=O+FVector(-414,(I-1)*107,163);
        Piece(TEXT("Hunter trophy oak shield"),TEXT("Sphere"),P+FVector(0,0,31),FVector(9,78,100),Oak);
        const FString Path=FString::Printf(TEXT("/Game/Wildline/Hunters/Meshes/SM_HunterTrophy%s.SM_HunterTrophy%s"),Hunters[I],Hunters[I]);
        auto* C=Piece(TEXT("Squirrel mounted hunter bust"),TEXT("Sphere"),P+FVector(15,0,0),FVector(100),nullptr,FRotator(0,-90,0));
        C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Path));C->SetRelativeScale3D(FVector(1.55f));
        Piece(TEXT("Hunter trophy nameplate"),TEXT("Cube"),P-FVector(-7,0,17),FVector(2,86,17),Brass);
        Sign(Captions[I],P+FVector(9,0,-13),4.8f,FRotator(0,0,0));
    }
    Piece(TEXT("Keepsake shelf"),TEXT("Cube"),O+FVector(45,403,122),FVector(260,44,8),Oak);
    Model(TEXT("SM_brass_goblets"),O+FVector(-20,395,128),FVector(0,0,24));
    Model(TEXT("SM_book_encyclopedia_set_01"),O+FVector(100,396,128),FVector(0,0,26));
    Model(TEXT("SM_AcornDetailed"),O+FVector(30,391,129),FVector(0,0,21));
    Sign(TEXT("FINDERS KEEPERS"),O+FVector(35,408,165),10,FRotator(0,-90,0));
    // Bark stepping stools, a lookout perch, and a table for planning raids.
    Model(TEXT("SM_tree_stump_02"),O+FVector(220,-245,0),FVector(90,80,68),FRotator(0,17,0),true);
    Model(TEXT("SM_dead_tree_trunk"),O+FVector(220,-160,0),FVector(100,42,28),FRotator(0,4,0),true);
    Model(TEXT("SM_wooden_bowl_01"),O+FVector(218,-247,69),FVector(0,0,16));
    for(int32 I=0;I<4;I++)Model(TEXT("SM_AcornDetailed"),O+FVector(207+I*7,-246,74),FVector(0,0,9),FRotator(0,I*48,30));
    Piece(TEXT("Lookout branch perch"),TEXT("Cylinder"),O+FVector(245,267,95),FVector(24,24,190),Oak,FRotator(0,0,90),true);
    Model(TEXT("SM_tree_stump_02"),O+FVector(200,160,0),FVector(65,60,30),FRotator(0,28,0),true);
    Model(TEXT("SM_tree_stump_02"),O+FVector(225,215,0),FVector(60,58,62),FRotator(0,67,0),true);
    Model(TEXT("SM_moss_01"),O+FVector(218,273,106),FVector(64,40,9));
    Sign(TEXT("NO HUNTERS / NO PROBLEM"),O+FVector(293,-291,152),8,FRotator(0,135,0));
}

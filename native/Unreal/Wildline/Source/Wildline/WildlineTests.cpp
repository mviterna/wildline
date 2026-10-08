#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "WildlineRules.h"
#include "WildlineUserSettings.h"
#include "WildlineCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "WildlineWorld.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWildlineModesTest,"Wildline.Gameplay.ModesAndShotgun",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWildlineModesTest::RunTest(const FString& Parameters){
    using namespace WildlineRules;
    TestEqual(TEXT("Classic timeout rewards squirrel survival"),TimeoutWinner(ClassicMode),2);
    TestEqual(TEXT("Farm timeout rewards pantry defense"),TimeoutWinner(FarmMode),1);
    TestEqual(TEXT("Rifle magazine remains five"),WeaponCapacity(RifleWeapon),5);
    TestEqual(TEXT("Shotgun carries two shells"),WeaponCapacity(ShotgunWeapon),2);
    TestTrue(TEXT("Shotgun reload and fire pacing are slower than rifle"),WeaponReload(1)>WeaponReload(0)&&WeaponInterval(1)>WeaponInterval(0));
    TestTrue(TEXT("Pellets lose damage with distance but remain bounded"),PelletDamage(0)>PelletDamage(4500)&&PelletDamage(-100)==PelletDamage(0)&&PelletDamage(10000)==PelletDamage(4500));
    TestEqual(TEXT("Farm plateau supports level interiors"),FarmGround(2200,0),210.f);
    TestTrue(TEXT("Farm blend returns to woodland height beyond 90 metres"),FMath::IsNearlyEqual(FarmGround(10000,0),Ground(10000,0)));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWildlineDifficultyTest,"Wildline.Gameplay.HunterDifficultyAndMap",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWildlineDifficultyTest::RunTest(const FString& Parameters){
    using namespace WildlineRules;
    const auto Easy=HunterDifficulty(0),Medium=HunterDifficulty(1),Hard=HunterDifficulty(2);
    TestTrue(TEXT("Higher difficulty shortens reactions and shot intervals"),Easy.ReactionTime>Medium.ReactionTime&&Medium.ReactionTime>Hard.ReactionTime&&Easy.ShotDelay>Medium.ShotDelay&&Medium.ShotDelay>Hard.ShotDelay);
    TestTrue(TEXT("Hard is more accurate, faster and sees farther"),Easy.AimError>Medium.AimError&&Medium.AimError>Hard.AimError&&Easy.WalkSpeed<Medium.WalkSpeed&&Medium.WalkSpeed<Hard.WalkSpeed&&Easy.SightRange<Medium.SightRange&&Medium.SightRange<Hard.SightRange);
    TestTrue(TEXT("Easy reveals every acorn and hunter even without sight"),RevealAcorn(0,MapRadius*2)&&RevealHunter(0,MapRadius*2,false));
    TestTrue(TEXT("Medium shows nearby forage and visible nearby hunters"),RevealAcorn(1,4000)&&RevealHunter(1,2000,true));
    TestFalse(TEXT("Medium hides distant forage"),RevealAcorn(1,5000));
    TestFalse(TEXT("Medium hides hunters behind cover"),RevealHunter(1,2000,false));
    TestFalse(TEXT("Medium hides distant hunters"),RevealHunter(1,4000,true));
    TestTrue(TEXT("Hard keeps nearby forage discoverable"),RevealAcorn(2,1800));
    TestFalse(TEXT("Hard hides distant forage"),RevealAcorn(2,3000));
    TestFalse(TEXT("Hard never reveals hunters"),RevealHunter(2,100,true));
    TestEqual(TEXT("Saved default is medium"),GetDefault<UWildlineUserSettings>()->HunterDifficulty,1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWildlineForageTest,"Wildline.Gameplay.ForagingAndRockCollision",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWildlineForageTest::RunTest(const FString& Parameters){
    const FVector Acorn(100,200,10);
    TestTrue(TEXT("Pickups allow a forgiving ground approach"),WildlineRules::CanPickAcorn(Acorn+FVector(190,0,13),Acorn));
    TestFalse(TEXT("Cannot collect acorns from high branches"),WildlineRules::CanPickAcorn(Acorn+FVector(0,0,250),Acorn));
    TestFalse(TEXT("Cannot collect faraway acorns"),WildlineRules::CanPickAcorn(Acorn+FVector(250,0,13),Acorn));
    const auto* Rocks=GetDefault<AWildlineWorld>()->Rocks;
    TestEqual(TEXT("Rocks enable physical collision"),Rocks->GetCollisionEnabled(),ECollisionEnabled::QueryAndPhysics);
    for(auto Channel:{ECC_Pawn,ECC_Visibility,ECC_Camera})TestEqual(TEXT("Rocks block players, bullets and cameras"),Rocks->GetCollisionResponseToChannel(Channel),ECR_Block);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWildlinePhotoHuntersTest,"Wildline.Hunters.PhotoPresets",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWildlinePhotoHuntersTest::RunTest(const FString& Parameters){
    auto P=FWildlineHunterProfile::Travis();
    TestTrue(TEXT("Travis uses the named photo head with no upload or extra beard"),P.Preset==1&&P.IsPhotoPreset()&&!P.bBeard&&!P.bUsePhoto);
    P.Outfit=2;P.Slimness=.2f;P.FaceWidth=-.4f;P.bCap=true;P.Clamp();
    TestTrue(TEXT("Appearance edits retain Travis"),P.Preset==1&&P.Outfit==2&&P.Slimness==.2f&&P.FaceWidth==-.4f&&P.bCap);
    P.Preset=2;P.bBeard=true;P.Clamp();
    TestTrue(TEXT("Saved Travis 2 migrates to Travis, preserving customization"),P.Preset==1&&P.Outfit==2&&P.Slimness==.2f&&!P.bBeard);
    auto E=FWildlineHunterProfile::Ethan();E.Clamp();
    TestTrue(TEXT("Ethan has a distinct stable ID and a built-in photo"),E.Preset==3&&E.IsPhotoPreset()&&!E.bUsePhoto&&!E.bBeard);
    E.Outfit=0;E.Slimness=.3f;E.Clamp();
    TestTrue(TEXT("Ethan remains selected after customization"),E.Preset==3&&E.Outfit==0&&E.Slimness==.3f);
    TestEqual(TEXT("Ethan uses the third visible option"),E.PresetOptionIndex(),2);
    TestEqual(TEXT("Travis uses the second visible option"),P.PresetOptionIndex(),1);
    E.Body=1;E.Clamp();TestFalse(TEXT("Female body uses the custom head controls"),E.IsPhotoPreset());
    auto G=FWildlineHunterProfile::Greyson();G.Clamp();
    TestTrue(TEXT("Greyson has a distinct stable ID and a built-in photo"),G.Preset==4&&G.IsPhotoPreset()&&!G.bUsePhoto&&!G.bBeard);
    TestEqual(TEXT("Greyson follows Ethan in the visible options"),G.PresetOptionIndex(),3);
    E.Body=0;
    TestTrue(TEXT("Ethan is 30 percent shorter than his previous 102 cm height"),FMath::IsNearlyEqual(E.HeightScale()*174.5f,102.f*.7f,.01f));
    TestTrue(TEXT("Greyson is 20 percent shorter than his previous 138 cm height"),FMath::IsNearlyEqual(G.HeightScale()*174.5f,138.f*.8f,.01f));
    TestEqual(TEXT("Travis retains adult height"),P.HeightScale(),1.f);
    auto Female=G;Female.Body=1;TestEqual(TEXT("Custom female body retains its original height"),Female.HeightScale(),1.f);
    TestTrue(TEXT("Both kids have smaller hands and feet relative to their bodies"),E.HandScale()<G.HandScale()&&G.HandScale()<1.f&&E.FootScale()<G.FootScale()&&G.FootScale()<1.f);
    TestTrue(TEXT("Adult proportions remain unchanged"),P.HandScale()==1.f&&P.FootScale()==1.f&&Female.HandScale()==1.f&&Female.FootScale()==1.f);
    G.Outfit=2;G.Slimness=.4f;G.FaceWidth=.2f;G.bCap=true;G.bBeard=true;G.Clamp();
    TestTrue(TEXT("Greyson customization retains the photo preset without an extra beard"),G.Preset==4&&G.Outfit==2&&G.Slimness==.4f&&G.FaceWidth==.2f&&G.bCap&&!G.bBeard);
    P.Preset=999;P.Slimness=10;P.Clamp();TestEqual(TEXT("Invalid preset clamps"),P.Preset,4);TestEqual(TEXT("Slimness bounded"),P.Slimness,1.f);
    const auto Reset=FWildlineHunterProfile::Travis();TestTrue(TEXT("Reset restores photo preset defaults"),Reset.Slimness>.7f&&Reset.Outfit==1&&!Reset.bBeard);
    TestEqual(TEXT("Existing hunter default stays custom"),FWildlineHunterProfile().Preset,0);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWildlineThermalTest,"Wildline.Gameplay.ThermalWindow",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWildlineThermalTest::RunTest(const FString& Parameters){
    TestTrue(TEXT("Hunter can activate initially"),WildlineRules::CanThermal(true,true,0,0));
    const float Activation=15,Until=Activation+WildlineRules::ThermalDuration,Ready=Until+WildlineRules::ThermalCooldown;
    TestEqual(TEXT("Thermal lasts exactly 20 seconds"),Until-Activation,20.f);
    TestFalse(TEXT("Cannot reuse at expiry"),WildlineRules::CanThermal(true,true,Until,Ready));
    TestFalse(TEXT("Cannot reuse just before cooldown ends"),WildlineRules::CanThermal(true,true,Ready-.01f,Ready));
    TestTrue(TEXT("Available exactly two minutes after expiry"),WildlineRules::CanThermal(true,true,Ready,Ready));
    TestFalse(TEXT("Squirrels cannot activate thermal"),WildlineRules::CanThermal(false,true,Ready,Ready));
    TestFalse(TEXT("Dead players cannot activate thermal"),WildlineRules::CanThermal(true,false,Ready,Ready));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWildlineViewTest,"Wildline.Controls.DefaultsAndInversion",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWildlineViewTest::RunTest(const FString& Parameters){
    const auto* Settings=GetDefault<UWildlineUserSettings>();const auto* Pawn=GetDefault<AWildlineCharacter>();
    TestTrue(TEXT("Y inversion defaults on"),Settings->bInvertMouseY);
    TestFalse(TEXT("Third person defaults off"),Pawn->bThirdPerson);
    TestTrue(TEXT("First person camera activates when registered"),Pawn->FirstCamera->bAutoActivate);
    TestFalse(TEXT("Third person camera does not auto activate"),Pawn->ThirdCamera->bAutoActivate);
    TestEqual(TEXT("Inverted input reverses pitch"),WildlineRules::PitchInput(1,true,.8f),-.8f);
    TestEqual(TEXT("Turning inversion off restores normal pitch"),WildlineRules::PitchInput(1,false,.8f),.8f);
    TestEqual(TEXT("Sensitivity applies in both directions"),WildlineRules::PitchInput(-2,false,.5f),-1.f);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWildlineRifleTest,"Wildline.Hunting.CollisionAndMagazine",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWildlineRifleTest::RunTest(const FString& Parameters){
    const auto* Pawn=GetDefault<AWildlineCharacter>();
    TestEqual(TEXT("Rifle traces can hit character capsules"),Pawn->GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Visibility),ECR_Block);
    TestEqual(TEXT("Five live rounds in the magazine"),Pawn->Ammo,5);
    TestTrue(TEXT("Bolt cycle blocks rapid firing"),WildlineRules::ShotInterval>=.75f);
    TestEqual(TEXT("Rifle reload duration"),WildlineRules::ReloadDuration,2.2f);return true;
}
#endif

#include "WildlineNeighborhoodConfig.h"
#include "WildlineMatchConfig.h"
#include "Engine/StaticMesh.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWildlineNeighborhoodAssetTest,"Wildline.Gameplay.NeighborhoodAssetsAndLayouts",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWildlineNeighborhoodAssetTest::RunTest(const FString& Parameters){
    for(const TCHAR* Name:{TEXT("MapleCrescent"),TEXT("CedarBackyards")}){
        const FString Path=FString::Printf(TEXT("/Game/Wildline/Residential/Configs/DA_%s.DA_%s"),Name,Name);
        const auto* C=LoadObject<UWildlineNeighborhoodConfig>(nullptr,*Path);if(!TestNotNull(TEXT("Cookable layout DataAsset"),C))continue;
        TestEqual(TEXT("Two distinct organic approaches"),C->Routes.Num(),2);TestTrue(TEXT("Full residential dressing"),C->Props.Num()>200);
        for(const auto& R:C->Routes){TestEqual(TEXT("Each approach has four ordered landings"),R.Targets.Num(),4);TestEqual(TEXT("HUD labels follow the authored route"),R.Labels.Num(),4);
            TestTrue(TEXT("Trampoline jump reaches the 1.5m rise"),R.TrampolineLaunchZ*R.TrampolineLaunchZ/(2*980.f*1.2f)>C->Feeder.Z-R.Targets.Last().Z);
            for(int32 I=1;I<R.Targets.Num();I++)TestTrue(TEXT("Ordinary upward jumps remain below their physics apex"),R.Targets[I].Z-R.Targets[I-1].Z<=360.f*360.f/(2*980.f*1.2f));
        }
        TSet<FString> Checked;for(const auto& P:C->Props)if(!Checked.Contains(P.Mesh)){Checked.Add(P.Mesh);TestNotNull(FString(TEXT("Available model: "))+P.Mesh,LoadObject<UStaticMesh>(nullptr,*P.Mesh));}
        for(const auto& P:C->Props){const FRotator R=P.Transform.Rotator();TestTrue(TEXT("Lawn props rotate around yaw without unintended tilt"),(P.Mesh.Contains(TEXT("SM_ServiceCable"))||FMath::Abs(R.Pitch)<.01f)&&FMath::Abs(R.Roll)<.01f);}
    }
    FWildlineMatchConfig A,B;B.NeighborhoodLayout=1;TestTrue(TEXT("Layout changes require a fresh selected map"),A!=B);B.NeighborhoodLayout=99;B.Clamp();TestEqual(TEXT("Unsupported layout indices clamp"),B.NeighborhoodLayout,1);
    return true;
}
#endif

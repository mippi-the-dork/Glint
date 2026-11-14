#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GlintLensProfile.h"
#include "GlintFlare.h"
#include "UObject/UObjectGlobals.h"
#include <type_traits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintDataLensDefaults,
    "Glint.Data.Lens.Defaults", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintDataLensDefaults::RunTest(const FString& Parameters)
{
    const UGlintLensProfile* Lens = NewObject<UGlintLensProfile>();
    const FGlintLensBlendData Data = Lens->MakeRuntimeData();
    TestTrue(TEXT("default intensity is positive"), Data.Intensity > 0.f);
    TestTrue(TEXT("default threshold is positive"), Data.Threshold > 0.f);
    TestTrue(TEXT("ghost spacing is positive"), Data.GhostSpacing > 0.f);
    TestTrue(TEXT("no UObject references in lens runtime data"), std::is_trivially_copy_constructible_v<FGlintLensBlendData>);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintDataFlareCount,
    "Glint.Data.Flare.Count", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintDataFlareCount::RunTest(const FString& Parameters)
{
    UGlintFlare* Flare = NewObject<UGlintFlare>();
    FGlintElement A; A.bEnabled = true;
    FGlintElement B; B.bEnabled = false;
    Flare->Elements.Add(A); Flare->Elements.Add(B);
    TestEqual(TEXT("disabled authoring elements excluded"), Flare->GetEnabledElementCount(), 1);
    return true;
}
#endif

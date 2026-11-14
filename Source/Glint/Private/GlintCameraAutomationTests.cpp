#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GlintCameraBindingComponent.h"
#include "GlintLensProfile.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintCameraBinding,
    "Glint.Camera.Binding.SetLens", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintCameraBinding::RunTest(const FString& Parameters)
{
    UGlintCameraBindingComponent* Binding = NewObject<UGlintCameraBindingComponent>();
    UGlintLensProfile* Lens = NewObject<UGlintLensProfile>();
    Binding->SetGlintLens(Lens, 3.f);
    TestTrue(TEXT("profile assigned"), Binding->LensProfile == Lens);
    TestEqual(TEXT("weight clamped"), Binding->BlendWeight, 1.f);
    Binding->SetGlintLens(nullptr, -1.f);
    TestEqual(TEXT("weight lower bound"), Binding->BlendWeight, 0.f);
    return true;
}
#endif

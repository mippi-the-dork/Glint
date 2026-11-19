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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintCameraLensTransition,
    "Glint.Camera.Binding.LensCrossfade", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintCameraLensTransition::RunTest(const FString& Parameters)
{
    UGlintCameraBindingComponent* Binding = NewObject<UGlintCameraBindingComponent>();
    UGlintLensProfile* First = NewObject<UGlintLensProfile>(Binding);
    UGlintLensProfile* Second = NewObject<UGlintLensProfile>(Binding);
    Binding->SetGlintLens(First);
    Binding->BlendToGlintLens(Second, 2.f);
    TestTrue(TEXT("transition starts"), Binding->IsGlintLensBlending());
    TestTrue(TEXT("previous lens retained"), Binding->GetOutgoingGlintLens() == First);
    float Old = 0.f, New = 0.f;
    Binding->GetGlintBlendWeights(Old, New);
    TestEqual(TEXT("initial old weight"), Old, 1.f);
    TestEqual(TEXT("initial new weight"), New, 0.f);
    Binding->AdvanceGlintBlend(1.f);
    Binding->GetGlintBlendWeights(Old, New);
    TestTrue(TEXT("half-way old weight"), FMath::IsNearlyEqual(Old, 0.5f));
    TestTrue(TEXT("half-way new weight"), FMath::IsNearlyEqual(New, 0.5f));
    Binding->AdvanceGlintBlend(1.f);
    Binding->GetGlintBlendWeights(Old, New);
    TestFalse(TEXT("transition finished"), Binding->IsGlintLensBlending());
    TestEqual(TEXT("finished old weight"), Old, 0.f);
    TestEqual(TEXT("finished new weight"), New, 1.f);
    TestTrue(TEXT("outgoing reference released"), Binding->GetOutgoingGlintLens() == nullptr);
    return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintCameraApertureTest,
    "Glint.Camera.Optics.PhysicalAperture", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintCameraApertureTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("reference aperture preserves size"),
        FMath::IsNearlyEqual(UGlintLensProfile::CalculateApertureScale(2.8f, 2.8f, 1.f), 1.f));
    TestTrue(TEXT("opening aperture enlarges response"),
        UGlintLensProfile::CalculateApertureScale(1.4f, 2.8f, 1.f) > 1.f);
    TestTrue(TEXT("closing aperture shrinks response"),
        UGlintLensProfile::CalculateApertureScale(5.6f, 2.8f, 1.f) < 1.f);
    TestTrue(TEXT("zero response ignores camera aperture"),
        FMath::IsNearlyEqual(UGlintLensProfile::CalculateApertureScale(22.f, 2.8f, 0.f), 1.f));
    return true;
}
#endif

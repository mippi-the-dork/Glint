#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GlintSourceSelection.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintSourceSeparationTest,
    "Glint.Source.Selection.Separation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintSourceSeparationTest::RunTest(const FString& Parameters)
{
    const TArray<FGlintSourcePoint> Candidates = {
        {FVector2f(0.40f, 0.40f), 6.f}, {FVector2f(0.41f, 0.41f), 5.f},
        {FVector2f(0.80f, 0.60f), 4.f}, {FVector2f(0.05f, 0.12f), 3.f}
    };
    const auto Sources = FGlintSourceSelection::Select(Candidates, 8, 0.1f, 0.04f);
    TestEqual(TEXT("neighboring maxima merged"), Sources.Num(), 3);
    if (!Sources.IsEmpty()) TestTrue(TEXT("brightest wins"), Sources[0].Brightness == 6.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintSourceBudgetTest,
    "Glint.Source.Selection.Budget", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintSourceBudgetTest::RunTest(const FString& Parameters)
{
    TArray<FGlintSourcePoint> Candidates;
    for (int32 Index=0; Index<30; ++Index)
        Candidates.Add({FVector2f(Index * 0.1f, 0.25f), float(Index + 1)});
    TestEqual(TEXT("GPU-equivalent eight source cap"),
        FGlintSourceSelection::Select(Candidates, 100, 0.01f, 0.02f).Num(), 8);
    TestEqual(TEXT("brightness threshold excludes all"),
        FGlintSourceSelection::Select(Candidates, 8, 100.f, 0.01f).Num(), 0);
    TestEqual(TEXT("zero limit excludes all"),
        FGlintSourceSelection::Select(Candidates, 0, 0.f, 0.01f).Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintSourceTemporalTest,
    "Glint.Source.Tracking.CameraCuts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintSourceTemporalTest::RunTest(const FString& Parameters)
{
    const TArray<FGlintSourcePoint> Previous = { {FVector2f(0.4f, 0.5f), 2.f} };
    TArray<FGlintSourcePoint> Selected = { {FVector2f(0.5f, 0.5f), 3.f} };
    FGlintSourceSelection::Smooth(Selected, Previous, 0.25f, false);
    TestTrue(TEXT("tracking blends positions"), FMath::IsNearlyEqual(Selected[0].UV.X, 0.425f, 0.001f));
    Selected[0].UV.X = 0.5f;
    FGlintSourceSelection::Smooth(Selected, Previous, 0.25f, true);
    TestTrue(TEXT("camera cut resets smoothing"), FMath::IsNearlyEqual(Selected[0].UV.X, 0.5f));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintSourcePersistenceTest,
    "Glint.Source.Tracking.MotionAndDropoutPersistence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintSourcePersistenceTest::RunTest(const FString& Parameters)
{
    const FGlintSourcePoint Sun{FVector2f(0.4f,0.5f), 8.f, 1.f, FLinearColor(8.f,6.f,3.f)};
    const TArray<FGlintSourcePoint> Previous{Sun};
    const TArray<FGlintSourcePoint> None;
    const TArray<FGlintSourcePoint> Held = FGlintSourceSelection::Stabilize(None, Previous, 8, 0.35f, 0.65f, false);
    TestEqual(TEXT("brief threshold loss retains source"), Held.Num(), 1);
    if (Held.Num() == 1)
    {
        TestTrue(TEXT("confidence decays rather than popping"), FMath::IsNearlyEqual(Held[0].Confidence, 0.65f));
        TestTrue(TEXT("HDR color survives missing sample"), Held[0].Color.Equals(Sun.Color));
    }
    TArray<FGlintSourcePoint> Fading = Previous;
    for (int32 Index=0; Index<6; ++Index)
        Fading = FGlintSourceSelection::Stabilize(None, Fading, 8, 0.35f, 0.65f, false);
    TestEqual(TEXT("stale sources fade away"), Fading.Num(), 0);
    TestEqual(TEXT("camera cuts clear even a valid history"),
        FGlintSourceSelection::Stabilize(None, Previous, 8, 0.35f, 0.65f, true).Num(), 0);
    const FGlintSourcePoint Moved{FVector2f(0.5f,0.5f), 9.f};
    const auto Resolved = FGlintSourceSelection::Stabilize({Moved}, Previous, 8, 0.35f, 0.65f, false);
    TestEqual(TEXT("motion uses one source, not a duplicate"), Resolved.Num(), 1);
    if (Resolved.Num() == 1)
        TestTrue(TEXT("rapid camera motion catches up"), Resolved[0].UV.X > 0.47f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintSourcePeakThresholdTest,
    "Glint.Source.Selection.MotionStablePeakGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintSourcePeakThresholdTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("lens threshold contributes to peak detection gate"),
        FMath::IsNearlyEqual(FGlintSourceSelection::CalculatePeakGate(0.025f, 1.75f, 1.f), 0.875f));
    TestTrue(TEXT("pre-exposure is applied once"),
        FMath::IsNearlyEqual(FGlintSourceSelection::CalculatePeakGate(0.025f, 1.75f, 0.5f), 0.4375f));
    TestTrue(TEXT("author minimum can override peak gate"),
        FMath::IsNearlyEqual(FGlintSourceSelection::CalculatePeakGate(3.f, 1.75f, 1.f), 3.f));
    return true;
}

#endif

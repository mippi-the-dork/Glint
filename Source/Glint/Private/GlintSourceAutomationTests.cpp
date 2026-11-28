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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintSourceGroupingTest,
    "Glint.Source.Selection.RegionalGrouping",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintSourceGroupingTest::RunTest(const FString& Parameters)
{
    const TArray<FGlintSourcePoint> Peaks = {
        {FVector2f(0.40f,0.5f), 12.f},
        {FVector2f(0.46f,0.5f), 11.f},
        {FVector2f(0.48f,0.52f), 10.f},
        {FVector2f(0.76f,0.5f), 9.f}
    };
    TestEqual(TEXT("prior tiny separation would split broad sun"),
        FGlintSourceSelection::Select(Peaks, 8, 0.1f, 0.04f).Num(), 3);
    const auto Grouped = FGlintSourceSelection::SelectGrouped(Peaks,8,0.1f,0.04f,0.125f);
    TestEqual(TEXT("one broad source and one separate lamp"), Grouped.Num(), 2);
    if (Grouped.Num() == 2)
    {
        TestTrue(TEXT("brightest peak retained"), FMath::IsNearlyEqual(Grouped[0].Brightness,12.f));
        TestTrue(TEXT("distant highlight preserved"), FMath::IsNearlyEqual(Grouped[1].UV.X,0.76f));
    }
    TestEqual(TEXT("group radius zero means legacy separation"),
        FGlintSourceSelection::SelectGrouped(Peaks,8,0.1f,0.04f,0.f).Num(), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintSourceEnergyLimitTest,
    "Glint.Source.Rendering.AuthoredEnergyShoulder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintSourceEnergyLimitTest::RunTest(const FString& Parameters)
{
    const FLinearColor Low(0.01f,0.01f,0.01f,1.f);
    const FLinearColor Medium(3.f,3.f,3.f,1.f);
    const FLinearColor Extreme(100.f,100.f,100.f,1.f);
    const auto A = FGlintSourceSelection::LimitAuthoredEnergy(Low,6.f);
    const auto B = FGlintSourceSelection::LimitAuthoredEnergy(Medium,6.f);
    const auto C = FGlintSourceSelection::LimitAuthoredEnergy(Extreme,6.f);
    TestTrue(TEXT("dim detail remains visible"),A.R > 0.009f);
    TestTrue(TEXT("bright stacking remains monotonic"), B.R > A.R && C.R > B.R);
    TestTrue(TEXT("large HDR contributions limited smoothly"), C.R < 6.f && C.R > 5.f);
    TestEqual(TEXT("zero disables limiter"),
        FGlintSourceSelection::LimitAuthoredEnergy(Extreme,0.f).R,100.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintAuthoredModeIsolationTest,
    "Glint.Source.Rendering.AuthoredModeIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintAuthoredModeIsolationTest::RunTest(const FString& Parameters)
{
    using Mode = EGlintAuthoredRenderMode;
    TestTrue(TEXT("blank flares never contribute authored effects"),
        FGlintSourceSelection::ResolveAuthoredMode(false, true, true, false) == Mode::Disabled);
    TestTrue(TEXT("source-driven uses analytic optics"),
        FGlintSourceSelection::ResolveAuthoredMode(true, true, true, false) == Mode::SourceTracked);
    TestTrue(TEXT("Source Driven Authored off suppresses authored, not automatic"),
        FGlintSourceSelection::ResolveAuthoredMode(true, false, true, false) == Mode::Disabled);
    TestTrue(TEXT("tracking disabled does not silently enable image warping"),
        FGlintSourceSelection::ResolveAuthoredMode(true, true, false, false) == Mode::Disabled);
    TestTrue(TEXT("explicit diagnostic opt-in restores only the legacy comparison"),
        FGlintSourceSelection::ResolveAuthoredMode(true, false, true, true) == Mode::LegacyImageField);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintSourceReleaseHysteresisTest,
    "Glint.Source.Tracking.ReleaseHysteresis",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintSourceReleaseHysteresisTest::RunTest(const FString& Parameters)
{
    const float Acquire = FGlintSourceSelection::CalculatePeakGate(0.025f,1.75f,1.f);
    const float Release = FGlintSourceSelection::CalculateReleaseGate(Acquire,0.65f);
    TestTrue(TEXT("established source has lower release gate"), Release < Acquire);
    TestTrue(TEXT("release gate is deterministic"), FMath::IsNearlyEqual(Release,0.56875f, 0.00001f));
    TestTrue(TEXT("hysteresis cannot produce negative gate"),
        FGlintSourceSelection::CalculateReleaseGate(-1.f,0.5f) == 0.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintAutomaticSourceIsolationTest,
    "Glint.Source.Rendering.AutomaticSourceIndependence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintAutomaticSourceIsolationTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("automatic optics track highlights without an authored flare"),
        FGlintSourceSelection::RequiresSources(false,true,false));
    TestTrue(TEXT("authoring checkbox cannot disable automatic source selection"),
        FGlintSourceSelection::RequiresSources(false,true,true));
    TestTrue(TEXT("authored source optics work with automatic disabled"),
        FGlintSourceSelection::RequiresSources(true,false,false));
    TestFalse(TEXT("no unnecessary source pass for disabled effects"),
        FGlintSourceSelection::RequiresSources(false,false,false));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintStableSourceSlotTest,
    "Glint.Source.Tracking.StableSourcesUnderJitter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintStableSourceSlotTest::RunTest(const FString& Parameters)
{
    const FGlintSourcePoint Sun{FVector2f(0.4f,0.4f), 9.f, 1.f, FLinearColor(9.f,8.f,6.f)};
    const FGlintSourcePoint Nearby{FVector2f(0.401f,0.4005f), 6.f, 1.f, FLinearColor(5.f,5.f,4.f)};
    const auto Preserved = FGlintSourceSelection::StabilizeHysteresis(
        {Nearby}, {Sun}, 8, 8.f, 0.65f, 0.35f, 0.65f, 0.0015f, false);
    TestEqual(TEXT("below-acquisition but above-release source stays tracked"), Preserved.Num(), 1);
    if (Preserved.Num() == 1)
    {
        TestTrue(TEXT("jitter dead zone keeps the optical center stationary"),
            Preserved[0].UV.Equals(Sun.UV,0.000001f));
        TestTrue(TEXT("brightness evolves rather than snapping to transient peak"),
            Preserved[0].Brightness > Nearby.Brightness && Preserved[0].Brightness < Sun.Brightness);
    }
    const auto NotYetAcquired = FGlintSourceSelection::StabilizeHysteresis(
        {Nearby}, {}, 8, 8.f, 0.65f, 0.35f, 0.65f, 0.0015f, false);
    TestEqual(TEXT("untracked sub-acquisition highlight cannot cause a new flare"),
        NotYetAcquired.Num(), 0);
    const auto Acquired = FGlintSourceSelection::StabilizeHysteresis(
        {Sun}, {}, 8, 8.f, 0.65f, 0.35f, 0.65f, 0.0015f, false);
    TestEqual(TEXT("bright source is acquired"), Acquired.Num(), 1);
    const auto Dropped = FGlintSourceSelection::StabilizeHysteresis(
        {}, {Sun}, 8, 8.f, 0.65f, 0.35f, 0.65f, 0.0015f, false);
    TestEqual(TEXT("one missing frame does not immediately pop"), Dropped.Num(), 1);
    if (Dropped.Num() == 1)
        TestTrue(TEXT("missing sample confidence attenuates"), Dropped[0].Confidence < 1.f);
    const auto Cut = FGlintSourceSelection::StabilizeHysteresis(
        {}, {Sun}, 8, 8.f, 0.65f, 0.35f, 0.65f, 0.0015f, true);
    TestEqual(TEXT("camera cuts discard source history"), Cut.Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintStationarySourceIdentityTest,
    "Glint.Source.Tracking.StationarySourceIdentity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintStationarySourceIdentityTest::RunTest(const FString& Parameters)
{
    const FGlintSourcePoint Old{FVector2f(0.40f,0.50f),10.f,1.f,FLinearColor::White};
    // A momentarily brighter reflection 150px away must not steal a source
    // when the camera rotation did not change.
    const FGlintSourcePoint Close{FVector2f(0.404f,0.501f),8.f,1.f,FLinearColor::White};
    const FGlintSourcePoint Far{FVector2f(0.50f,0.50f),80.f,1.f,FLinearColor::White};
    const auto Still = FGlintSourceSelection::StabilizeHysteresis(
        {Far,Close},{Old},1,6.f,0.65f,0.35f,0.65f,0.0015f,false,0.f);
    TestEqual(TEXT("stationary source retains one slot"), Still.Num(),1);
    if (Still.Num()==1)
        TestTrue(TEXT("brightness competitor cannot teleport old source"), Still[0].UV.X<0.405f);
    const auto Moving = FGlintSourceSelection::StabilizeHysteresis(
        {Far},{Old},1,6.f,0.65f,0.35f,0.65f,0.0015f,false,1.f);
    TestEqual(TEXT("moving camera can reacquire distant source"), Moving.Num(),1);
    if (Moving.Num()==1)
        TestTrue(TEXT("moving camera does not hijack a distant highlight"),
            FMath::IsNearlyEqual(Moving[0].UV.X, Old.UV.X, 0.00001f));
    TestEqual(TEXT("unmoving camera has zero motion factor"),
        FGlintSourceSelection::CameraMotionFactor(0.f),0.f);
    TestEqual(TEXT("two degree camera rotation permits fast source response"),
        FGlintSourceSelection::CameraMotionFactor(0.04f),1.f);
    TestEqual(TEXT("camera translation permits fast reacquisition"),
        FGlintSourceSelection::CameraTranslationFactor(60.f),1.f);
    TestEqual(TEXT("stationary camera translation is zero"),
        FGlintSourceSelection::CameraTranslationFactor(0.f),0.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintStationaryHDRRetentionTest,
    "Glint.Source.Tracking.StationaryHDRRetention",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintStationaryHDRRetentionTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("bright local HDR keeps static optical center"),
        FGlintSourceSelection::ShouldHoldStationarySource(5.f,3.f,0.f));
    TestFalse(TEXT("locally occluded source must not be held indefinitely"),
        FGlintSourceSelection::ShouldHoldStationarySource(0.2f,3.f,0.f));
    TestFalse(TEXT("camera motion must permit source tracking"),
        FGlintSourceSelection::ShouldHoldStationarySource(9.f,3.f,0.3f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintDynamicResolutionHistoryTest,
    "Glint.Source.Tracking.DynamicResolutionHistory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintDynamicResolutionHistoryTest::RunTest(const FString& Parameters)
{
    using Selection = FGlintSourceSelection;
    TestFalse(TEXT("TAA/upscaling render size change retains normalized UV history"),
        Selection::ShouldResetHistory(FIntPoint(1920,1080),FIntPoint(1728,972),false,true));
    TestFalse(TEXT("slight dynamic-resolution fluctuation retains tracking"),
        Selection::ShouldResetHistory(FIntPoint(1728,972),FIntPoint(1716,966),false,true));
    TestTrue(TEXT("camera cut discards history even with same size"),
        Selection::ShouldResetHistory(FIntPoint(1920,1080),FIntPoint(1920,1080),true,true));
    TestTrue(TEXT("no previous GPU texture initializes empty history"),
        Selection::ShouldResetHistory(FIntPoint(1920,1080),FIntPoint(1920,1080),false,false));
    TestTrue(TEXT("meaningful viewport aspect change clears history"),
        Selection::ShouldResetHistory(FIntPoint(1920,1080),FIntPoint(1920,1200),false,true));
    TestTrue(TEXT("invalid size resets history"),
        Selection::ShouldResetHistory(FIntPoint::ZeroValue,FIntPoint(1920,1080),false,true));
    return true;
}

#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintRegionRankTest,
    "Glint.Source.Detection.RegionalRanking",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintRegionRankTest::RunTest(const FString& Parameters)
{
    const float Narrow = FGlintSourceSelection::CalculateDetectionRank(100.f, 9.f);
    const float Flat = FGlintSourceSelection::CalculateDetectionRank(9.f, 9.f);
    TestTrue(TEXT("small intense emitters survive spatial filtering"), Narrow > Flat);
    TestTrue(TEXT("diffuse light regions remain eligible"), Flat > 0.f);
    TestEqual(TEXT("empty HDR never yields a candidate"), FGlintSourceSelection::CalculateDetectionRank(0.f, 0.f), 0.f);
    const float Regional = FGlintSourceSelection::CalculateDetectionRank(10.f, 7.f);
    TestTrue(TEXT("distinct compact HDR point wins against similarly bright flat cloud"),
        FGlintSourceSelection::CalculateDetectionRank(10.f, 1.f) > Regional);
    TestTrue(TEXT("broad, diffuse region still produces valid ranking evidence"), Regional > 0.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintRelativeScoreGateTest,
    "Glint.Source.Detection.RelativeScoreGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintRelativeScoreGateTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("weak cloud can be excluded relative to dominant sun"),
        1.f < FGlintSourceSelection::CalculateRelativeScoreFloor(10.f, 0.22f));
    TestTrue(TEXT("distinct plausible source remains eligible"),
        5.f > FGlintSourceSelection::CalculateRelativeScoreFloor(10.f, 0.22f));
    TestEqual(TEXT("zero floor means no relative suppression"),
        FGlintSourceSelection::CalculateRelativeScoreFloor(10.f, 0.f), 0.f);
    TestEqual(TEXT("ratio is bounded"),
        FGlintSourceSelection::CalculateRelativeScoreFloor(10.f, 12.f), 10.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintSourceChallengerTest,
    "Glint.Source.Tracking.SustainedChallenger",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintSourceChallengerTest::RunTest(const FString& Parameters)
{
    const FVector2f Sun(0.40f,0.30f);
    int32 Count = 0;
    Count = FGlintSourceSelection::AdvanceChallenger(Sun,Sun,Count,0.01f);
    TestFalse(TEXT("one-frame HDR flash cannot evict source"),
        FGlintSourceSelection::CanReplaceSource(10.f,4.f,1.7f,Count,3));
    Count = FGlintSourceSelection::AdvanceChallenger(Sun,Sun,Count,0.01f);
    TestFalse(TEXT("two frames insufficient by default"),
        FGlintSourceSelection::CanReplaceSource(10.f,4.f,1.7f,Count,3));
    Count = FGlintSourceSelection::AdvanceChallenger(Sun,Sun,Count,0.01f);
    TestTrue(TEXT("persistent dominant sun can replace a weak tracked cloud"),
        FGlintSourceSelection::CanReplaceSource(10.f,4.f,1.7f,Count,3));
    TestFalse(TEXT("same-pixel TAA wobble never counts as nearby competing emitter"),
        FGlintSourceSelection::IsDistinctChallenger(0.003f,0.0015f));
    TestTrue(TEXT("nearby independent sun can challenge grouped cloud"),
        FGlintSourceSelection::IsDistinctChallenger(0.08f,0.0015f));
    TestFalse(TEXT("only slight rank advantage must not evict"),
        FGlintSourceSelection::CanReplaceSource(5.f,4.f,1.7f,Count,3));
    Count = FGlintSourceSelection::AdvanceChallenger(Sun,FVector2f(0.8f,0.7f),Count,0.01f);
    TestEqual(TEXT("different bright object resets challenge"), Count,1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintDetectionBlurParityTest,
    "Glint.Source.Detection.FilteredHDR",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintDetectionBlurParityTest::RunTest(const FString& Parameters)
{
    TArray<float> Grid;
    Grid.Init(0.f,49);
    Grid[3*7+3] = 100.f; // Tiny high-energy sun; its RAW peak must remain 100.
    const float RawPeak = Grid[3*7+3];
    TestEqual(TEXT("filter off retains raw center sample"),
        FGlintSourceSelection::FilterDetectionLuminance(Grid,FIntPoint(7,7),FIntPoint(3,3),0),100.f);
    TestTrue(TEXT("3x3 filter reduces a subpixel fluctuation without deleting raw peak"),
        FMath::IsNearlyEqual(FGlintSourceSelection::FilterDetectionLuminance(
            Grid,FIntPoint(7,7),FIntPoint(3,3),1),25.f));
    TestTrue(TEXT("raw sun remains bright after filtering"),
        RawPeak > FGlintSourceSelection::FilterDetectionLuminance(Grid,FIntPoint(7,7),FIntPoint(3,3),2));
    TestEqual(TEXT("no out-of-frame bright border replication"),
        FGlintSourceSelection::FilterDetectionLuminance(Grid,FIntPoint(7,7),FIntPoint(0,0),1),0.f);
    for (float& V : Grid) V=8.f;
    TestTrue(TEXT("uniform luminous area keeps coherent intensity"),
        FMath::IsNearlyEqual(FGlintSourceSelection::FilterDetectionLuminance(
            Grid,FIntPoint(7,7),FIntPoint(0,0),2),8.f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintCompactSunPriorityTest,
    "Glint.Source.Detection.CompactSunPriority",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintCompactSunPriorityTest::RunTest(const FString& Parameters)
{
    // The same raw peak may be inside a broad region and a tiny disk. A
    // compact disk should be ranked more strongly rather than suppressed by
    // a high average luminance in a nearby ground/cloud area.
    const float SunRank = FGlintSourceSelection::CalculateDetectionRank(20.f,2.f);
    const float CloudRank = FGlintSourceSelection::CalculateDetectionRank(16.f,13.f);
    TestTrue(TEXT("compact sun outprioritizes broad cloud at similar HDR peak"),SunRank>CloudRank);
    TestTrue(TEXT("unchanged flat clouds retain finite scores"),CloudRank>0.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintCameraReprojectionParityTest,
    "Glint.Source.Tracking.CameraReprojection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintCameraReprojectionParityTest::RunTest(const FString& Parameters)
{
    const FVector2f OldUV(0.25f,0.6f);
    const FVector2f IdentityResult = FGlintSourceSelection::ReprojectSourceUV(OldUV,FMatrix::Identity);
    TestTrue(TEXT("a stationary unjittered view keeps source UV"),
        FVector2f::DistSquared(IdentityResult,OldUV)<1.0e-7f);
    FMatrix ShiftClip = FMatrix::Identity;
    ShiftClip.M[3][0] = 0.2; // Row-vector multiplication, shift +0.1 viewport UV.
    ShiftClip.M[3][1] = 0.1; // Screen UV runs downward, shift -0.05 viewport UV.
    const FVector2f Shifted = FGlintSourceSelection::ReprojectSourceUV(OldUV,ShiftClip);
    TestTrue(TEXT("camera-induced clip movement is compensated before tracking"),
        FMath::IsNearlyEqual(Shifted.X,0.35f,0.0001f) &&
        FMath::IsNearlyEqual(Shifted.Y,0.55f,0.0001f));
    FMatrix BehindCamera = FMatrix::Identity;
    BehindCamera.M[3][3] = -1.0;
    TestTrue(TEXT("invalid projection leaves old UV unchanged"),
        FVector2f::DistSquared(FGlintSourceSelection::ReprojectSourceUV(OldUV,BehindCamera),OldUV)<1.0e-7f);
    return true;
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintCompactGroupingSunTest,
    "Glint.Source.Detection.CompactGroupingSun",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintCompactGroupingSunTest::RunTest(const FString& Parameters)
{
    constexpr float Broad = 0.125f;
    constexpr float Independent = 0.04f;
    const float CloudRadius = FGlintSourceSelection::CalculateCompactGroupingRadius(8.f,7.8f,Independent,Broad);
    const float SunRadius = FGlintSourceSelection::CalculateCompactGroupingRadius(80.f,8.f,Independent,Broad);
    TestTrue(TEXT("broad cloud retains broad grouping"), CloudRadius > 0.11f);
    TestTrue(TEXT("sun disk has compact grouping"), SunRadius < 0.045f);
    TestTrue(TEXT("sun 0.07 UV away is not merged into nearby cloud"), SunRadius < 0.07f && CloudRadius > 0.07f);
    TestTrue(TEXT("camera motion cannot force a teleport snap"),
        FGlintSourceSelection::CalculateTrackingResponse(0.35f,1.f) < 0.56f);
    TestTrue(TEXT("stationary and moving view share the same correction limit"),
        FMath::IsNearlyEqual(FGlintSourceSelection::CalculateTrackingResponse(0.35f,0.f),
                             FGlintSourceSelection::CalculateTrackingResponse(0.35f,1.f)));
    return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintAtmosphereSunGateTest,
    "Glint.Source.Detection.AtmosphereSunHDRGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintAtmosphereSunGateTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("bright compact sunlight is eligible independently of cloud candidates"),
        FGlintSourceSelection::ShouldAcceptAtmosphereSun(40.f, 5.f, 0.875f, true));
    TestFalse(TEXT("cloud without bright concentrated disk is not automatically the sun"),
        FGlintSourceSelection::ShouldAcceptAtmosphereSun(6.f, 5.f, 0.875f, true));
    TestFalse(TEXT("default atmospheric direction must not create phantom sun"),
        FGlintSourceSelection::ShouldAcceptAtmosphereSun(40.f, 5.f, 0.875f, false));
    TestFalse(TEXT("too dim disk cannot bypass source acquisition"),
        FGlintSourceSelection::ShouldAcceptAtmosphereSun(0.4f, 0.02f, 0.875f, true));
    return true;
}
#endif

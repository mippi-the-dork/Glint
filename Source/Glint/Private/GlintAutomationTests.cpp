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
    TestTrue(TEXT("automatic rendering enabled by default"), Data.bEnableAutomatic);
    TestTrue(TEXT("runtime data remains UObject-free, trivially copyable"), std::is_trivially_copy_constructible_v<FGlintLensBlendData>);
    TestEqual(TEXT("default flare has no rendered elements"), Data.ElementCount, uint32(0));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintDataElementPacking,
    "Glint.Data.Flare.RuntimePacking", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintDataElementPacking::RunTest(const FString& Parameters)
{
    UGlintLensProfile* Lens = NewObject<UGlintLensProfile>();
    UGlintFlare* Flare = NewObject<UGlintFlare>(Lens);
    Lens->DetailedFlare = Flare;
    FGlintElement Glow;
    Glow.Type = EGlintElementType::Glow;
    Glow.bEnabled = false;
    Flare->Elements.Add(Glow);
    FGlintElement Ring;
    Ring.Type = EGlintElementType::Ring;
    Ring.Color = FLinearColor(0.2f, 0.3f, 0.4f);
    Ring.Intensity = 2.f;
    Ring.Size = 3.f;
    Ring.AxisPosition = -1.5f;
    Ring.RotationDegrees = 90.f;
    Ring.ScreenOffset = FVector2D(0.12, -0.08);
    Flare->Elements.Add(Ring);
    const FGlintLensBlendData Packed = Lens->MakeRuntimeData();
    TestEqual(TEXT("disabled elements excluded"), Packed.ElementCount, uint32(1));
    TestEqual(TEXT("type packing preserves enum order"), Packed.Elements[0].Geometry.X, float(static_cast<uint8>(EGlintElementType::Ring)));
    TestEqual(TEXT("axis position preserved"), Packed.Elements[0].Geometry.Y, -1.5f);
    TestEqual(TEXT("size preserved"), Packed.Elements[0].Geometry.Z, 3.f);
    TestTrue(TEXT("rotation converted to radians"), FMath::IsNearlyEqual(Packed.Elements[0].Geometry.W, UE_HALF_PI, 0.0001f));
    TestEqual(TEXT("R packed"), Packed.Elements[0].ColorStrength.X, 0.2f);
    TestEqual(TEXT("element intensity packed"), Packed.Elements[0].ColorStrength.W, 2.f);
    TestEqual(TEXT("offset X GPU packed"), Packed.Elements[0].Offset.X, 0.12f);
    TestEqual(TEXT("offset Y GPU packed"), Packed.Elements[0].Offset.Y, -0.08f);
    FGlintElement Offscreen=Ring;
    Offscreen.ScreenOffset=FVector2D(-2.5,3.1);
    const auto OffscreenPacked=UGlintLensProfile::PackElement(Offscreen);
    TestEqual(TEXT("offscreen X retained in renderer"),OffscreenPacked.Offset.X,-2.5f);
    TestEqual(TEXT("offscreen Y retained in renderer"),OffscreenPacked.Offset.Y,3.1f);
    TestEqual(TEXT("element GPU footprint matches HLSL five float4 layout"), sizeof(FGlintRuntimeElement), SIZE_T(80));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintDataElementBudget,
    "Glint.Data.Flare.ElementBudget", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintDataElementBudget::RunTest(const FString& Parameters)
{
    UGlintLensProfile* Lens = NewObject<UGlintLensProfile>();
    UGlintFlare* Flare = NewObject<UGlintFlare>(Lens);
    Lens->DetailedFlare = Flare;
    // Exercise an actual overflow: the authored array MUST exceed the runtime cap.
    // The old fixture supplied only 30 entries while expecting the new 32-entry cap.
    const int32 AuthoredCount = FGlintLensBlendData::MaxElementsPerLens + 8;
    for (int32 Index = 0; Index < AuthoredCount; ++Index)
    {
        FGlintElement Element;
        Element.bEnabled = true;
        Element.Size = 100.f;
        Element.Intensity = 1.f + 0.01f * Index;
        Flare->Elements.Add(Element);
    }
    const FGlintLensBlendData Data = Lens->MakeRuntimeData();
    TestEqual(TEXT("per-lens runtime cap"), Data.ElementCount, uint32(FGlintLensBlendData::MaxElementsPerLens));
    TestEqual(TEXT("size sanitized"), Data.Elements[0].Geometry.Z, 8.f);
    TestEqual(TEXT("first element retained"), Data.Elements[0].ColorStrength.W, 1.f);
    TestTrue(TEXT("last in-budget element retained"),
        FMath::IsNearlyEqual(Data.Elements[FGlintLensBlendData::MaxElementsPerLens-1].ColorStrength.W,
            1.f + 0.01f * (FGlintLensBlendData::MaxElementsPerLens-1)));
    TestEqual(TEXT("all enabled authoring elements remain in asset"),
        Flare->GetEnabledElementCount(), AuthoredCount);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintDataLensSanitize,
    "Glint.Data.Lens.RuntimeSanitization", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintDataLensSanitize::RunTest(const FString& Parameters)
{
    UGlintLensProfile* Lens = NewObject<UGlintLensProfile>();
    Lens->Intensity = -5.f;
    Lens->Threshold = -3.f;
    Lens->PeakMix = 2.f;
    Lens->SoftKnee = -2.f;
    Lens->GhostCount = 100;
    Lens->StreakLength = 0.f;
    Lens->bEnableAutomatic = false;
    const FGlintLensBlendData Data = Lens->MakeRuntimeData();
    TestEqual(TEXT("intensity clamped"), Data.Intensity, 0.f);
    TestEqual(TEXT("threshold clamped"), Data.Threshold, 0.f);
    TestEqual(TEXT("peak mix clamped"), Data.PeakMix, 1.f);
    TestEqual(TEXT("knee clamped"), Data.SoftKnee, 0.f);
    TestEqual(TEXT("ghost count clamped"), Data.GhostCount, 8);
    TestEqual(TEXT("streak length clamped"), Data.StreakLength, 0.25f);
    TestFalse(TEXT("automatic disable preserved"), Data.bEnableAutomatic);
    return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintSourceControls,
    "Glint.Data.SourceControls.Sanitize", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintSourceControls::RunTest(const FString& Parameters)
{
    UGlintLensProfile* Lens = NewObject<UGlintLensProfile>();
    Lens->MaxSources = 100;
    Lens->SourceMinBrightness = -10.f;
    Lens->SourceSeparation = 2.f;
    Lens->SourceSmoothing = -1.f;
    Lens->SourceEdgeFade = 0.99f;
    Lens->SourceGain = -20.f;
    Lens->SourcePersistence = 3.f;
    const FGlintLensBlendData Packed = Lens->MakeRuntimeData();
    TestEqual(TEXT("edge fade clamped"), Packed.SourceEdgeFade, 0.25f);
    TestEqual(TEXT("source gain clamped"), Packed.SourceGain, 0.f);
    TestEqual(TEXT("source persistence clamped"), Packed.SourcePersistence, 0.95f);
    TestTrue(TEXT("source rendering enabled by default"), Packed.bSourceDrivenAuthored);
    TestEqual(TEXT("source budget capped at eight"), Packed.MaxSources, 8);
    TestEqual(TEXT("minimum source brightness clamped"), Packed.SourceMinBrightness, 0.001f);
    TestEqual(TEXT("source separation clamped"), Packed.SourceSeparation, 0.25f);
    TestEqual(TEXT("source response clamped"), Packed.SourceSmoothing, 0.f);
    return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintEditorPreviewPacking,
    "Glint.Data.Flare.PreviewPacking", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintEditorPreviewPacking::RunTest(const FString& Parameters)
{
    FGlintElement E;
    E.Type = EGlintElementType::MultiIris;
    E.AxisPosition = 5.f;
    E.Size = 0.01f;
    E.RotationDegrees = 180.f;
    E.Intensity = -1.f;
    const FGlintRuntimeElement Packed = UGlintLensProfile::PackElement(E);
    TestEqual(TEXT("preview and runtime share type encoding"), Packed.Geometry.X, 8.f);
    TestEqual(TEXT("invalid axis sanitized"), Packed.Geometry.Y, 4.f);
    TestEqual(TEXT("invalid size sanitized"), Packed.Geometry.Z, 0.05f);
    TestEqual(TEXT("negative intensity sanitized"), Packed.ColorStrength.W, 0.f);
    TestTrue(TEXT("angle converted to radians"), FMath::IsNearlyEqual(Packed.Geometry.W, PI));
    return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintTypeSpecificDefaults,
    "Glint.Data.Flare.TypeSpecificDefaults", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGlintTypeSpecificDefaults::RunTest(const FString& Parameters)
{
    const FGlintElement Glow=UGlintFlare::MakeDefaultElement(EGlintElementType::Glow);
    const FGlintElement Ghost=UGlintFlare::MakeDefaultElement(EGlintElementType::Ghost);
    const FGlintElement Streak=UGlintFlare::MakeDefaultElement(EGlintElementType::Streak);
    const FGlintElement Ring=UGlintFlare::MakeDefaultElement(EGlintElementType::Ring);
    TestTrue(TEXT("source glow is placed on its light"),Glow.AxisPosition>0.9f);
    TestTrue(TEXT("reflected ghost lives across optical center"),Ghost.AxisPosition<0.f);
    TestTrue(TEXT("streak is anisotropic"),Streak.AspectRatio>1.0f);
    TestTrue(TEXT("ring has a deliberate thin edge"),Ring.RingThickness<0.2f);
    for (int32 Kind=0;Kind<=static_cast<int32>(EGlintElementType::DustCluster);++Kind)
    {
        const EGlintElementType Type=static_cast<EGlintElementType>(Kind);
        const FGlintElement Element=UGlintFlare::MakeDefaultElement(Type);
        TestEqual(TEXT("default type matches request"),int32(Element.Type),Kind);
        TestTrue(TEXT("default element is useful at creation"),Element.Intensity>0.f && Element.Size>0.f && !Element.Label.IsNone());
        TestTrue(TEXT("default axis stays within authored bounds"),FMath::Abs(Element.AxisPosition)<=4.f);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintElementShapePayload,
    "Glint.Data.Flare.ShapeParameterPacking", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGlintElementShapePayload::RunTest(const FString& Parameters)
{
    FGlintElement Element=UGlintFlare::MakeDefaultElement(EGlintElementType::Sparkle);
    Element.Falloff=2.4f; Element.RingThickness=0.28f; Element.AspectRatio=1.8f;
    Element.RayCount=10; Element.RaySharpness=80.f; Element.SecondaryStrength=0.75f;
    Element.DetailFrequency=13.f;
    const FGlintRuntimeElement Packed=UGlintLensProfile::PackElement(Element);
    TestEqual(TEXT("soft falloff reaches shader"),Packed.Shape.X,2.4f);
    TestEqual(TEXT("ring width reaches shader"),Packed.Shape.Y,0.28f);
    TestEqual(TEXT("aspect reaches shader"),Packed.Shape.Z,1.8f);
    TestEqual(TEXT("ray count reaches shader"),Packed.Shape.W,10.f);
    TestEqual(TEXT("ray sharpness reaches shader"),Packed.Detail.X,80.f);
    TestEqual(TEXT("secondary strength reaches shader"),Packed.Detail.Y,0.75f);
    TestEqual(TEXT("detail frequency reaches shader"),Packed.Detail.Z,13.f);
    return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintExpandedElementFamilies,
    "Glint.Data.Flare.ExpandedFamilies",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGlintExpandedElementFamilies::RunTest(const FString& Parameters)
{
    const int32 NumTypes = static_cast<int32>(EGlintElementType::DustCluster) + 1;
    TestEqual(TEXT("26 element families are available"),NumTypes,26);
    TestTrue(TEXT("every default element fits into a single lens"),
        NumTypes<=FGlintLensBlendData::MaxElementsPerLens);
    for(int32 i=0;i<NumTypes;++i)
    {
        const EGlintElementType Type=static_cast<EGlintElementType>(i);
        const FGlintElement A=UGlintFlare::MakeDefaultElement(Type);
        TestEqual(TEXT("default type stable"),int32(A.Type),i);
        TestTrue(TEXT("default element has a useful nonzero contribution"),
            A.Intensity>0.f && A.Size>0.f);
        TestFalse(TEXT("known type has a name"),FString(UGlintFlare::GetTypeName(Type)).Equals(TEXT("Unknown")));
        const auto Packed=UGlintLensProfile::PackElement(A);
        TestEqual(TEXT("HLSL element index stable"),int32(Packed.Geometry.X),i);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintPresetVariants,
    "Glint.Data.Flare.PresetVariants",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGlintPresetVariants::RunTest(const FString& Parameters)
{
    for(int32 i=0;i<=static_cast<int32>(EGlintElementType::DustCluster);++i)
    {
        const auto Type=static_cast<EGlintElementType>(i);
        const FGlintElement Default=UGlintFlare::MakeDefaultElement(Type);
        const FGlintElement Subtle=UGlintFlare::MakePresetElement(Type,1);
        const FGlintElement Cinematic=UGlintFlare::MakePresetElement(Type,2);
        TestTrue(TEXT("subtle preset is less energetic"),Subtle.Intensity<Default.Intensity);
        TestTrue(TEXT("cinematic preset is more energetic"),Cinematic.Intensity>=Default.Intensity);
        TestEqual(TEXT("all preset types preserved"),int32(Cinematic.Type),i);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintExpandedLensBudget,
    "Glint.Data.Flare.LensBudget32",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGlintExpandedLensBudget::RunTest(const FString& Parameters)
{
    UGlintFlare* Flare=NewObject<UGlintFlare>();
    for(int32 i=0;i<55;++i)
        Flare->Elements.Add(UGlintFlare::MakeDefaultElement(EGlintElementType::Ghost));
    UGlintLensProfile* Lens=NewObject<UGlintLensProfile>();
    Lens->DetailedFlare=Flare;
    const auto Data=Lens->MakeRuntimeData();
    TestEqual(TEXT("packed per lens budget"),int32(Data.ElementCount),32);
    return true;
}
#endif

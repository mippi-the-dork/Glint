#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GlintLensProfile.h"
#include "GlintLensDetailsCustomization.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintLensResetToDefaultsTest,
    "Glint.Editor.Lens.PerPropertyResetDefaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintLensResetToDefaultsTest::RunTest(const FString& Parameters)
{
    UGlintLensProfile* Lens = NewObject<UGlintLensProfile>();
    const FFloatProperty* IntensityProperty = FindFProperty<FFloatProperty>(
        UGlintLensProfile::StaticClass(), GET_MEMBER_NAME_CHECKED(UGlintLensProfile, Intensity));
    const FFloatProperty* PersistenceProperty = FindFProperty<FFloatProperty>(
        UGlintLensProfile::StaticClass(), GET_MEMBER_NAME_CHECKED(UGlintLensProfile, SourcePersistence));
    TestTrue(TEXT("intensity UPROPERTY"), IntensityProperty != nullptr);
    TestTrue(TEXT("persistence UPROPERTY"), PersistenceProperty != nullptr);
    if (!IntensityProperty || !PersistenceProperty) return false;
    TestFalse(TEXT("fresh profile matches CDO"), GlintLensDefaults::DiffersFromClassDefault(Lens, IntensityProperty));
    Lens->Intensity = 2.5f;
    TestTrue(TEXT("modified property exposes reset arrow"), GlintLensDefaults::DiffersFromClassDefault(Lens, IntensityProperty));
    const FString ClassDefault = GlintLensDefaults::ExportClassDefault(IntensityProperty);
    TestTrue(TEXT("reset action uses C++ initial default"), FMath::IsNearlyEqual(FCString::Atof(*ClassDefault), 0.75f));
    IntensityProperty->SetPropertyValue_InContainer(Lens, FCString::Atof(*ClassDefault));
    TestFalse(TEXT("reset hides arrow after restoring CDO value"), GlintLensDefaults::DiffersFromClassDefault(Lens, IntensityProperty));
    Lens->SourcePersistence = 0.85f;
    TestTrue(TEXT("new source controls also expose reset"),
        GlintLensDefaults::DiffersFromClassDefault(Lens, PersistenceProperty));
    return true;
}
#endif

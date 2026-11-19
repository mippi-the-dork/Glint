#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GlintPreviewSelection.h"
#include "GlintElementDetailsProxy.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintVisualEditorHitTest,
    "Glint.Editor.Selection.OverlapCycle", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGlintVisualEditorHitTest::RunTest(const FString& Parameters)
{
    UGlintFlare* Flare=NewObject<UGlintFlare>();
    FGlintElement A;
    A.AxisPosition=1.f;
    FGlintElement B=A;
    Flare->Elements.Add(A);Flare->Elements.Add(B);
    const FVector2D Source(0.74,0.39),Extents(800,500);
    const FVector2D Marker=FGlintPreviewFrame::FromWorkspace(Extents).CameraToWorkspace(Source);
    const TArray<int32> Hits=FGlintPreviewSelectionOps::FindHits(*Flare,Source,Marker,Extents);
    TestEqual(TEXT("two overlapping markers found"),Hits.Num(),2);
    TestEqual(TEXT("first selection"),FGlintPreviewSelectionOps::CycleHit(Hits,INDEX_NONE),0);
    TestEqual(TEXT("second overlapping selection"),FGlintPreviewSelectionOps::CycleHit(Hits,0),1);
    TestEqual(TEXT("overlap cycling wraps"),FGlintPreviewSelectionOps::CycleHit(Hits,1),0);
    Flare->Elements[1].bEnabled=false;
    const TArray<int32> Visible=FGlintPreviewSelectionOps::FindHits(*Flare,Source,Marker,Extents);
    TestEqual(TEXT("hidden element not selectable"),Visible.Num(),1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintVisualEditorSolo,
    "Glint.Editor.Preview.SoloVisibility", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGlintVisualEditorSolo::RunTest(const FString& Parameters)
{
    UGlintFlare* Flare=NewObject<UGlintFlare>();
    // 16 elements exercised no cap at all once the budget grew to 24/32.
    // Create MORE than the supported budget to exercise actual truncation.
    for(int32 i=0;i<FGlintLensBlendData::MaxElementsPerLens+9;++i)
    {
        FGlintElement E; E.Intensity=1.f;
        Flare->Elements.Add(E);
    }
    TArray<FGlintRuntimeElement> Packed;
    FGlintPreviewSelectionOps::PackVisible(*Flare,INDEX_NONE,Packed);
    TestEqual(TEXT("preview cap matches runtime per-lens cap"),Packed.Num(),FGlintLensBlendData::MaxElementsPerLens);
    const int32 Last=FGlintLensBlendData::MaxElementsPerLens+8;
    FGlintPreviewSelectionOps::PackVisible(*Flare,Last,Packed);
    TestEqual(TEXT("solo works beyond the packed budget"),Packed.Num(),1);
    Flare->Elements[Last].bEnabled=false;
    FGlintPreviewSelectionOps::PackVisible(*Flare,Last,Packed);
    TestEqual(TEXT("disabled solo produces no contributions"),Packed.Num(),0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintVisualEditorDetails,
    "Glint.Editor.Preview.SelectionProxy", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGlintVisualEditorDetails::RunTest(const FString& Parameters)
{
    UGlintElementDetailsProxy* Proxy=NewObject<UGlintElementDetailsProxy>();
    Proxy->Element.Label=FName(TEXT("Outer Halo"));
    Proxy->Element.Type=EGlintElementType::Hoop;
    TestEqual(TEXT("native details edit proxy stores element label"),Proxy->Element.Label,FName(TEXT("Outer Halo")));
    TestEqual(TEXT("new element type is reflected"),int32(Proxy->Element.Type),7);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintEditorCameraFrame,
    "Glint.Editor.Preview.CameraFrameAndOffscreen", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGlintEditorCameraFrame::RunTest(const FString& Parameters)
{
    const FVector2D Workspace(1360.0,760.0);
    const FGlintPreviewFrame Frame=FGlintPreviewFrame::FromWorkspace(Workspace);
    TestTrue(TEXT("frame has a border around all four edges"),Frame.Origin.X>20 && Frame.Origin.Y>20 &&
             Frame.Origin.X+Frame.Size.X<Workspace.X-20 && Frame.Origin.Y+Frame.Size.Y<Workspace.Y-20);
    TestTrue(TEXT("frame matches GPU preview aspect"),FMath::IsNearlyEqual(Frame.Size.X/Frame.Size.Y,768.0/480.0,0.001));
    const FVector2D OffscreenUV(1.12,-0.09);
    const FVector2D RoundTrip=Frame.WorkspaceToCamera(Frame.CameraToWorkspace(OffscreenUV));
    TestTrue(TEXT("offscreen source coordinates round-trip without cropping"),
        RoundTrip.Equals(OffscreenUV,0.0001));
    TestFalse(TEXT("offscreen position is distinct from the camera frame"),Frame.IsInCamera(OffscreenUV));
    TestTrue(TEXT("on-screen position falls inside the frame"),Frame.IsInCamera(FVector2D(0.5,0.5)));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintEditorOffscreenHandle,
    "Glint.Editor.Preview.OffscreenHandlesRemainSelectable", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGlintEditorOffscreenHandle::RunTest(const FString& Parameters)
{
    UGlintFlare* Flare=NewObject<UGlintFlare>();
    FGlintElement E=UGlintFlare::MakeDefaultElement(EGlintElementType::Ghost);
    E.AxisPosition=-4.f;
    E.ScreenOffset.X=-2.f;
    Flare->Elements.Add(E);
    const FVector2D Workspace(900,540),Source(0.95,0.5);
    const FGlintPreviewFrame Frame=FGlintPreviewFrame::FromWorkspace(Workspace);
    const FVector2D Real=Frame.CameraToWorkspace(FGlintPreviewSelectionOps::ElementUV(E,Source));
    const FVector2D Handle=Frame.ConstrainHandle(Real);
    TestTrue(TEXT("off-workspace optics use a boundary marker"),Real.X<0 && Handle.X>=0);
    const TArray<int32> Hits=FGlintPreviewSelectionOps::FindHits(*Flare,Source,Handle,Workspace);
    TestEqual(TEXT("boundary marker remains selectable"),Hits.Num(),1);
    return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintEditorReferenceAspect,
    "Glint.Editor.Preview.ReferenceTextureAspect", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGlintEditorReferenceAspect::RunTest(const FString& Parameters)
{
    const FVector2D Area(1400,900);
    const double ScreenshotAspect=1920.0/1080.0;
    const FGlintPreviewFrame Frame=FGlintPreviewFrame::FromWorkspace(Area,ScreenshotAspect);
    TestTrue(TEXT("camera frame matches screenshot proportions"),FMath::IsNearlyEqual(Frame.Size.X/Frame.Size.Y,ScreenshotAspect,0.001));
    const FVector2D SourceUV(1.12,-0.08);
    const FVector2D Position=Frame.CameraToWorkspace(SourceUV);
    TestTrue(TEXT("offscreen source remains outside the image"),!Frame.IsInCamera(SourceUV));
    TestTrue(TEXT("non-default aspect preserves optical UV positions"),Frame.WorkspaceToCamera(Position).Equals(SourceUV,1.e-5));
    const FGlintPreviewFrame Portrait=FGlintPreviewFrame::FromWorkspace(Area,9.0/16.0);
    TestTrue(TEXT("portrait screenshot does not stretch"),FMath::IsNearlyEqual(Portrait.Size.X/Portrait.Size.Y,9.0/16.0,0.001));
    const FIntPoint PortraitTarget=FGlintPreviewFrame::TargetResolutionForAspect(9.0/16.0);
    const FIntPoint WideTarget=FGlintPreviewFrame::TargetResolutionForAspect(21.0/9.0);
    TestTrue(TEXT("portrait GPU preview preserves texture aspect"),FMath::IsNearlyEqual(double(PortraitTarget.X)/PortraitTarget.Y,9.0/16.0,0.002));
    TestTrue(TEXT("widescreen GPU preview preserves texture aspect"),FMath::IsNearlyEqual(double(WideTarget.X)/WideTarget.Y,21.0/9.0,0.004));
    TestTrue(TEXT("GPU targets are bounded by 768 pixels"),PortraitTarget.Y<=768 && WideTarget.X<=768);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintEditorScreenshotIsolation,
    "Glint.Editor.Preview.BackgroundIsNotRuntimeAssetData", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGlintEditorScreenshotIsolation::RunTest(const FString& Parameters)
{
    UGlintFlare* Flare=NewObject<UGlintFlare>();
    TestNotNull(TEXT("flare authoring asset exists"),Flare);
    TestTrue(TEXT("reference screenshot is not a serialized flare property"),
        UGlintFlare::StaticClass()->FindPropertyByName(TEXT("BackgroundTexture"))==nullptr);
    TestTrue(TEXT("reference screenshot is not a runtime lens property"),
        UGlintLensProfile::StaticClass()->FindPropertyByName(TEXT("BackgroundTexture"))==nullptr);
    return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
#include "GlintFlareEditorToolkit.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintEditorPreviewModeModel,
    "Glint.Editor.Toolbar.RightStripModes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlintEditorPreviewModeModel::RunTest(const FString& Parameters)
{
    // The actual mode strip is composed inside BuildEditorContent; we test
    // independent mode data here and verify its placement manually in UE.
    FGlintFlareSelection Selection;
    TestEqual(TEXT("editor initially opens in Hybrid"),int32(Selection.Mode),int32(EGlintPreviewMode::Hybrid));
    Selection.Mode=EGlintPreviewMode::Schematic;
    TestEqual(TEXT("switch to Schematic"),int32(Selection.Mode),0);
    Selection.Mode=EGlintPreviewMode::Rendered;
    TestEqual(TEXT("switch to Rendered"),int32(Selection.Mode),1);
    return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintUndoSelectionRecovery,
    "Glint.Editor.Undo.SelectionRecovery",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGlintUndoSelectionRecovery::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("selection recovers when final element deleted"),
        FGlintPreviewSelectionOps::RepairSelectionIndex(6,4),3);
    TestEqual(TEXT("selection selects first item after empty asset refilled"),
        FGlintPreviewSelectionOps::RepairSelectionIndex(INDEX_NONE,3),0);
    TestEqual(TEXT("empty asset clears invalid selection"),
        FGlintPreviewSelectionOps::RepairSelectionIndex(2,0),INDEX_NONE);
    TestEqual(TEXT("solo cleared after undo shrinks asset"),
        FGlintPreviewSelectionOps::RepairOptionalIndex(8,4),INDEX_NONE);
    TestEqual(TEXT("valid solo survives edit"),
        FGlintPreviewSelectionOps::RepairOptionalIndex(2,4),2);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintPreviewExposureAndOpacity,
    "Glint.Editor.Preview.OpacityAndModes",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGlintPreviewExposureAndOpacity::RunTest(const FString& Parameters)
{
    // Check editor-only visibility settings are not baked into a runtime asset.
    TestNull(TEXT("Flare asset does not serialize schematic opacity"),
        UGlintFlare::StaticClass()->FindPropertyByName(TEXT("SchematicOpacity")));
    TestNull(TEXT("Lens profile does not serialize schematic opacity"),
        UGlintLensProfile::StaticClass()->FindPropertyByName(TEXT("SchematicOpacity")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlintReorderState,
    "Glint.Editor.Reorder.SelectionMapping",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGlintReorderState::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("insert below source is a no-op"),
        FGlintPreviewSelectionOps::DestinationAfterDrop(3,4,10),3);
    TestEqual(TEXT("drop below item becomes the correct destination"),
        FGlintPreviewSelectionOps::DestinationAfterDrop(2,10,10),9);
    TestEqual(TEXT("drop above item becomes the correct destination"),
        FGlintPreviewSelectionOps::DestinationAfterDrop(8,2,10),2);
    TestEqual(TEXT("dragged item tracks its new index"),
        FGlintPreviewSelectionOps::RemapIndexAfterMove(2,2,9),9);
    TestEqual(TEXT("intermediate items retain order after downwards move"),
        FGlintPreviewSelectionOps::RemapIndexAfterMove(5,2,9),4);
    TestEqual(TEXT("intermediate items retain order after upwards move"),
        FGlintPreviewSelectionOps::RemapIndexAfterMove(2,6,1),3);
    return true;
}
#endif

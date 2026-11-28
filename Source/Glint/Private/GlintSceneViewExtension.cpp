#include "GlintSceneViewExtension.h"
#include "GlintLensProfile.h"
#include "GlintSourceSelection.h"
#include "GlintCameraBindingComponent.h"
#include "GlintProjectSettings.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraTypes.h"
#include "BlendableManager.h"
#include "FinalPostProcessSettings.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "SceneView.h"
#include "SceneManagement.h"
#include "ScreenPass.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "HAL/IConsoleManager.h"
#include "RenderTargetPool.h"
#include "GlintPreviewRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UnrealClient.h"
#include "RenderCommandFence.h"
#include "RenderingThread.h"

static TAutoConsoleVariable<int32> CVarGlintEnabled(
    TEXT("r.Glint.Enabled"), 1, TEXT("Enable Glint when a Glint Lens is present in per-view blendables."),
    ECVF_RenderThreadSafe | ECVF_Scalability);
static TAutoConsoleVariable<int32> CVarGlintDebug(
    TEXT("r.Glint.DebugBrightPass"), 0, TEXT("Show the Glint HDR bright-pass instead of final composition."),
    ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarGlintAutomatic(
    TEXT("r.Glint.Automatic"), 1, TEXT("Enable the procedural automatic ghost/streak field."),
    ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarGlintAuthored(
    TEXT("r.Glint.Authored"), 1, TEXT("Enable authored elements from the Glint Flare asset."),
    ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarGlintDebugSources(
    TEXT("r.Glint.DebugSources"), 0, TEXT("Source crosses: 0=off; 1=magenta; 2=confidence; 3=stable source slots; 4=filtered detection-luminance diagnostic plus crosses."),
    ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarGlintDetectionRadius(
    TEXT("r.Glint.DetectionRadius"), 1, TEXT("HDR detection-only smoothing: 0=off, 1=3x3 (default), 2=5x5. Does not blur the scene or finished flare."),
    ECVF_RenderThreadSafe | ECVF_Scalability);
static TAutoConsoleVariable<float> CVarGlintRelativeSourceScore(
    TEXT("r.Glint.SourceRelativeScoreFloor"), 0.22f, TEXT("Minimum new source score relative to the strongest HDR region in this frame (0..1)."),
    ECVF_RenderThreadSafe);
static TAutoConsoleVariable<float> CVarGlintReplacementRatio(
    TEXT("r.Glint.SourceReplacementRatio"), 1.70f, TEXT("Score advantage required to replace a weaker source when all slots are occupied."),
    ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarGlintReplacementFrames(
    TEXT("r.Glint.SourceReplacementFrames"), 3, TEXT("Consecutive frames a stronger source must persist before replacing a tracked slot (2..12)."),
    ECVF_RenderThreadSafe);
// Use the projected atmospheric light direction as a stable sun anchor, but
// require an actual visible HDR disk before giving it a tracked source slot.
// Mode 2 forces a projection for diagnosis only, not normal game rendering.
static TAutoConsoleVariable<int32> CVarGlintAtmosphereSource(
    TEXT("r.Glint.AtmosphereSource"), 1,
    TEXT("Sky Atmosphere sun source: 0=HDR candidates only, 1=HDR-validated sky sun (default), 2=force sun anchor for projection diagnosis only."),
    ECVF_RenderThreadSafe);
// Sky Atmosphere supports two atmospheric light slots. Index 0 is the usual sun;
// projects may intentionally place their main visible sun at index 1.
static TAutoConsoleVariable<int32> CVarGlintAtmosphereLightIndex(
    TEXT("r.Glint.AtmosphereLightIndex"), 0,
    TEXT("Atmospheric directional light to anchor (0=default sun, 1=secondary atmosphere light)."),
    ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarGlintSourceTracking(
    TEXT("r.Glint.SourceTracking"), 1, TEXT("Enable GPU source-driven authored elements. When disabled, authored drawing is off unless LegacyAuthoredField is enabled."),
    ECVF_RenderThreadSafe);
// The old image-field renderer warps the HDR scene itself into each optical
// element. Keep it only for deliberate comparisons; it is not a reliable
// representation of an authored Glow, Halo, Iris or Ring.
static TAutoConsoleVariable<int32> CVarGlintLegacyAuthoredField(
    TEXT("r.Glint.LegacyAuthoredField"), 0, TEXT("Development comparison only: allow the old image-field authored fallback when Source Driven Authored is off."),
    ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarGlintDebugFlareOnly(
    TEXT("r.Glint.DebugFlareOnly"), 0, TEXT("Display Glint alone over pure black, without changing the HDR scene used for source sampling."),
    ECVF_RenderThreadSafe);
// The legacy automatic mode resamples mirrored copies of the scene. It is
// useful for A/B screenshots, not for photographic lens ghosts.
static TAutoConsoleVariable<int32> CVarGlintLegacyAutomaticField(
    TEXT("r.Glint.LegacyAutomaticField"), 0,
    TEXT("Diagnostic only: restore the old repeated-image automatic ghosts/streaks. Default 0 uses source-relative optics."),
    ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarGlintFlareResolutionDivisor(
    TEXT("r.Glint.FlareResolutionDivisor"), 2, TEXT("Glint flare buffer downsample factor: 1=full, 2=half (default), 4=quarter (original). Larger values are faster but blur small optics."),
    ECVF_RenderThreadSafe | ECVF_Scalability);

struct FGlintSourceHistory
{
    TRefCountPtr<IPooledRenderTarget> Previous;
    FIntPoint ViewSize = FIntPoint::ZeroValue;
    FQuat LastViewRotation = FQuat::Identity;
    FMatrix LastNoAAViewProjection = FMatrix::Identity;
    bool bHasNoAAProjection = false;
    FVector LastViewOrigin = FVector::ZeroVector;
    bool bHasViewRotation = false;
    uint64 LastUsed = 0;
};

static TAutoConsoleVariable<int32> CVarGlintMaxSources(
    TEXT("r.Glint.MaxSources"), 8, TEXT("Global performance cap on source-driven authored flares (1..8)."),
    ECVF_RenderThreadSafe | ECVF_Scalability);
static TAutoConsoleVariable<int32> CVarGlintMaxElements(
    TEXT("r.Glint.MaxElements"), 32, TEXT("Runtime cap on authored elements per view (0..32)."),
    ECVF_RenderThreadSafe | ECVF_Scalability);

class FGlintExtractCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FGlintExtractCS);
    SHADER_USE_PARAMETER_STRUCT(FGlintExtractCS, FGlobalShader);
public:
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, HDRScene)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, BrightOutput)
        SHADER_PARAMETER(FIntPoint, SourceMin)
        SHADER_PARAMETER(FIntPoint, SourceSize)
        SHADER_PARAMETER(FIntPoint, BrightSize)
        SHADER_PARAMETER(uint32, SampleStride)
        SHADER_PARAMETER(float, BrightThreshold)
        SHADER_PARAMETER(float, KneeWidth)
        SHADER_PARAMETER(float, PeakMix)
        SHADER_PARAMETER(float, MaxContribution)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Params)
    { return IsFeatureLevelSupported(Params.Platform, ERHIFeatureLevel::SM5); }
};
IMPLEMENT_GLOBAL_SHADER(FGlintExtractCS, "/Plugin/Glint/Private/Glint.usf", "GlintExtractCS", SF_Compute);

class FGlintGhostCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FGlintGhostCS);
    SHADER_USE_PARAMETER_STRUCT(FGlintGhostCS, FGlobalShader);
public:
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BrightTexture)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, GhostOutput)
        SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FGlintRuntimeElement>, FlareElements)
        SHADER_PARAMETER(FIntPoint, BrightSize)
        SHADER_PARAMETER(float, GhostStrength)
        SHADER_PARAMETER(float, StreakStrength)
        SHADER_PARAMETER(float, ChromaticShift)
        SHADER_PARAMETER(float, GhostSpacing)
        SHADER_PARAMETER(float, StreakLength)
        SHADER_PARAMETER(uint32, GhostCount)
        SHADER_PARAMETER(uint32, ElementCount)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SourcePositions)
        SHADER_PARAMETER(uint32, MaxSources)
        SHADER_PARAMETER(uint32, UseSourcePositions)
        SHADER_PARAMETER(float, SourceEdgeFade)
        SHADER_PARAMETER(float, SourceGain)
        SHADER_PARAMETER(float, AuthoredEnergyLimit)
        SHADER_PARAMETER(uint32, UseSourceAutomatic)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Params)
    { return IsFeatureLevelSupported(Params.Platform, ERHIFeatureLevel::SM5); }
};
IMPLEMENT_GLOBAL_SHADER(FGlintGhostCS, "/Plugin/Glint/Private/Glint.usf", "GlintGhostCS", SF_Compute);

class FGlintBuildDetectionCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FGlintBuildDetectionCS);
    SHADER_USE_PARAMETER_STRUCT(FGlintBuildDetectionCS, FGlobalShader);
public:
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BrightTexture)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, DetectionOutput)
        SHADER_PARAMETER(FIntPoint, BrightSize)
        SHADER_PARAMETER(uint32, DetectionRadius)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Params)
    { return IsFeatureLevelSupported(Params.Platform, ERHIFeatureLevel::SM5); }
};
IMPLEMENT_GLOBAL_SHADER(FGlintBuildDetectionCS, "/Plugin/Glint/Private/Glint.usf", "GlintBuildDetectionCS", SF_Compute);

class FGlintFindCandidatesCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FGlintFindCandidatesCS);
    SHADER_USE_PARAMETER_STRUCT(FGlintFindCandidatesCS, FGlobalShader);
public:
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BrightTexture)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, DetectionTexture)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, CandidateOutput)
        SHADER_PARAMETER(FIntPoint, BrightSize)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Params)
    { return IsFeatureLevelSupported(Params.Platform, ERHIFeatureLevel::SM5); }
};
IMPLEMENT_GLOBAL_SHADER(FGlintFindCandidatesCS, "/Plugin/Glint/Private/Glint.usf", "GlintFindCandidatesCS", SF_Compute);

class FGlintSelectSourcesCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FGlintSelectSourcesCS);
    SHADER_USE_PARAMETER_STRUCT(FGlintSelectSourcesCS, FGlobalShader);
public:
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, CandidateTexture)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, DetectionTexture)
        SHADER_PARAMETER(FIntPoint, BrightSize)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, HDRScene)
        SHADER_PARAMETER(FIntPoint, SourceMin)
        SHADER_PARAMETER(FIntPoint, SourceSize)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SourceHistory)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, SourcesOutput)
        SHADER_PARAMETER(FIntPoint, CandidateSize)
        SHADER_PARAMETER(uint32, MaxSources)
        SHADER_PARAMETER(float, SourceMinBrightness)
        SHADER_PARAMETER(float, SourceSeparation)
        SHADER_PARAMETER(float, SourceGroupingRadius)
        SHADER_PARAMETER(float, SourceSmoothing)
        SHADER_PARAMETER(float, SourcePersistence)
        SHADER_PARAMETER(float, SourceHysteresis)
        SHADER_PARAMETER(float, SourcePositionDeadZone)
        SHADER_PARAMETER(float, SourceCameraMotion)
        SHADER_PARAMETER(FMatrix44f, SourcePreviousToCurrentClip)
        SHADER_PARAMETER(uint32, UseSourceReprojection)
        // Engine-provided atmospheric sunlight direction and illuminance.
        SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
        SHADER_PARAMETER(FMatrix44f, SourceSunWorldToClipNoAA)
        SHADER_PARAMETER(uint32, AtmosphereSunMode)
        SHADER_PARAMETER(uint32, AtmosphereLightIndex)
        SHADER_PARAMETER(float, SourceRelativeScoreFloor)
        SHADER_PARAMETER(float, SourceReplacementRatio)
        SHADER_PARAMETER(uint32, SourceReplacementFrames)
        SHADER_PARAMETER(uint32, ResetSourceHistory)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Params)
    { return IsFeatureLevelSupported(Params.Platform, ERHIFeatureLevel::SM5); }
};
IMPLEMENT_GLOBAL_SHADER(FGlintSelectSourcesCS, "/Plugin/Glint/Private/Glint.usf", "GlintSelectSourcesCS", SF_Compute);

class FGlintCompositeCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FGlintCompositeCS);
    SHADER_USE_PARAMETER_STRUCT(FGlintCompositeCS, FGlobalShader);
public:
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, HDRScene)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, FlareTexture)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, DetectionTexture)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, SceneOutput)
        SHADER_PARAMETER(FIntPoint, FullSize)
        SHADER_PARAMETER(FIntPoint, SourceMin)
        SHADER_PARAMETER(FIntPoint, SourceSize)
        SHADER_PARAMETER(FIntPoint, BrightSize)
        SHADER_PARAMETER(float, FlareIntensity)
        SHADER_PARAMETER(uint32, DebugBrightPass)
        SHADER_PARAMETER(uint32, DebugFlareOnly)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SourcePositions)
        SHADER_PARAMETER(uint32, MaxSources)
        SHADER_PARAMETER(uint32, DebugSources)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Params)
    { return IsFeatureLevelSupported(Params.Platform, ERHIFeatureLevel::SM5); }
};
IMPLEMENT_GLOBAL_SHADER(FGlintCompositeCS, "/Plugin/Glint/Private/Glint.usf", "GlintCompositeCS", SF_Compute);

class FGlintPreviewCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FGlintPreviewCS);
    SHADER_USE_PARAMETER_STRUCT(FGlintPreviewCS, FGlobalShader);
public:
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FGlintRuntimeElement>, FlareElements)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, PreviewOutput)
        SHADER_PARAMETER(FIntPoint, PreviewSize)
        SHADER_PARAMETER(FIntPoint, BrightSize)
        SHADER_PARAMETER(FVector2f, PreviewSourceUV)
        SHADER_PARAMETER(FVector4f, PreviewSourceColor)
        SHADER_PARAMETER(float, PreviewExposure)
        SHADER_PARAMETER(uint32, PreviewElementCount)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Params)
    { return IsFeatureLevelSupported(Params.Platform, ERHIFeatureLevel::SM5); }
};
IMPLEMENT_GLOBAL_SHADER(FGlintPreviewCS, "/Plugin/Glint/Private/Glint.usf", "GlintPreviewCS", SF_Compute);

void QueueGlintPreviewRender(UTextureRenderTarget2D* Target, const FGlintPreviewRequest& Request)
{
    if (!IsInGameThread() || !Target) return;
    FTextureRenderTargetResource* Resource = Target->GameThread_GetRenderTargetResource();
    if (!Resource) return;
    TArray<FGlintRuntimeElement> Packed = Request.Elements;
    if (Packed.IsEmpty()) Packed.Add(FGlintRuntimeElement());
    const uint32 Count = uint32(Request.Elements.Num());
    const FVector2f SourceUV(FMath::Clamp(float(Request.SourceUV.X), -2.f, 3.f),
                              FMath::Clamp(float(Request.SourceUV.Y), -2.f, 3.f));
    const FVector4f Color(Request.SourceColor.R, Request.SourceColor.G,
                          Request.SourceColor.B, 1.f);
    const float Exposure = FMath::Clamp(Request.Exposure, 0.05f, 8.f);
    ENQUEUE_RENDER_COMMAND(GlintEditorGpuPreview)(
        [Resource, Packed=MoveTemp(Packed), Count, SourceUV, Color, Exposure](FRHICommandListImmediate& RHICmdList)
        {
            FRHITexture* Texture = Resource->GetRenderTargetTexture().GetReference();
            if (!Texture) return;
            FRDGBuilder GraphBuilder(RHICmdList);
            FRDGTextureRef Output = RegisterExternalTexture(GraphBuilder, Texture, TEXT("Glint.EditorPreview"));
            const FIntPoint Size = Output->Desc.Extent;
            FRDGBufferRef PackedBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("Glint.PreviewElements"), Packed);
            FGlintPreviewCS::FParameters* Parameters = GraphBuilder.AllocParameters<FGlintPreviewCS::FParameters>();
            Parameters->FlareElements = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(PackedBuffer));
            Parameters->PreviewOutput = GraphBuilder.CreateUAV(Output);
            Parameters->PreviewSize = Size;
            Parameters->BrightSize = Size;
            Parameters->PreviewSourceUV = SourceUV;
            Parameters->PreviewSourceColor = Color;
            Parameters->PreviewExposure = Exposure;
            Parameters->PreviewElementCount = Count;
            FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("Glint.EditorPreview"),
                GetGlobalShaderMap(GMaxRHIFeatureLevel)->GetShader<FGlintPreviewCS>(), Parameters,
                FComputeShaderUtils::GetGroupCount(Size, FIntPoint(8, 8)));
            GraphBuilder.Execute();
        });
}

void FGlintSceneViewExtension::SetupViewPoint(APlayerController* Player, FMinimalViewInfo& InViewInfo)
{
    if (!Player) return;
    const UGlintProjectSettings* Settings = GetDefault<UGlintProjectSettings>();
    const FSoftObjectPath RequestedPath = Settings->DefaultLensProfile.ToSoftObjectPath();
    if (RequestedPath != DefaultLensPath)
    {
        DefaultLensPath = RequestedPath;
        DefaultLensAsset.Reset(RequestedPath.IsNull() ? nullptr : Settings->DefaultLensProfile.LoadSynchronous());
    }

    const UGlintCameraBindingComponent* Binding = Player->FindComponentByClass<UGlintCameraBindingComponent>();
    const UGlintLensProfile* Default = DefaultLensAsset.Get();
    auto AddLens = [&InViewInfo](const UGlintLensProfile* Lens, float Weight)
    {
        if (Lens && Weight > KINDA_SMALL_NUMBER)
        {
            InViewInfo.PostProcessSettings.WeightedBlendables.Array.Add(
                FWeightedBlendable(FMath::Clamp(Weight, 0.f, 1.f), const_cast<UGlintLensProfile*>(Lens)));
            // This path explicitly opts into a blended camera profile. The
            // renderer still uses the normal per-view postprocess stack.
            InViewInfo.PostProcessBlendWeight = 1.f;
        }
    };

    if (Binding && Binding->IsGlintLensBlending())
    {
        float Outgoing = 0.f, Incoming = 0.f;
        Binding->GetGlintBlendWeights(Outgoing, Incoming);
        AddLens(Binding->GetOutgoingGlintLens() ? Binding->GetOutgoingGlintLens() : Default, Outgoing);
        AddLens(Binding->LensProfile ? Binding->LensProfile.Get() : Default, Incoming);
    }
    else if (Binding && Binding->LensProfile)
    {
        AddLens(Binding->LensProfile.Get(), Binding->BlendWeight);
    }
    else
    {
        AddLens(Default, 1.f);
    }
}

void FGlintSceneViewExtension::SubscribeToPostProcessingPass(
    EPostProcessingPass Pass, const FSceneView& View,
    FPostProcessingPassDelegateArray& Callbacks, bool bIsPassEnabled)
{
    // UE5.8: immediately before Scene Color Before Bloom materials; runs even
    // when the Motion Blur effect itself is disabled.
    if (Pass == EPostProcessingPass::MotionBlur && CVarGlintEnabled.GetValueOnAnyThread() != 0)
    {
        Callbacks.Add(FPostProcessingPassDelegate::CreateRaw(this, &FGlintSceneViewExtension::AddGlintPass));
    }
}

FScreenPassTexture FGlintSceneViewExtension::AddGlintPass(
    FRDGBuilder& GraphBuilder, const FSceneView& View,
    const FPostProcessMaterialInputs& Inputs) const
{
    // Render thread reads per-view copies exclusively; never dereference a
    // Glint Lens, Glint Flare, or PlayerController UObject here.
    FGlintLensBlendData Blended;
    float TotalWeight = 0.f;
    float AutomaticWeight = 0.f;
    TArray<FGlintRuntimeElement> Elements;
    Elements.Reserve(24);
    FBlendableEntry* Entry = nullptr;
    const FBlendableManager& Manager = View.FinalPostProcessSettings.BlendableManager;
    const int32 MaxElements = FMath::Clamp(CVarGlintMaxElements.GetValueOnRenderThread(), 0, 32);

    while (const FGlintLensBlendData* Lens = Manager.IterateBlendables<FGlintLensBlendData>(Entry))
    {
        const float W = FMath::Clamp(Entry->Weight, 0.f, 1.f);
        if (W <= 0.f) continue;
        if (TotalWeight == 0.f) Blended = *Lens;
        else
        {
            const float Alpha = W / (TotalWeight + W);
            Blended.Intensity = FMath::Lerp(Blended.Intensity, Lens->Intensity, Alpha);
            Blended.Threshold = FMath::Lerp(Blended.Threshold, Lens->Threshold, Alpha);
            Blended.SoftKnee = FMath::Lerp(Blended.SoftKnee, Lens->SoftKnee, Alpha);
            Blended.PeakMix = FMath::Lerp(Blended.PeakMix, Lens->PeakMix, Alpha);
            Blended.MaxContribution = FMath::Lerp(Blended.MaxContribution, Lens->MaxContribution, Alpha);
            Blended.GhostStrength = FMath::Lerp(Blended.GhostStrength, Lens->GhostStrength, Alpha);
            Blended.GhostCount = FMath::RoundToInt(FMath::Lerp(float(Blended.GhostCount), float(Lens->GhostCount), Alpha));
            Blended.StreakStrength = FMath::Lerp(Blended.StreakStrength, Lens->StreakStrength, Alpha);
            Blended.StreakLength = FMath::Lerp(Blended.StreakLength, Lens->StreakLength, Alpha);
            Blended.ChromaticShift = FMath::Lerp(Blended.ChromaticShift, Lens->ChromaticShift, Alpha);
            Blended.GhostSpacing = FMath::Lerp(Blended.GhostSpacing, Lens->GhostSpacing, Alpha);
            Blended.MaxSources = FMath::RoundToInt(FMath::Lerp(float(Blended.MaxSources), float(Lens->MaxSources), Alpha));
            Blended.SourceMinBrightness = FMath::Lerp(Blended.SourceMinBrightness, Lens->SourceMinBrightness, Alpha);
            Blended.SourceSeparation = FMath::Lerp(Blended.SourceSeparation, Lens->SourceSeparation, Alpha);
            Blended.SourceGroupingRadius = FMath::Lerp(Blended.SourceGroupingRadius, Lens->SourceGroupingRadius, Alpha);
            Blended.SourceSmoothing = FMath::Lerp(Blended.SourceSmoothing, Lens->SourceSmoothing, Alpha);
            Blended.SourcePersistence = FMath::Lerp(Blended.SourcePersistence, Lens->SourcePersistence, Alpha);
            Blended.SourceHysteresis = FMath::Lerp(Blended.SourceHysteresis, Lens->SourceHysteresis, Alpha);
            Blended.SourcePositionDeadZone = FMath::Lerp(Blended.SourcePositionDeadZone, Lens->SourcePositionDeadZone, Alpha);
            Blended.SourceEdgeFade = FMath::Lerp(Blended.SourceEdgeFade, Lens->SourceEdgeFade, Alpha);
            Blended.SourceGain = FMath::Lerp(Blended.SourceGain, Lens->SourceGain, Alpha);
            Blended.AuthoredEnergyLimit = FMath::Lerp(Blended.AuthoredEnergyLimit, Lens->AuthoredEnergyLimit, Alpha);
            Blended.bSourceDrivenAuthored = Lens->bSourceDrivenAuthored;
            Blended.bFollowCameraAperture = Lens->bFollowCameraAperture;
            Blended.ReferenceFStop = FMath::Lerp(Blended.ReferenceFStop, Lens->ReferenceFStop, Alpha);
            Blended.ApertureScaleResponse = FMath::Lerp(Blended.ApertureScaleResponse, Lens->ApertureScaleResponse, Alpha);
        }
        AutomaticWeight += Lens->bEnableAutomatic ? W : 0.f;
        if (CVarGlintAuthored.GetValueOnRenderThread() != 0)
        {
            const uint32 Count = FMath::Min(Lens->ElementCount, uint32(FGlintLensBlendData::MaxElementsPerLens));
            for (uint32 Index = 0; Index < Count; ++Index)
            {
                FGlintRuntimeElement E = Lens->Elements[Index];
                E.ColorStrength.W *= W;
                if (Elements.Num() < MaxElements)
                {
                    Elements.Add(E);
                }
                else if (MaxElements > 0)
                {
                    // Keep the strongest items across *all* blending lenses;
                    // do not let a large outgoing flare hide an incoming rig.
                    int32 WeakestIndex = 0;
                    for (int32 Candidate = 1; Candidate < Elements.Num(); ++Candidate)
                    {
                        if (Elements[Candidate].ColorStrength.W < Elements[WeakestIndex].ColorStrength.W)
                        {
                            WeakestIndex = Candidate;
                        }
                    }
                    if (E.ColorStrength.W > Elements[WeakestIndex].ColorStrength.W)
                    {
                        Elements[WeakestIndex] = E;
                    }
                }
            }
        }
        TotalWeight += W;
    }

    if (View.GetFeatureLevel() < ERHIFeatureLevel::SM5 || TotalWeight <= 0.f ||
        Blended.Intensity <= 0.f || Inputs.OverrideOutput.Texture)
    {
        return Inputs.ReturnUntouchedSceneColorForPostProcessing(GraphBuilder);
    }

    if (AutomaticWeight <= 0.f || CVarGlintAutomatic.GetValueOnRenderThread() == 0)
    {
        Blended.GhostStrength = 0.f;
        Blended.StreakStrength = 0.f;
    }
    else
    {
        const float Fraction = AutomaticWeight / TotalWeight;
        Blended.GhostStrength *= Fraction;
        Blended.StreakStrength *= Fraction;
    }
    const float ApertureScale = Blended.bFollowCameraAperture
        ? UGlintLensProfile::CalculateApertureScale(View.FinalPostProcessSettings.DepthOfFieldFstop,
            Blended.ReferenceFStop, Blended.ApertureScaleResponse)
        : 1.f;
    for (FGlintRuntimeElement& Element : Elements)
    {
        Element.ColorStrength.W /= TotalWeight; // weighted profile mix, not additive doubling
        Element.Geometry.Z *= ApertureScale;
    }
    if (Blended.GhostStrength == 0.f && Blended.StreakStrength == 0.f && Elements.IsEmpty() &&
        CVarGlintDebug.GetValueOnRenderThread() == 0 && CVarGlintDebugSources.GetValueOnRenderThread() == 0 &&
        CVarGlintDebugFlareOnly.GetValueOnRenderThread() == 0)
    {
        return Inputs.ReturnUntouchedSceneColorForPostProcessing(GraphBuilder);
    }

    FScreenPassTexture Scene = FScreenPassTexture::CopyFromSlice(
        GraphBuilder, Inputs.GetInput(EPostProcessMaterialInput::SceneColor));
    if (!Scene.IsValid()) return Scene;
    const FIntPoint ViewSize = Scene.ViewRect.Size();
    if (ViewSize.X < 4 || ViewSize.Y < 4) return Scene;
    // The original quarter-resolution buffer visibly pixelated fine Iris,
    // Ring and aperture elements. Half resolution is now the default; 4 is a
    // profiling fallback, and 1 is the quality reference.
    const int32 RequestedDivisor = CVarGlintFlareResolutionDivisor.GetValueOnRenderThread();
    const int32 SampleStride = RequestedDivisor <= 1 ? 1 : (RequestedDivisor <= 2 ? 2 : 4);
    const FIntPoint SmallSize = FIntPoint::DivideAndRoundUp(ViewSize,
        FIntPoint(SampleStride, SampleStride));
    const FRDGTextureDesc SmallDesc = FRDGTextureDesc::Create2D(
        SmallSize, PF_FloatRGBA, FClearValueBinding::Black,
        TexCreate_ShaderResource | TexCreate_UAV);
    FRDGTextureRef Bright = GraphBuilder.CreateTexture(SmallDesc, TEXT("Glint.Bright"));
    FRDGTextureRef Ghosts = GraphBuilder.CreateTexture(SmallDesc, TEXT("Glint.Ghosts"));

    FGlintExtractCS::FParameters* Extract = GraphBuilder.AllocParameters<FGlintExtractCS::FParameters>();
    Extract->HDRScene = Scene.Texture;
    Extract->BrightOutput = GraphBuilder.CreateUAV(Bright);
    Extract->SourceMin = Scene.ViewRect.Min;
    Extract->SourceSize = ViewSize;
    Extract->BrightSize = SmallSize;
    Extract->SampleStride = uint32(SampleStride);
    // UE5 scene color is pre-exposed. Express thresholds in stable scene-linear
    // units to avoid the lens popping with eye adaptation changes.
    const float PreExposure = (View.State && View.State->GetPreExposure() > 0.f)
        ? View.State->GetPreExposure() : 1.f;
    Extract->BrightThreshold = Blended.Threshold * PreExposure;
    Extract->KneeWidth = Blended.SoftKnee;
    Extract->PeakMix = Blended.PeakMix;
    Extract->MaxContribution = Blended.MaxContribution * PreExposure;
    FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("Glint.Extract"),
        GetGlobalShaderMap(View.GetFeatureLevel())->GetShader<FGlintExtractCS>(), Extract,
        FComputeShaderUtils::GetGroupCount(SmallSize, FIntPoint(8, 8)));

    // Select at most eight bright source locations without GPU -> CPU readback.
    // The first pass finds tile maxima; the second performs bounded source
    // ranking, minimum separation and matches positions to last frame's view.
    const EGlintAuthoredRenderMode AuthoredMode = FGlintSourceSelection::ResolveAuthoredMode(
        !Elements.IsEmpty(), Blended.bSourceDrivenAuthored,
        CVarGlintSourceTracking.GetValueOnRenderThread() != 0,
        CVarGlintLegacyAuthoredField.GetValueOnRenderThread() != 0);
    const bool bTrackSources = AuthoredMode == EGlintAuthoredRenderMode::SourceTracked;
    const bool bUseLegacyField = AuthoredMode == EGlintAuthoredRenderMode::LegacyImageField;
    const bool bDebugSources = CVarGlintDebugSources.GetValueOnRenderThread() != 0;
    FRDGTextureRef DetectionDebugTexture = Bright;
    // Row 0 = UV/brightness/confidence; row 1 = HDR color; row 2 = challenger evidence.
    const FIntPoint SourceTextureSize(8, 3);
    const FRDGTextureDesc SourcesDesc = FRDGTextureDesc::Create2D(
        SourceTextureSize, PF_FloatRGBA, FClearValueBinding::Black,
        TexCreate_ShaderResource | TexCreate_UAV);
    FRDGTextureRef Sources = GraphBuilder.CreateTexture(SourcesDesc, TEXT("Glint.Sources"));
    AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(Sources), FLinearColor::Black);
    const bool bSourceAutomatic = ((Blended.GhostStrength > 0.f && Blended.GhostCount > 0) ||
        Blended.StreakStrength > 0.f) &&
        CVarGlintLegacyAutomaticField.GetValueOnRenderThread() == 0;
    if (FGlintSourceSelection::RequiresSources(bTrackSources, bSourceAutomatic, bDebugSources))
    {
        // Detection texture retains smoothed HDR luminance separately from the
        // raw peak channel; final optics still use the unblurred HDR scene.
        FRDGTextureRef Detection = GraphBuilder.CreateTexture(SmallDesc, TEXT("Glint.Detection"));
        DetectionDebugTexture = Detection;
        FGlintBuildDetectionCS::FParameters* Filter = GraphBuilder.AllocParameters<FGlintBuildDetectionCS::FParameters>();
        Filter->BrightTexture = Bright;
        Filter->DetectionOutput = GraphBuilder.CreateUAV(Detection);
        Filter->BrightSize = SmallSize;
        Filter->DetectionRadius = uint32(FMath::Clamp(CVarGlintDetectionRadius.GetValueOnRenderThread(), 0, 2));
        FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("Glint.BuildDetection"),
            GetGlobalShaderMap(View.GetFeatureLevel())->GetShader<FGlintBuildDetectionCS>(), Filter,
            FComputeShaderUtils::GetGroupCount(SmallSize, FIntPoint(8,8)));
        // Two candidates per tile: strongest coherent region and strongest
        // untouched HDR peak. A bright sun must not vanish behind a cloud in
        // the same tile. Source selection receives the doubled candidate width.
        const FIntPoint CandidateTiles = FIntPoint::DivideAndRoundUp(SmallSize, FIntPoint(32, 32));
        const FIntPoint CandidateSize(CandidateTiles.X*2, CandidateTiles.Y);
        const FRDGTextureDesc CandidateDesc = FRDGTextureDesc::Create2D(
            CandidateSize, PF_FloatRGBA, FClearValueBinding::Black,
            TexCreate_ShaderResource | TexCreate_UAV);
        FRDGTextureRef Candidates = GraphBuilder.CreateTexture(CandidateDesc, TEXT("Glint.SourceCandidates"));
        FGlintFindCandidatesCS::FParameters* Find = GraphBuilder.AllocParameters<FGlintFindCandidatesCS::FParameters>();
        Find->BrightTexture = Bright;
        Find->DetectionTexture = Detection;
        Find->CandidateOutput = GraphBuilder.CreateUAV(Candidates);
        Find->BrightSize = SmallSize;
        FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("Glint.FindSourceCandidates"),
            GetGlobalShaderMap(View.GetFeatureLevel())->GetShader<FGlintFindCandidatesCS>(), Find,
            FIntVector(CandidateTiles.X, CandidateTiles.Y, 1));

        TSharedPtr<FGlintSourceHistory> History;
        FRDGTextureRef Previous = GraphBuilder.CreateTexture(SourcesDesc, TEXT("Glint.PreviousSourcesDefault"));
        AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(Previous), FLinearColor::Black);
        bool bResetHistory = true;
        float SourceCameraMotion = 1.f; // no temporal history: acquire directly
        FMatrix44f SourceReprojection = FMatrix44f::Identity;
        bool bUseSourceReprojection = false;
        if (View.State)
        {
            const FSceneViewStateInterface* ViewKey = View.State;
            TSharedPtr<FGlintSourceHistory>& EntryHistory = SourceHistories.FindOrAdd(ViewKey);
            if (!EntryHistory.IsValid()) EntryHistory = MakeShared<FGlintSourceHistory>();
            History = EntryHistory;
            History->LastUsed = ++HistoryVisitCounter;
            bResetHistory = FGlintSourceSelection::ShouldResetHistory(
                History->ViewSize, ViewSize, View.bCameraCut, History->Previous.IsValid());
            // A changing camera may legitimately move a light tens of pixels
            // in one frame. When the camera is stationary, large jumps between
            // GPU tile peaks are almost certainly *not* camera movement.
            // Use the projection without TAA jitter. Reproject previous source
            // directions into the current frame before matching candidates;
            // otherwise a rotating camera appears to teleport every emitter.
            const FMatrix CurrentNoAAViewProjection =
                View.ViewMatrices.GetWorldToView() * View.ViewMatrices.GetViewToClipNoAA();
            if (!bResetHistory && History->bHasNoAAProjection)
            {
                SourceReprojection = FMatrix44f(
                    History->LastNoAAViewProjection.Inverse() * CurrentNoAAViewProjection);
                bUseSourceReprojection = true;
            }
            History->LastNoAAViewProjection = CurrentNoAAViewProjection;
            History->bHasNoAAProjection = true;
            const FQuat CurrentViewRotation = View.ViewMatrices.GetViewToWorld().ToQuat();
            const FVector CurrentViewOrigin = View.ViewMatrices.GetViewOrigin();
            if (!bResetHistory && History->bHasViewRotation)
            {
                const float AngularDelta = float(CurrentViewRotation.AngularDistance(History->LastViewRotation));
                const float TranslationDelta = float(FVector::Dist(CurrentViewOrigin, History->LastViewOrigin));
                SourceCameraMotion = FMath::Max(
                    FGlintSourceSelection::CameraMotionFactor(AngularDelta),
                    FGlintSourceSelection::CameraTranslationFactor(TranslationDelta));
            }
            History->LastViewRotation = CurrentViewRotation;
            History->LastViewOrigin = CurrentViewOrigin;
            History->bHasViewRotation = true;
            if (!bResetHistory)
                Previous = GraphBuilder.RegisterExternalTexture(History->Previous, TEXT("Glint.PreviousSources"));
            History->ViewSize = ViewSize;
            // Avoid retaining GPU histories indefinitely for destroyed views.
            // 2048 callback visits exceeds the lifetime of two normal frames.
            if ((HistoryVisitCounter % 256u) == 0u)
            {
                for (auto It = SourceHistories.CreateIterator(); It; ++It)
                    if (HistoryVisitCounter - It.Value()->LastUsed > 2048u) It.RemoveCurrent();
            }
        }
        FGlintSelectSourcesCS::FParameters* Select = GraphBuilder.AllocParameters<FGlintSelectSourcesCS::FParameters>();
        Select->CandidateTexture = Candidates;
        Select->DetectionTexture = Detection;
        Select->BrightSize = SmallSize;
        Select->HDRScene = Scene.Texture;
        Select->SourceMin = Scene.ViewRect.Min;
        Select->SourceSize = ViewSize;
        Select->SourceHistory = Previous;
        Select->SourcesOutput = GraphBuilder.CreateUAV(Sources);
        Select->CandidateSize = CandidateSize;
        Select->MaxSources = uint32(FMath::Clamp(FMath::Min(Blended.MaxSources, CVarGlintMaxSources.GetValueOnRenderThread()), 1, 8));
        // Source detection uses *raw HDR peak luminance* (Bright.a), not
        // artistic thresholded Bright.rgb. Gate by a fraction of the lens
        // threshold to avoid choosing ordinary dim scene pixels.
        Select->SourceMinBrightness = FGlintSourceSelection::CalculatePeakGate(
            Blended.SourceMinBrightness, Blended.Threshold, PreExposure);
        Select->SourceSeparation = Blended.SourceSeparation;
        Select->SourceGroupingRadius = Blended.SourceGroupingRadius;
        Select->SourceSmoothing = Blended.SourceSmoothing;
        Select->SourcePersistence = Blended.SourcePersistence;
        Select->SourceHysteresis = Blended.SourceHysteresis;
        Select->SourcePositionDeadZone = Blended.SourcePositionDeadZone;
        Select->SourceCameraMotion = SourceCameraMotion;
        Select->SourcePreviousToCurrentClip = SourceReprojection;
        Select->UseSourceReprojection = bUseSourceReprojection ? 1u : 0u;
        Select->View = View.ViewUniformBuffer;
        Select->SourceSunWorldToClipNoAA = FMatrix44f(
            View.ViewMatrices.GetWorldToView() * View.ViewMatrices.GetViewToClipNoAA());
        Select->AtmosphereSunMode = uint32(FMath::Clamp(
            CVarGlintAtmosphereSource.GetValueOnRenderThread(), 0, 2));
        Select->AtmosphereLightIndex = uint32(FMath::Clamp(
            CVarGlintAtmosphereLightIndex.GetValueOnRenderThread(), 0, 1));
        Select->SourceRelativeScoreFloor = FMath::Clamp(CVarGlintRelativeSourceScore.GetValueOnRenderThread(), 0.f, 1.f);
        Select->SourceReplacementRatio = FMath::Clamp(CVarGlintReplacementRatio.GetValueOnRenderThread(), 1.05f, 10.f);
        Select->SourceReplacementFrames = uint32(FMath::Clamp(CVarGlintReplacementFrames.GetValueOnRenderThread(), 2, 12));
        Select->ResetSourceHistory = bResetHistory ? 1u : 0u;
        FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("Glint.SelectAndTrackSources"),
            GetGlobalShaderMap(View.GetFeatureLevel())->GetShader<FGlintSelectSourcesCS>(), Select,
            FIntVector(1, 1, 1));
        // QueueTextureExtraction writes a pooled result at graph execution.
        // History is a shared heap allocation, so the output pointer remains
        // stable even if the lookup table is expanded by another view.
        if (History.IsValid())
            GraphBuilder.QueueTextureExtraction(Sources, &History->Previous);
    }

    // RDG structured buffer is uploaded once per view, never per highlight.
    // Empty element stacks are represented by the helper's single dummy value.
    FRDGBufferRef ElementBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("Glint.Elements"), Elements);
    FGlintGhostCS::FParameters* Ghost = GraphBuilder.AllocParameters<FGlintGhostCS::FParameters>();
    Ghost->BrightTexture = Bright;
    Ghost->GhostOutput = GraphBuilder.CreateUAV(Ghosts);
    Ghost->FlareElements = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(ElementBuffer));
    Ghost->BrightSize = SmallSize;
    Ghost->GhostStrength = Blended.GhostStrength;
    Ghost->GhostCount = uint32(FMath::Clamp(Blended.GhostCount, 0, 8));
    Ghost->StreakStrength = Blended.StreakStrength;
    Ghost->StreakLength = Blended.StreakLength;
    Ghost->ChromaticShift = Blended.ChromaticShift;
    Ghost->GhostSpacing = Blended.GhostSpacing;
    // Disabling Source Driven Authored must not silently switch to the old
    // image-field approximation. That renderer can be explicitly re-enabled
    // for A/B diagnostics using r.Glint.LegacyAuthoredField 1.
    Ghost->ElementCount = (bTrackSources || bUseLegacyField) ? uint32(Elements.Num()) : 0u;
    Ghost->SourcePositions = Sources;
    Ghost->MaxSources = uint32(FMath::Clamp(FMath::Min(Blended.MaxSources, CVarGlintMaxSources.GetValueOnRenderThread()), 1, 8));
    Ghost->UseSourcePositions = bTrackSources ? 1u : 0u;
    Ghost->SourceEdgeFade = Blended.SourceEdgeFade;
    Ghost->SourceGain = Blended.SourceGain;
    Ghost->AuthoredEnergyLimit = Blended.AuthoredEnergyLimit;
    Ghost->UseSourceAutomatic = bSourceAutomatic ? 1u : 0u;
    FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("Glint.AutomaticAndAuthored"),
        GetGlobalShaderMap(View.GetFeatureLevel())->GetShader<FGlintGhostCS>(), Ghost,
        FComputeShaderUtils::GetGroupCount(SmallSize, FIntPoint(8, 8)));

    FRDGTextureDesc FullDesc = FRDGTextureDesc::Create2D(
        Scene.Texture->Desc.Extent, PF_FloatRGBA, FClearValueBinding::Black,
        TexCreate_ShaderResource | TexCreate_UAV | TexCreate_RenderTargetable);
    FRDGTextureRef Combined = GraphBuilder.CreateTexture(FullDesc, TEXT("Glint.HDRComposite"));
    FGlintCompositeCS::FParameters* Composite = GraphBuilder.AllocParameters<FGlintCompositeCS::FParameters>();
    Composite->HDRScene = Scene.Texture;
    Composite->FlareTexture = CVarGlintDebug.GetValueOnRenderThread() ? Bright : Ghosts;
    Composite->DetectionTexture = DetectionDebugTexture;
    Composite->SceneOutput = GraphBuilder.CreateUAV(Combined);
    Composite->FullSize = FullDesc.Extent;
    Composite->SourceMin = Scene.ViewRect.Min;
    Composite->SourceSize = ViewSize;
    Composite->BrightSize = SmallSize;
    Composite->FlareIntensity = Blended.Intensity * FMath::Min(TotalWeight, 1.f);
    Composite->DebugBrightPass = CVarGlintDebug.GetValueOnRenderThread() ? 1u : 0u;
    Composite->DebugFlareOnly = CVarGlintDebugFlareOnly.GetValueOnRenderThread() ? 1u : 0u;
    Composite->SourcePositions = Sources;
    Composite->MaxSources = Ghost->MaxSources;
    Composite->DebugSources = uint32(FMath::Clamp(CVarGlintDebugSources.GetValueOnRenderThread(), 0, 4));
    FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("Glint.CompositeHDR"),
        GetGlobalShaderMap(View.GetFeatureLevel())->GetShader<FGlintCompositeCS>(), Composite,
        FComputeShaderUtils::GetGroupCount(FullDesc.Extent, FIntPoint(8, 8)));

    return FScreenPassTexture(Combined, Scene.ViewRect);
}

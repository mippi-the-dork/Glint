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
    TEXT("r.Glint.DebugSources"), 0, TEXT("Draw source-tracking crosses on the flare field."),
    ECVF_RenderThreadSafe);
static TAutoConsoleVariable<int32> CVarGlintSourceTracking(
    TEXT("r.Glint.SourceTracking"), 1, TEXT("Enable GPU source-driven authored elements; 0 uses the older image-field mode."),
    ECVF_RenderThreadSafe);

struct FGlintSourceHistory
{
    TRefCountPtr<IPooledRenderTarget> Previous;
    FIntPoint ViewSize = FIntPoint::ZeroValue;
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
        SHADER_PARAMETER(uint32, DebugSources)
        SHADER_PARAMETER(float, SourceEdgeFade)
        SHADER_PARAMETER(float, SourceGain)
        SHADER_PARAMETER(float, AuthoredEnergyLimit)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Params)
    { return IsFeatureLevelSupported(Params.Platform, ERHIFeatureLevel::SM5); }
};
IMPLEMENT_GLOBAL_SHADER(FGlintGhostCS, "/Plugin/Glint/Private/Glint.usf", "GlintGhostCS", SF_Compute);

class FGlintFindCandidatesCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FGlintFindCandidatesCS);
    SHADER_USE_PARAMETER_STRUCT(FGlintFindCandidatesCS, FGlobalShader);
public:
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BrightTexture)
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
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BrightTexture)
        SHADER_PARAMETER(FIntPoint, BrightSize)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SourceHistory)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, SourcesOutput)
        SHADER_PARAMETER(FIntPoint, CandidateSize)
        SHADER_PARAMETER(uint32, MaxSources)
        SHADER_PARAMETER(float, SourceMinBrightness)
        SHADER_PARAMETER(float, SourceSeparation)
        SHADER_PARAMETER(float, SourceGroupingRadius)
        SHADER_PARAMETER(float, SourceSmoothing)
        SHADER_PARAMETER(float, SourcePersistence)
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
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, SceneOutput)
        SHADER_PARAMETER(FIntPoint, FullSize)
        SHADER_PARAMETER(FIntPoint, SourceMin)
        SHADER_PARAMETER(FIntPoint, SourceSize)
        SHADER_PARAMETER(FIntPoint, BrightSize)
        SHADER_PARAMETER(float, FlareIntensity)
        SHADER_PARAMETER(uint32, DebugBrightPass)
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
        CVarGlintDebug.GetValueOnRenderThread() == 0 && CVarGlintDebugSources.GetValueOnRenderThread() == 0)
    {
        return Inputs.ReturnUntouchedSceneColorForPostProcessing(GraphBuilder);
    }

    FScreenPassTexture Scene = FScreenPassTexture::CopyFromSlice(
        GraphBuilder, Inputs.GetInput(EPostProcessMaterialInput::SceneColor));
    if (!Scene.IsValid()) return Scene;
    const FIntPoint ViewSize = Scene.ViewRect.Size();
    if (ViewSize.X < 4 || ViewSize.Y < 4) return Scene;
    const FIntPoint SmallSize = FIntPoint::DivideAndRoundUp(ViewSize, FIntPoint(4, 4));
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
    const bool bTrackSources = !Elements.IsEmpty() && Blended.bSourceDrivenAuthored &&
        CVarGlintSourceTracking.GetValueOnRenderThread() != 0;
    const bool bDebugSources = CVarGlintDebugSources.GetValueOnRenderThread() != 0;
    // Row 0 = UV/brightness/confidence; row 1 = the last reliable HDR source color.
    const FIntPoint SourceTextureSize(8, 2);
    const FRDGTextureDesc SourcesDesc = FRDGTextureDesc::Create2D(
        SourceTextureSize, PF_FloatRGBA, FClearValueBinding::Black,
        TexCreate_ShaderResource | TexCreate_UAV);
    FRDGTextureRef Sources = GraphBuilder.CreateTexture(SourcesDesc, TEXT("Glint.Sources"));
    AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(Sources), FLinearColor::Black);
    if (bTrackSources || bDebugSources)
    {
        const FIntPoint CandidateSize = FIntPoint::DivideAndRoundUp(SmallSize, FIntPoint(32, 32));
        const FRDGTextureDesc CandidateDesc = FRDGTextureDesc::Create2D(
            CandidateSize, PF_FloatRGBA, FClearValueBinding::Black,
            TexCreate_ShaderResource | TexCreate_UAV);
        FRDGTextureRef Candidates = GraphBuilder.CreateTexture(CandidateDesc, TEXT("Glint.SourceCandidates"));
        FGlintFindCandidatesCS::FParameters* Find = GraphBuilder.AllocParameters<FGlintFindCandidatesCS::FParameters>();
        Find->BrightTexture = Bright;
        Find->CandidateOutput = GraphBuilder.CreateUAV(Candidates);
        Find->BrightSize = SmallSize;
        FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("Glint.FindSourceCandidates"),
            GetGlobalShaderMap(View.GetFeatureLevel())->GetShader<FGlintFindCandidatesCS>(), Find,
            FIntVector(CandidateSize.X, CandidateSize.Y, 1));

        TSharedPtr<FGlintSourceHistory> History;
        FRDGTextureRef Previous = GraphBuilder.CreateTexture(SourcesDesc, TEXT("Glint.PreviousSourcesDefault"));
        AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(Previous), FLinearColor::Black);
        bool bResetHistory = true;
        if (View.State)
        {
            const FSceneViewStateInterface* ViewKey = View.State;
            TSharedPtr<FGlintSourceHistory>& EntryHistory = SourceHistories.FindOrAdd(ViewKey);
            if (!EntryHistory.IsValid()) EntryHistory = MakeShared<FGlintSourceHistory>();
            History = EntryHistory;
            History->LastUsed = ++HistoryVisitCounter;
            bResetHistory = View.bCameraCut || History->ViewSize != ViewSize || !History->Previous.IsValid();
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
        Select->BrightTexture = Bright;
        Select->BrightSize = SmallSize;
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
    Ghost->ElementCount = uint32(Elements.Num());
    Ghost->SourcePositions = Sources;
    Ghost->MaxSources = uint32(FMath::Clamp(FMath::Min(Blended.MaxSources, CVarGlintMaxSources.GetValueOnRenderThread()), 1, 8));
    Ghost->UseSourcePositions = bTrackSources ? 1u : 0u;
    Ghost->DebugSources = bDebugSources ? 1u : 0u;
    Ghost->SourceEdgeFade = Blended.SourceEdgeFade;
    Ghost->SourceGain = Blended.SourceGain;
    Ghost->AuthoredEnergyLimit = Blended.AuthoredEnergyLimit;
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
    Composite->SceneOutput = GraphBuilder.CreateUAV(Combined);
    Composite->FullSize = FullDesc.Extent;
    Composite->SourceMin = Scene.ViewRect.Min;
    Composite->SourceSize = ViewSize;
    Composite->BrightSize = SmallSize;
    Composite->FlareIntensity = Blended.Intensity * FMath::Min(TotalWeight, 1.f);
    Composite->DebugBrightPass = CVarGlintDebug.GetValueOnRenderThread() ? 1u : 0u;
    FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("Glint.CompositeHDR"),
        GetGlobalShaderMap(View.GetFeatureLevel())->GetShader<FGlintCompositeCS>(), Composite,
        FComputeShaderUtils::GetGroupCount(FullDesc.Extent, FIntPoint(8, 8)));

    return FScreenPassTexture(Combined, Scene.ViewRect);
}

#include "GlintSceneViewExtension.h"
#include "GlintLensProfile.h"
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
#include "ScreenPass.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarGlintEnabled(
    TEXT("r.Glint.Enabled"), 1,
    TEXT("Enable Glint when a Glint Lens is present in per-view blendables."),
    ECVF_RenderThreadSafe | ECVF_Scalability);
static TAutoConsoleVariable<int32> CVarGlintDebug(
    TEXT("r.Glint.DebugBrightPass"), 0,
    TEXT("Show the Glint HDR bright-pass instead of the final composite."),
    ECVF_RenderThreadSafe);

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
        SHADER_PARAMETER(FIntPoint, BrightSize)
        SHADER_PARAMETER(float, GhostStrength)
        SHADER_PARAMETER(float, StreakStrength)
        SHADER_PARAMETER(float, ChromaticShift)
        SHADER_PARAMETER(float, GhostSpacing)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Params)
    { return IsFeatureLevelSupported(Params.Platform, ERHIFeatureLevel::SM5); }
};
IMPLEMENT_GLOBAL_SHADER(FGlintGhostCS, "/Plugin/Glint/Private/Glint.usf", "GlintGhostCS", SF_Compute);

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

void FGlintSceneViewExtension::SetupViewPoint(APlayerController* Player, FMinimalViewInfo& InViewInfo)
{
    // GT camera callback. Precedence: player-controller binding > project default.
    // Camera ownership and optical response remain independent of the camera stack.
    if (!Player) return;
    const UGlintProjectSettings* Settings = GetDefault<UGlintProjectSettings>();
    const FSoftObjectPath RequestedPath = Settings->DefaultLensProfile.ToSoftObjectPath();
    if (RequestedPath != DefaultLensPath)
    {
        DefaultLensPath = RequestedPath;
        // Only loads when changed, never on the render thread or every frame.
        DefaultLensAsset.Reset(RequestedPath.IsNull() ? nullptr : Settings->DefaultLensProfile.LoadSynchronous());
    }

    const UGlintCameraBindingComponent* Binding = Player->FindComponentByClass<UGlintCameraBindingComponent>();
    UGlintLensProfile* Lens = DefaultLensAsset.Get();
    float Weight = 1.f;
    if (Binding && Binding->LensProfile)
    {
        Lens = Binding->LensProfile;
        Weight = FMath::Clamp(Binding->BlendWeight, 0.f, 1.f);
    }
    if (!Lens || Weight <= 0.f) return;

    InViewInfo.PostProcessSettings.WeightedBlendables.Array.Add(FWeightedBlendable(Weight, Lens));
    // A viewpoint whose normal postprocessing weight is zero must still apply the
    // blendable. This intentionally participates in the camera's PP blend settings.
    InViewInfo.PostProcessBlendWeight = 1.f;
}

void FGlintSceneViewExtension::SubscribeToPostProcessingPass(
    EPostProcessingPass Pass, const FSceneView& View,
    FPostProcessingPassDelegateArray& Callbacks, bool bIsPassEnabled)
{
    // In UE5.8, this callback is executed after the MotionBlur stage and before the
    // SceneColorBeforeBloom post-process material chain. It also runs when blur is off.
    if (Pass == EPostProcessingPass::MotionBlur &&
        CVarGlintEnabled.GetValueOnAnyThread() != 0)
    {
        Callbacks.Add(FPostProcessingPassDelegate::CreateRaw(
            this, &FGlintSceneViewExtension::AddGlintPass));
    }
}

FScreenPassTexture FGlintSceneViewExtension::AddGlintPass(
    FRDGBuilder& GraphBuilder, const FSceneView& View,
    const FPostProcessMaterialInputs& Inputs) const
{
    // Render thread: only inspect the immutable, copied view payload.
    FGlintLensBlendData Blended;
    float TotalWeight = 0.f;
    FBlendableEntry* Entry = nullptr;
    const FBlendableManager& Manager = View.FinalPostProcessSettings.BlendableManager;
    while (const FGlintLensBlendData* Lens = Manager.IterateBlendables<FGlintLensBlendData>(Entry))
    {
        const float W = FMath::Clamp(Entry->Weight, 0.f, 1.f);
        if (W > 0.f)
        {
            if (TotalWeight == 0.f) Blended = *Lens;
            else
            {
                const float Alpha = W / (TotalWeight + W);
                Blended.Intensity = FMath::Lerp(Blended.Intensity, Lens->Intensity, Alpha);
                Blended.Threshold = FMath::Lerp(Blended.Threshold, Lens->Threshold, Alpha);
                Blended.SoftKnee = FMath::Lerp(Blended.SoftKnee, Lens->SoftKnee, Alpha);
                Blended.GhostStrength = FMath::Lerp(Blended.GhostStrength, Lens->GhostStrength, Alpha);
                Blended.StreakStrength = FMath::Lerp(Blended.StreakStrength, Lens->StreakStrength, Alpha);
                Blended.ChromaticShift = FMath::Lerp(Blended.ChromaticShift, Lens->ChromaticShift, Alpha);
                Blended.GhostSpacing = FMath::Lerp(Blended.GhostSpacing, Lens->GhostSpacing, Alpha);
            }
            TotalWeight += W;
        }
    }

    if (View.GetFeatureLevel() < ERHIFeatureLevel::SM5 ||
        TotalWeight <= 0.f || Blended.Intensity <= 0.f || Inputs.OverrideOutput.Texture)
    {
        // No profile: zero rendering work. Override output cases defer until tested.
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
    Extract->BrightThreshold = Blended.Threshold;
    Extract->KneeWidth = Blended.SoftKnee;
    FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("Glint.Extract"),
        GetGlobalShaderMap(View.GetFeatureLevel())->GetShader<FGlintExtractCS>(), Extract,
        FComputeShaderUtils::GetGroupCount(SmallSize, FIntPoint(8, 8)));

    FGlintGhostCS::FParameters* Ghost = GraphBuilder.AllocParameters<FGlintGhostCS::FParameters>();
    Ghost->BrightTexture = Bright;
    Ghost->GhostOutput = GraphBuilder.CreateUAV(Ghosts);
    Ghost->BrightSize = SmallSize;
    Ghost->GhostStrength = Blended.GhostStrength;
    Ghost->StreakStrength = Blended.StreakStrength;
    Ghost->ChromaticShift = Blended.ChromaticShift;
    Ghost->GhostSpacing = Blended.GhostSpacing;
    FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("Glint.FieldGhosts"),
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

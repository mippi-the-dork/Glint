#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/BlendableInterface.h"
#include "GlintLensProfile.generated.h"

/** HLSL-compatible, 80-byte payload for one authored optical element. */
struct GLINT_API FGlintRuntimeElement
{
    FVector4f ColorStrength = FVector4f(1.f, 1.f, 1.f, 0.f); // RGB + intensity
    FVector4f Geometry = FVector4f(0.f, -1.f, 1.f, 0.f); // type, axis position, size, rotation in radians
    FVector4f Offset = FVector4f(0.f, 0.f, 0.f, 0.f); // normalized XY displacement
    FVector4f Shape = FVector4f(3.5f, 0.16f, 1.f, 6.f); // falloff, ring width, aspect, rays
    FVector4f Detail = FVector4f(55.f, 0.35f, 8.f, 0.f); // ray sharpness, secondary, frequency
};

/** Render-thread payload. No UObject references; its content is snapshotted on the game thread. */
struct GLINT_API FGlintLensBlendData
{
    static constexpr int32 MaxElementsPerLens = 32;
    static const FName& GetFName()
    {
        static const FName Name(TEXT("GlintLensBlendData"));
        return Name;
    }

    float Intensity = 0.75f;
    float Threshold = 1.75f;
    float SoftKnee = 0.5f;
    float PeakMix = 0.35f;
    float MaxContribution = 64.f;
    float GhostStrength = 0.6f;
    float StreakStrength = 0.5f;
    float ChromaticShift = 0.003f;
    float GhostSpacing = 0.55f;
    float StreakLength = 1.f;
    int32 GhostCount = 4;
    bool bSourceDrivenAuthored = true;
    int32 MaxSources = 8;
    float SourceMinBrightness = 0.025f;
    float SourceSeparation = 0.04f;
    float SourceGroupingRadius = 0.125f; // Collapse multiple tile peaks from one luminous region.
    float SourceSmoothing = 0.35f;
    float SourcePersistence = 0.65f; // Retain confidence briefly if a bright candidate is lost for one or more frames.
    float SourceHysteresis = 0.65f; // Previously tracked sources can remain below the acquisition gate.
    float SourcePositionDeadZone = 0.0015f; // Normalized viewport UV deadzone against small jitter.
    float SourceEdgeFade = 0.08f;
    float SourceGain = 1.f;
    float AuthoredEnergyLimit = 6.f; // Soft HDR shoulder for overlapping authored optical elements.
    bool bFollowCameraAperture = false;
    float ReferenceFStop = 2.8f;
    float ApertureScaleResponse = 0.35f;
    bool bEnableAutomatic = true;
    uint32 ElementCount = 0;
    FGlintRuntimeElement Elements[MaxElementsPerLens];
};

/** Assign to a camera or Post Process Volume through Weighted Blendables. */
// A lens is a Content Browser asset. UDataAsset inherits EditInlineNew, which
// incorrectly adds a Create New choice inside Post Process Weighted Blendables.
UCLASS(BlueprintType, NotEditInlineNew)
class GLINT_API UGlintLensProfile : public UDataAsset, public IBlendableInterface
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Overview", meta=(ClampMin="0.0", UIMax="3.0"))
    float Intensity = 0.75f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Highlights", meta=(ClampMin="0.0", UIMax="20.0", ToolTip="Threshold in scene-linear luminance; Glint compensates for Unreal pre-exposure."))
    float Threshold = 1.75f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Highlights", meta=(ClampMin="0.0", ClampMax="1.0"))
    float SoftKnee = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Highlights", meta=(ClampMin="0.0", ClampMax="1.0", ToolTip="0 = stable neighborhood average; 1 = preserve the brightest subpixel."))
    float PeakMix = 0.35f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Highlights", meta=(ClampMin="1.0", UIMax="128.0", ToolTip="Cap extracted per-pixel flare energy after thresholding (HDR units)."))
    float MaxContribution = 64.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Automatic", meta=(ToolTip="Disable the built-in ghost/streak field without disabling the authored Glint Flare."))
    bool bEnableAutomatic = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Automatic", meta=(ClampMin="0.0", UIMax="2.0"))
    float GhostStrength = 0.6f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Automatic", meta=(ClampMin="0", ClampMax="8"))
    int32 GhostCount = 4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Automatic", meta=(ClampMin="0.0", UIMax="2.0"))
    float StreakStrength = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Automatic", meta=(ClampMin="0.25", UIMax="4.0"))
    float StreakLength = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics", meta=(ClampMin="0.0", UIMax="0.02"))
    float ChromaticShift = 0.003f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics", meta=(ClampMin="0.05", UIMax="1.0"))
    float GhostSpacing = 0.55f;
    /** Draw authored optics around GPU-detected HDR sources. Disabling turns off
        authored drawing without changing automatic ghosts/streaks. The deprecated
        image-warp renderer is available only through r.Glint.LegacyAuthoredField. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source Detection",
        meta=(ToolTip="Enable source-relative authored optical elements. OFF disables authored drawing and does not affect automatic flares. The old image-field approximation is diagnostic-only: r.Glint.LegacyAuthoredField 1."))
    bool bSourceDrivenAuthored = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source Detection", meta=(ClampMin="1", ClampMax="8")) int32 MaxSources = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source Detection", meta=(ClampMin="0.001", UIMax="1.0", ToolTip="Minimum extracted HDR brightness needed for an individual flare source.")) float SourceMinBrightness = 0.025f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source Detection", meta=(ClampMin="0.005", ClampMax="0.25", ToolTip="Minimum distance between independent candidate sources, measured in viewport UV.")) float SourceSeparation = 0.04f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source Detection", meta=(ClampMin="0.0", ClampMax="0.3", UIMax="0.2", ToolTip="Merge nearby HDR maxima from the same luminous region before creating independent flares. Increase for broad suns; decrease for tightly spaced separate lamps. Zero falls back to Source Separation."))
    float SourceGroupingRadius = 0.125f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source Detection", meta=(ClampMin="0.0", ClampMax="1.0", ToolTip="Controls smoothing of tracked source positions when motion is small. Rapid motion adapts automatically to prevent lag.")) float SourceSmoothing = 0.35f;
    /** Reduces one-frame source dropouts from HDR threshold changes, TAA or fast camera motion. Not an occlusion override. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source Detection", meta=(ClampMin="0.0", ClampMax="0.95", ToolTip="Per-frame confidence retained when a source temporarily disappears from the bright candidate image. Zero disables persistence; ~0.65 fades after a few frames. Camera cuts reset all histories."))
    float SourcePersistence = 0.65f;
    /** Once a source is being tracked, retain it down to this fraction of the acquisition
        threshold. Stabilizes brightness jitter from temporal reconstruction and pre-exposure. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source Detection", meta=(ClampMin="0.25", ClampMax="1.0", ToolTip="Release threshold as a fraction of the source acquisition threshold. 0.65 means an existing source can remain visible at 65% of the acquisition threshold; new sources still require the full threshold."))
    float SourceHysteresis = 0.65f;
    /** Do not move tracked source centers for subpixel/temporal jitter. In normalized viewport UV. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source Detection", meta=(ClampMin="0.0", ClampMax="0.01", UIMax="0.005", ToolTip="Normalized UV dead zone for stationary-source position changes. Default 0.0015 is around 1-3 pixels depending on viewport size. Larger values are steadier but can cause stepped motion."))
    float SourcePositionDeadZone = 0.0015f;
    /** Smoothly fade image-selected sources as they leave any screen edge. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source Detection", meta=(ClampMin="0.0", ClampMax="0.25", ToolTip="Normalized distance from the viewport edge over which source-aware flares fade out. Zero disables edge fade."))
    float SourceEdgeFade = 0.08f;
    /** Apply a gain to tracked-source authored optics without changing the automatic HDR field. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source Detection", meta=(ClampMin="0.0", ClampMax="4.0"))
    float SourceGain = 1.f;
    /** Soft HDR shoulder for all authored elements after they have been combined. Zero disables. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Composition", meta=(DisplayName="Authored Energy Soft Limit", ClampMin="0.0", UIMax="32.0", ToolTip="HDR soft shoulder, not an intensity gain or hard maximum. Larger values approach unmodified brightness; zero disables. Use Source Gain / element Strength to raise flare intensity."))
    float AuthoredEnergyLimit = 6.f;
    /** Uses the resolved FPostProcessSettings aperture, which is also populated by Cine Cameras and Gameplay Cameras. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera Optics") bool bFollowCameraAperture = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera Optics", meta=(ClampMin="0.1", ClampMax="32.0")) float ReferenceFStop = 2.8f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera Optics", meta=(ClampMin="0.0", ClampMax="2.0")) float ApertureScaleResponse = 0.35f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Composition", meta=(ToolTip="Elements are rendered from the same HDR highlight field as the automatic flare, without requiring light actors."))
    TObjectPtr<class UGlintFlare> DetailedFlare = nullptr;

    /** Camera-system-independent aperture correction, computed from the evaluated final rendered view. */
    static float CalculateApertureScale(float CameraFStop, float ReferenceFStop, float Response);
    /** Shared sanitization/packing for runtime, editor GPU preview, and tests. */
    static FGlintRuntimeElement PackElement(const struct FGlintElement& Element);
    FGlintLensBlendData MakeRuntimeData() const;
    virtual void OverrideBlendableSettings(class FSceneView& View, float Weight) const override;
};

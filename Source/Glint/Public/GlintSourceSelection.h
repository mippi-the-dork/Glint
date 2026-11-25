#pragma once

#include "CoreMinimal.h"

/** Explicit authored rendering choice; the legacy image-warp renderer is diagnostic-only. */
enum class EGlintAuthoredRenderMode : uint8
{
    Disabled,
    SourceTracked,
    LegacyImageField
};

/** CPU reference for source selection parity tests. Runtime source tracking is GPU-only. */
struct GLINT_API FGlintSourcePoint
{
    FVector2f UV = FVector2f::ZeroVector;
    float Brightness = 0.f;
    float Confidence = 1.f;
    FLinearColor Color = FLinearColor::White; // Last reliable HDR candidate color; retained during short dropouts.
};

struct GLINT_API FGlintSourceSelection
{
    /** Keep authored/automatic modes independent. False does not imply legacy field rendering. */
    static EGlintAuthoredRenderMode ResolveAuthoredMode(bool bHasElements,
        bool bSourceDriven, bool bSourceTrackingEnabled, bool bAllowLegacyField);

    /** Peak-based source acquisition ignores the artistic bright-pass RGB threshold. */
    static float CalculatePeakGate(float MinimumSourceBrightness, float LensThreshold, float PreExposure);
    /** Normalized UV history survives dynamic-resolution changes with stable aspect ratio. */
    static bool ShouldResetHistory(FIntPoint PreviousViewSize, FIntPoint CurrentViewSize,
        bool bCameraCut, bool bHasHistory);
    /** A tracked source releases at a lower gate than a brand-new source acquires. */
    static float CalculateReleaseGate(float AcquireGate, float Hysteresis);
    /** 0 for an unmoving view, 1 for fast rotation; independent of source tile jitter. */
    static float CameraMotionFactor(float AngularDeltaRadians);
    /** Coarse fallback for camera translation without depth-based reprojection. */
    static float CameraTranslationFactor(float CameraTranslationUU);
    /** Hold a stationary source while raw HDR still confirms light at the old UV. */
    static bool ShouldHoldStationarySource(float LocalHDRPeak, float ReleaseGate, float CameraMotion);
    /** Automatic source optics and authored source optics share detection, not image warping. */
    static bool RequiresSources(bool bAuthoredSource, bool bAutomaticSource, bool bDebugSources);

    /** Shader-equivalent rank: raw HDR peak plus coherent, smoothed luminance.
        No scene/flare image is blurred; rank is distinct from brightness gate. */
    static float CalculateDetectionRank(float RawPeak, float FilteredLuminance);
    /** Low-resolution HDR luminance filter, CPU reference for the detection shader.
        Edges discard off-image samples instead of repeating bright border pixels. */
    static float FilterDetectionLuminance(const TArray<float>& Grid, FIntPoint Size,
        FIntPoint Pixel, int32 Radius);
    /** CPU parity for infinite-depth, unjittered screen-space source reprojection.
        The shader applies this matrix to previous source UV before matching. */
    static FVector2f ReprojectSourceUV(FVector2f PreviousUV,
        const FMatrix& PreviousToCurrentClip);
    /** Reject very weak new regions without evicting already tracked sources. */
    static float CalculateRelativeScoreFloor(float StrongestRank, float Ratio);
    /** A challenger can evict only after sustained superiority. */
    static bool CanReplaceSource(float ChallengerRank, float TrackedRank, float Ratio,
        int32 ConsecutiveFrames, int32 RequiredFrames);
    /** A nearby competing source must be visibly separate, not subpixel jitter. */
    static bool IsDistinctChallenger(float NormalizedDistance, float PositionDeadZone);
    /** Resets confirmation when a stronger region switches screen position. */
    static int32 AdvanceChallenger(FVector2f PreviousUV, FVector2f CurrentUV,
        int32 PreviousCount, float MatchRadius);

    /** Pick brightest candidates while preventing nearby peaks from spawning duplicates. */
    static TArray<FGlintSourcePoint> Select(const TArray<FGlintSourcePoint>& Candidates,
        int32 Limit, float MinimumBrightness, float MinimumDistance);

    /** CPU reference for GPU grouping radius, preserves separate highlights beyond the configured region. */
    static TArray<FGlintSourcePoint> SelectGrouped(const TArray<FGlintSourcePoint>& Candidates,
        int32 Limit, float MinimumBrightness, float MinimumDistance, float GroupingRadius);

    /** CPU reference to soft HDR authored energy shoulder (zero disables). */
    static FLinearColor LimitAuthoredEnergy(FLinearColor Input, float Limit);

    /** CPU reference for acquisition/release hysteresis and source-slot continuity.
        This runs in automation tests; the shipping tracker remains GPU-only. */
    static TArray<FGlintSourcePoint> StabilizeHysteresis(
        const TArray<FGlintSourcePoint>& Detected, const TArray<FGlintSourcePoint>& Previous,
        int32 MaxSources, float AcquireGate, float Hysteresis, float PositionResponse,
        float Persistence, float PositionDeadZone, bool bCameraCut, float CameraMotion = 0.f);

    /** Match by screen distance and smooth tracked positions, resetting on camera cuts. */
    static void Smooth(TArray<FGlintSourcePoint>& Selected, const TArray<FGlintSourcePoint>& Previous,
        float PositionResponse, bool bCameraCut);

    /** CPU reference for the GPU's adaptive camera-motion response and short dropout persistence. */
    static TArray<FGlintSourcePoint> Stabilize(const TArray<FGlintSourcePoint>& Detected,
        const TArray<FGlintSourcePoint>& Previous, int32 MaxSources, float PositionResponse,
        float Persistence, bool bCameraCut);
};

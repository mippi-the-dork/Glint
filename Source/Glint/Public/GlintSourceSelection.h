#pragma once

#include "CoreMinimal.h"

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
    /** Peak-based source acquisition ignores the artistic bright-pass RGB threshold. */
    static float CalculatePeakGate(float MinimumSourceBrightness, float LensThreshold, float PreExposure);

    /** Pick brightest candidates while preventing nearby peaks from spawning duplicates. */
    static TArray<FGlintSourcePoint> Select(const TArray<FGlintSourcePoint>& Candidates,
        int32 Limit, float MinimumBrightness, float MinimumDistance);

    /** Match by screen distance and smooth tracked positions, resetting on camera cuts. */
    static void Smooth(TArray<FGlintSourcePoint>& Selected, const TArray<FGlintSourcePoint>& Previous,
        float PositionResponse, bool bCameraCut);

    /** CPU reference for the GPU's adaptive camera-motion response and short dropout persistence. */
    static TArray<FGlintSourcePoint> Stabilize(const TArray<FGlintSourcePoint>& Detected,
        const TArray<FGlintSourcePoint>& Previous, int32 MaxSources, float PositionResponse,
        float Persistence, bool bCameraCut);
};

#include "GlintSourceSelection.h"

float FGlintSourceSelection::CalculatePeakGate(float MinimumSourceBrightness, float LensThreshold, float PreExposure)
{
    return FMath::Max(FMath::Max(0.f, MinimumSourceBrightness), FMath::Max(0.f, LensThreshold) * 0.5f)
        * FMath::Max(0.001f, PreExposure);
}

TArray<FGlintSourcePoint> FGlintSourceSelection::Select(const TArray<FGlintSourcePoint>& Candidates,
    int32 Limit, float MinimumBrightness, float MinimumDistance)
{
    TArray<FGlintSourcePoint> Selected;
    const int32 SafeLimit = FMath::Clamp(Limit, 0, 8);
    const float SafeRadiusSq = FMath::Square(FMath::Max(0.f, MinimumDistance));
    for (int32 SourceIndex = 0; SourceIndex < SafeLimit; ++SourceIndex)
    {
        const FGlintSourcePoint* Best = nullptr;
        for (const FGlintSourcePoint& Candidate : Candidates)
        {
            if (!FMath::IsFinite(Candidate.Brightness) || Candidate.Brightness <= MinimumBrightness ||
                (Best && Candidate.Brightness <= Best->Brightness)) continue;
            bool bNearExisting = false;
            for (const FGlintSourcePoint& Existing : Selected)
            {
                if (FVector2f::DistSquared(Candidate.UV, Existing.UV) < SafeRadiusSq)
                { bNearExisting = true; break; }
            }
            if (!bNearExisting) Best = &Candidate;
        }
        if (!Best) break;
        Selected.Add(*Best);
    }
    return Selected;
}

void FGlintSourceSelection::Smooth(TArray<FGlintSourcePoint>& Selected,
    const TArray<FGlintSourcePoint>& Previous, float PositionResponse, bool bCameraCut)
{
    if (bCameraCut || Previous.IsEmpty()) return;
    const float Alpha = FMath::Clamp(PositionResponse, 0.f, 1.f);
    TArray<bool> Used;
    Used.Init(false, Previous.Num());
    for (FGlintSourcePoint& Current : Selected)
    {
        float NearestSq = FMath::Square(0.11f);
        int32 Match = INDEX_NONE;
        for (int32 Index = 0; Index < Previous.Num(); ++Index)
        {
            if (Used[Index] || Previous[Index].Confidence < 0.001f) continue;
            const float DistanceSq = FVector2f::DistSquared(Current.UV, Previous[Index].UV);
            if (DistanceSq < NearestSq) { NearestSq = DistanceSq; Match = Index; }
        }
        if (Match != INDEX_NONE)
        {
            Used[Match] = true;
            Current.UV = FMath::Lerp(Previous[Match].UV, Current.UV, Alpha);
        }
    }
}

TArray<FGlintSourcePoint> FGlintSourceSelection::Stabilize(
    const TArray<FGlintSourcePoint>& Detected, const TArray<FGlintSourcePoint>& Previous,
    int32 MaxSources, float PositionResponse, float Persistence, bool bCameraCut)
{
    TArray<FGlintSourcePoint> Output;
    const int32 Cap = FMath::Clamp(MaxSources, 0, 8);
    const float Retention = FMath::Clamp(Persistence, 0.f, 0.95f);
    TArray<bool> Used;
    Used.Init(false, Previous.Num());

    for (const FGlintSourcePoint& Current : Detected)
    {
        if (Output.Num() >= Cap) break;
        FGlintSourcePoint Resolved = Current;
        Resolved.Confidence = 1.f;
        if (!bCameraCut)
        {
            int32 Match = INDEX_NONE;
            float NearestDistance = 0.20f;
            for (int32 Index = 0; Index < Previous.Num(); ++Index)
            {
                const float Distance = FMath::Sqrt(FVector2f::DistSquared(Current.UV, Previous[Index].UV));
                if (!Used[Index] && Previous[Index].Confidence > 0.05f && Distance < NearestDistance)
                {
                    NearestDistance = Distance;
                    Match = Index;
                }
            }
            if (Match != INDEX_NONE)
            {
                Used[Match] = true;
                const float MotionResponse = FMath::Lerp(FMath::Clamp(PositionResponse, 0.f, 1.f),
                    1.f, FMath::Clamp((NearestDistance - 0.012f) / 0.10f, 0.f, 1.f));
                Resolved.UV = FMath::Lerp(Previous[Match].UV, Current.UV, MotionResponse);
                Resolved.Color = FMath::Lerp(Previous[Match].Color, Current.Color, MotionResponse);
            }
        }
        Output.Add(Resolved);
    }
    if (bCameraCut || Retention == 0.f) return Output;
    // A candidate must be *truly missing* to use a cached source. A nearby
    // measured highlight supersedes its old location and avoids double-ghosting.
    while (Output.Num() < Cap)
    {
        int32 Best = INDEX_NONE;
        float BestConfidence = 0.12f;
        for (int32 Index = 0; Index < Previous.Num(); ++Index)
        {
            if (Used[Index] || Previous[Index].Confidence <= BestConfidence) continue;
            bool bNearMeasured = false;
            for (const FGlintSourcePoint& Current : Detected)
                if (FMath::Sqrt(FVector2f::DistSquared(Previous[Index].UV, Current.UV)) < 0.18f)
                    bNearMeasured = true;
            if (!bNearMeasured)
            {
                Best = Index;
                BestConfidence = Previous[Index].Confidence;
            }
        }
        if (Best == INDEX_NONE) break;
        Used[Best] = true;
        FGlintSourcePoint Carried = Previous[Best];
        Carried.Confidence *= Retention;
        if (Carried.Confidence >= 0.12f) Output.Add(Carried);
    }
    return Output;
}

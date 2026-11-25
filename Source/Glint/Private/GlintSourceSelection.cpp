#include "GlintSourceSelection.h"

EGlintAuthoredRenderMode FGlintSourceSelection::ResolveAuthoredMode(bool bHasElements,
    bool bSourceDriven, bool bSourceTrackingEnabled, bool bAllowLegacyField)
{
    if (!bHasElements) return EGlintAuthoredRenderMode::Disabled;
    if (bSourceDriven && bSourceTrackingEnabled) return EGlintAuthoredRenderMode::SourceTracked;
    return bAllowLegacyField ? EGlintAuthoredRenderMode::LegacyImageField
                             : EGlintAuthoredRenderMode::Disabled;
}

float FGlintSourceSelection::CalculatePeakGate(float MinimumSourceBrightness, float LensThreshold, float PreExposure)
{
    return FMath::Max(FMath::Max(0.f, MinimumSourceBrightness), FMath::Max(0.f, LensThreshold) * 0.5f)
        * FMath::Max(0.001f, PreExposure);
}

bool FGlintSourceSelection::ShouldResetHistory(FIntPoint PreviousViewSize,
    FIntPoint CurrentViewSize, bool bCameraCut, bool bHasHistory)
{
    if (bCameraCut || !bHasHistory || PreviousViewSize.X <= 0 || PreviousViewSize.Y <= 0 ||
        CurrentViewSize.X <= 0 || CurrentViewSize.Y <= 0) return true;
    // GPU source histories store *normalized view UV*, never absolute pixels.
    // A TAA/TSR dynamic-resolution change does not invalidate those coordinates.
    const double OldAspect = double(PreviousViewSize.X) / double(PreviousViewSize.Y);
    const double NewAspect = double(CurrentViewSize.X) / double(CurrentViewSize.Y);
    return FMath::Abs(OldAspect - NewAspect) / FMath::Max(OldAspect, 0.0001) > 0.02;
}

float FGlintSourceSelection::CalculateReleaseGate(float AcquireGate, float Hysteresis)
{
    return FMath::Max(0.f, AcquireGate) * FMath::Clamp(Hysteresis, 0.25f, 1.f);
}

float FGlintSourceSelection::CameraMotionFactor(float AngularDeltaRadians)
{
    return FMath::Clamp(FMath::Max(0.f, AngularDeltaRadians) / 0.025f, 0.f, 1.f);
}

float FGlintSourceSelection::CameraTranslationFactor(float CameraTranslationUU)
{
    return FMath::Clamp(FMath::Max(0.f, CameraTranslationUU) / 50.f, 0.f, 1.f);
}

bool FGlintSourceSelection::ShouldHoldStationarySource(float LocalHDRPeak,
    float ReleaseGate, float CameraMotion)
{
    return CameraMotion < 0.08f && FMath::IsFinite(LocalHDRPeak)
        && LocalHDRPeak >= FMath::Max(0.f, ReleaseGate);
}

bool FGlintSourceSelection::RequiresSources(bool bAuthoredSource, bool bAutomaticSource, bool bDebugSources)
{
    return bAuthoredSource || bAutomaticSource || bDebugSources;
}

float FGlintSourceSelection::CalculateDetectionRank(float RawPeak, float FilteredLuminance)
{
    const float Peak = FMath::Max(0.f, RawPeak);
    const float Region = FMath::Max(0.f, FilteredLuminance);
    const float Contrast = FMath::Clamp((Peak - Region) / FMath::Max(Region, 0.1f), 0.f, 1.f);
    return FMath::Sqrt(Peak) * (0.65f + 1.65f * Contrast) + 0.30f * FMath::Sqrt(Region);
}

float FGlintSourceSelection::FilterDetectionLuminance(const TArray<float>& Grid,
    FIntPoint Size, FIntPoint Pixel, int32 Radius)
{
    if (Size.X <= 0 || Size.Y <= 0 || Grid.Num() != Size.X * Size.Y ||
        Pixel.X < 0 || Pixel.Y < 0 || Pixel.X >= Size.X || Pixel.Y >= Size.Y)
        return 0.f;
    const int32 SafeRadius = FMath::Clamp(Radius, 0, 2);
    float Weighted = 0.f, Sum = 0.f;
    for (int32 Y = -SafeRadius; Y <= SafeRadius; ++Y)
    for (int32 X = -SafeRadius; X <= SafeRadius; ++X)
    {
        const int32 Px = Pixel.X + X, Py = Pixel.Y + Y;
        if (Px < 0 || Py < 0 || Px >= Size.X || Py >= Size.Y) continue;
        const float W = float((SafeRadius + 1 - FMath::Abs(X)) * (SafeRadius + 1 - FMath::Abs(Y)));
        Weighted += FMath::Max(0.f, Grid[Py * Size.X + Px]) * W;
        Sum += W;
    }
    return Weighted / FMath::Max(1.f, Sum);
}

FVector2f FGlintSourceSelection::ReprojectSourceUV(FVector2f PreviousUV,
    const FMatrix& PreviousToCurrentClip)
{
    const FVector4 PreviousClip(double(PreviousUV.X)*2.0-1.0,
        1.0-double(PreviousUV.Y)*2.0,0.0,1.0);
    const FVector4 CurrentClip = PreviousToCurrentClip.TransformFVector4(PreviousClip);
    if (CurrentClip.W <= 1.0e-5 || !FMath::IsFinite(CurrentClip.W)) return PreviousUV;
    const FVector2f CurrentUV(float(0.5+0.5*CurrentClip.X/CurrentClip.W),
        float(0.5-0.5*CurrentClip.Y/CurrentClip.W));
    return (FMath::IsFinite(CurrentUV.X) && FMath::IsFinite(CurrentUV.Y) &&
        FMath::Abs(CurrentUV.X)<=4.f && FMath::Abs(CurrentUV.Y)<=4.f)
        ? CurrentUV : PreviousUV;
}

float FGlintSourceSelection::CalculateRelativeScoreFloor(float StrongestRank, float Ratio)
{
    return FMath::Max(0.f, StrongestRank) * FMath::Clamp(Ratio, 0.f, 1.f);
}

bool FGlintSourceSelection::CanReplaceSource(float ChallengerRank, float TrackedRank,
    float Ratio, int32 ConsecutiveFrames, int32 RequiredFrames)
{
    return ConsecutiveFrames >= FMath::Max(2, RequiredFrames) &&
        ChallengerRank > FMath::Max(0.f, TrackedRank) * FMath::Max(1.05f, Ratio);
}

bool FGlintSourceSelection::IsDistinctChallenger(float NormalizedDistance, float PositionDeadZone)
{
    return NormalizedDistance > FMath::Max(0.025f, PositionDeadZone*8.f);
}

int32 FGlintSourceSelection::AdvanceChallenger(FVector2f PreviousUV, FVector2f CurrentUV,
    int32 PreviousCount, float MatchRadius)
{
    return FVector2f::DistSquared(PreviousUV, CurrentUV) < FMath::Square(FMath::Max(0.01f, MatchRadius))
        ? PreviousCount + 1 : 1;
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

TArray<FGlintSourcePoint> FGlintSourceSelection::SelectGrouped(
    const TArray<FGlintSourcePoint>& Candidates, int32 Limit, float MinimumBrightness,
    float MinimumDistance, float GroupingRadius)
{
    return Select(Candidates, Limit, MinimumBrightness,
        FMath::Max(MinimumDistance, FMath::Clamp(GroupingRadius, 0.f, 0.3f)));
}

FLinearColor FGlintSourceSelection::LimitAuthoredEnergy(FLinearColor Input, float Limit)
{
    if (Limit <= 0.001f) return Input;
    const float Luminance = 0.2126f * FMath::Max(0.f, Input.R)
                          + 0.7152f * FMath::Max(0.f, Input.G)
                          + 0.0722f * FMath::Max(0.f, Input.B);
    const float Scale = Limit / (Limit + Luminance);
    return FLinearColor(Input.R * Scale, Input.G * Scale, Input.B * Scale, Input.A);
}

TArray<FGlintSourcePoint> FGlintSourceSelection::StabilizeHysteresis(
    const TArray<FGlintSourcePoint>& Detected, const TArray<FGlintSourcePoint>& Previous,
    int32 MaxSources, float AcquireGate, float Hysteresis, float PositionResponse,
    float Persistence, float PositionDeadZone, bool bCameraCut, float CameraMotion)
{
    TArray<FGlintSourcePoint> Output;
    const int32 Cap = FMath::Clamp(MaxSources, 0, 8);
    const float ReleaseGate = CalculateReleaseGate(AcquireGate, Hysteresis);
    const float Motion = FMath::Clamp(CameraMotion, 0.f, 1.f);
    const float MotionRadius = FMath::Lerp(0.024f, 0.2f, Motion);
    TArray<bool> Used;
    Used.Init(false, Detected.Num());
    if (!bCameraCut)
    {
        for (const FGlintSourcePoint& Old : Previous)
        {
            if (Output.Num() >= Cap || Old.Confidence <= 0.10f) continue;
            int32 Best = INDEX_NONE;
            float BestDistance = MotionRadius;
            float BestPeak = 0.f;
            for (int32 Index = 0; Index < Detected.Num(); ++Index)
            {
                const FGlintSourcePoint& Current = Detected[Index];
                if (Used[Index] || Current.Brightness < ReleaseGate) continue;
                const float Distance = FMath::Sqrt(FVector2f::DistSquared(Old.UV, Current.UV));
                if (Distance > MotionRadius) continue;
                // Proximity wins over bright tile changes; never abandon a
                // nearby established source for a remote brighter contender.
                if (Distance < BestDistance || (FMath::IsNearlyEqual(Distance, BestDistance, 0.0005f)
                    && Current.Brightness > BestPeak))
                { BestDistance = Distance; BestPeak = Current.Brightness; Best = Index; }
            }
            if (Best != INDEX_NONE)
            {
                Used[Best] = true;
                const FGlintSourcePoint& Current = Detected[Best];
                FGlintSourcePoint Held = Old;
                const float Distance = FMath::Sqrt(FVector2f::DistSquared(Old.UV, Current.UV));
                const float Response = FMath::Lerp(FMath::Clamp(PositionResponse, 0.f, 1.f),
                    1.f, Motion);
                if (Distance > PositionDeadZone)
                    Held.UV = FMath::Lerp(Old.UV, Current.UV, Response);
                Held.Brightness = FMath::Lerp(Old.Brightness, Current.Brightness, 0.25f);
                Held.Confidence = FMath::Lerp(Old.Confidence, 1.f, 0.35f);
                Held.Color = FMath::Lerp(Old.Color, Current.Color, 0.25f);
                Output.Add(Held);
            }
            else
            {
                FGlintSourcePoint Retained = Old;
                Retained.Confidence *= FMath::Clamp(Persistence, 0.f, 0.95f);
                if (Retained.Confidence > 0.12f) Output.Add(Retained);
            }
        }
    }
    while (Output.Num() < Cap)
    {
        int32 Best = INDEX_NONE;
        float Brightest = FMath::Max(0.f, AcquireGate);
        for (int32 Index = 0; Index < Detected.Num(); ++Index)
        {
            const FGlintSourcePoint& Current = Detected[Index];
            if (Used[Index] || Current.Brightness <= Brightest) continue;
            bool bTooClose = false;
            for (const FGlintSourcePoint& Tracked : Output)
                if (FVector2f::DistSquared(Current.UV, Tracked.UV) < FMath::Square(0.04f))
                    bTooClose = true;
            if (!bTooClose) { Brightest = Current.Brightness; Best = Index; }
        }
        if (Best == INDEX_NONE) break;
        Used[Best] = true;
        FGlintSourcePoint NewSource = Detected[Best];
        NewSource.Confidence = 0.55f;
        Output.Add(NewSource);
    }
    return Output;
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

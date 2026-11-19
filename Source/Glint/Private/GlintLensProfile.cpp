#include "GlintLensProfile.h"
#include "GlintFlare.h"
#include "SceneView.h"
#include "FinalPostProcessSettings.h"
#include "BlendableManager.h"

float UGlintLensProfile::CalculateApertureScale(float CameraFStop, float ReferenceFStop, float Response)
{
    return FMath::Pow(FMath::Clamp(FMath::Max(0.1f, ReferenceFStop) /
        FMath::Max(0.1f, CameraFStop), 0.25f, 4.f), FMath::Clamp(Response, 0.f, 2.f));
}

FGlintRuntimeElement UGlintLensProfile::PackElement(const FGlintElement& Element)
{
    FGlintRuntimeElement Result;
    Result.ColorStrength = FVector4f(
        FMath::Max(0.f, Element.Color.R),
        FMath::Max(0.f, Element.Color.G),
        FMath::Max(0.f, Element.Color.B),
        FMath::Max(0.f, Element.Intensity));
    Result.Geometry = FVector4f(
        static_cast<float>(static_cast<uint8>(Element.Type)),
        FMath::Clamp(Element.AxisPosition, -4.f, 4.f),
        FMath::Clamp(Element.Size, 0.05f, 8.f),
        FMath::DegreesToRadians(Element.RotationDegrees));
    Result.Offset = FVector4f(
        FMath::Clamp(static_cast<float>(Element.ScreenOffset.X), -4.f, 4.f),
        FMath::Clamp(static_cast<float>(Element.ScreenOffset.Y), -4.f, 4.f), 0.f, 0.f);
    Result.Shape = FVector4f(
        FMath::Clamp(Element.Falloff, 0.2f, 8.f),
        FMath::Clamp(Element.RingThickness, 0.02f, 1.f),
        FMath::Clamp(Element.AspectRatio, 0.2f, 5.f),
        float(FMath::Clamp(Element.RayCount, 2, 16)));
    Result.Detail = FVector4f(
        FMath::Clamp(Element.RaySharpness, 4.f, 160.f),
        FMath::Clamp(Element.SecondaryStrength, 0.f, 2.f),
        FMath::Clamp(Element.DetailFrequency, 1.f, 32.f), 0.f);
    return Result;
}

FGlintLensBlendData UGlintLensProfile::MakeRuntimeData() const
{
    FGlintLensBlendData Data;
    Data.Intensity = FMath::Max(0.f, Intensity);
    Data.Threshold = FMath::Max(0.f, Threshold);
    Data.SoftKnee = FMath::Clamp(SoftKnee, 0.f, 1.f);
    Data.PeakMix = FMath::Clamp(PeakMix, 0.f, 1.f);
    Data.MaxContribution = FMath::Clamp(MaxContribution, 1.f, 1024.f);
    Data.bEnableAutomatic = bEnableAutomatic;
    Data.GhostStrength = FMath::Max(0.f, GhostStrength);
    Data.GhostCount = FMath::Clamp(GhostCount, 0, 8);
    Data.StreakStrength = FMath::Max(0.f, StreakStrength);
    Data.StreakLength = FMath::Clamp(StreakLength, 0.25f, 4.f);
    Data.ChromaticShift = FMath::Max(0.f, ChromaticShift);
    Data.GhostSpacing = FMath::Max(0.05f, GhostSpacing);
    Data.bSourceDrivenAuthored = bSourceDrivenAuthored;
    Data.MaxSources = FMath::Clamp(MaxSources, 1, 8);
    Data.SourceMinBrightness = FMath::Clamp(SourceMinBrightness, 0.001f, 100.f);
    Data.SourceSeparation = FMath::Clamp(SourceSeparation, 0.005f, 0.25f);
    Data.SourceSmoothing = FMath::Clamp(SourceSmoothing, 0.f, 1.f);
    Data.SourcePersistence = FMath::Clamp(SourcePersistence, 0.f, 0.95f);
    Data.SourceEdgeFade = FMath::Clamp(SourceEdgeFade, 0.f, 0.25f);
    Data.SourceGain = FMath::Clamp(SourceGain, 0.f, 4.f);
    Data.bFollowCameraAperture = bFollowCameraAperture;
    Data.ReferenceFStop = FMath::Clamp(ReferenceFStop, 0.1f, 32.f);
    Data.ApertureScaleResponse = FMath::Clamp(ApertureScaleResponse, 0.f, 2.f);

    if (DetailedFlare)
    {
        for (const FGlintElement& Element : DetailedFlare->Elements)
        {
            if (!Element.bEnabled || Element.Intensity <= 0.f) continue;
            if (Data.ElementCount >= FGlintLensBlendData::MaxElementsPerLens) break;
            Data.Elements[Data.ElementCount++] = PackElement(Element);
        }
    }
    return Data;
}

void UGlintLensProfile::OverrideBlendableSettings(FSceneView& View, float Weight) const
{
    if (Weight > 0.f)
    {
        // All asset lookup and packing occurs on the game thread. No UObject is
        // stored in a render-thread blendable or accessed by GPU pass setup.
        View.FinalPostProcessSettings.BlendableManager.PushBlendableData(
            FMath::Clamp(Weight, 0.f, 1.f), MakeRuntimeData());
    }
}

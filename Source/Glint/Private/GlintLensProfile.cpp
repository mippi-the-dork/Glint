#include "GlintLensProfile.h"
#include "SceneView.h"
#include "FinalPostProcessSettings.h"
#include "BlendableManager.h"

FGlintLensBlendData UGlintLensProfile::MakeRuntimeData() const
{
    FGlintLensBlendData Data;
    Data.Intensity = FMath::Max(0.f, Intensity);
    Data.Threshold = FMath::Max(0.f, Threshold);
    Data.SoftKnee = FMath::Clamp(SoftKnee, 0.f, 1.f);
    Data.GhostStrength = FMath::Max(0.f, GhostStrength);
    Data.StreakStrength = FMath::Max(0.f, StreakStrength);
    Data.ChromaticShift = FMath::Max(0.f, ChromaticShift);
    Data.GhostSpacing = FMath::Max(.05f, GhostSpacing);
    return Data;
}

void UGlintLensProfile::OverrideBlendableSettings(FSceneView& View, float Weight) const
{
    if (Weight > 0.f)
    {
        // Copies the data into the view. No UObject references or GT access in the render pass.
        View.FinalPostProcessSettings.BlendableManager.PushBlendableData(
            FMath::Clamp(Weight, 0.f, 1.f), MakeRuntimeData());
    }
}

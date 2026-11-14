#include "GlintCameraBindingComponent.h"
#include "GlintLensProfile.h"

UGlintCameraBindingComponent::UGlintCameraBindingComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}
void UGlintCameraBindingComponent::SetGlintLens(UGlintLensProfile* NewProfile, float NewWeight)
{
    LensProfile = NewProfile;
    BlendWeight = FMath::Clamp(NewWeight, 0.f, 1.f);
}

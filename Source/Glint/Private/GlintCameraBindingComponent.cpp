#include "GlintCameraBindingComponent.h"
#include "GlintLensProfile.h"

UGlintCameraBindingComponent::UGlintCameraBindingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UGlintCameraBindingComponent::SetGlintLens(UGlintLensProfile* NewProfile, float NewWeight)
{
    LensProfile = NewProfile;
    BlendWeight = FMath::Clamp(NewWeight, 0.f, 1.f);
    bIsTransitioning = false;
    OutgoingLensProfile = nullptr;
    OutgoingWeight = 0.f;
    BlendDuration = 0.f;
    BlendElapsed = 0.f;
    SetComponentTickEnabled(false);
}

void UGlintCameraBindingComponent::BlendToGlintLens(UGlintLensProfile* NewProfile, float Duration, float NewWeight)
{
    if (Duration <= KINDA_SMALL_NUMBER || NewProfile == LensProfile)
    {
        SetGlintLens(NewProfile, NewWeight);
        return;
    }

    // If called during an existing transition, snap its previous destination
    // to the outgoing side. This avoids a growing chain of references/states.
    OutgoingLensProfile = LensProfile;
    OutgoingWeight = BlendWeight;
    LensProfile = NewProfile;
    BlendWeight = FMath::Clamp(NewWeight, 0.f, 1.f);
    BlendDuration = FMath::Max(Duration, KINDA_SMALL_NUMBER);
    BlendElapsed = 0.f;
    bIsTransitioning = true;
    SetComponentTickEnabled(true);
}

void UGlintCameraBindingComponent::AdvanceGlintBlend(float DeltaSeconds)
{
    if (!bIsTransitioning) return;
    BlendElapsed = FMath::Min(BlendDuration, BlendElapsed + FMath::Max(0.f, DeltaSeconds));
    if (BlendElapsed >= BlendDuration)
    {
        bIsTransitioning = false;
        OutgoingLensProfile = nullptr;
        OutgoingWeight = 0.f;
        SetComponentTickEnabled(false);
    }
}

void UGlintCameraBindingComponent::GetGlintBlendWeights(float& OutOutgoing, float& OutIncoming) const
{
    const float T = bIsTransitioning ? FMath::Clamp(BlendElapsed / FMath::Max(BlendDuration, KINDA_SMALL_NUMBER), 0.f, 1.f) : 1.f;
    const float Smoothed = T * T * (3.f - 2.f * T);
    OutOutgoing = bIsTransitioning ? (1.f - Smoothed) * OutgoingWeight : 0.f;
    OutIncoming = Smoothed * BlendWeight;
}

void UGlintCameraBindingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    AdvanceGlintBlend(DeltaTime);
}

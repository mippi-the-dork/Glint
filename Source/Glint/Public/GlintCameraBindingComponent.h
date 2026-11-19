#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GlintCameraBindingComponent.generated.h"

class UGlintLensProfile;

/** Optional PlayerController-side adapter. Works with any camera driving its final view,
 * including Gameplay Cameras; it does not require a direct dependency on that plugin. */
UCLASS(ClassGroup=(Rendering), meta=(BlueprintSpawnableComponent))
class GLINT_API UGlintCameraBindingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UGlintCameraBindingComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Glint")
    TObjectPtr<UGlintLensProfile> LensProfile = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Glint", meta=(ClampMin="0.0", ClampMax="1.0"))
    float BlendWeight = 1.f;

    UFUNCTION(BlueprintCallable, Category="Glint")
    void SetGlintLens(UGlintLensProfile* NewProfile, float NewWeight = 1.f);

    /** Crossfades the previous lens to the new lens, independently of how the
     * camera itself is controlled. Null resolves to the project default lens. */
    UFUNCTION(BlueprintCallable, Category="Glint")
    void BlendToGlintLens(UGlintLensProfile* NewProfile, float Duration, float NewWeight = 1.f);

    UFUNCTION(BlueprintPure, Category="Glint")
    bool IsGlintLensBlending() const { return bIsTransitioning; }

    /** Also callable by native automation tests without constructing a World. */
    void AdvanceGlintBlend(float DeltaSeconds);
    void GetGlintBlendWeights(float& OutOutgoing, float& OutIncoming) const;
    const UGlintLensProfile* GetOutgoingGlintLens() const { return OutgoingLensProfile.Get(); }
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY(Transient) TObjectPtr<UGlintLensProfile> OutgoingLensProfile = nullptr;
    UPROPERTY(Transient) float OutgoingWeight = 0.f;
    UPROPERTY(Transient) float BlendDuration = 0.f;
    UPROPERTY(Transient) float BlendElapsed = 0.f;
    UPROPERTY(Transient) bool bIsTransitioning = false;
};

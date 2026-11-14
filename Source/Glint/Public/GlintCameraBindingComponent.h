#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GlintCameraBindingComponent.generated.h"

class UGlintLensProfile;

/** Optional bridge for gameplay camera controllers: place on PlayerController. */
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
};

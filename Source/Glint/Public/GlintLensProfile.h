#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/BlendableInterface.h"
#include "GlintLensProfile.generated.h"

/** POD-only transfer from GT post-process blending into RT view processing. */
struct GLINT_API FGlintLensBlendData
{
    static const FName& GetFName()
    {
        static const FName Name(TEXT("GlintLensBlendData"));
        return Name;
    }
    float Intensity = 0.75f;
    float Threshold = 1.75f;
    float SoftKnee = 0.5f;
    float GhostStrength = 0.6f;
    float StreakStrength = 0.5f;
    float ChromaticShift = 0.003f;
    float GhostSpacing = 0.55f;
};

/** Place in a camera's or Post Process Volume's Weighted Blendables. */
UCLASS(BlueprintType)
class GLINT_API UGlintLensProfile : public UDataAsset, public IBlendableInterface
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Overview", meta=(ClampMin="0.0", UIMax="3.0")) float Intensity = 0.75f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Highlights", meta=(ClampMin="0.0", UIMax="20.0")) float Threshold = 1.75f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Highlights", meta=(ClampMin="0.0", ClampMax="1.0")) float SoftKnee = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics", meta=(ClampMin="0.0", UIMax="2.0")) float GhostStrength = 0.6f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics", meta=(ClampMin="0.0", UIMax="2.0")) float StreakStrength = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics", meta=(ClampMin="0.0", UIMax="0.02")) float ChromaticShift = 0.003f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics", meta=(ClampMin="0.05", UIMax="1.0")) float GhostSpacing = 0.55f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Composition") TObjectPtr<class UGlintFlare> DetailedFlare = nullptr;

    FGlintLensBlendData MakeRuntimeData() const;
    virtual void OverrideBlendableSettings(class FSceneView& View, float Weight) const override;
};

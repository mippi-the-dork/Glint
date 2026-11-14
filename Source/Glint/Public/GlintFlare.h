#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GlintFlare.generated.h"

UENUM(BlueprintType)
enum class EGlintElementType : uint8
{
    Glow, Halo, Ghost, Starburst, Streak, Iris, Ring
};

/** Authoring schema, not yet consumed by the initial automatic field renderer. */
USTRUCT(BlueprintType)
struct GLINT_API FGlintElement
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Element") FName Label = TEXT("Element");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Element") EGlintElementType Type = EGlintElementType::Glow;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Element") bool bEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Element", meta=(ClampMin="0.0")) float Intensity = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Element", meta=(ClampMin="0.0")) float Size = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Element") FLinearColor Color = FLinearColor::White;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Element") float AxisPosition = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Element") float RotationDegrees = 0.f;
};

/** Reusable Optical Flares-style element composition. */
UCLASS(BlueprintType)
class GLINT_API UGlintFlare : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Composition") TArray<FGlintElement> Elements;
    UFUNCTION(BlueprintPure, Category="Glint") int32 GetEnabledElementCount() const;
};

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GlintFlare.generated.h"

/** Authored optical families. Enum values are serialized and matched by Glint.usf; append only. */
UENUM(BlueprintType)
enum class EGlintElementType : uint8
{
    Glow, Halo, Ghost, Starburst, Streak, Iris, Ring, Hoop, MultiIris, SpikeBall, Shimmer, Sparkle, Caustic,
    PolygonIris, ApertureBladeRing, ApertureGhost, CrossStar, VerticalStreak, DualStreak,
    Smear, ArcReflection, EdgeRing, SoftDisc, VeilingGlow, PetalIris, DustCluster
};

USTRUCT(BlueprintType)
struct GLINT_API FGlintElement
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Element") FName Label = TEXT("Element");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Element") EGlintElementType Type = EGlintElementType::Glow;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Element") bool bEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance", meta=(ClampMin="0.0", UIMax="3.0")) float Intensity = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance", meta=(ClampMin="0.05", UIMax="8.0")) float Size = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance") FLinearColor Color = FLinearColor::White;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Placement", meta=(ClampMin="-4.0", ClampMax="4.0", ToolTip="Optical axis displacement: +1 on the highlight, -1 mirrored, negative values are opposite the source.")) float AxisPosition = -1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Placement", meta=(UIMin="-180.0", UIMax="180.0")) float RotationDegrees = 0.f;
    /** Screen-space offset, normalized by viewport dimensions, relative to the optical axis placement. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Placement", meta=(ClampMin="-4.0", ClampMax="4.0")) FVector2D ScreenOffset = FVector2D::ZeroVector;

    // Only meaningful shape parameters are shown in the selected-element Details panel.
    // All seven values are consumed by the GPU source-aware renderer and preview.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shape | Soft", meta=(ClampMin="0.2", ClampMax="8.0", EditCondition="Type == EGlintElementType::Glow || Type == EGlintElementType::Ghost || Type == EGlintElementType::Sparkle || Type == EGlintElementType::SoftDisc || Type == EGlintElementType::VeilingGlow || Type == EGlintElementType::DustCluster || Type == EGlintElementType::Smear", EditConditionHides, ToolTip="Rate at which light fades from the center."))
    float Falloff = 3.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shape | Rings", meta=(ClampMin="0.02", ClampMax="1.0", EditCondition="Type == EGlintElementType::Halo || Type == EGlintElementType::Iris || Type == EGlintElementType::Ring || Type == EGlintElementType::Hoop || Type == EGlintElementType::MultiIris || Type == EGlintElementType::PolygonIris || Type == EGlintElementType::ApertureBladeRing || Type == EGlintElementType::ArcReflection || Type == EGlintElementType::EdgeRing || Type == EGlintElementType::PetalIris", EditConditionHides, ToolTip="Radial width of the reflection or iris outline."))
    float RingThickness = 0.16f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shape | Geometry", meta=(ClampMin="0.2", ClampMax="5.0", EditCondition="Type == EGlintElementType::Ghost || Type == EGlintElementType::Iris || Type == EGlintElementType::Streak || Type == EGlintElementType::Shimmer || Type == EGlintElementType::Caustic || Type == EGlintElementType::ApertureGhost || Type == EGlintElementType::Smear", EditConditionHides, ToolTip="Horizontal stretch relative to the vertical dimension."))
    float AspectRatio = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shape | Rays", meta=(ClampMin="2", ClampMax="16", EditCondition="Type == EGlintElementType::Starburst || Type == EGlintElementType::SpikeBall || Type == EGlintElementType::Sparkle || Type == EGlintElementType::CrossStar || Type == EGlintElementType::PolygonIris || Type == EGlintElementType::ApertureBladeRing || Type == EGlintElementType::ApertureGhost || Type == EGlintElementType::PetalIris", EditConditionHides))
    int32 RayCount = 6;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shape | Rays", meta=(ClampMin="4.0", ClampMax="160.0", EditCondition="Type == EGlintElementType::Starburst || Type == EGlintElementType::SpikeBall || Type == EGlintElementType::Streak || Type == EGlintElementType::Sparkle || Type == EGlintElementType::CrossStar || Type == EGlintElementType::VerticalStreak || Type == EGlintElementType::DualStreak || Type == EGlintElementType::Smear", EditConditionHides, ToolTip="Higher values produce narrower, sharper rays."))
    float RaySharpness = 55.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shape | Detail", meta=(ClampMin="0.0", ClampMax="2.0", EditCondition="Type == EGlintElementType::MultiIris || Type == EGlintElementType::Shimmer || Type == EGlintElementType::Sparkle || Type == EGlintElementType::Caustic || Type == EGlintElementType::DualStreak || Type == EGlintElementType::PetalIris || Type == EGlintElementType::DustCluster", EditConditionHides, ToolTip="Intensity of secondary optical detail."))
    float SecondaryStrength = 0.35f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shape | Detail", meta=(ClampMin="1.0", ClampMax="32.0", EditCondition="Type == EGlintElementType::Shimmer || Type == EGlintElementType::Sparkle || Type == EGlintElementType::Caustic || Type == EGlintElementType::DustCluster", EditConditionHides, ToolTip="Fine-detail repetition within the optical shape."))
    float DetailFrequency = 8.f;
};

/** A reusable authored element stack; consumed by the runtime renderer through the Glint Lens profile. */
UCLASS(BlueprintType)
class GLINT_API UGlintFlare : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Composition") TArray<FGlintElement> Elements;
    UFUNCTION(BlueprintPure, Category="Glint") int32 GetEnabledElementCount() const;
    /** Explicit authoring defaults; no generic -1 axis for every new element. */
    static FGlintElement MakeDefaultElement(EGlintElementType Type);
    static const TCHAR* GetTypeName(EGlintElementType Type);
    /** Three deliberate authoring presets; all feed the same renderer and asset schema. */
    static FGlintElement MakePresetElement(EGlintElementType Type, int32 Preset);
};

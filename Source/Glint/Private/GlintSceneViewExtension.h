#pragma once
#include "SceneViewExtension.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/SoftObjectPath.h"
#include "GlintLensProfile.h"

struct FGlintSourceHistory;

class FGlintSceneViewExtension final : public FSceneViewExtensionBase
{
public:
    explicit FGlintSceneViewExtension(const FAutoRegister& AutoReg)
        : FSceneViewExtensionBase(AutoReg) {}

    virtual void SetupViewPoint(APlayerController* Player, FMinimalViewInfo& InViewInfo) override;

    virtual void SubscribeToPostProcessingPass(
        EPostProcessingPass Pass, const FSceneView& View,
        FPostProcessingPassDelegateArray& Callbacks, bool bIsPassEnabled) override;

private:
    FSoftObjectPath DefaultLensPath;
    TStrongObjectPtr<UGlintLensProfile> DefaultLensAsset;
    // Only accessed by the per-view render callback. History values use stable
    // shared allocations because RDG holds their extraction output addresses.
    mutable TMap<const FSceneViewStateInterface*, TSharedPtr<FGlintSourceHistory>> SourceHistories;
    mutable uint64 HistoryVisitCounter = 0;

    FScreenPassTexture AddGlintPass(
        FRDGBuilder& GraphBuilder, const FSceneView& View,
        const FPostProcessMaterialInputs& Inputs) const;
};

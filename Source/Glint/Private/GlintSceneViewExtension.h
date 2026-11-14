#pragma once
#include "SceneViewExtension.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/SoftObjectPath.h"
#include "GlintLensProfile.h"

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

    FScreenPassTexture AddGlintPass(
        FRDGBuilder& GraphBuilder, const FSceneView& View,
        const FPostProcessMaterialInputs& Inputs) const;
};

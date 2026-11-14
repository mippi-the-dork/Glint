#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GlintProjectSettings.generated.h"

class UGlintLensProfile;

/** Default for gameplay camera views; volumes and per-controller overrides remain supported. */
UCLASS(config=Game, defaultconfig, meta=(DisplayName="Glint"))
class GLINT_API UGlintProjectSettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

    UPROPERTY(config, EditAnywhere, Category="Automatic Lens")
    TSoftObjectPtr<UGlintLensProfile> DefaultLensProfile;
};

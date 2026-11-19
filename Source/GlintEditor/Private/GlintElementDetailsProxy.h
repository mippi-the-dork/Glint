#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GlintFlare.h"
#include "GlintElementDetailsProxy.generated.h"

/** Editor-only transient object exposing ONE selected element in native Details. */
UCLASS(Transient)
class UGlintElementDetailsProxy : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Selected Element", meta=(ShowOnlyInnerProperties))
    FGlintElement Element;
};

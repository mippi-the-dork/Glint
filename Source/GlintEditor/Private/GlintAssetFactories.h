#pragma once
#include "Factories/Factory.h"
#include "GlintAssetFactories.generated.h"

UCLASS()
class UGlintLensFactory : public UFactory
{
    GENERATED_BODY()
public:
    UGlintLensFactory();
    virtual UObject* FactoryCreateNew(UClass* Class, UObject* Parent, FName Name,
        EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS()
class UGlintFlareFactory : public UFactory
{
    GENERATED_BODY()
public:
    UGlintFlareFactory();
    virtual UObject* FactoryCreateNew(UClass* Class, UObject* Parent, FName Name,
        EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

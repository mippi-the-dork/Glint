#include "GlintAssetFactories.h"
#include "GlintLensProfile.h"
#include "GlintFlare.h"

UGlintLensFactory::UGlintLensFactory()
{
    SupportedClass = UGlintLensProfile::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}
UObject* UGlintLensFactory::FactoryCreateNew(UClass* Class, UObject* Parent,
    FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    return NewObject<UGlintLensProfile>(Parent, Class, Name, Flags | RF_Transactional);
}

UGlintFlareFactory::UGlintFlareFactory()
{
    SupportedClass = UGlintFlare::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}
UObject* UGlintFlareFactory::FactoryCreateNew(UClass* Class, UObject* Parent,
    FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    return NewObject<UGlintFlare>(Parent, Class, Name, Flags | RF_Transactional);
}

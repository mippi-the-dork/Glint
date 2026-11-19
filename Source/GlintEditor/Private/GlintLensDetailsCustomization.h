#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class FProperty;
class UObject;

/** Shared helpers make the Lens Profile reset-arrow behavior independently testable. */
namespace GlintLensDefaults
{
    bool DiffersFromClassDefault(const UObject* Lens, const FProperty* Property);
    FString ExportClassDefault(const FProperty* Property);
}

/** Keeps native Details layout while providing explicit reset arrows for data-asset instances. */
class FGlintLensDetailsCustomization final : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};

#include "GlintLensDetailsCustomization.h"
#include "GlintLensProfile.h"
#include "DetailLayoutBuilder.h"
#include "IDetailPropertyRow.h"
#include "PropertyHandle.h"
#include "UObject/UnrealType.h"

bool GlintLensDefaults::DiffersFromClassDefault(const UObject* Lens, const FProperty* Property)
{
    if (!IsValid(Lens) || !Property || !Lens->IsA<UGlintLensProfile>()) return false;
    return !Property->Identical_InContainer(Lens, GetDefault<UGlintLensProfile>());
}

FString GlintLensDefaults::ExportClassDefault(const FProperty* Property)
{
    FString Text;
    if (Property)
    {
        const UGlintLensProfile* Default = GetDefault<UGlintLensProfile>();
        Property->ExportTextItem_Direct(Text,
            Property->ContainerPtrToValuePtr<void>(Default), nullptr,
            const_cast<UGlintLensProfile*>(Default), PPF_None);
    }
    return Text;
}

namespace
{
    bool ShouldShowReset(TSharedPtr<IPropertyHandle> Handle)
    {
        if (!Handle.IsValid()) return false;
        const FProperty* Property = Handle->GetProperty();
        if (!Property) return false;
        TArray<UObject*> Objects;
        Handle->GetOuterObjects(Objects);
        for (const UObject* Object : Objects)
        {
            if (GlintLensDefaults::DiffersFromClassDefault(Object, Property)) return true;
        }
        return Objects.IsEmpty() && Handle->DiffersFromDefault();
    }

    void ResetToLensDefault(TSharedPtr<IPropertyHandle> Handle)
    {
        if (!Handle.IsValid() || !Handle->GetProperty()) return;
        // Use the C++ class default explicitly. Generic UObject reset-to-default
        // can treat a standalone asset instance as its own archetype.
        const FString DefaultText = GlintLensDefaults::ExportClassDefault(Handle->GetProperty());
        Handle->SetValueFromFormattedString(DefaultText);
    }

}

TSharedRef<IDetailCustomization> FGlintLensDetailsCustomization::MakeInstance()
{
    return MakeShared<FGlintLensDetailsCustomization>();
}

void FGlintLensDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    // Retain the normal engine property editors, categories and tooltips.
    // Add only a Glint-specific reset to the actual C++ class defaults.
    for (TFieldIterator<FProperty> It(UGlintLensProfile::StaticClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
    {
        FProperty* Property = *It;
        if (!Property->HasAnyPropertyFlags(CPF_Edit)) continue;
        TSharedRef<IPropertyHandle> Handle = DetailBuilder.GetProperty(Property->GetFName(), UGlintLensProfile::StaticClass());
        if (!Handle->IsValidHandle()) continue;
        // Edit the EXISTING default property row. Adding a second property row
        // creates a duplicate whose reset override is not the row shown by the
        // simple native Data Asset editor (the cause of the missing arrows).
        if (IDetailPropertyRow* Row = DetailBuilder.EditDefaultProperty(Handle))
        {
            Row->OverrideResetToDefault(FResetToDefaultOverride::Create(
                FIsResetToDefaultVisible::CreateStatic(&ShouldShowReset),
                FResetToDefaultHandler::CreateStatic(&ResetToLensDefault), false));
        }
    }
}

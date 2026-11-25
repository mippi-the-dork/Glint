#include "GlintLensDetailsCustomization.h"
#include "GlintLensProfile.h"
#include "DetailLayoutBuilder.h"
#include "IDetailPropertyRow.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "UObject/UnrealType.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Styling/AppStyle.h"

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
        // Keep Unreal's default editors for object/class properties. For simple
        // scalar Lens settings, explicitly place the Reset-to-Class-Default
        // button next to the existing native value widget. Data Assets can use
        // themselves as the archetype for Unreal's built-in reset-arrow logic;
        // an OverrideResetToDefault handler alone is not reliably painted there.
        if (IDetailPropertyRow* Row = DetailBuilder.EditDefaultProperty(Handle))
        {
            Row->OverrideResetToDefault(FResetToDefaultOverride::Create(
                FIsResetToDefaultVisible::CreateStatic(&ShouldShowReset),
                FResetToDefaultHandler::CreateStatic(&ResetToLensDefault), false));
            if (CastField<FNumericProperty>(Property) || CastField<FBoolProperty>(Property))
            {
                Row->CustomWidget()
                    .NameContent()[Handle->CreatePropertyNameWidget()]
                    .ValueContent().MinDesiredWidth(210.f)
                    [SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
                            [Handle->CreatePropertyValueWidget()]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(3.f,0.f)
                            [SNew(SButton)
                                .ButtonStyle(FAppStyle::Get(),TEXT("NoBorder"))
                                .ContentPadding(0.f)
                                .Visibility_Lambda([Handle]()
                                    { // Hidden keeps the right-side slot allocated while scrubbing.
                                    // Collapsed shortened the numeric field mid-drag.
                                    return ShouldShowReset(Handle) ? EVisibility::Visible : EVisibility::Hidden; })
                                .ToolTipText(FText::FromString(TEXT("Reset to Glint Lens class default")))
                                .OnClicked_Lambda([Handle]()
                                    { ResetToLensDefault(Handle); return FReply::Handled(); })
                                [SNew(SImage)
                                    .Image(FAppStyle::GetBrush(TEXT("PropertyWindow.DiffersFromDefault")))]]];
            }
        }
    }
}

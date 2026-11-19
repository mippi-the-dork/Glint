#pragma once

#include "ToolMenu.h"
#include "ToolMenuEntry.h"
#include "ToolMenuSection.h"
#include "Widgets/SWidget.h"

/**
 * Adds the Glint preview controls through UE's supported ToolMenus interface.
 * This never reaches into FAssetEditorToolkit's private toolbar Slate widgets.
 * The dedicated trailing section keeps the controls on the Save/Browse toolbar.
 */
struct FGlintToolbarLayout
{
    static FName SectionName() { return FName(TEXT("GlintPreview")); }
    static FName EntryName() { return FName(TEXT("GlintPreviewControls")); }

    static bool IsInstalled(UToolMenu* ToolbarMenu)
    {
        if (!ToolbarMenu)
        {
            return false;
        }
        FToolMenuSection& Section = ToolbarMenu->FindOrAddSection(SectionName());
        return Section.FindEntry(EntryName()) != nullptr;
    }

    static bool Install(UToolMenu* ToolbarMenu, const TSharedRef<SWidget>& PreviewControls)
    {
        if (!ToolbarMenu)
        {
            return false;
        }

        FToolMenuSection& Section = ToolbarMenu->FindOrAddSection(SectionName());
        Section.Alignment = EToolMenuSectionAlign::Last;
        if (Section.FindEntry(EntryName()) != nullptr)
        {
            return false;
        }

        Section.AddEntry(FToolMenuEntry::InitWidget(
            EntryName(), PreviewControls, FText::GetEmpty(), true, false));
        return true;
    }
};

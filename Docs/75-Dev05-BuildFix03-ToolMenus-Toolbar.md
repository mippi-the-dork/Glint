# Glint 0.5.3 — UE5.8 ToolMenus Toolbar Fix (Dev05 Build Fix 03)

## User build failure

Glint 0.5.2 did not compile because the toolbar integration accessed the private `FAssetEditorToolkit::ToolbarWidgetContent` member (C2248) and compared an `SWidget` reference with an `SHorizontalBox*` pointer (C2678). The generic MSB3073 code 6 reflected these compile errors. UE5 third-party CS8981 warnings are unrelated and untouched.

## Change

- Override the public/protected virtual UE5.8 `FAssetEditorToolkit::GenerateCommonActionsToolbar(FToolMenuContext&)`, which assembles standard asset actions (including Save).
- Call `FAssetEditorToolkit::GenerateCommonActionsToolbar()` first, retaining standard Save/Browse actions.
- Append the Schematic, Rendered, Hybrid and Settings Slate widget via `FToolMenuEntry::InitWidget` in `FToolMenuSection` named `GlintPreview`, with `EToolMenuSectionAlign::Last`. No private engine internals or Slate widget reparenting are used.
- Skip duplicate menu-entry registration and update the automation test to inspect the ToolMenus entry instead of synthetic `SBorder` content.
- Depend on `ToolMenus` in the GlintEditor module only.

## Validation

The source and ZIP have been statically inspected. A UE5.8.3 Windows compile and visual confirmation of the rightmost toolbar group are required. The suite contains 23 registered Glint tests, including `Glint.Editor.Toolbar.SameRowAndRegeneration`.

Run:

```text
Automation RunTests Glint
```

Manual QA:

1. Open a Glint Flare and confirm Save/Browse are still available.
2. Confirm the Schematic / Rendered / Hybrid buttons and Settings gear appear in the **same asset toolbar** (rightmost group, without a separate row).
3. Switch modes, edit a flare, confirm save and Content Browser actions.
4. Open more than one Glint Flare editor and regenerate toolbars; make sure the actions apply to the active editor and do not appear twice.

This package changes **Glint only**. No renderer, shader, or camera behavior changed.

# Dev08 — Solo/Delete icon resources and Lens reset rows

## User-reported regressions

In 0.7.2, drag handles rendered, but **Solo and Delete were still blank square buttons**. Lens Profile properties did not show reset arrows when changed, although test coverage for underlying CDO values passed.

## Icon root cause and fix

`FSlateStyleSet::RootToContentDir()` supplies `.png` by default. The Glint stylesheet previously passed names *already containing* `.png`, which could resolve to `.png.png`. All four brush registrations now use an extension-free stem plus explicit `.png` extension. The new native test `Glint.Editor.Outliner.IconFilesResolve` verifies both on-disk PNGs and the registered brush resource paths. The square widget layout and Focus-inspired Solo icon files remain unchanged.

## Lens Profile Details reset

`FGlintLensDetailsCustomization::CustomizeDetails` previously used `Category.AddProperty(Handle)`, creating new rows instead of customizing the original rows displayed in the native Data Asset details editor. It now uses `IDetailLayoutBuilder::EditDefaultProperty` to modify the **existing row** with an `FResetToDefaultOverride`. The handler writes the C++ class default via the property handle (with standard UE change notifications), and the visibility predicate compares edited instances with the class default object. The existing CDO helper test was insufficient to verify that the UI actually attached the override; **manual validation is required**.

## Manual acceptance in UE5.8.3

1. Open Glint Flare; verify the square Solo icon visibly depicts Solo, Trash visibly depicts a trashcan, and grip shows a native-sized drag marker. Test hover and high/low DPI.
2. Solo an element, then clear Solo, then delete/undo an element; verify behavior and icons.
3. Open a **Glint Lens Profile** asset, change `Intensity`, `Threshold`, `Source Persistence`, and `Source Grouping Radius`. Each changed row should show a reset arrow; click it and confirm its C++ default returns. Save/reopen; repeat with a previously authored Lens Profile.
4. If reset arrows are still missing, check the Lens asset's editor is creating the registered `FGlintLensDetailsCustomization` and that the row comes from the correct class; the helper unit test alone does not validate Slate visibility.

## Automation

`Glint.Editor.Outliner.IconFilesResolve` is new. `Glint.Editor.Lens.PerPropertyResetDefaults` continues to validate value comparison and CDO export only; it is **not** a UI rendering test.

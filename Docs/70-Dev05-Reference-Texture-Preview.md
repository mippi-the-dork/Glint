# Glint Development Pass 05 — Background Texture Asset

## Goal

Use a `Texture2D` asset (for example, an imported screenshot) as a visual reference while authoring an optical flare. Keep it an **editor-only preference**, not a new runtime dependency of `UGlintFlare` or `UGlintLensProfile`.

## Steps

1. Import a screenshot into the Unreal Content Browser as a regular Texture2D.
2. Open a **Glint Flare** asset and select **Rendered** or **Hybrid**.
3. Open the **gear** beside the view-mode buttons and select the texture under **Camera background (Texture2D)**.
4. Use **right-button drag** to move the synthetic flare source over the image or into the off-camera workspace.
5. Clear the texture in the picker to return to the neutral background.

Glint draws the reference image **inside the camera frame**, then draws the existing GPU-authored flare preview on a transparent texture above it. Schematic deliberately omits the screenshot, remaining a clear editing diagram. The preview is display-referred and uses Glint's approximate editor display transform, not the full project tonemapper. This is a reference, not frame-perfect compositing with the source screenshot's original exposure.

The selected texture's object path is stored per Flare under the current project editor user settings (GEditorPerProjectIni) and loaded on reopen. The Flare asset itself is not modified, dirtied or made dependent on that texture. Screenshot resolution is accommodated by matching the preview aspect and maintaining a bounded GPU output size. Texture selection/clearing does not change game runtime rendering.

## Future automatic analysis

The backdrop is **not yet** an input to `GlintExtractCS` or the automatic source tracker. Future source-from-image preview should use the same selected asset as an optional input, with a separate simulation toggle and clear warning that a typical LDR screenshot lacks the HDR values needed for accurate scene thresholds and exposure. Avoid claiming source detection behaves identically to a live HDR scene until the color pipeline has been implemented and validated.

## Tests

`Glint.Editor.Preview.BackgroundIsNotRuntimeAssetData` verifies the reference isn't declared as a runtime asset field. Visually verify portrait/widescreen texture aspect, clearing/reopening, transparency, and mode changes.

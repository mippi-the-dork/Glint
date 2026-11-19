# Glint Dev07 — Editor Refresh and Preview Presentation

**Implementation target:** UE5.8.0–5.8.3; **validation state:** source/static checks only; Windows build and live editor behavior still pending.

## Editor correctness

- The Flare editor now registers an `FEditorUndoClient` with `GEditor` for the toolkit's lifetime. `PostUndo` and `PostRedo` call `RefreshAfterUndoRedo`.
- Refresh repairs selected and solo indexes when a transaction removes/reorders items, clears obsolete hover, rebinds the transient selected-element Details proxy, rebuilds outliner rows, invalidates the Slate canvas, and forces a fresh GPU render request.
- Existing asset transactions remain scoped to the Flare. Changing preview opacity and reference backgrounds does not dirty the Flare.
- Drag and drop offers an insertion indicator on the upper or lower edge of a hovered row. Insertion indices are translated into stable remove/insert operations and update selected/solo indices.

## Preview presentation

- Schematic outer workspace uses near-black neutral gray (`0.026` linear) and inner camera frame a darker neutral gray (`0.019` linear); Rendered/Hybrid frame uses `0.012` linear. Actual displayed values depend on Slate gamma and monitor settings.
- Camera borders, axes and origin guide are toned down. The user-supplied reference texture still fills only the framed camera image.
- Circle paths are closed once with equal angular spacing; the angular overdraw from 0.6 was removed because its overlap created a thick seam. Exact appearance requires visual testing at DPI scales and frame sizes.
- A small blue Slate marker represents the *source position* in Schematic/Hybrid. Rendered draws no source glyph; the GPU preview shader no longer adds a hardcoded glowing white disc. Authored Glow elements are still rendered normally.
- `Schematic Opacity` defaults to 38%, configured in the compact gear menu, and applies only to the visualizer's diagram, not the GPU flare. Selected and hovered elements display at full strength. Soloed-out handles are faint.
- Gear menu is width-constrained to 304 Slate units, with exposure, schematic opacity, W/R/G/B preview source color, and the editor-only Texture2D background picker. Right-drag source movement is unchanged.
- Drag, Solo, and Delete controls use 16px PNG artwork in fixed icon buttons. The Solo artwork retains the previous supplied Focus-like icon. Icon rendering needs visual verification.

## Remaining considerations

- The GPU preview is synthetic and source-aware, not an HDR extraction pass over the reference texture. A future separate reference-image automatic detection mode will require an exposure/HDR policy.
- An automated test can confirm selection-repair rules and regression source contracts but not that a particular UE5.8 Slate window is visually current; use manual Undo/Redo checks below.

## Toolbar placement

The active Schematic/Rendered/Hybrid/gear buttons are constructed in `BuildPreviewStrip` at the top right of the **preview pane** (not in the native Save/Browse toolbar). The prior `GenerateCommonActionsToolbar`/`ToolMenus` workaround was removed to avoid duplicate or invisible controls. The built-in asset editor retains its normal Save/Browse actions.

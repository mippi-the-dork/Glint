> **Superseded by [0.5.3 ToolMenus fix](75-Dev05-BuildFix03-ToolMenus-Toolbar.md):** The approach below failed to compile against UE5.8 because `ToolbarWidgetContent` is private. Retained only as development history.

# Glint 0.5.2 — Native Toolbar Controls Fix (Dev05 Build Fix 02)

## Problem and user evidence

The 0.5.1 Flare editor displayed the stock asset toolbar's **Save** and **Browse** controls, but not **Schematic**, **Rendered**, **Hybrid**, or the preview **Settings gear**. All **22** existing native automation tests passed; none tested the toolbar host's visual layout. The user's optical screenshot showed a 13-element default flare; no optical renderer issue was reported.

## Change

- `GlintFlareEditorToolkit.cpp/.h`: stop relying on `AddToolbarWidget(BuildPreviewToolbar())` after initialization; instead override `FAssetEditorToolkit::PostRegenerateMenusAndToolbars()`.
- `GlintToolbarLayout.h`: preserve the real toolbar host's existing Save/Browse widget; reparent it into a *fill-width left* `SHorizontalBox` slot. Put the **existing** `BuildPreviewToolbar()` controls in an *auto-width right* slot. This places both groups on one row and uses the free space to push the preview modes and gear to the far right.
- The toolbar wrapper is idempotent. Each regeneration checks whether the host already contains the wrapper, avoiding duplicate controls. If UE replaces the native toolbar, the next regeneration restores Glint controls around the new content.
- No new render shaders, runtime classes, source detection changes, element assets, or camera integration changes.

## Testing

```text
Automation RunTests Glint
```

Expected **23 tests** (22 previously passed plus `Glint.Editor.Toolbar.SameRowAndRegeneration`). The new test creates a synthetic standard-toolbar host, wraps it, verifies that repeated installation does not nest wrappers, and simulates rebuilding the native toolbar. This is a layout-helper test **not** a live Windows Slate screenshot or integration test.

### Manual editor acceptance

1. Open a Glint Flare. Confirm Save and Browse remain at left, **Schematic / Rendered / Hybrid / Settings** appear at the far right **on the same toolbar row**.
2. Toggle each mode and verify the canvas changes accordingly; the gear reveals exposure, source color, and Texture2D background controls.
3. Reopen the Flare. Confirm mode and reference image persist and preview controls still appear.
4. Resize editor narrow/wide; confirm all controls are accessible without duplicating the row.
5. Confirm 13 default element types and authored runtime composition are unaffected; run the previously passing 22 tests.

## Validation status

- Source/header and archive checks performed outside Unreal. The user must compile and test this version under UE5.8.3.
- Original user-provided GIF contained only one accessible frame here, so no motion-stability conclusion was drawn.

## Scope

Only files inside `Glint` changed. No engine modifications or changes to other plugins.

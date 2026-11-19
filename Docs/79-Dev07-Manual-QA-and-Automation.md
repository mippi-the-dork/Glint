# Glint Dev07 — UE5.8.3 Validation Checklist

**Local validation pending.** A successful source checker is not an Unreal/Windows build or a GPU correctness test.

## Automated

Run from the project editor console:

```text
Automation RunTests Glint
```

**Expected registrations:** 29. In 0.6.1, the user reported 22/23 passing. The one failure (`Glint.Editor.Preview.SoloVisibility`) was caused by a test that constructed just 16 elements but expected the cap of 24. Dev07 constructs 41 elements against a 32-element cap, and tests Solo on the last index. Passing Dev07 tests is not yet confirmed.

From any working directory:

```text
python Plugins/Glint/Tools/CheckShaderContracts.py
python Plugins/Glint/Tools/CheckDev07Source.py
```

The source checks cover shader parameter binding names, element-to-HLSL index continuity, existing enum stability, default and preset functions, icon resources, bracket nesting, the undo hooks, and test registration counts. They do **not** compile C++ or HLSL in UE.

## Manual — native editor

1. Open an existing Flare, make an element edit through Details and on the canvas; press **Ctrl+Z** and **Ctrl+Y**. Confirm both the outliner label/value, selected-element Details and schematic/GPU preview reflect every change **without reopening**.
2. Add an item, Undo and Redo; then test delete, visibility, reorder and element type changes. The selected/solo state should remain valid and no stale row widgets should remain.
3. Confirm 16px grip, Solo and Trash glyphs are visible on standard and high-DPI display scales, and the buttons are square. Test Solo on and off.
4. Drag an item and hover above/below other rows. Look for a **thin blue insertion line** on the correct edge and verify stable resulting order, including drops at the very top and bottom.
5. Compare the neutral canvas and inner frame against UE's native dark panels. Resize the editor and check that lines and circles are neither broken nor doubled at their seam.
6. In the compact gear popup, change schematic opacity to minimum/maximum. Selected or hovered marker should remain fully readable while unselected markers become faint; **Rendered optical output must not change**.
7. Test source movement in Schematic, Rendered and Hybrid. In Schematic/Hybrid the source is a small blue *editor marker*, not a fuzzy illuminated disc. Rendered has **no source marker**, while authored Glow still appears if included in the Flare.
8. Select and clear an imported Texture2D background. Check aspect ratio and persistence, no runtime asset dependency, and no unintended dirty state.
9. Add all 26 types (Default), then at least six duplicates to make 32. Confirm the first 32 appear in the GPU preview and a 33rd is bounded by the cap. Check no crash or runaway GPU load.
10. Try Subtle and Cinematic presets for at least a ghost, a ring, a streak and a dust cluster. Alter parameter values and save/reload. Previous 0.6.1 assets should open without schema migration.

## Manual — game rendering

Use an unbound Post Process Volume with a Glint Lens and Detailed Flare:

```text
r.Glint.Automatic 0
r.Glint.Authored 1
r.Glint.SourceTracking 1
r.Glint.DebugSources 0
r.Glint.MaxElements 32
Automation RunTests Glint
```

Test with single sun, red/green/blue emissive spheres and many highlights. Move past all viewport edges; verify zero-outside-viewport behavior remains intact, no flickering and the optical differences among the new types. Compare at 8/16/32 element limits and collect `profilegpu` results if possible.

**Quality caveat:** Our source-driven renderer is analytic, not a calibrated measured-lens implementation. A textured diffraction/optical-bake pipeline and direct Gameplay Camera Rig nodes remain future work.

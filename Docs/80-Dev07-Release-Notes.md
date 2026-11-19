> Dev07 patch: see [0.7.1 budget-test fixture correction](81-Dev07-01-Automation-Budget-Correction.md).

# Glint 0.7.0 — Dev07 Release Notes (Source Candidate)

- Corrected the editor GPU preview cap regression *test* and raised actual authored per-lens/per-view caps to 32 (the 0.6 test incorrectly created 16 elements while expecting 24).
- Added `FEditorUndoClient` registration and editor-side refresh for Undo/Redo: outliner, selected Details proxy, schematic and GPU preview.
- Expanded authored library from 13 to 26 types; source-aware HLSL shapes, schematic symbols and type-specific default settings for new families.
- Added Default, Subtle and Cinematic per-family starting presets to the + Add popup.
- Refined neutral-dark visualizer palette, removed synthetic GPU preview source halo, added blue schematic/hybrid source marker and saved schematic-opacity gear slider.
- Constrained the gear menu to a compact width, retained reference Texture2D picker and exposure/color options.
- Added 16px grip and trash PNG icons, corrected Solo button tint, and implemented a row insertion-position indicator with stable reordering.
- Removed obsolete native-toolbar `ToolMenus` override and its legacy test now that the active preview strip lives in the right pane beside the canvas. The standard Save/Browse toolbar remains.
- Reverted overlapping circle endpoints to evenly sampled single closed paths to avoid the thick seam.
- Updated GPU offset packing to respect the editor's documented `ScreenOffset` range of −4..4.
- New native tests for family/preset coverage, 32-element runtime packing, editor undo index repair, opacity isolation and reorder behavior; total **29 test registrations**.
- Added `Tools/CheckDev07Source.py` for source contract assertions, alongside the existing shader contract check.

**Validation:** Static shader-contract and source-contract checks passed in the creation environment. UE5.8.3 Windows compilation, UE global shader compilation, all 29 native automated tests and actual visual QA **must still be run**. All modifications are within Glint. This is not marked production ready.

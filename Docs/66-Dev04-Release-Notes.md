# Glint Dev04 / 0.4.0 — Development Notes

**Source delivery, not a compiled or runtime-validated release.**

- Added three-mode Schematic / Rendered / Hybrid native Slate authoring with a synthetic-source, actual GPU RDG preview that reuses the runtime's source-aware optical shader function.
- Added per-element Details proxy, shared list/canvas selection, overlapping-handle cycling, list search, asset visibility controls, editor-only Solo, simulated RGB/white light source and exposure controls.
- Added source dragging with Shift, mode/light/exposure persistence per asset, and change-triggered preview dispatch to avoid continuous GPU work when idle.
- Added Hoop, Multi Iris, and Spike Ball authored optics, including a source-aware path and whole-HDR fallback approximations.
- Added SourceEdgeFade and SourceGain to lens controls and shader payloads.
- Retained zero-outside-screen sampling, temporal source matching, automatic ghost/streak field, camera blending, physical aperture option, and earlier tests.
- Expanded native test registrations from 12 to 16 and six global shader entry-point parameter checks.

**Open:** UE5.8.3 build, PCD3D_SM6 startup compilation, actual Slate preview rendering, editor Undo/Redo and GPU/cook performance. Gameplay Cameras runtime rendering still uses shared camera/view data; dedicated Camera Rig node/evaluator integration is planned, not part of Dev04.

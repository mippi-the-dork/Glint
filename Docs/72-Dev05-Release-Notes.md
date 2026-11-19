# Glint 0.5.0 — Development Pass 05 Release Notes

- **Editor reorganized** as left-side Elements outliner / Selected Element Details, with a right-side GPU/schematic canvas and Schematic/Rendered/Hybrid mode controls plus Settings gear on the native asset toolbar.
- New elements start with **type-specific composition defaults**, including optical-axis placement, shape, scale, color and strengths. Changing type reapplies family defaults with custom label preservation.
- **Three added source-driven optical families:** Shimmer, Sparkle, Caustic (13 total).
- **Seven type-conditioned shape controls** drive the source-relative GPU renderer and its editor preview; the runtime and preview share the compact 80-byte payload.
- Native drag/drop element reordering, Solo icons, visibility and trash control per outliner row, and removal of the old Up/Down buttons.
- **Right-drag** repositions the synthetic light, replacing light X/Y controls. Preview exposure and source colors moved to Settings gear.
- **Camera Frame Guide** separates actual image from offscreen editing space. Workspace-clamped handles remain selectable without clipping authored optical values.
- **Camera background Texture2D** chosen from the gear menu, displayed below GPU preview in Rendered/Hybrid, with correct screenshot aspect and per-Flare editor-only persistence. Does not yet drive automatic highlight detection.
- GPU preview now writes alpha to support transparent flare overlays on screenshots. Actual runtime HDR compositing remains unchanged.
- Six additional native tests registered: authoring defaults, GPU shape payload, camera guide, selectable offscreen handles, screenshot aspect and background runtime isolation; 22 total.

**Status:** Source-level pass; not compiled or run on the user's Windows UE5.8.3 installation. Shader contract script checks six entry points, but only UE5.8.3 can validate Slate, RenderCore API and actual GPU preview behavior.

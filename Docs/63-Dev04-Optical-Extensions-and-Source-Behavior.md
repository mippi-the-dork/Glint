# Dev04 — Optical Extensions and Source Behavior

## New authored element families

Three more runtime-authored shader kinds are appended **without renumbering** the seven 0.3 enums and assets:

- **Hoop (7):** an expanded, relatively narrow optical ring.
- **Multi Iris (8):** several concentric diaphragm rings.
- **Spike Ball (9):** a finer eight-direction radial diffraction pattern.

The source-driven GPU renderer implements all ten kinds analytically. The older whole-image fallback also includes approximations for the three new types, keeping the `r.Glint.SourceTracking 0` comparison mode usable. The native schematic uses corresponding mark-up primitives. These are still procedural proxy optics, not simulated wave-optical caustics, imported masks or physically reconstructed lenses.

## New source-aware Lens controls

`SourceEdgeFade`, default **0.08 viewport UV**, gradually reduces a source's authored contributions near any edge. Zero disables fading. This supplements, but does not replace, 0.1.3's zero-outside-screen sampling protection. `SourceGain`, default **1**, scales source-driven contributions without changing the automatic image-wide ghost/streak field. Both are sanitized in `MakeRuntimeData` and blended when multiple Lens profiles contribute to a rendered view.

These two controls are exclusively part of the `Source Detection` category on the Glint Lens. Their C++ runtime data, HLSL shader parameters, render-pass assignments, and static contract gate match. Scene-HDR bright threshold and source-selection budget stay independent.

## Production cautions

Fading based on on-screen source UV doesn't generate offscreen flares or simulate scene occlusion. Source selection still uses tile maxima; huge or patterned emissive regions can be selected repeatedly. Tracked sources are matched in image space without actor identity or occlusion history. Performance needs measurement with 1–8 sources at 12–24 authored elements and at 1080p/1440p/4K.

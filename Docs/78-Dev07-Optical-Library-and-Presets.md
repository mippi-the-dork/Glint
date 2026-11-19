# Glint Dev07 — Expanded Optical Element Library

**Implemented:** 26 authored element *types* and three starting presets (Default / Subtle / Cinematic). The 13 existing enum values retain their exact serialized order and numeric identities. New values are appended.

## New types and intended role

| Element | Role | Important controls |
|---|---|---|
| Polygon Iris | Diaphragm-shaped ring | Blade count (Ray Count), thickness, rotation |
| Aperture Blade Ring | Outer polygonal ghost | Blade count, radius, thickness |
| Aperture Ghost | Filled angular reflection | Blade count, size, aspect |
| Cross Star | Four-point diffraction spikes | Rotation, size, sharpness |
| Vertical Streak | Vertical bright streak | Rotation (default 90°), length/size, sharpness |
| Dual Streak | Primary and orthogonal streak | Rotation, secondary strength, sharpness |
| Smear | Directional comet-like ghost | Rotation, aspect, falloff |
| Arc Reflection | Ring segment | Rotation, radius, thickness |
| Edge Ring | Large dim outer halo | Radius, thinness |
| Soft Disc | Diffuse rounded ghost | Soft falloff, size |
| Veiling Glow | Wide, low-energy veil | Falloff, radius |
| Petal Iris | Wavy diaphragm ring | Petal/blade count, secondary strength |
| Dust Cluster | Small, stable optical speckles | Frequency, density via secondary strength, falloff |

All 13 new families are recognized by the source-driven HLSL renderer **and** the synthetic GPU editor preview, which share the same `GlintEvaluateSourceElement` function. The legacy untracked *image-field* authored path has an intentionally inexpensive generic fallback for the new shapes rather than full physically modeled geometry; compare with `r.Glint.SourceTracking 1`. These are stylized analytic lens artifacts, not a measured physical optical simulation.

## Authoring defaults and presets

Use the **+ Add** list to create each family with type-specific positions, sizes and optical controls. Use that family's **Presets** submenu to choose:

- **Default:** standard per-type optical placement.
- **Subtle:** 40% baseline energy, 80% baseline size, reduced secondary detail.
- **Cinematic:** 150% energy and 130% size with slightly amplified secondary detail, respecting existing parameter bounds.

These presets create ordinary elements, not a new runtime asset format. They can be freely edited after creation. Type changes in selected-element Details retain the editor's existing behavior: preserve a custom label, otherwise reset to the new family's defaults.

## Budgets and compatibility

- **Per Lens:** `FGlintLensBlendData::MaxElementsPerLens = 32`.
- **Per blended view:** `r.Glint.MaxElements` defaults to 32 and clamps to 0–32; performance/quality can be managed with this console variable.
- **GPU sources:** existing source-selection cap remains 8 per view.
- **Packing:** the editor and runtime share `UGlintLensProfile::PackElement`, with optical offsets now allowed over the documented −4..4 authoring range rather than silently clipping at ±1.
- **Performance:** 32 elements × 8 sources is a maximum evaluation envelope, not a performance guarantee; complex shaders and multiple sources require GPU profiling under representative projects and quality settings. This pass does not implement precomputed lens transfer textures or a new ray-transport system.

## Source and tests

- `Source/Glint/Public/GlintFlare.h`, `Private/GlintFlare.cpp`: schema, presets and defaults.
- `Shaders/Private/Glint.usf`: new source-driven shapes and image-field fallback.
- `Source/Glint/Public/GlintLensProfile.h`: per-lens maximum.
- `Source/Glint/Private/GlintSceneViewExtension.cpp`: per-view maximum.
- `Glint.Data.Flare.ExpandedFamilies`, `.PresetVariants`, `.LensBudget32` verify basic consistency, in addition to the previous tests.

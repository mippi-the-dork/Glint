# Dev01 — Edge Sampling Regression and QA

**Scope:** Glint shader-only correction, UE5.8.0–5.8.3.

## Observed failure (reported on 0.1.2)

- Sun and other high-luminance features near any viewport boundary generated elongated horizontal or vertical bands.
- The distortion could appear on both the bright flare highlights and chromatic components as the camera rotated slightly.
- The user reported no temporal flickering. `Glint.Camera.Binding.SetLens`, `Glint.Data.Flare.Count`, and `Glint.Data.Lens.Defaults` all passed.

## Root cause

`GlintSampleBright` in `Shaders/Private/Glint.usf` used an unconditional integer-pixel clamp. Every ghost and streak tap with UVs outside the source frame sampled its nearest bright edge texel. Repeated taps accumulated the same texel across the flare field, creating screen-aligned stretching. The extraction pass also used pixel clamping to fill partial 4x4 edge tiles.

## Fix in 0.1.3

1. `GlintLoadBrightOrBlack` checks both axes before issuing a texture Load. Samples outside the quarter-resolution image contribute zero.
2. `GlintSampleBright` uses manual bilinear reconstruction from four in-bounds-or-zero texels. This permits a smooth subpixel fade at the edge, without generating infinite border extension.
3. Final flare compositing also uses transparent-black bilinear neighbors rather than clamping against the reduced flare texture border.
4. Extraction visits only real HDR pixels and normalizes partial cells using their valid count rather than duplicating the last row or column.
5. C++ shader parameter structures and Unreal module dependencies remain unchanged.

## Manual acceptance tests (must be run in UE5.8)

- **All boundaries:** With an unbound Post Process Volume and Glint Lens, move the sun slowly from center to beyond each screen side (left, right, top, bottom). Repeat at each corner. Inspect the *entire* screen, not only the sun.
- **Finite highlights:** Ghosts/streaks may naturally clip or fade at the screen edge; no long screen-aligned lines should grow from edge pixels or persist after the generating highlight has left the source image.
- **Debug extraction:** With `r.Glint.DebugBrightPass 1`, the image should show the extracted thresholded bright field without border replication. Return to 0 for the final composition.
- **Odd view sizes:** Try a non-multiple-of-four viewport width and height, resize editor viewport and repeat the edge sweep.
- **Motion:** Rotate/pan camera gradually, watching for crawling, abrupt jumps, or brightness pumping. The reported no-flicker baseline should be retained.
- **Other systems:** Test one emissive point light-like source and one bright cloud/horizon scene, compare Glint disabled vs enabled.
- **Automated:** Run `Automation RunTests Glint`; passing the existing data tests does not verify GPU behavior.

## Limits of this fix

This fixes texture border extension in the prototype automatic image-driven renderer. It does not implement off-screen physical light-source tracking, authored optical elements, exposure normalization, temporal source tracking, occlusion, or GPU performance optimization. An off-screen light cannot cause a new flare without a separate source or temporal-history path.

**Verification:** Shader reviewed and source checked offline; not compiled or visually tested in Unreal Engine from this environment.

# 49 — Renderer Budget and Next Pass

## Where current GPU work is spent

- `Glint.Extract`: 16 HDR samples per 4×4 reduction cell to retain isolated 1-pixel highlights.
- `Glint.FieldGhosts`: shared automatic image field at quarter width/height, about 22 samples per small pixel, bounded regardless of number of source regions.
- `Glint.CompositeHDR`: full-resolution HDR copy and four-tap flare upsample; currently the likely largest bandwidth cost at 4K, particularly if the callback had to copy an array slice.
- No GPU work if the lens asset is absent. No CPU scene-to-light readback.

## Most important next technical work

1. Verify actual C++/USF compile and RDG pass execution in UE5.8.3 and fix platform-specific declarations.
2. Normalize thresholds by pre-exposure; test histogram auto exposure, manual exposure and camera cuts.
3. Add source-free GPU tile statistics (peak, energy, area, centroid), bounded candidate ranking and simple procedural full flares.
4. Introduce a separate high-resolution sharp response without increasing cost of soft ghosts/streaks.
5. Add temporal selection stability, bounded update frequency and history reset for cuts/resizes.
6. Add actual authored Glint Flare element rendering and Slate editor; the data asset currently stores definitions but is not rendered.
7. Implement native Gameplay Cameras node+evaluated lens state transition system, optional module.
8. Improve the composite to avoid bandwidth-heavy full-resolution compute output and unnecessary scene copies. Only after profiling investigate tile ghost bins and cached optical transfer.
9. Add opt-in authored world source occlusion and offscreen source behavior; avoid using depth-based pseudo-occlusion for automatically detected screen pixels.
10. Add packaged-build QA and GPU automated capture/quality tests.

## Explicit non-goals for initial pass

No claim of 0.6 ms, no high-end optical simulation, no actor-light discovery, no cooked/platform test results. Visual results must drive future choices.

# Dev08 — Coherent HDR sources and energy control

## Observed in UE5.8.3

The user validated **32/32 Glint 0.7.2 native tests**, and continuous camera rotation no longer causes wholesale flare disappearance. However, the 26-element all-Cinematic test produced severe repeated ring/disc stacks and clipped energy. With broad HDR lights and clouds, tile maxima can describe several peaks of a **single** source, spawning multiple full authored compositions.

## Implemented in 0.8.0

- **Source Grouping Radius** (`UGlintLensProfile::SourceGroupingRadius`, default `0.125` UV, 0–0.3) is an additional minimum radius during GPU peak selection. The effective radius is `max(SourceSeparation, SourceGroupingRadius)`. Setting it to 0 restores legacy separation behavior. Adjustable because physically separate close lamps must remain possible. This is an **image-space heuristic**, not yet connected-component HDR region labeling or actor-aware identity.
- **Authored Energy Limit** (`UGlintLensProfile::AuthoredEnergyLimit`, default 6 linear HDR energy units, 0–128 in sanitized runtime data; 0 disables) soft-limits the *sum* of authored optical contributions per pixel with `limit / (limit + luminance)`. This is not a hard clamp and does not suppress automatic ghost/streak emission.
- Shader evaluation now uses a conservative bounding-sphere test **before** per-element rotation and branch-intensive calculations. The existing fine `r > 6` branch remains. All sampled coordinates and viewport edge guards are unchanged.
- CPU reference helpers `FGlintSourceSelection::SelectGrouped` and `LimitAuthoredEnergy` are tested against the expected grouping and energy behavior. GPU code is distinct and requires live testing.

## Limits and open work

The detector still ranks one peak per 32×32 quarter-resolution tile. Grouping improves suppression but does not measure or label true connected bright regions. Two emitters closer than the configured radius may be merged. A bright textured cloud can still be treated as multiple independent sources outside that radius. For robust source identity, future work should implement an HDR region-labeling pass with confidence/extent, explicitly benchmarked. No changes to engine source required.

The approximate work remains bounded by up to eight GPU sources and up to 32 elements. The new bounding sphere avoids many costly analytic shape evaluations but **does not** eliminate the source × element loop itself. GPU timings must still be measured using `ProfileGPU`, particularly with four/eight lights and 32 elements, before release.

## Recommended acceptance test

1. Apply a detailed 26-element flare; `r.Glint.Automatic 0`, `r.Glint.Authored 1`, `r.Glint.SourceTracking 1`, and `r.Glint.DebugSources 1`.
2. Look at the sun. Compare Source Grouping Radius `0` and default `0.125`; inspect the number of markers and repeated rings. Rotate the camera continuously and look for loss of established sources.
3. Add two emissive spheres close together (same and different colors); lower grouping radius if Glint merges them.
4. Compare Authored Energy Limit `0` vs `6` with all 26 Cinematic elements on. Check that highlights roll off instead of dominating the image while dim ghosts remain visible.
5. Check `r.Glint.Automatic 1` shows unchanged automatic contributions; check edge fades and camera-cut history reset.
6. Run `Automation RunTests Glint` and the six-compute shader contract script.

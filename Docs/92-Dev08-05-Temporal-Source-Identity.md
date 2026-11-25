# Glint 0.8.5: Temporal source identity, HDR tint, and outliner icon recovery

## Current state and evidence

This is a **Glint-only, source-plugin** update for UE 5.8 Win64. No engine source changes and no schema/asset migrations.

The user tested 0.8.4.1 on November 22, 2026: **40 successful tests, one failure** (`Glint.Source.Tracking.ReleaseHysteresis`). They observed the following while the camera was stationary: source debug crosses sometimes shifted by tens or hundreds of pixels and often switched colors between magenta, amber, and cyan. This *mode 2* color variation represents tracker confidence/status, **not the RGB color of the light**. Source-driven flare appearance changed accordingly. The Solo and Delete icons were blank again even though both buttons functioned.

Two same-session `.profViz` frames were supplied. These are not a statistically valid performance sweep:

| Capture | GPU frame |
| --- | ---: |
| Glint off, 14:46:16 | 6.0827 ms |
| Glint on, 14:46:51 | 7.2362 ms |

The Glint-on GPU Visualizer identified `Glint.Extract` 0.0048 ms, sources clear 0.0103 ms, `Glint.FindSourceCandidates` 0.3840 ms, `Glint.SelectAndTrackSources` 0.4373 ms, `Glint.AutomaticAndAuthored` 0.0881 ms and `Glint.CompositeHDR` 0.0174 ms. Sum approximately **0.9419 ms**. The candidate and tracking passes constitute 0.8213 ms, around **87% of identified Glint work**. GPU frame delta (~1.1535 ms) also includes other frame-to-frame variation. These are measurements of 0.8.4.1, **not projected performance for this revision**.

## Changes in 0.8.5

1. **Camera-aware source tracking.** The renderer stores the previous camera view rotation and origin per `FSceneViewStateInterface` history. UE's view rotation comes from `View.ViewMatrices.GetViewToWorld().ToQuat()`, independently of projection-matrix TAA jitter. Angular motion (0 to 1, with 1 near 0.025 radians) and a coarse view-translation response (50 Unreal units to 1) adjust source-match radius and interpolation responsiveness. Translation is not depth-aware and needs in-engine acceptance. Stationary cameras search a narrow local neighborhood; deliberate camera movement permits up to 0.2 normalized UV of reacquisition. New sources still need the full acquisition gate; existing ones use release hysteresis.
2. **Nearest eligible candidate for established slots.** Within the permitted motion radius, source positions now prefer the nearest candidate, not whichever candidate is brightest. Brightness remains the gate. This reduces sudden identity switches to distant checkerboard/cloud peaks. This is source continuity, not physically identified object tracking.
3. **Raw-HDR local presence fallback.** When stationary and no candidate lands in the narrow matching radius, a 5x5 raw HDR scene neighborhood is tested at the *previous source UV*. If that area is still bright above the release gate, retain its position rather than switching to a distant tile maximum. If local HDR has disappeared, ordinary confidence persistence/dropout applies. Large actual source motion in a stationary camera may therefore appear delayed before reacquisition; monitor moving emissives separately.
4. **Stable emitter color.** Source tint is now calculated from a 5x5 raw HDR scene neighborhood for each of at most eight sources, rather than one possibly threshold-clipped `Bright.rgb` pixel. Tint intensity is bounded, and the RGB result blends with the history more slowly while the camera is stationary. This aims to avoid spurious changes in authored/automatic source color; it does not disable scene-derived color.
5. **Outliner Solo/Delete images.** Replaced the custom `SLeafWidget` geometry-drawn glyph that disappeared in live UE despite a nonzero desired-size test with ordinary `SImage` brush content, using the plugin's registered `solo-outline.png` and `trash-icon.png` brushes inside centered **16x16** widgets within **24x24** buttons. The new native test exercises brush resource names and performs a Slate prepass. Real editor verification remains necessary.
6. **Source debug mode 3.** `r.Glint.DebugSources 3` now colors each source by its stable slot index (eight distinct colors) rather than by confidence, so an actual slot reassignment is visually distinguishable from a mode-2 confidence transition. Modes 0, 1 and 2 remain available.
7. **Release test precision.** The prior exact float test was failing because UE's default near-equality tolerance is too strict for the computed float. It now explicitly tests within 1e-5. The same release-gate formula remains in use.

## QA commands

For a project using the plugin at `Plugins/Glint/`:

```
Automation RunTests Glint
```

41 existing + 2 new = **43 tests registered total**: the two new tests are `Glint.Source.Tracking.StationarySourceIdentity` and `Glint.Source.Tracking.StationaryHDRRetention` (0.8.4.1 had 41). Relevant existing tests: `Glint.Source.Tracking.ReleaseHysteresis`, `StableSourcesUnderJitter`, `DynamicResolutionHistory`, `CameraCuts`, `MotionAndDropoutPersistence`.

Static checks (from project root or plugin folder):

```
python Plugins/Glint/Tools/CheckShaderContracts.py
python Plugins/Glint/Tools/CheckDev0841ShaderScope.py
python Plugins/Glint/Tools/CheckDev085Source.py
python Plugins/Glint/Tools/CheckDev084Source.py
python Plugins/Glint/Tools/CheckDev083Source.py
```

The static tests check source invariants and C++/HLSL bindings; they are **not** substitutes for UE5.8 C++ and SM6 shader compilation.

## Explicit manual acceptance

1. Close UE, replace the entire `Plugins/Glint` folder, rebuild `ProjectHeavyWaterEditor` Win64 Development on UE5.8.3. Open the project. Capture the *first* Glint C++/HLSL error if compilation fails.
2. Run `Automation RunTests Glint`; all **43** Glint tests should succeed. Record only Glint failures, if any.
3. Open your existing all-Cinematic Flare. Confirm Solo and Delete icons appear **inside** their gray buttons in the first, middle, and last visible rows. Hover and click Solo and make sure it works; click Delete on a throwaway element and Undo. Resize the panel; the icons should not disappear or shift outside button bounds.
4. Use your original Lens and the existing sun/sky scene. Set:

```
r.Glint.DebugFlareOnly 1
r.Glint.DebugSources 2
r.Glint.LegacyAutomaticField 0
r.Glint.LegacyAuthoredField 0
r.Glint.FlareResolutionDivisor 2
```

Keep the camera still **15 seconds** after a short warm-up. Most established sources should remain magenta. Previously, alternating pink/amber/cyan at a fixed UV indicated changing confidence; now an existing bright HDR source should usually retain its confidence. Watch for positional jumps. Then move the view slowly 5 degrees, stop, and repeat. Move rapidly ~30 degrees and confirm sources follow, without sticking at abandoned image positions.
5. Set `r.Glint.DebugSources 3`. With the camera stationary, each cross should keep the same slot color. If one switches color at a fixed point, the source was assigned another slot. Use this observation to distinguish identity swaps from confidence changes (mode 2). Return to `2` for dropouts.
6. Compare with TAA enabled and with your real upscaling enabled, at the same fixed viewport size and view. Inspect bright colored spheres separately from the sun. Look for changes in source position **and** flare hue. The new source color is derived from raw HDR and should change with actual emitter color, not every TAA jitter phase.
7. Turn off source markers and black mode (`r.Glint.DebugSources 0`, `r.Glint.DebugFlareOnly 0`). Reproduce the original level's real look and camera motion. Do not conclude success from black debug mode alone.
8. Profile the same camera pose at the same viewport size and scalability settings with `r.Glint.Enabled 0`, then `1`. Capture several frames for each and report GPU times for `Glint.FindSourceCandidates` and `Glint.SelectAndTrackSources`. These measured ~0.38 and ~0.44 ms respectively in the pre-update capture and are the main optimization targets. Confirm debug views are disabled during timing.

## Constraints and known limitations

- The updated shader has not been compiled by UE5.8.3/SM6 in this authoring environment. The 43 native tests have **not** been executed against 0.8.5. Performance improvement is **not** claimed; additional HDR neighborhood loads could change GPU cost.
- The algorithm is still a tile-peak selection system with temporal association, not region labeling or motion-vector/depth-based optical source identity. Very bright or broad image areas may still spawn multiple candidate sources, and moving emitters with a static camera may be temporarily retained at their previous location. This is the next architecture target, not something the present stabilizer entirely solves.
- The current source selection pass scans candidate tiles serially for up to eight slots. That pass and candidate generation dominate observed Glint GPU cost. The next performance pass should test a parallel selection reduction or a tiered candidate shortlist while maintaining stable slot identity and grouping.
- The source debug markers are visualization overlay; they do not feed source extraction. Confidence colors (mode 2) are **not** the measured RGB of the source. Mode 3 tracks slot index.
- Current plugin is Windows 64-bit UE5.8 source. No engine modification, no Probe dependency.

## Next work

1. Verify C++ and SM6 shader compilation plus native 43/43 tests.
2. Validate fixed-camera source stability and editor icons in the user's actual Unreal viewport/editor at their TAA/upscaling settings.
3. Use multi-frame on/off GPU measurements to target `Glint.FindSourceCandidates` and `Glint.SelectAndTrackSources` (~87% of observed Glint time), without sacrificing multi-emitter separation.
4. Move source association beyond tile peak/nearest matching toward GPU region coherence and temporal velocity/reprojection when practical.

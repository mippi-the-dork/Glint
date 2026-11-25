# Glint 0.8.4 — Stable Sources, Source-Relative Automatic Optics, and Editor Glyph Alignment

**Scope:** Glint only. Complete source plugin; no engine edits. Target UE5.8.0–5.8.3, Windows Editor/Win64.

**Status:** Source implementation and static contract checks completed. Unreal Windows C++ compilation, global shader compilation, UE native automated tests, GPU profiling, and visual verification **not** performed in this environment. The last USER-CONFIRMED baseline was 37/37 Glint native tests passing on 0.8.3.

## Why this pass exists

Manual screenshots of the 26-type Cinematic stack on black confirmed the debug view was valuable and that source-relative elements can be visually isolated. They also showed: (1) Solo and Delete glyphs rendered offset to the right of their small buttons, and (2) selected HDR sources continued appearing/disappearing with a stationary camera under temporal AA/upscaling. The old automatic system was also producing repeated, chromatically fractured copies of scene content rather than lens-like optical responses.

## Changes

### GPU source identification and persistence

- Candidate acquisition continues on the GPU using **raw HDR peak** from the extracted bright field, with pre-exposure compensation, not the artist-thresholded flare RGB. Candidate centroid now uses a 5×5 HDR-weighted neighborhood around the winning brightest pixel rather than assigning its exact texel center.
- Each previously tracked source has first opportunity to **retain its previous slot**, using a release threshold below the acquisition threshold. A new highlight requires the full acquisition gate. Only vacant slots are filled by new sources. The old strongest-first global re-ranking could replace a good source with a transient brighter winner even while stationary.
- Reacquired positions use existing source smoothing with a small **position dead zone**. Luminosity and color interpolate temporally rather than copying a noisy sample each frame. New sources fade in via confidence; missing detections decay using existing Source Persistence. Camera cuts and meaningful aspect-ratio changes reset history; render resolution changes with stable aspect ratio do **not** reset normalized source UV history (important for dynamic screen percentage).
- Two new Lens Profile parameters under **Source Detection**:
  - **Source Hysteresis** (`0.65`, range `0.25`–`1`): a previously tracked source remains eligible at 65% of the acquisition threshold. Increasing toward 1 means less hysteresis; reducing toward 0.25 means longer retention in low-brightness conditions.
  - **Source Position Dead Zone** (`0.0015`, range `0`–`0.01`) in normalized viewport UV: suppresses subpixel source-center wander. Too large may produce visibly stepped response to slow pans.
- These parameters are packed/sanitized per lens and blended along with the existing Source Detection controls. GPU paths remain independent of camera type and do not read back source positions to the CPU.

**Limits:** This is temporal stabilization, not motion vectors or physical object identity. Very rapid camera turns, occlusion, competition with many other HDR peaks, upscaler/shader history resets and some scene edge cases may still cause changes. No intentional infinite persistence through occlusion.

### Automatic optics no longer mirror the rendered sky

- Default automatic ghost/streak rendering is now **analytical and source-relative**, driven by selected HDR sources: compact ring/disc ghost families along the optical axis, modest RGB fringe offsets, and a source-centered bounded anamorphic streak.
- Automatic and Authored remain independent (`r.Glint.Automatic` / `r.Glint.Authored`, and the Lens Profile's Source Driven Authored checkbox).
- The previous image-warped automatic renderer is still accessible for isolated A/B checks through **`r.Glint.LegacyAutomaticField 1`**, default `0`. This is a *comparison path*, not the intended production image. The old authored image field is a different diagnostic: `r.Glint.LegacyAuthoredField 1`.
- Automatic source detection now runs when automatic source optics need it, even with an empty Flare or Source Driven Authored OFF. It is skipped when source-driven optics, debug overlay and authored tracking are all disabled.
- The new automatic optics are still artist-facing analytical approximations, **not** a calibrated multi-surface ray trace, measured lens ghost map or photographic diffraction solver. The next quality stage can improve calibrated lens data and asymmetry.

### Slate editor actions

- Solo and Delete glyph content now uses 0-pixel button content padding, explicit centered horizontal/vertical alignment, and a centered 18×18 paint surface. The glyph no longer scales beyond its allocated bounds. Drag grip/reorder behavior unchanged.
- These were visually offset in the user's 0.8.3 screenshot; new visual acceptance is needed to confirm UE5.8's live layout under the user's DPI and editor theme.

### More useful source debugger

- `r.Glint.DebugSources 1` retains the previous magenta + black-outline marker.
- `r.Glint.DebugSources 2` color-codes confidence: **magenta = established** (confidence ≥ 0.85), **amber = recently acquired / rebuilding** (0.50–0.85), **cyan = temporarily retained after missing detection** (confidence below 0.50). The marker is drawn only after final HDR compositing and is not part of the optical flare.
- `r.Glint.DebugFlareOnly 1` still displays black background while HDR source selection uses the real scene. This does not override the source debugger.

## Performance considerations

- All source selection is done in a single small GPU pass (up to 8 sources, with candidate tiles). Preserving previous sources adds scanning work. The new analytical automatic pass replaces full-scene bright-image warping, but may cost more or less depending on source count, ghost count, viewport resolution and scene. It has not been profiled in UE5.8 yet.
- Keep the default `r.Glint.FlareResolutionDivisor 2` for the first test. Compare divisor 1 for final-image details and divisor 4 for low-cost regression. Use `profilegpu` / Unreal Insights for matched GPU comparisons. Record CPU Game/Draw/RHI times separately.
- No claims of full TSR/DLSS/FSR frame-history integration or source motion-vector reprojection; scene sampling is within the existing UE5.8 MotionBlur post-process callback. TAA/TSR testing is essential before making compatibility claims.

## Quick manual QA (specific order)

1. Close UE, replace the **complete** project `Plugins/Glint` folder, rebuild ProjectHeavyWaterEditor Win64 Development UE5.8.3. If the compile fails, capture the **first Glint C++ error** or **first Glint.usf shader error** and its context.
2. Run `Automation RunTests Glint`. Expected: **41 registered tests**, including `Glint.Source.Tracking.ReleaseHysteresis`, `Glint.Source.Tracking.StableSourcesUnderJitter`, `Glint.Source.Tracking.DynamicResolutionHistory` and `Glint.Source.Rendering.AutomaticSourceIndependence`.
3. Open Glint Flare editor. Inspect Solo and Delete icon alignment on the **first**, **middle** and **last visible** row. Hover and click both, verify actions and icon remain fully inside 24×24 buttons. Repeat with resized editor and optionally alternate DPI.
4. In a sunny level using your existing Lens/Flare, set `r.Glint.DebugFlareOnly 1`, `r.Glint.DebugSources 2`, `r.Glint.FlareResolutionDivisor 2`. Keep the camera **absolutely still for 10 seconds**. Watch whether crosses shift or blink, and whether previously bright rings vanish. Move the camera a few pixels and stop for another 10 seconds. The primary sun cross should stay magenta after initial acquisition; cyan flashes mean a dropout is being held. It should not repeatedly vanish/reappear if its HDR sample stays above the lower release gate.
5. Change `Source Hysteresis` from `0.65` to `1.0`, then `0.4`, while holding the camera still, observing dropout rates. Change Source Position Dead Zone from `0` to `0.0015` then `0.004`, confirming stationary stability versus slow-pan lag. Return defaults afterwards.
6. Disable authored (`r.Glint.Authored 0`), leave automatic enabled (`r.Glint.Automatic 1`), set `r.Glint.LegacyAutomaticField 0`. There should be *no repeated sky/checkerboard fragments*; only optical sources, compact ghosts, and bounded streaks. Compare against `r.Glint.LegacyAutomaticField 1` to confirm the previous raster-warp effect is diagnostic-only. Return it to `0`.
7. Turn automatic off and authored on (`r.Glint.Automatic 0`, `r.Glint.Authored 1`). The authored Cinematic optics should match the previous pass in shape/behavior other than steadier source motion; the automatic mode must not affect this check. Use one or two elements for clarity rather than all 26 Cinematic presets together.
8. Run the stationary test separately with the project's **TAA, TSR/upscaling, and native-resolution** configurations, especially the one that produced the flicker. The plugin does not modify any engine anti-aliasing settings. Restore your original project settings.
9. For GPU timing, capture identical static viewpoints using `r.Glint.Enabled 0` vs `1`, and `r.Glint.LegacyAutomaticField 0` vs `1`. Keep `r.Glint.DebugSources 0` and `r.Glint.DebugFlareOnly 0` for meaningful performance results.

## Static checks and test status

- `python Plugins/Glint/Tools/CheckShaderContracts.py` (six shader entry points, **67 parameter bindings**)
- `python Plugins/Glint/Tools/CheckDev084Source.py` (structural invariants, 41-test registry)
- `python Plugins/Glint/Tools/CheckDev083Source.py` (older mode isolation and debug contracts)
- `python Plugins/Glint/Tools/CheckDev072Source.py` (prior history contracts, updated to support renamed local shader variables)

There is no Unreal Engine 5.8 executable/toolchain in the authoring environment. Passing source checks does **not** prove compilation, GPU performance, or runtime behavior. The 0.8.3 37-test suite was passed by the user and is the baseline for manual regression.

## Known limitations / next work

1. Test temporal AA/upscaler behavior in the real scene and investigate any remaining source swapping with the colored confidence debugger.
2. Visual quality: authored chromatic dispersion and more realistic off-axis variation are **not** yet added; automatic source optics are now shape-based but still approximations.
3. Performance: profile new source selection and automatic optics versus the previous image-warp path; especially at 8 active sources and 32 authored elements.
4. Camera cut, multi-viewport, Cine Camera and experimental UE5 Gameplay Cameras are supported through the same view path but need repeated integration acceptance after renderer changes.
5. Improve source identity matching beyond proximity and brightness, potentially with GPU region moments and depth/occlusion signals, rather than unlimited temporal smoothing.

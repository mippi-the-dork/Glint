# Glint 0.8.3 — Authored Renderer Isolation and Debug Quality

Target: Unreal Engine 5.8.3, Win64. Glint plugin only. Based on 0.8.2.

## Why this pass exists

User A/B screenshots (fresh Lens Profile, empty Flare, then single Cinematic Halo/Iris/Ring) confirmed a behavior discrepancy:

- Empty Flare: `Source Driven Authored` on/off yields essentially the same automatic effect.
- One authored element: enabling it uses GPU-tracked source-relative analytic shapes; disabling it previously switched to the much older inverse-UV/bright-image resampling fallback.
- That fallback can show boxes, replicated highlights, fragments of scene content and inconsistent shapes. The automatic ghost/streak shader is independently unchanged, but both paths were composited into one image, making the whole result seem to change.

**Decision:** `Source Driven Authored` **OFF** now disables authored contributions; it does **not** silently enable the old field renderer. Automatic flare stays controlled by the separate `Enable Automatic` profile setting and `r.Glint.Automatic`. The old image-field fallback remains available **only** for development comparisons via `r.Glint.LegacyAuthoredField 1`. With Source Driven Authored ON, the analytic GPU-source path is used as before. Existing serialized booleans and assets retain their data.

## Other corrections

- New `r.Glint.DebugFlareOnly 1` replaces the *displayed* scene RGB with black at the **final composite**, after scene HDR extraction and all source selection/color/optics calculations. This does not turn scene sampling off. `r.Glint.DebugSources` may be enabled simultaneously to include tracked marker crosses. Set to 0 for normal display.
- New `r.Glint.FlareResolutionDivisor` controls temporary flare buffer size: **2 (default)** = half-res; 4 = old quarter-res; 1 = full-res for visual-quality investigation. A higher-quality buffer reduces blocky edges but raises shader work and memory bandwidth, so benchmark it. `GlintExtractCS` now samples 1×1, 2×2 or 4×4 HDR subpixels per flare texel correctly.
- The Lens Profile reset action remains visible beside modified fields. Its Slate visibility switches from **Collapsed** to **Hidden** when values are at defaults, reserving the same right-hand slot at all times. Numeric fields no longer resize when the reset arrow appears during scrubbing.
- Solo/Delete buttons give their native Slate vector glyph explicit 18×18 box constraints inside each 24×24 button and protect their paint stroke scale from accidental subpixel allocation. These still require visual confirmation on actual Windows Slate.
- Added `Glint.Source.Rendering.AuthoredModeIsolation` automated test for empty Flare, source-aware rendering, disabled authored rendering and deliberate legacy opt-in.

## Focused testing

1. **Build and automation:** `Automation RunTests Glint`. Expected **37 registered tests**; new shader bindings must compile on UE5.8.3 (static contract audit does not replace shader compilation).
2. **Source-driven switch:** Lens with 1 Halo (then Iris, Ring). With automatic enabled, compare `Source Driven Authored ON/OFF`. OFF should show **only the automatic** effect. ON adds the analytic authored optical shape. Confirm the automatic field does not visibly change apart from the added/removed element. Repeat with an empty flare (should be equivalent). `r.Glint.LegacyAuthoredField 1` deliberately restores the original field behavior only for comparison, and should be reset to 0 after.
3. **Black isolation:** `r.Glint.DebugFlareOnly 1`. The world goes black; Glint remains present and responds to the **real** sun, emissive colors, exposure and camera movement. `r.Glint.DebugFlareOnly 0` restores scene display. Source markers can be overlaid independently.
4. **Quality:** same camera, element and lens profile; compare `r.Glint.FlareResolutionDivisor 4`, `2`, `1`. Look closely at Ring/Iris edges. Report GPU time at each. The render-target size changes per frame; no restart is intended.
5. **Native editor:** Change Lens Intensity or Grouping Radius with a scrub and confirm its field does not shorten or jump when the reset arrow comes/goes. Verify Solo/Delete glyphs are now visible, centered and clickable; actions and undo still work.
6. **Source stability:** slowly pan a visible sun. The tile-maximum source selection can still jump on very small camera movements. This pass **does not** implement connected bright-region extraction or source-identity hysteresis; capture a short video if it persists.

## Known limitations / next work

- Automatic ghost/streak still uses image-field samples, including mirrored bright-field images. This is likely responsible for the user's large, crunchy, repeated-sky patterns; it has **not** been replaced in 0.8.3. Next pass should move large optical ghosts to source-relative analytic/baked shapes and keep a separate subtle continuous HDR veil for all-pixel response.
- Source detection is based on tile maxima and bounded top-K, not connected HDR regions; broad skies/clouds can create unstable candidates despite temporal smoothing.
- Authored per-element chromatic distortion is not yet driven by Lens `Chromatic Shift` the way the legacy automatic ghost code is.
- Most authored elements are currently fixed shapes per source position, with only their axis-offset geometry and aperture-size response. Physically-inspired location-dependent distortion, vignette, spectral dispersion and off-axis ghost transformation are future work.
- The energy shoulder is applied to **authored HDR RGB before additive composite**. `0` disables limiting; small positive limits strongly attenuate RGB, so it behaves like gain reduction across the whole rendered authored contribution. A separate physically justified energy/threshold model should replace this if visual tests demand it.
- Host-rendered Windows Slate visuals and actual SM6 shader compilation remain unverified in this environment.

## Static checks performed

`python Glint/Tools/CheckShaderContracts.py` checks all six compute entrypoint contracts, including `SampleStride` and `DebugFlareOnly`. The new mode-isolation test is registered in the native automation suite. ZIP validation checks one `Glint/` root folder and excludes build outputs.

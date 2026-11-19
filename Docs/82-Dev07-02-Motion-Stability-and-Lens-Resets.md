# Glint 0.7.2 — Motion Stability & Lens Profile Reset Arrows

**Scope:** Glint only. Replaces the complete Glint 0.7.1 plugin. No engine changes; no changes to other plugins.

## Observed problems

- Moving or rotating the camera sometimes makes authored flare elements disappear or reappear even though the bright sun remains in view and unobstructed.
- The Lens Profile's numeric/boolean properties lack reliable one-click **Reset to Default** affordances after editing.

## Root cause and implementation

### Transient source loss

In 0.7.1, the GPU source selector overwrote each output slot with zero whenever the ranked bright-candidate pass failed to return a candidate above the threshold. The authored shader then fetched the source RGB from the *current* bright-pass pixel, which was also zero if that candidate had temporarily vanished. The history never retained energy or color across a missed frame. Source smoothing only operated on detected positions, not loss of detection.

0.7.2 changes:

1. **Peak-preserving detection:** `GlintExtractCS` now writes the maximum raw HDR sample luminance of each 4×4 extraction block into `Bright.a` while leaving its existing artistic, thresholded RGB unaffected. Source-candidate ranking reads that alpha channel, so a bright highlight can remain trackable through brief under-threshold RGB frames during camera movement. The selection gate is at least half the lens highlight threshold (pre-exposure compensated) to avoid selecting normal dim surfaces.
2. **Bounded 8×2 GPU history:** row 0 stores UV / measured luminance / confidence; row 1 stores the last reliable HDR RGB value, removing the dependence on resampling an empty bright pixel to reproduce a held source. The source selector binds the existing quarter-resolution BrightTexture directly.
3. **Persistence:** unclaimed, recently observed highlights are retained for a few frames with geometrically decreasing confidence. `SourcePersistence` defaults to **0.65**; it can be disabled using **0**. Candidates below the threshold cannot replenish confidence. Sources are discarded once confidence drops below 0.12.
4. **Camera-motion response:** small position changes retain artist smoothing; larger changes rapidly catch up with new measurements. Matches accept up to 0.20 normalized UV movement while short-lived leftovers near currently measured highlights are suppressed.
5. **Camera cuts / resize:** retained history is discarded on camera cuts or viewport size changes, preserving the previous safety behavior. Per-view history remains GPU-side, with no readback or special Camera Actor requirement.
6. **No occlusion override:** this is a brief visual-stability mechanism. It does not establish true physical source visibility or compensate for occlusion. A moving sun passing behind an opaque obstacle can linger for a few frames. Future work should add world-source association and more robust HDR-region clustering.

### Lens Profile per-property resets

A `UGlintLensProfile`-specific `IDetailCustomization` registers with `PropertyEditor`. It preserves native numeric, checkbox, object-picker and category widgets, while adding an `IDetailPropertyRow::OverrideResetToDefault` action for each editable property. The arrow appears when a property is not identical to its C++ Class Default Object value; clicking resets it using a property handle so UE notifications, transactions and asset dirtiness are respected. Reset logic supports previously saved Lens Profile assets without migration.

## Test coverage and commands

Run `Automation RunTests Glint`. Newly registered:

- `Glint.Source.Selection.MotionStablePeakGate`: verifies the HDR peak acquisition gate and pre-exposure handling.
- `Glint.Source.Tracking.MotionAndDropoutPersistence`: a one-frame candidate dropout retains confidence and HDR color; confidence decays to zero; camera cut resets immediately; faster movement follows the current source without generating an extra one.
- `Glint.Editor.Lens.PerPropertyResetDefaults`: checks that the reflected class default is recoverable and a modified property is distinguishable from the CDO, including new source controls.

Static checks: `python Plugins/Glint/Tools/CheckShaderContracts.py` (6 GPU entry points, 58 shader bindings), `python Plugins/Glint/Tools/CheckDev07Source.py` (32 tests), and `python Plugins/Glint/Tools/CheckDev072Source.py` (source-history and Details customization contracts). Static checks are not substitutes for Unreal compilation.

## Manual UE5.8.3 acceptance

1. Load a Glint Flare with **Automatic** off and **Authored** on, and face a sun or stable emissive source. Pan and rotate at low, medium and fast speeds, including reversing direction. Previously detected authored elements should not suddenly disappear for one frame; they should recover smoothly when measurement returns.
2. Keep source debug available with `r.Glint.DebugSources 1` and watch for excessive duplicate source crosses or long streaks. Compare `Source Persistence = 0` with `0.65` while panning. There may still be duplicate bright regions; clustering remains separate future work.
3. Test true camera cuts and an abrupt viewport resize. No old-frame flare positions should survive those events.
4. Open a Glint Lens asset, change e.g. `Threshold`, `Source Smoothing`, `Source Persistence`, and `Ghost Count`. Verify the native reset arrow appears after each deviation, and resets that specific property to the C++ default, with save and undo support. The arrow should no longer appear when the value is restored.
5. Run `Automation RunTests Glint` and confirm **32/32**, then visually verify both standard level viewport and a camera view. Do not claim validated until executed on the Windows engine.

## Known limits / follow-up

- This patch targets transient source dropout and caching. It does **not** solve multi-tile detection of a single sun, all possible selection swaps under heavy HDR brightness changes, offscreen sources, actual world-light occlusion, or physical lens optics. These are separate source detection improvements for Dev08.
- Authored/automatic flare rendering performance under a 32-element, eight-source composition still needs GPU profiling. Avoid raising caps based only on a passing unit test.
- The editor reset custom layout and GPU shader binding changes need a full rebuild and first-launch shader compilation in UE5.8.3; automated static checks are not proof of C++ compilation or runtime stability.

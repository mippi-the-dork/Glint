# Glint 0.8.6 | Filtered HDR region detection and source-budget recovery

## Why this pass exists

The user's manual QA of 0.8.5.1 on November 22, 2026 found that Solo/Delete icon content still appeared blank despite passing Slate geometry tests, and that source markers still moved at a fixed camera, though less than previously. `r.Glint.DebugSources 3` retained slot colors despite visible movement. In several supplied screenshots, bright clouds and checkerboard terrain were selected while the visible sun frequently was not. Glint 0.8.5.1 had passed **43/43** native automation tests, so native tests alone did not demonstrate GPU source correctness.

**Critical distinction:** mode-3 color means a source *slot*, not a persistent physical emitter identity. The existing GPU selector chose a raw HDR maximum in each 32x32 low-resolution tile; temporal matching then guarded already occupied slots, making the initial selection sticky. Repeated centroid jumps and budget saturation were not solved by simply increasing source-history smoothing.

## Implementation in 0.8.6

1. **Separate filtered HDR detection texture.** New `GlintBuildDetectionCS` reads *unthresholded peak luminance* from `Bright.a`, computes a weighted spatial mean in a 3x3 neighborhood by default, and writes to a distinct RDG texture. `r.Glint.DetectionRadius` allows 0 (unfiltered), 1 (3x3), or 2 (5x5). There is no blur applied to final scene color, optics, HDR emitter tint, or raw HDR peaks. The detection pass runs only when source tracking or diagnostics require sources.
2. **Coherent neighborhood candidate ranking.** `GlintFindCandidatesCS` now retains the strongest combined raw-peak / filtered-region rank per tile. The winning candidate position is a filtered-luminance-weighted centroid in an 11x11 low-resolution neighborhood that can cross tile boundaries. Candidate texture data are `XY=centroid UV`, `Z=original unfiltered HDR peak`, `W=ranking evidence`. The score is `sqrt(peak)*(0.45+0.55*contrast)+0.75*sqrt(filtered)` with contrast `saturate((peak-filtered)/max(filtered,0.1))`. This balances tiny bright emitters with coherent larger areas; it is **not** semantic sun detection or GPU connected-component labeling.
3. **Relative new-source suppression.** The selector finds the strongest eligible region score, and requires new slots to exceed a configurable fraction of that score (`r.Glint.SourceRelativeScoreFloor`, default 0.22). Absolute HDR peak acquisition and release gates are preserved independently. Previously tracked slots may remain under their established release-hysteresis contract. This helps avoid filling all eight source slots with very weak highlights in a high-contrast scene.
4. **Sustained source challenger.** If all active slots are occupied, an unmatched candidate must exceed the weakest tracked slot's rank by `r.Glint.SourceReplacementRatio` (default 1.7) for `r.Glint.SourceReplacementFrames` consecutive frames (default 3), within a small spatial neighborhood. If the incoming candidate is distinct but within the configured grouping radius of an occupied source, it challenges **that nearby source** rather than being discarded; this is necessary when the visible sun sits near a selected cloud. This state lives in a third row of the 8-wide pooled source-history texture. It allows a newly visible dominant emitter to replace a weaker highlight without every transient specular flash instantly stealing an established slot. Slots still retain their identity color and the existing history reset behavior, except when intentionally replaced.
5. **Filtered HDR debug view.** `r.Glint.DebugSources 4` displays grayscale log-scaled *filtered detection luminance* and the existing tracked-source crosses. This helps diagnose where the detector sees large bright regions. Mode 3 still uses the stable eight-slot palette; it cannot prove object identity.
6. **Font-rendered editor actions.** The Solo and Delete buttons now render high-contrast `S` and `X` text using a standard `STextBlock`, centered inside the existing 24x24 buttons. Both previous custom drawing and PNG `SImage` controls had yielded blank *live* buttons despite passing layout tests. This is a functional, explicitly non-final icon style, chosen to eliminate action-brush resource dependencies. Native geometry tests still cannot guarantee live editor rendering.
7. **Clarified energy control.** The serialized `AuthoredEnergyLimit` property is unchanged, but its Details display name is now `Authored Energy Soft Limit` and tooltip explains its shoulder, zero-off behavior, and diminishing returns at high values.

## Native QA / checks

- Existing native automated tests before this pass: **43/43 success reported by the user** (0.8.5.1).
- Four new source automation tests added: `Glint.Source.Detection.FilteredHDR`, `Glint.Source.Detection.RegionalRanking`, `Glint.Source.Detection.RelativeScoreGate`, and `Glint.Source.Tracking.SustainedChallenger`. **47 tests registered** in 0.8.6; their UE execution is pending.
- Existing historical static checkers were updated **only where the intentionally replaced icon architecture, source history depth, region centroid, or bumped plugin version invalidated their former implementation-specific assertions.** Separate `Tools/CheckDev086Source.py` validates the new architecture, debug bindings, tests, and fonts. `Tools/CheckShaderContracts.py` validates seven shader entry points including the new detection pass and their per-entrypoint parameter scope.
- **Unverified in this authoring environment:** UE5.8.3 C++ compile, DXC/SM6 shader compile, GPU output correctness, and performance. No engine sources were modified. No Probe dependency. No file/asset migration intended, but existing runtime history must rebuild because the pooled source texture grew from 8x2 to 8x3; restart Editor when installing.

## Manual QA, in order

1. Replace the entire plugin folder, rebuild UE 5.8.3, and run `Automation RunTests Glint` (**47 expected**). Report first Glint-only compiler or shader error if any.
2. Open the original all-Cinematic Glint Flare asset and confirm visible *S* / *X* contents at the top, center, and bottom of the element list, after resizing and after reopening. Solo, Delete, Undo, and drag grips should work. S/X are intentional temporary action glyphs.
3. Use the same sunny scene at a fixed viewport size with normal upscaling. Start with `r.Glint.DebugFlareOnly 1`, `r.Glint.DebugSources 3`, and `r.Glint.DetectionRadius 1`. Hold completely still at least 15 seconds. Observe whether positions settle or still jump independently of slot color. Repeat slow/fast camera rotation.
4. Set `r.Glint.DebugSources 4` to see *which image regions* the new filtered detector considers bright. Compare default radius 1 to radius 0 and radius 2 **at the same camera pose**; make sure the tiny sun is still visibly represented in detection. Record whether the selected marker lands on or near the sun and whether ground/cloud regions are overrepresented.
5. Test `r.Glint.MaxSources 1` briefly to isolate ranking. Reset `r.Glint.MaxSources 8` when done. Test a lone colored emissive sphere, then two distinguishable emissive sources. Rotate and stop; track whether source color follows a genuinely changing emitter rather than TAA color noise.
6. When the sun is excluded in the default eight-source view, test whether leaving it in view for 3+ frames lets it displace a weak tracked source. If not, compare `r.Glint.SourceRelativeScoreFloor 0` and `0.22`, and `r.Glint.SourceReplacementRatio 1.2` vs `1.7`. Return defaults when done. Avoid drawing conclusions from one screenshot because slot-challenger confirmation is temporal.
7. Reset `r.Glint.DebugSources 0` and `r.Glint.DebugFlareOnly 0`, verify natural optics, and check that final HDR scene appearance has not been blurred. Compare TAA/TSR settings if available.
8. **Performance:** capture multiple consistent frames (same pose, settings, resolution) with Glint on and off, and report GPU Visualizer times for `Glint.Extract`, **`Glint.BuildDetection`**, `Glint.FindSourceCandidates`, `Glint.SelectAndTrackSources`, `Glint.AutomaticAndAuthored`, `Glint.CompositeHDR`. The added full low-resolution filter costs extra GPU bandwidth. Do not claim a speed-up from the static tests.

## Known limitations and next work

- This is a **region-aware weighted detector**, not full component segmentation, semantic identification of the directional sun, motion-vector/depth reprojection, or optical occlusion. Bright ground glints may still outrank a physically visible sun. A dedicated optional directional-light source could provide deterministic sun priority in a later pass.
- Repeated per-source scans through tile candidates remain in `Glint.SelectAndTrackSources`. A relative rank gate or challenger may increase that pass's measured cost; optimize only after visual acceptance and a sustained GPU benchmark.
- A filtered signal is inherently subject to merging adjacent bright lights, especially at radius 2. Preserve raw peaks, keep radius adjustable, and verify closely spaced bulbs.
- The challenger accepts sustained **ranked-screen evidence**, not a physical object ID. Rapidly moving or flashing lights may miss a three-frame confirmation. The position matching and confidence behavior remain approximations.
- The 11x11 regional centroid can shift the source location a few pixels from the absolute peak; always inspect the sun and edge-of-screen cases visually.
- S/X icons are robust text fallbacks rather than intended final art. Confirm live Slate behavior before considering a more expressive built-in glyph implementation.
- The `Authored Energy Soft Limit` shoulder is still bounded by the existing `128` sanitizer. It is not an exposure/intensity control.

## Default configuration for baseline comparison

```
r.Glint.DetectionRadius 1
r.Glint.SourceRelativeScoreFloor 0.22
r.Glint.SourceReplacementRatio 1.7
r.Glint.SourceReplacementFrames 3
r.Glint.MaxSources 8
r.Glint.DebugSources 0
r.Glint.DebugFlareOnly 0
```

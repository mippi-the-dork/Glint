# Glint Dev08.1 — Manual QA Hotfix and GPU Review

Target: Unreal Engine 5.8.3 / Win64 / Editor, complete replacement plugin. Only Glint changed.

## Prior acceptance

- Native Glint v0.8.0 automation: 35/35 Success (user).
- Source grouping radius reduces duplicate sun flares and merges closer independently colored emissive sources as designed.
- Undo/Redo and camera-motion source persistence are working in user's manual testing.
- Still failing in real Slate/UI despite prior source tests: Solo/Delete button imagery and Lens Profile reset arrows.
- Authored Energy Limit appears ineffective in one test configuration; code review shows the HDR shoulder is still wired to the authored output, but intense contributions and debug overlays may hide its effect. No numeric rendering bug established.

## Changes in 0.8.1

1. `r.Glint.DebugSources 1` renders outlined **magenta +** markers for valid tracked sources at the final composite resolution; no debug-marker pixels are added to the quarter-resolution ghost/authoring texture. Markers are diagnostics only, not an authored flare element, and do not appear when debugging is off. Each marker corresponds to one selected tracked source; an image can contain multiple source markers.
2. The two outliner Solo and Delete actions now use `SGlintRowActionGlyph`, a small native Slate-painted vector drawing independent of plugin PNG brush resolution. Drag grip remains unchanged.
3. Existing numeric/Boolean Lens Profile property rows retain their native editable value widget but also display an explicit Reset-to-Class-Default action when they differ from the `UGlintLensProfile` CDO. Existing object-reference rows remain native. The prior reset override is retained as fallback.
4. Added `Glint.Editor.Outliner.ActionGlyphGeometry` native test to validate that production widgets can be constructed with useful nonzero square space. This still does **not** validate actual host painting; manual screenshot acceptance is required.
5. Shader parameter binding check updated for three debug composite parameters moved from the ghost pass to the composite pass. 6 shader entrypoints, 62 declared bindings; checkers do not compile shaders.

## Screenshot QA

### Debug sources

- Keep existing Lens + 26-element flare assigned.
- Console `r.Glint.Automatic 0`, `r.Glint.Authored 1`, `r.Glint.SourceTracking 1`, `r.Glint.DebugSources 1`.
- Look toward the sun: count distinct **magenta plus signs with black outlines**. Do not count ring/ghost images.
- Compare Lens Profile `Source Grouping Radius` 0.0 -> 0.125 -> 0.2; expected fewer nearby crosses at larger radius, with a source persisting for the visible sun.
- Restore `r.Glint.DebugSources 0` for all other quality/performance testing.

### Energy limit (isolate the authored contribution)

- Keep a single bright source (Lens `Max Sources=1`). Turn off debugging and automatic effects: `r.Glint.DebugSources 0`, `r.Glint.Automatic 0`, `r.Glint.Authored 1`, `r.Glint.SourceTracking 1`.
- Use a moderate-intensity authored flare to avoid complete HDR/tonemapper saturation; ideally one Glow and one Ring. Use a fixed camera and stable exposure.
- Compare `Authored Energy Limit`: 0 (disabled), 0.5 (strong shoulder), 6 (default), with the same image.
- The profile setting affects **only authored optical energy**, not scene sunlight or source debug overlays. If all three remain indistinguishable, investigate whether the active Lens Profile is the one being edited or whether scene HDR tonemapping hides differences. Do not claim this is fixed in 0.8.1.

### Editor acceptance

- Open the same Flare. Both Solo and trash buttons should now display recognizable white geometry inside square action buttons. Test click/hover and Undo/Redo.
- Open the Lens Profile asset. Change Intensity away from 0.75, Source Grouping Radius away from 0.125, and Authored Energy Limit away from 6. A reset-to-class-default control should be visible beside the changed numeric editor. Click and verify each returns to the class default.

## GPU capture: one user-provided `.profViz`

Extracted named GPU events (ms): `Glint.Extract` 0.0236; `Glint.FindSourceCandidates` 0.0051; `Glint.SelectAndTrackSources` 0.1218; `Glint.AutomaticAndAuthored` 0.0922; `Glint.CompositeHDR` 0.0348. Total of these *named passes*: **0.2775 ms**. GPU frame event: **7.4325 ms**. The user reported approximately 65 FPS with their test Post Process Volume active and 85 FPS with it inactive (interpreted from wording), equivalent to ~15.38 and ~11.76 ms frame intervals. A single GPU capture cannot explain this disparity, identify the critical CPU thread, or isolate the volume's other effects.

For matched profiling: fixed camera and level; same viewport resolution and Post Process Volume state; disable debug overlays; use `r.Glint.Enabled 0` versus `1` without removing the volume; `stat unit` to record Game, Draw, GPU for both; capture `ProfileGPU` for both. Do not infer GPU shader cost from FPS alone. Avoid comparing different exposure configurations.

## Automated/static checks

```
Automation RunTests Glint
python Plugins/Glint/Tools/CheckShaderContracts.py
python Plugins/Glint/Tools/CheckDev081Source.py
```

Expected 36 registered tests in this package. Previous 35 passed in UE5.8.3; new test and changed C++/shader require build and runtime acceptance. No modifications to engine source or other plugins.

## Remaining architectural work

- Camera-aware, connected HDR bright-region identification rather than tile-maximum grouping alone.
- Full-screen source diagnostic visualization with source IDs and counts if cross-only is insufficient.
- Native Details reset behavior for non-scalar asset fields and a host-driven UI construction test.
- Cost attribution across CPU and GPU with controlled paired captures; performance under multiple highlighted sources and 32 authored elements.

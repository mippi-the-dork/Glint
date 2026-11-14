# 08 — Testing and Validation

[← Documentation index](README.md)

## Native test requirement

Use Unreal Engine's built-in automation framework and keep tests independently runnable using the provisional console command:

```text
Automation RunTests Glint
```

The stable test prefix is `Glint.*`. No third-party test tool dependency. The official UE framework supports low-level/editor/feature tests and screenshot comparisons.

## Test groups

| Prefix | Expected coverage |
|---|---|
| `Glint.Data.*` | Element GUID identity, serialization, reorder/group/copy, defaults, invalid textures, deterministic seeds |
| `Glint.Math.*` | Source-to-screen placement, axis factors, aspect correction, rotation, bounds, behind-camera math |
| `Glint.Runtime.*` | Source registration, handle generation, fade lifecycle, world teardown, source caps, priority |
| `Glint.Render.*` | Per-view packet integrity, shader permutation selection, texture refs, resource lifetime, no-source bypass |
| `Glint.Occlusion.*` | Depth comparison, soft sample math, bias, smoothing, cut resets, directional semantics |
| `Glint.Editor.*` | Factory, asset definition/launch, transactions, copy/paste, dirty state, preview isolation |
| `Glint.Perf.*` | Benchmark fixtures and instrumentation sanity; numbers evaluated against hardware-specific baselines |

Tests should avoid coupling to a user project or external test plugin. Keep original sample assets within plugin test content if necessary.

## Deterministic renderer visual checks

Screenshot/image comparison cases:

- Glow / Ring / Streak / Starburst / Ghost / Texture Sprite / Dirt baseline images.
- Source at center, left/right/top/offscreen, behind camera; letterbox and varied aspect ratios.
- Opaque blocker fully covers the point source, partially covers, and passes through source center.
- High/low exposure; bloom on/off; TSR/TAA vs FXAA/none where supported; dynamic resolution changes.
- 1/2/4 independent camera views where supported, with distinct occlusion and transforms.
- Camera cuts, abrupt source teleport, PIE restart and resized editor preview.

For screenshot tests, define tolerant image metrics and reference conditions; never claim pixel-perfect equivalence across arbitrary GPU vendors.

## Editor manual test checklist

- [ ] Create asset from Content Browser category, double-click opens proper editor.
- [ ] Add each supported element; inspect expected defaults, per-type Details and tooltips.
- [ ] Move elements with handles; undo/redo; cancel drag; save/reopen; duplicate/reorder/group.
- [ ] Hide/solo/lock work at element and group level; clearly distinguish preview-only from saved values.
- [ ] Import and assign custom texture; texture streams/cooks and doesn't create persistent broken reference.
- [ ] Store element and whole-asset preset; reuse in second asset.
- [ ] Preview uses live renderer, updates at acceptable interaction latency and is consistent with level rendering.
- [ ] Dock/undock tabs, restart editor and reopen different assets; no stale selection or asset mutation.
- [ ] Validate accessibility at 100% and 150% UI scale, narrow panes, long names and keyboard navigation.

## Runtime manual checklist

- [ ] Placed component on normal Actor and attached to a light.
- [ ] World transient source generated without an Actor; change intensity/location; fade/stop safely.
- [ ] Occluder moves in front of source, then moves away; no unsightly flash or sustained wrong visibility.
- [ ] Source behind camera, at edge and inside close geometry.
- [ ] Several simultaneous sources have independent asset and transform settings.
- [ ] Multiple levels/PIE sessions and world transitions do not retain invalid handles or resources.
- [ ] Entire effect disabled and re-enabled via project setting and runtime cvar.
- [ ] Native lens flare can be separately enabled/disabled without plugin modifying the user's PP settings.
- [ ] Packaged Development and Shipping-mode test builds produce equivalent visuals, without Editor modules.

## Build compatibility matrix

Required compilation/cook/package smoke on **5.8.0**, **5.8.1**, **5.8.2**, **5.8.3** (where the builds are available) with readable UTF-8 `.uplugin`, complete source and shader files. Smoke test DX12/SM6 as the initial baseline; record other RHI/rendering paths as verified or unsupported.

When a test fails, report exact test name, environment, expected vs actual and logs. Do not mark a runtime feature verified just because its data-only test passed.

## Release quality gates

- Pass all relevant automated tests and known manual checks.
- Validate cooked shader inclusion and asset references.
- Complete benchmark protocol and publish actual measured numbers.
- Verify Open/Close/PIE/Shutdown leak/crash behavior under repeated cycles.
- A clean project with only the plugin enabled can author, preview and run a sample flare.

## Automatic camera test expansion (required)

Additional native families: `Glint.Data.Lens.*` (lens assets, project/camera precedence), `Glint.Math.HDR.*` (pre-exposure, threshold and scene radiance), `Glint.Auto.Detection.*` (source mask, merge, top-K), `Glint.Auto.Temporal.*` (candidate stability/camera cuts), `Glint.Auto.Routing.*` (field vs candidate vs authored and dedup), and `Glint.Editor.Lens.*` (native editor, dirty/undo/visualization). Automated screenshot tests and packaged smoke must include HDR emissive **without any Glint Source Actors**. See [28 — Automatic mode test and release gates](28-Automatic-Mode-Testing-and-Release-Gates.md) for comprehensive fixtures and performance measurement.

## Perceptual LOD performance validation

Add the `Glint.Perf.*` invariant tests, comparative packaged benchmarks and motion-quality checks in [33 — Performance Experiments](33-Performance-Experiments-and-Gates.md). Ensure the test suite covers budget stability, LOD transitions, GPU-only automatic candidates, history per-view isolation, and idle/Off cases. Native data tests do **not** replace measured GPU timings.

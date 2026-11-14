# 16 — Technical Proofs, Research Spikes and v1.0 Quality Gates

[← Documentation index](README.md) · [09 — Roadmap](09-Development-Roadmap.md) · [14 — Rendering research](14-Optics-and-Rendering-Research.md)

> **Target:** UE5.8.0–5.8.3 Win64 deferred desktop, editor preview + cooked game. **Not yet built or benchmarked.** The gates below define what must actually be demonstrated, not what documentation alone can guarantee.

## P0: Feasibility gates before committing to detailed tool UI

| Spike | Minimal prototype | Proof / rejection criteria |
|---|---|---|
| **S0: Plugin-only renderer** | Scene View Extension + plugin-shipped GPU shader draws bounded Glow and Streak for one registered world source | Works on editor level viewport, PIE, standalone and packaged Win64 **without modifying Engine**; no editor-only shader asset dependency |
| **S1: Scene depth occlusion** | Four-sample depth visibility on an opaque blocker; smooth edge fade | Source dimming correct when moving behind opaque/Masked objects, no readback stall; directional/sky tests recorded |
| **S2: HDR vs LDR placement** | Same composition composited pre/post tonemap in test branch | Visual evidence for exposure, bloom, color, additive, Screen-like blending and cost; renderer stage chosen with written rationale |
| **S3: Temporal stability** | Thin Glint + 1px Streak with fast-moving camera under TAA, TSR and dynamic resolution | No unacceptable screen-space trail/jitter/popping; correct view coordinates and large-scale behavior |
| **S4: Multi-source batch and lens dirt** | 8 registered sources × 10 elements; one dirt mask illuminated by several lights | Measurable timings for submission, draw, occlusion, dirt/composite; no one-pass-per-source dirt growth |
| **S5: World and view isolation** | Two cameras / scene captures / PIE multiple worlds and transient handles | Independent projection, occlusion history, source behavior; safe teardown and no cross-view leakage |
| **S6: Native asset editor parity** | One true Glint Asset + basic Element Stack/Details + preview using actual render code | Undo/redo, dirty/save, cook references, preview/live view output comparable; no editor runtime dependency |

**Fail-fast policy:** If S0 or S2 cannot be done with safe public UE5.8 plugin hooks, revise renderer architecture or adjust scope. Do not silently depend on an engine patch, fork or a system that only works in editor preview.

## Initial benchmark scenarios (provisional, not verified numbers)

Establish a reference machine with CPU, GPU model, driver, UE version, renderer settings, resolution and scalability. Measure **incremental render cost** after warmup with RenderDoc/Unreal GPU profiler, per pass, across multiple stable runs.

| Scenario | Sources | Elements/source | Notable variable |
|---|---:|---:|---|
| Empty baseline | 0 | 0 | No-cost bypass, hooks and resources |
| One small flare | 1 | 3 | Minimum draw/setup overhead |
| Gameplay typical | 8 | 10 | Previous draft suggested **~0.6 ms at 1080p** as a target; evaluate rather than assume |
| Lights in corridor | 32 | 5 | Number of sources / batching |
| Hero flare | 1 | 30 | Complex curves, shimmer, ghosts |
| Fill-rate stress | 4 | 20 | Full-width streaks, huge glows, translucent layering |
| Dirt stress | 16 | 8 | Shared dirt illumination vs per-source passes |
| Camera behavior | 8 | 10 | Split-screen, 21:9, vertical, cut/teleport, dynamic res |

GPU costs must be broken into: source gather/projection, visibility/occlusion, render-record generation, analytic procedural element draw, texture sample, lens-dirt accumulation/composite, output composite. CPU time for asset/registry lookup and draw submission separately.

**Do not market '0.6ms' as measured** until the fixture passes. Ideally ship with scalability presets whose target caps are supported by benchmark results, not arbitrary guessing.

## Visual fixture library

All assets used in tests should be **original Glint content**, not Optical Flares preset files or textures. Required fixtures:

- One neutral photographic Glow, one high contrast Glint, one thin anamorphic Streak.
- One clean Iris and one Multi Iris with 3/5/12 children, both procedural and textured.
- Spectrum Ring/Hoop with partial completion and correct aspect ratio.
- Shimmer / Spike Ball / Sparkle with seeded animation, pause/resume and replay.
- One complex 20-element flare for full-stack testing, with group overrides and varied blend modes.
- Dirt and scratches texture authored for test; overlay illumination response from three different source tints.
- Occlusion scene: opaque boxes, Masked fence/foliage, glass surface, sloped silhouette, sun/sky, fine geometry.
- Camera suite: perspective, orthographic (if supported), Cine Camera asymmetric projection, 16:9, 21:9, vertical, splitscreen, TAA/TSR, variable exposure.

Image regression comparisons must tolerate small GPU-vendor numerical differences; use feature-specific geometric/photometric metrics instead of claiming byte-for-byte cross-GPU determinism.

## Native UE automation tests — `Automation RunTests Glint`

| Prefix | Coverage |
|---|---|
| `Glint.Data.*` | Asset, element types, grouping, serializer, presets, texture references, stable GUIDs |
| `Glint.Math.*` | Source projection, axis distance, lens center, aspect, rotation, offscreen extent |
| `Glint.Behavior.*` | Driver mapping, curve response, determinism, source isolation, cut reset |
| `Glint.Render.*` | Render data composition, bounds, alpha, HDR output, thread safety, shader resources |
| `Glint.Occlusion.*` | Visibility/bias, area sampling, directionals, per-view state and smoothing |
| `Glint.Runtime.*` | Actor and Actor-free sources, Blueprint API, world and handle lifetime |
| `Glint.Editor.*` | Asset definition, custom editor launch, transactions, presets, preview state |
| `Glint.Perf.*` | Benchmark fixture sanity, no-source bypass, caps and tier switching |

Native test cases should be deterministic when not explicitly visual. A manual/packaged checklist accompanies automation; editor tests alone do not prove shader cooking.

## Document and design signoff gates

**Gate A: Vision fidelity.** The 12-family palette, crosswalk to Optical Flares, quickstart, default examples, controls and dynamic triggers are understandable to an experienced Optical Flares artist.

**Gate B: Rendering feasibility.** Shader cooks and scene hook work on target UE versions and packaged game; per-view GPU occlusion tested.

**Gate C: Behavior stability.** Camera movement, source movement, time evolution, seed determinism, offscreen crossing and cuts have no major visible errors.

**Gate D: Tool quality.** Slate editor is navigable, scalable, native; transactions and Content Browser assets work; can save a reusable flare and individual element.

**Gate E: Performance.** Published baseline machine, pass timings and profile limits measured in a real level; quality tiers/LOD can preserve intended hero look when reducing cost.

**Gate F: Release.** `Automation RunTests Glint` passes; no critical crashes in packaging/PIE/save; example scenes and documentation are shipped with full plugin source.

## Decisions explicitly not yet made

- Exact renderer API callback and compositing stage.
- Whether lens dirt uses half/quarter-resolution render target or a compute pass.
- GPU kernel/permutation layout per radial family.
- Precise Soft Screen math under HDR.
- Precomputed atlas vs analytic solution for high-density Shimmer/Spike Ball.
- Normalized view/filmback mapping of lens shift under all camera classes.
- Whether to support forward renderer / stereo for the initial release after benchmark and renderer tests.

**Most valuable next implementation batch:** **R0A–R0D** from [28](28-Automatic-Mode-Testing-and-Release-Gates.md): HDR bright-mask in packaged view, automatic field ghost/streak with **zero authored sources**, GPU top-K bright-region candidates driving Glint elements, followed by authored-source occlusion and dedup. Then integrate that proven renderer into the actual native Asset Editor in S6.

## Automatic camera mode: non-negotiable new feasibility gates

The earlier S0–S2 source-driven renderer checks are necessary but **not sufficient**. Add R0A–R0D: GPU HDR image extraction with exposure-correct threshold, image-space ghosts and anamorphic streaks from unassigned emissive/specular pixels, bounded GPU top-K source candidates used to render source-relative Glint elements, and same-renderer integration with manual sources. The first automatic proof must be in a **cooked Win64 executable**, not editor-only screenshots. The native test families and benchmark matrix are in [28](28-Automatic-Mode-Testing-and-Release-Gates.md).

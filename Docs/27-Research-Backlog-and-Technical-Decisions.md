# 27 — Additional Research Backlog: Priorities, Unknowns and Decisions

[← Index](README.md) · [16 — Earlier spikes](16-Technical-Spikes-and-Validation.md) · [28 — New release gates](28-Automatic-Mode-Testing-and-Release-Gates.md)

> **Status categories:** `Documented precedent`, `Architectural proposal`, `Needs UE5.8 spike`, `Deferred/conditional`. Research is not an engine implementation or performance measurement.

## Ranked research questions

| Priority | Research area | Why it matters | Next experiment | Status |
|---|---|---|---|---|
| **P0** | HDR input access, RDG pass location | Automatic flare detection requires the right image in the right color space | Minimal 5.8 plugin renders threshold mask from HDR and composites safely in packaged build | Needs UE5.8 spike |
| **P0** | GPU highlight extraction, top-K source list | Hundreds of highlights must not create thousands of iris ghosts | Compare field-only, tile peaks, peak merging and GPU top-K | Needs spike |
| **P0** | Pre-exposure and auto exposure | Unstable detection when entering light/dark areas is a product-breaking failure | Manual EV sweep, auto-exposure scene switch | Documented engine behavior; needs spike |
| **P0** | Temporal tracking and camera cuts | Highlights can rapidly blink/morph, creating severe artifacts | Specular highlight moving at 30/60/120 fps, camera cut | Needs spike |
| **P0** | Performance cost isolation | Full-screen sample count, overdraw and buffer traffic can dominate | 1080p/1440p/4K stage-by-stage GPU profiling | Needs spike |
| **P0** | Glint Lens vs Glint Flare schema and defaults | Automatic and manual workflows must remain legible | Native asset prototype with project default + camera override | Proposed |
| **P1** | Field/complex candidate balance | Avoid duplicating flares and overwhelming highlights | Field-only/candidate-only/both comparisons against reference photos | Needs spike |
| **P1** | Extended vs point highlight geometry | Neon bars and bright windows should not spawn dozens of ghosts | Area/covariance classification on corridor, strips and sky | Proposed |
| **P1** | UE bloom and UE lens flare coexistence | Duplicated native flares and bloom coupling can produce blown-out effects | Toggle native postprocessing; compare Glint-owned bright-pass and bloom reuse | Needs spike |
| **P1** | Translucency, fog/clouds, particles | Real games have VFX and transparent blockers | GPU source field before/after translucency and fog; no false claims | Needs spike |
| **P1** | Optical profile response fields | Ghosts should morph with off-axis source and focal length, not just slide | Analytic 1D curves vs compact 2D UV LUT | Proposed |
| **P1** | Lens dirt/schmutz illumination | Per-view camera dirt should be driven by many highlights cheaply | One shared dirt texture + summed source field | Proposed; research precedent |
| **P1** | Cinematic camera consistency | Lens should track focal length, filmback, aperture blades and anamorphic squeeze | CineCamera source movement and zoom matrix | Documented engine data; needs spike |
| **P1** | Content Browser and native editor | Familiar Optical Flares workflow, clean UE UX | Glint Lens + Flare asset type, editor UX usability test | Proposed |
| **P1** | Suppression/exclusion control | Developers may not want flares in menus, emissive UI screens, water, sky | Camera region mask, material/gameplay tags alternative, no stencil assumption | Research |
| **P2** | Offscreen sun and lens barrel response | Image detection cannot see what is offscreen | Authored sun source with constrained edge fade | Proposed |
| **P2** | Multi-camera / scene capture / MRQ | Per-view history and output colors differ | Split-screen, CineCapture, MRQ temporal samples | Validation target |
| **P2** | PSF, diffraction and photographic matching | Gives recognizable photographic personality to lenses | Offline original aperture response lookup; compare analytic shapes | Research |
| **P2** | Hardware platform matrix | RDG/compute support differs by RHI | DX12/SM6 first, other RHIs only when validated | Deferred until desktop pass |
| **P2** | Dynamic resolution and high DPI | Candidate UVs and streak blur may scale wrongly | Resolution and viewport aspect sweeps | Needs spike |
| **P2** | Accessibility and reduced flashing | Strong glints can obscure gameplay or cause discomfort | Reduced Flash preset, luminance clamp and gameplay reviews | Product proposal |
| **P3** | Physically traced lens stack | Could provide superior ghost movement but very costly | Editor-only precompute and baked profile vs runtime trace | Deferred |
| **P3** | Exact commercial preset import | Proprietary formats, licensing and implementation risks | No import for v1; author original looks | Excluded |

## Significant research-based corrections to the earlier design

1. **Automatic is not an optional future feature.** Built-in UE already demonstrates image-based lens flares; Unity HDRP/URP, Video Copilot's luminance tracking and Flares OFX demonstrate variants of automatic highlight-driven flare authoring. What Glint adds is deep **artist-authored lens behavior** in a native UE asset with source-bounded GPU execution.
2. **A 'Glint Flare' composition alone is insufficient** because it doesn't define how the entire camera detects and responds to highlights. Create an explicit Glint Lens Profile.
3. **The no-source benchmark needs redefinition.** Even with zero authored sources, automatic mode still scans a reduced HDR input. Benchmark `Glint off`, `auto on no bright pixels`, `auto field only`, `auto candidates`, and `auto plus authored` independently.
4. **A camera sees visual brightness, not every Light Actor.** Explicit light detection is not a replacement for screen-derived HDR highlights.
5. **The original 0.6 ms/8-source budget doesn't describe a whole auto pipeline.** Set separate measured GPU envelopes for detection, field, candidates, dirt and compositing, then agree total budget on reference hardware.
6. **Automatic input and authored input need different occlusion rules.** Auto sources are screen-visible by construction; world point sources require depth. Neither should get duplicated at composition time.

## Recommended decision log additions

| ID | Decision | Status |
|---|---|---|
| D21 | Automatic camera-driven image response is a must-have v1.0 mode | Adopt |
| D22 | Dedicated `Glint Lens` asset distinct from source-oriented `Glint Flare` | Adopt design; API validation pending |
| D23 | Automatic field effects plus bounded highlight candidate pathway share a renderer | Prototype/Gate |
| D24 | Authored and automatic source inputs coexist with visible dedup policy | Adopt design; GPU cost pending |
| D25 | HDR input before Glint/tonemap with explicit pre-exposure handling | Required principle; integration spike |
| D26 | No synchronous CPU readback or per-highlight Actor spawning | Required |
| D27 | Highlight debug display is part of v1, not optional polish | Adopt |
| D28 | No promises of all HDR/forward/stereo/capture pipelines before testing | Required |
| D29 | Reduced-Flashes intensity/accessibility settings | Proposed v1 acceptance |

## Research literature and direct competitive precedents

- [UE5.8 Post Process Effects](https://dev.epicgames.com/documentation/unreal-engine/post-process-effects-in-unreal-engine) — built-in image-based flare.
- [UE5.8 Auto Exposure](https://dev.epicgames.com/documentation/en-us/unreal-engine/auto-exposure-in-unreal-engine) — pre-exposure and scene brightness.
- [UE5.8 Scene View Extension callback](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/ISceneViewExtension/SubscribeToPostProcessingPass) — renderer extension point.
- [Unity HDRP Screen Space Lens Flare](https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4017.0/manual/shared/lens-flare/Override-Screen-Space-Lens-Flare.html) — luminance field, warp/reverse/streak.
- [Video Copilot Optical Flares](https://www.videocopilot.net/products/opticalflares/) — automatic luminance tracking; configurable resolution/threshold.
- [Flares OFX pixel-driven positioning](https://palmpixel.eu/flaresofx-documentation/openfx-integration/plugin-features/pixel-driven-flares/) — region ranking/top-K and detailed vs starburst source tiers.
- [REAL FLARES Fab listing](https://www.fab.com/listings/7d4eddfd-4be4-4d6b-a9d2-07aa3c11d30c) — explicitly source-component-centric advertised workflow; no verified automatic HDR candidate equivalent in listing.
- [Maxon Real Lens Flares](https://www.maxon.net/en/product-detail/red-giant/vfx/real-lens-flares) — optical modeling and dynamic morph/zoom.
- [Lee & Eisemann (2013)](https://graphics.tudelft.nl/Publications-new/2013/LE13/PracticalReal-TimeLens.pdf) — fast approximated lens ghosts.
- [Tile-based lens ghost rendering (2023)](https://www.sciencedirect.com/science/article/pii/S0097849323001486) — overdraw optimization research.

## New researched implementation risks (November 12, 2026)

- **P0 / resolved at research level:** a before-bloom *postprocess material* location exists in UE5.8; an identically named SVE postprocess callback is **not** documented. The actual RDG injection remains unverified. See [30](30-UE58-Postprocess-Integration-Research.md).
- **P0 / API correctness:** UE5.8 documents the old three-argument SVE postprocess subscribe overload as deprecated/no longer called; use the view-aware overload.
- **P1 / camera integration:** `IBlendableInterface` and `AddOrUpdateBlendable` suggest Glint lens overrides can participate in standard postprocess volumes; confirm per-view weight and renderer visibility before promising seamless volume blending.
- **P1 / optical profile blending:** never interpolate unrelated flare element arrays. Blend compatible parameters; select a dominant flare composition or explicitly crossfade complete results under a measured budget.
- **P1 / TSR:** postprocess placement can change whether effects operate at render resolution or display resolution; test coordinate mapping across scaling and aspect ratios.

See the detailed [integration findings](30-UE58-Postprocess-Integration-Research.md) and [acceptance plan](31-Camera-Workflow-and-Renderer-Acceptance-Plan.md).

## Cross-engine research resolutions and new open questions

- **Resolved:** Source-driven and automatic screen-space flares should coexist; Unity documents both. Glint's renderer remains shared with explicit dedup. [34](34-Other-Engines-Real-Time-Lens-Flares.md).
- **Resolved:** Generic repeated elements are useful and familiar; make authoring modifier bounded with deterministic seed and safe nested reuse.
- **Needs UE5.8 proof:** Access to existing HDR/bloom pyramid at an appropriate plugin extension stage, including when stock Bloom is disabled. Never make reuse required.
- **Needs GPU proof:** Peak/energy preservation for tiny highlights, region grouping/NMS tradeoffs for broad lights and two closely spaced headlights.
- **Needs visual proof:** Sample-based partial-source visibility response for radial shafts and ghosts versus one uniform occlusion factor, especially with temporal jitter.
- **Research only:** First-order physical optics as editor-generated positions; tile-binned rasterization; polynomial lens reconstruction as premium/post-v1 technique.

See [36](36-Rendering-Papers-and-Architectural-Lessons.md), [37](37-Cross-Engine-Findings-and-Prototype-Gates.md).

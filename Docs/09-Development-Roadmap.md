# 09 — Development Roadmap

[← Documentation index](README.md)

## Execution strategy

Progress in **meaningful, testable batches**. The renderer and editor must converge around a shared asset representation; neither should become a throwaway prototype. Do not spend a long sequence of iterations polishing the element list before confirming the GPU rendering path.

| Phase | Focus | Demonstrable deliverable | Gate to progress |
|---|---|---|---|
| **R0 — Feasibility** | UE 5.8 HDR renderer hook, automatic extraction, field ghosts/streaks, candidate/author integration, packaged build | **R0A–D**: HDR mask → no-source automatic flare → bounded candidate Glint elements → authored source/dedup | No Engine modifications; camera auto actually works and GPU costs are measured |
| **R1 — Asset & Runtime Foundation** | Dedicated Glint Lens + Glint Flare assets, global/camera override schema, subsystems, components and transient API | Project lens generates automatic flares; explicit Flare asset can be authored and reused | Native tests, correct lifetime/undo/resource handling |
| **R2 — Core Composer** | Asset editor: stack, Details, live preview, transactions, guides | Artist can author multi-element flare and save/reopen | Preview/runtime visual parity, usability checks |
| **R3 — Element Library** | the 12 familiar visual families + Texture Sprite through a small number of GPU primitive kernels | Complex authored flare, controls and original presets | Stable rendering, no major performance regression |
| **R4 — Motion & Occlusion Quality** | Soft GPU depth, temporal smoothing, edge triggers, shimmer, per-camera semantics | Dynamic scene demonstration | No camera-cut flashes or noticeable occlusion jitter |
| **R5 — Optimization & Scalability** | Batching, LOD, source priority, buffer reuse, diagnostics | Benchmark map, quality tiers and live stats | Release-grade measurements, budget adjustments documented |
| **R6 — v1.0 Hardening** | Validation matrix, cooked builds, docs, sample levels, workflow polish | Complete portable plugin and test assets | v1.0 scope and release gates passed |

## R0 — Most important technical spike

Set up the minimum real plugin, not pseudo-shader code. Validate `FSceneViewExtensionBase` 5.8 callback lifetime and view-aware API. Confirm the depth and HDR pass resources exposed to the plugin. Prove actual packaged Win64 rendering with a projected point and animated streak. Benchmark under TSR, bloom and camera motion.

**If HDR integration requires changing Engine files**, do not quietly adopt that as product architecture. Explore a valid unmodified-engine callback/compositing alternative, explicitly document differences, and make the tradeoff decision *before* proceeding to R2.

## R1–R2 contract

The editor operates on the same `UGlintAsset` composition records that runtime serialization and render compilation consume. Design schema and transaction IDs in R1, so R2 does not need to rewrite source data. Runtime scene composition should be decoupled from having the editor tab open.

## R3–R4 fidelity

Implement element types in small **families of compatible rendering primitives** to avoid excessive shader specialization. Shape, color and placement semantics must be consistent across types. For photo-style lens artifacts, allow user-supplied textures while maintaining deterministic, inexpensive procedural defaults.

## R5 profiling gates

Use a sample real scene, not a synthetic zero-fill benchmark alone. Capture GPU timings for element rendering, occlusion and composite separately. Make perf tiers visibly meaningful without destroying the identity of the authored look. Publish measured results by platform, renderer and test configuration.

## R6 release handoff

- Complete plugin folder with editable source, shaders, Resources, Content and Docs.
- UTF-8 text descriptors and source, no external runtime assets or engine patches.
- README onboarding and editor usage guide, screenshot examples and API docs.
- `Automation RunTests Glint`, validated packaging, actual performance results.
- Known supported and unsupported renderer cases; remaining limitations and troubleshooting.

## First implementation batch recommendation

The first substantive development pass should include **R0A–D**: plugin HDR hook and pre-exposure-correct threshold preview, **automatic image-space ghost/streak from an emissive source without Glint Actors**, bounded GPU candidate→Glint Glow/Iris, registered authored world source with depth occlusion and dedup, native math/GPU sanity tests and a packaged smoke. Avoid starting with 12 types of elements or complex preset UI until this spike works.

## Extended research-informed sequence

- **Before R0:** Confirm artifact names and artist-facing controls against [Optical Flares audit](11-Optical-Flares-Reference-Audit.md). No vendor runtime/code/presets required.
- **R0:** Complete the seven feasibility proofs in [technical spikes](16-Technical-Spikes-and-Validation.md), prioritizing HDR/postprocess stage, depth access and TAA/TSR.
- **R1:** Freeze the **shared element schema** in [12](12-Glint-Element-Parameter-Schema.md), with tested source/center placement and seeded behavior definitions.
- **R2:** Implement full native [Glint editor UX workflow](15-Optical-Flares-to-Glint-Workflow.md): stack, live source drag, Global row, element types, triggering UI and preset browser shell.
- **R3:** Batch similar rendering families: radial Glow/Iris/Ring/Hoop, rays Streak/Glint/Shimmer/Spike Ball/Sparkle, generated Multi Iris, advanced Caustic, Texture Sprite and shared Dirt; ensure all 12 authoring types have meaningful defaults.
- **R4:** Complete [behavior system](13-Behavior-and-Trigger-System.md), preview heatmap, occlusion/edge motion and per-camera deterministic time.
- **R5–R6:** Validate benchmark data and all artist-familiarity, native UI and package gates; ship original element and full-flare preset examples.

## Updated optical research gates (November 2026)

**R0:** actual automatic HDR flare generation from emissive/specular pixels and top-K candidate extraction **as well as** authored source motion, scene-linear HDR and depth occlusion must be validated in packaged UE 5.8.

**R1:** define a **first-class Glint Lens asset**, project default/camera override, automatic classification policy and shared linked/local optic overrides. Do not design a runtime physical ray tracer.

**R2:** optionally dock a Lens Profile panel and preview focal zoom/source motion alongside the normal element stack.

**R3–R4:** prototype coherent linked iris/starburst changes and camera zoom response; promote to v1 only with measured cost/quality.

**R5–R6:** use the validation matrix in [20](20-Hybrid-Lens-Design-and-Validation.md), including translucent/fog research as separately gated future features.

## Revised execution order

The original source-first spike is insufficient for the new camera-first v1.0. The **first proof must show automatic flares from pixels with no Glint source Actors**. Follow the revised R0A–D gates in [28](28-Automatic-Mode-Testing-and-Release-Gates.md), and add camera lens authoring in R1/R2. Preserve all useful authored-source work but do not postpone the automatic pipeline until after a mature artist editor exists.

## Before beginning R0: updated UE5.8 API decision

The [postprocess integration audit](30-UE58-Postprocess-Integration-Research.md) identifies a distinction between `BL_SceneColorBeforeBloom` (material blendable location) and SVE `EPostProcessingPass` (no explicitly named `BeforeBloom` callback). R0 must test a real 5.8 HDR integration; do not assume API equivalence. The current view-aware four-parameter `SubscribeToPostProcessingPass` overload is required. The first batch should include [camera ownership/acceptance checks](31-Camera-Workflow-and-Renderer-Acceptance-Plan.md) before freezing asset serialization.

## Optimization architecture must start at R0, not R5

The original R5 focus on optimization remains the **profiling and tuning milestone**, but the foundations are now required **before** R5.

- **R0:** choose HDR reduction, mixed-resolution render outputs and GPU-only candidate buffers. Implement stage profiler markers and compare full-res vs half/quarter soft response. Prove zero-source and disabled work is bounded.
- **R1:** source/element data includes quality priority and possible compiled response references. Design shared per-camera buffers and renderer family grouping, not per-source materials.
- **R2–R3:** preview quality independent of runtime quality; preserve the same authored meaning across compiled/baked renderer representations. Expose basic quality controls without turning every element into a performance engineering form.
- **R4:** establish candidate/LOD hysteresis, history reset, stable occlusion and crossfade for motion.
- **R5:** compare optional LUTs, tile-bin compositing, advanced sampling, async compute and any dynamic budget controller **against** the measured simple baseline; do not adopt speculative complexity by default.
- **R6:** independent `Glint.Perf.*` tests, QA scenes and actual Win64 packaged GPU metrics from [33](33-Performance-Experiments-and-Gates.md).

## New cross-engine validation work (before performance hardening)

- **R0**: high-intensity tiny-pixel reduction, broad-region suppression and neighboring highlight separation; optional reuse of bloom mips must be proven with a fallback; baseline visibility policy and packaged profiler.
- **Editor R1/R2**: generic Repeater authored modifier, safe nesting/cycle prevention, radial edge deformation, Lens Context presets and stock flare coexistence diagnostics.
- **Rendering R2/R3**: optional sample-based source shape occlusion response and first-order optics lookup bake comparison. Implement only where measured cost/quality justifies it; uniform visibility remains reliable baseline.
- **Optimization**: test intermediate format/bandwidth and full composite path; tile-based accumulation deferred unless overlapping ghost profiles prove it saves total GPU ms.

Detailed experiment acceptance in [37](37-Cross-Engine-Findings-and-Prototype-Gates.md).

## Froyok and recent-paper validation gates (2026-11-12)

See [38–44](README.md#latest-research-addendum-froyok-and-uploaded-papers). Front-load the **stable HDR bright-pass/filtering experiment** with subpixel moving highlights, camera cut and screen-percentage changes. Froyok's UE4.25 source patch is a rendering reference, **not** Glint integration code. A no-engine-source Win64 packaged R0 proof remains mandatory.

Prepare (but do not yet freeze) a small response-bake interface so measured lens behavior (Walch 2017) could eventually be evaluated as a compact GPU response alongside regular authored Glint elements. The 2026 Meta representation is valuable for captured lens fidelity but **its 3.92 ms per-light deformation measurement** argues against any runtime network dependency in standard gameplay. Tile rendering is to be evaluated only when baseline ghost overdraw is measured; the 2023 paper's 4.04–12.79 ms examples are not release performance targets.

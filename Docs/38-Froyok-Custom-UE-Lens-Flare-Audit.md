# 38 — Froyok: Custom Lens-Flare Post-Process in Unreal Engine

[← Index](README.md) · [30 — UE5.8 integration](30-UE58-Postprocess-Integration-Research.md) · [44 — Prototype gates](44-New-Research-Prototype-Gates.md)

**Primary source:** Léna Piquet (Froyok), *Custom Lens-Flare Post-Process in Unreal Engine*, September 15, 2021: https://www.froyok.fr/blog/2021-09-ue4-custom-lens-flare/

**Status:** Verified against the public article; examples are **UE4.25 with engine-source changes**, not tested UE5.8 plugin code. Descriptions of Cyberpunk 2077 and Batman: Arkham Knight are the author's RenderDoc-based interpretations, not statements by their developers.

## Why the implementation matters to Glint

It demonstrates a detailed, image-driven flare system working in Unreal's RDG rendering environment. A source does not need an Actor. The author decomposes the pipeline into (1) HDR threshold/downsample, (2) ghost/chromatic/halo, (3) procedural glare, (4) bloom/flare compositing. This is a direct reference for **Glint Automatic Field**, separate from bounded **Automatic Candidates** and **authored world sources**.

## Pass architecture (as described, not copied)

| Stage | Technique | Glint interpretation |
|---|---|---|
| Source | HDR scene color, independently thresholded rather than solely relying on bloom | Keep detection independent from Unreal's built-in bloom. Quantify pre-exposure and tone mapping. |
| Downsample | Filtered downsample and additional blur on HDR threshold | Introduce stable highlight-reduction experiment first; preserve tiny hot pixels. |
| Ghosts | Repeated, scaled, shifted samples of bright image with chromatic offsets | Use a bounded image-response kernel for ubiquitous light details; do not instantiate complete authored flares per pixel. |
| Halo | Distorted/radial samples, inspired by John Chapman's method | Offer per-lens global shape/falloff instead of only a fixed halo. |
| Glare | GPU geometry-driven radial streak elements, colored with small mask textures | Compare instanced, procedural and textured rays for cost/quality. Avoid relying on legacy geometry-shader route without measurement. |
| Composite | Mix field response with bloom and scene | Prove exact 5.8 render location; define HDR-before/after-bloom expectations. |

## The technical pitfall: bright-pass temporal aliasing

Froyok found a soft threshold alone insufficient: a very bright point sampled after aggressive downsampling changed abruptly as its location moved between pixels. All subsequent ghost and streak stages amplified the error. Weighted downsampling and an additional blur reduced it. This is **not merely an aesthetic problem**; it can create visible flicker and source ranking instability in Glint.

Glint's minimum R0 test should compare: naive box/downsample; weighted HDR reduction; high-peak-preserving reduction; optional anti-firefly clamping; explicit temporal smoothing after source tracking. Measure both source spatial jitter and final optical flicker under fast camera pan, especially with moving tiny emissive objects. Blur has a fidelity tradeoff: preserving energy is not the same as preserving sharp highlight location.

## Benchmarks: contextual only

- UE4 original lens flare approximately **0.376 ms** at 1080p.
- Froyok's richer custom effect approximately **0.65 ms** at 1080p.
- Test GPU: **AMD RX 5600 XT, Vulkan on Linux**. These are historical measurements, not valid Glint targets without replication.
- Author identified opportunity to reuse engine downsample/bloom buffers, merge passes and explore compute shaders. Treat these as hypotheses, not proven improvements.

## Do not use this implementation as a direct UE5.8 plugin template

Froyok modified private engine-renderer code, changed postprocess data structures and introduced a custom delegate. It uses engine-private headers and includes, and explicitly cautions that its delegate/subsystem registration might not be thread-safe as written. It also relied on `FGlobalShader` definitions and stage-specific `FScreenPassTexture` handling from 4.25. Glint's retail/Fab requirement is **no engine fork**. Reimplement concepts using documented 5.8 interfaces and test Win64 package + view lifetimes.

UE5.8 publicly documents the current **per-view four-argument** `ISceneViewExtension::SubscribeToPostProcessingPass` and states that the old three-argument overload is not called. UE5.8 post-process materials support `BL_SceneColorBeforeBloom` but SVE passes are not a 1:1 equivalent. Explicitly prove if and when Glint can sample a valid HDR buffer and produce the intended result. See [30](30-UE58-Postprocess-Integration-Research.md).

## Other good ideas from Froyok's analysis

- Native post effects are not necessarily the only cheap solution: an image-space field can build camera-wide effects at approximately bounded cost.
- He interpreted **Cyberpunk 2077** as using roughly half-resolution bloom-derived ghosts/halo in one pass, and **Batman: Arkham Knight** as mixing low-res ghosts with separate glare geometry. These are informed observations from captures, not verified engine implementations.
- The handling of **viewport subrectangles** matters: custom editor viewports and viewport resizing may have texture extents larger than their active view rects. Avoid extra full-size copies if correct UV/view-rect math suffices; measure both paths.
- Built-in shutter, chromatic, native UE bloom and Glint need a deliberate order. Unspecified compositing risks duplicated effects or toned-down flare color.

## Decision

**Adopt conceptual algorithms, not engine modifications or unreviewed code.** An R0 proof must deliver an automatic HDR streak/ghost with stable panning and packaged runtime before higher-end optical primitives become a dependency.

# 14 — Optics, Real-Time Rendering and Fidelity Research

[← Documentation index](README.md) · [06 — Renderer and Occlusion](06-Renderer-and-Occlusion.md) · [16 — Technical spikes](16-Technical-Spikes-and-Validation.md)

> This document separates **optical visual phenomena**, **editor interpretation**, and **verified Unreal 5.8 rendering capabilities**. A physically accurate lens simulator is *not* a v1.0 promise; physically informed compositional rules should nevertheless inform sane defaults and movement.

## 1. The two visual mechanisms

### A. Scattering / diffraction around light sources

Usually visible near the bright source: Glow (glare), fine Shimmer/Glint/Starburst rays, and Streaks. The Flare7K++ and 2025 physically-based flare research categorize these as distinct sources of appearance: glow from scattering/defocus, streaks from directional optical defects, and radial structures from aperture diffraction/impurities. They are visually correlated but do not need to be simulated with full spectral wave optics for game production.

**Glint:** Start with one strong light-anchored Glow; add subtle high-frequency glint/shimmer; add horizontal anamorphic streak only when the authored lens style warrants it. Avoid combining every element at full brightness by default.

### B. Internal reflections and lens surface artifacts

A sequence of reflections between glass elements produces *ghosts* that generally track a line connecting the bright source and optical center. Their positions change at different apparent rates as the source moves. Aperture blade count can produce polygonal bokeh/ghosts; chromatic displacement can give colored rims. A reflection grouping is materially different from an additive glow glued to the light source.

**Glint:** `Axis Distance` is a foundational property; it must be visualized in the editor. A source at left can cause ghosts at center and right automatically. Allow optical center offset and independent axis rotation/scale dynamics to achieve stylized and camera-like looks.

### C. Lens dirt / contamination

Dust, fingerprints and microscopic defects often remain fixed in lens/image-space and only become visible when strong light illuminates the contaminated area. A realistic dirt overlay is a **lighting response** on a stable mask, not a dirt-textured ghost that travels across the scene with each light.

**Glint:** Render dirt once per view with a bounded source-illumination accumulation mask. One camera can see overlapping source illumination without N full-screen dirt passes. Make expensive texture/channel requirements explicit in authoring and profiling.

## 2. Physical relation presets (artist-friendly, not mandatory simulation)

Rather than asking artists for exact lens manufacturing stacks, offer several **starting presets**:

| Look | Optical behavior | Suggested element stack |
|---|---|---|
| **Photographic Prime** | Restrained ghosts, subtle aperture influence, nonuniform spectral edges | Glow + discrete 5–7 blade Irises + delicate Ring + subtle Dirt |
| **Anamorphic Cinema** | Broad horizontal streak, oval ghosts, soft horizontal bloom | Glow + Streak + stretched Multi Iris + thin Glint |
| **Vintage Imperfect** | Asymmetric reflections, irregular caustics, strong color separation | Soft Glow + Hoop + Caustic + sparse Textured Iris + Dirt |
| **Sci-Fi Energy** | Stylized dense streak/spikes, greater saturated response | Bright Glow + multiple Glints + Spike Ball + animated Shimmer |
| **Night Street Lights** | Many sources, strong visible point cores, controlled cost | Small Glow + compact Glint + limited Iris group |
| **Sun / Outdoor** | Large-area source, stable view direction, natural occlusion edges | Sun Glow + small Ring/Hoop + optional Ghosts; directional source semantics |

These are *original presets and textures* created for Glint, not imported or replicated commercial presets. Provide one-button **Realistic / Stylized** starting preferences without hard-locking any parameters.

## 3. Unreal 5.8: compositor integration decisions

Epic documents **Before Tonemapping** as HDR scene-color and **After Tonemapping** as more economical LDR. Its Scene View Extension API exposes post-processing callbacks, but the exact set of accessible buffers and ordering for our plugin in 5.8.0–5.8.3 remains to be demonstrated in a source-level spike. [Epic 5.8 Post Process Materials; ISceneViewExtension; EPostProcessingPass]

### Candidate A: Pre-tonemap HDR

**Benefits:** Scene-linear radiance can contribute naturally to exposure/tone-map; color rolloff more plausible; glare can be evaluated in HDR. **Costs:** More bandwidth, uncertainty about TAA/TSR and available callbacks, more complex composite and dirt/Screen math.

### Candidate B: Post-tonemap LDR

**Benefits:** Lower bandwidth; sharp crisp VFX can be stable after temporal reconstruction. **Costs:** Worse physical relationship with exposure, bright highlights and HDR output; appearance may diverge when game exposure or grading changes.

### Recommended R0 proof

Prototype **both** with the *same* authored source and compare effects through exposure changes, TAA/TSR, sharp 1px glints, bloom, HDR output and performance. Select the pipeline that preserves the strongest combination of correct visual behavior and stable packaged runtime. Do not commit renderer design solely from a convenient callback name.

## 4. Screen and additive blending: the HDR trap

Classical Screen blending operates on normalized channels: `Screen(A,B) = 1 - (1-A)*(1-B)`. For LDR 0..1 that visually becomes lighter without plain additive clipping. In unconstrained scene-linear HDR, values can exceed one; simple Screen math can stop being monotonic or produce impossible sign/brightness changes. A literature/reconstruction analysis discusses a technique for emulating Screen from restricted GPU blending, but it is not sufficient proof for modern UE HDR behavior.

**Glint decision:**

- **Required:** robust Additive in linear space; ordinary Alpha/Soft Add with documented premultiplication and destination behavior.
- **Required UX:** present a familiar **Screen-like** mode but mark its output as bounded/tonemap-dependent and demonstrate parity expectations through image tests. Do not call it mathematically identical to AE Screen in HDR without measured comparisons.
- **Gated:** If proper per-object Screen requires excessive readback/intermediate passes, composite compatible objects together or offer an approximation with a **clear label and stable behavior**.
- **Stack ordering:** Screen-like, alpha and lens-dirt interactions require ordered composition; only additive-only groups are freely reorderable at render time.

## 5. Occlusion: do the correct comparison at the source

A point source is visible if its projected depth is **in front of** the opaque scene depth at the relevant location (with configurable depth bias). A soft/broad emitter should sample around a projected source disk and build a continuous visibility fraction. Scene depth generally does not include all translucent surfaces; an opacity threshold on a translucent waterfall cannot be assumed from an opaque depth buffer.

### Proposed modes

- **None:** Artistic effect always visible when within projection/edge bounds.
- **Depth Point:** Cheap hard-ish compare for small sources; spatial multi-tap recommended to avoid thin-geometry popping.
- **Depth Area:** Sample disk/cone with soft edge / sample count quality; smooth 0..1 visibility.
- **Custom Depth:** Optional advanced control for explicitly participating occluders; precise setup docs required.
- **Sun/Directional:** Source direction projected at infinity, geometry/depth behavior distinct from a point at far distance. Sky pixels may not have finite geometry depth.
- **Translucent occlusion:** v1 must clearly document limitation and possible art-directable custom-depth/visibility APIs rather than pretending arbitrary transparency is solved.

Never use a blocking per-source GPU-to-CPU query on the critical render path. GPU depth sampling/HZB is relevant, but UE's general Actor HZB culling is **not itself** a usable flare-visibility API. R0 must verify depth surfaces, projection and synchronization. Histories should be per-view, with cut/teleport resets. [Epic 5.8 Visibility and Occlusion Culling]

## 6. Stability and source projection

| Problem | Why it happens | Glint mitigation/research |
|---|---|---|
| **TAA/TSR ghost trails** | High-contrast thin rays fight temporal filtering | Compare pre/post temporal stage; analytic AA and subpixel coverage; avoid uncontrolled temporally unstable noise |
| **Dynamic resolution changes flare size** | Naive quad sizes scale with scene buffer | Use view output/reference coordinates; correct source projection for variable internal resolution |
| **Exposure pumps brightness** | HDR scene adaptation modifies tone mapping | Choose artist-friendly Exposure Response: Scene Relative / Compensated / Fixed Display (verify pipeline) |
| **Lens center shifts with Cine Camera** | Nonzero lens shift/crop and constrained aspect | Source/center from active view projection, not hard-coded screen midpoint |
| **Offscreen ghost pop** | Culling based only on source on-screen status | Compute conservative element bounds and offscreen margin; ghost still valid when source slightly outside view |
| **Occlusion pops through thin objects** | Single depth sample, jitter, moving silhouettes | Tunable area taps, bias, spatial and temporal fade with stable cut behavior |
| **One light, many cameras** | Per-source single visibility state reused incorrectly | Source registry world-wide; projection and visibility **per view** |
| **Camera roll and rotated optics** | Anamorphic streak orientation can be screen-locked or camera-relative | Explicit rotation modes; no hidden coupling to world roll |
| **Geometry silhouettes and sun** | Depth does not represent sky as finite surface | Treat directional sunlight separately; verify horizon/atmosphere behavior |
| **Very bright dirt and glints** | Large/overlapping translucent regions cause overdraw | Quarter/half-res pass where suitable; bounded element regions; one dirt composite |
| **Eye adaptation in editor preview** | Black isolated preview differs from authored game scene | Preview controls for exposure/background plus **real level camera** comparison |

## 7. Other tools worth comparing

- **Unreal's native lens flare:** image-based, driven by threshold and image brightness, with BokehSize, BokehShape and Tints. Works without authored sources, but not a direct substitute for this procedural element stack. Users need clear instructions to avoid accidental double flares.
- **Unity HDRP Lens Flare:** useful independent real-time reference for data-authored ghost texture shapes and camera-driven behavior. Review conceptual UX, not assets.
- **CryEngine Optical Flare System:** useful practical precedent for multi-ghost scattering, chromatic rings, source relationships and screen-space costs.
- **Autodesk VRED:** separated Glow, Starburst, Streak, Ring and Ghost Line creation; corroborates our element-first architecture.
- **Boris/Ultraflares/Knoll:** comparative examples for terminology, directional gradients and optical realism; never infer that their controls exist in Video Copilot's exact product.

## 8. Research backlog ranked by risk

| Priority | Unknown | Required validation / consequence |
|---|---|---|
| **P0** | UE5.8 plugin-only HDR render callback integration | R0 shader + packaged build on 5.8.0–5.8.3; if unavailable revise architecture before editor |
| **P0** | Scene depth sampling + directional visibility | Verify depth conventions, area taps, masked geometry and sky cases in actual engine |
| **P0** | TAA/TSR, exposure and bloom ordering | Same authored flare screenshots + GPU captures across candidate pipeline stages |
| **P0** | Frame cost scaling with large bright quads | Cost by covered pixels, per-type kernel and multi-source batching, not only draw counts |
| **P1** | Correct AE-era per-object property vocabulary | Hands-on audit of licensed Optical Flares; update crosswalk and tooltips |
| **P1** | Aperture ghosts / photoreal behavior | Reference real camera motion videos; build matching test harness with axis placement |
| **P1** | Lens dirt illumination and textured alpha | Test source color accumulation, compositing order, texture streaming and alpha modes |
| **P1** | Multi-camera, viewport, PIE and Scene Capture | Prove source registry independence and avoid camera-history corruption |
| **P1** | Content pipeline and preset thumbnails | Full cook of UTexture2D/curve references; gallery caching and asset redirector handling |
| **P2** | Physically derived lens library | Optional optics parameterization and sample libraries beyond basic artistic workflow |
| **P2** | Performance on forward, VR, consoles/mobile | Research after Win64 deferred path; not unsupported promises in v1 |
| **P2** | Emissive pixel / luminance source detection | Later separate feature, with performance/false-positive tradeoffs |

## Sources

- [Video Copilot Optical Flares](https://www.videocopilot.net/products/opticalflares/)
- [Epic UE5.8 Post Process Materials](https://dev.epicgames.com/documentation/unreal-engine/post-process-materials-in-unreal-engine)
- [Epic UE5.8 Post Process Effects (native image-based lens flare)](https://dev.epicgames.com/documentation/unreal-engine/post-process-effects-in-unreal-engine)
- [Epic UE5.8 Scene View Extension API](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/ISceneViewExtension)
- [Epic UE5.8 EPostProcessingPass](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/ISceneViewExtension/EPostProcessingPass)
- [Epic UE5.8 Visibility and Occlusion](https://dev.epicgames.com/documentation/unreal-engine/visibility-and-occlusion-culling-in-unreal-engine)
- [Unity HDRP Lens Flare documentation search](https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4012.1/manual/whats-new-12.html)
- [CryEngine Optical Flare System](https://www.cryengine.com/docs/static/engines/cryengine-3/categories/1114113/pages/1048578)
- [Autodesk VRED Lens Flare Elements](https://help.autodesk.com/cloudhelp/2022/ENU/VRED-Lights/files/VRED_Lights_About_Lens_Flares_and_Elements_html.html)
- [Flare7K++: Mixing Synthetic and Real Datasets for Nighttime Flare Removal](https://arxiv.org/abs/2306.04236) — optical morphology and physically motivated distinctions; not implementation instructions.
- [Independent real-time reconstruction of Optical Flares](https://wiki.nuaj.net/index.php?title=Lens_Flares) — historical research, not a licensed implementation recipe.

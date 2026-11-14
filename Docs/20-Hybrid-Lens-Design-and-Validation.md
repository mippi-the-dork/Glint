# 20 — Glint Hybrid Lens System: Design Decision and Validation

[← Documentation index](README.md) · [18 — Maxon](18-Maxons-Real-Lens-Flares-Research.md) · [19 — REAL FLARES](19-REAL-FLARES-Unreal-Research.md) · [02 — v1 Scope](02-v1.0-Scope.md)

> **Design proposal:** Not a implemented feature, and not a promise of physically traced lens simulation. Priority remains a performant standalone UE 5.8 plugin and an artist-friendly editor.

## Recommended product model

**Core authoring workflow: Art Directed.** Keep the Optical Flares-familiar Glint element stack and properties; in gameplay, **Automatic Lens** is now the primary way that authors apply a lens to a camera. Artists can create a convincing effect from Glow, Iris, Multi Iris, Streak, Rays, Ring, etc., edit each directly, and control behavior with Dynamic Triggering.

**Optional enhancement: Lens Inspired.** A shared Lens Profile changes related elements *together*: aperture shape, starburst, ghost shape, chromatic response, sensor/focal scaling and source type. An artist can opt out of each inherited property or apply a local modifier. This is a layer of behavior, not a second incompatible asset type or mandatory physical optics editor.

**Advanced model (future):** Lens-reflection response fields that describe how an optical artifact deforms throughout camera/source position and focal length. Physical offline lens-profile generation or true ray-tracing is not a v1 requirement.

## Authoring data architecture

`UGlintAsset` (**Glint Flare**) may reference a shared first-class `UGlintLensProfile` (**Glint Lens**) or inherit the active camera lens, and contains a set of ordered `Elements` with stable IDs, their base properties, and per-property ownership (Local / Linked / Linked+Offset / Linked×Multiplier). A referenced `UGlintLensProfile` is now a **required first-class Glint Lens Content Browser asset**. Lens sensitivity, HDR source detection and image-field response are owned by that profile, not improvised in the Glint Flare element asset. Cooked data may compile to per-element GPU descriptors and small lookup tables.

A Lens Profile includes:

- **Aperture:** blade count, curvature, twist, rotation, softness, diffraction, irregularity.
- **Ghost optics:** ghost grouping/spread, chromatic dispersion, axis/tangential distortion, asymmetry, edge response, aperture inheritance.
- **Camera:** reference focal length, sensor/aspect and anamorphic factor, zoom response policy.
- **Source:** approximate spectral color look (named tungsten/LED/fire/etc.), light size, facing and soft-occlusion behavior.
- **Lens surface:** dirt texture, defocus, illumination strength, lens-space transform, optional influence on ghost/starburst.
- **Quality:** max ghost instances, render mode and deterministic behavior budget.

Do not claim accurate manufacturer-specific prescriptions. An "Optical Style" or "Lens Inspired" label makes the fidelity clear.

## Behavior and spatial mapping

Basic Optical Flares-like placement is interpolating between screen-projected source position and optical center: `P = Psource + distance * (Pcenter - Psource) + offset`. This produces attractive source-center-axis ghost movement, including mirrored ghosts when `distance > 1`.

Maxon demonstrates a more complex phenomenon: reflection shapes can deform, overlap and morph as a source shifts off-axis. Glint should investigate response functions for *position, 2D scale, rotation, chromatic separation, ring/iris geometry and opacity* as source UV varies. Initial implementation: cheap analytic curves. Optional expansion: small deterministic 2D response fields, potentially with discrete focal-length samples interpolated by zoom. No full spectral per-frame ray tracing.

Inputs to a Glint source/view evaluation: source screen UV, physical/directional source location, offscreen distance, view/sensor/focal length, source size and color, facing angle, per-view opaque visibility, per-view time, quality tier. Output: per-element instanced geometry and HDR intensity/color. Reuse one per-view renderer pass, not a dedicated full-screen pass per source unless a special effect truly demands it.

**Property inheritance must be visible.** A linked blue-style indicator in the native Details view, Effective Value tooltip and a `Make Local` action prevent confusion; changing the shared aperture should update only linked elements, with correct Undo/Redo and asset dirty semantics.

## Rendering and behavior policies

- **Color-space:** confirm the UE 5.8 injection point and HDR blending with Auto Exposure, Bloom and Tonemap; Screen-like modes require explicit scene-linear definitions.
- **Temporal:** maintain independent occlusion/animation history per camera/view, reset on cuts or invalid render state. Avoid ghost jitter from TAA/TSR and screen-space rounding.
- **Occlusion:** opaque/masked scene-depth soft fade is required. Translucent attenuation, fog and volumetric-cloud response require separate technical proofs; **not** inherited from other plugin claims.
- **Lens dirt:** separate camera-space illuminated dirt from source-space moving reflections. Optionally allow dirt to influence starburst/ghosts, with explicit GPU budget. Background-highlights-driven dirt differs from explicitly registered-source dirt.
- **Focal response:** offer Screen Constant, Physical-ish, and Custom Curve choices. Test CineCamera lens zoom as well as generic camera FOV.
- **View semantics:** do not share occlusion history across split-screen cameras; preview must use same runtime render logic when possible.

## UX requirements

Editor default is **Elements | Preview | Details**, plus preset browser. Optional Lens Profile tab (with a simplified side-view optical diagram) can be opened by choice. Preview lets artists drag source, animate source path, adjust camera focal length, switch between isolated element/optical ghosts/final image, show occluder, and scrub dynamic triggers. Shared profile edits and local edits are both immediate and undoable.

Do not force artists to author a glass stack, learn coating physics or use node graphs. An Optical Flares user should immediately know where to add elements and how to change brightness, scale, axis distance and rotation.

## Proposed v1.0 decisions and feature gates

**Keep guaranteed v1:** existing 12 family element workflow, full native authoring, runtime source data, GPU integration, opaque occlusion, original presets, quality controls and native tests.

**Required v1:** first-class shared Lens Profile with automatic HDR sensitivity, image-field response and bounded candidate mapping. **Feasibility-gated refinements:** deeper linked aperture/iris optics, camera zoom behavior, sophisticated source styles and optical LUTs. Build the data contract in R1 and test against R0 HDR/cost proofs.

**Research / defer:** true physical tracing, exact coating/sensor simulation, multi-dimensional optical lookup maps, advanced background-illuminated Schmutz interactions, per-source high-cost shafts and universal translucency/fog occlusion. **Automatic image-based lens response and bounded source candidates are mandatory v1.0**, documented in [21](21-Automatic-Lens-Vision-and-Workflows.md)–[28](28-Automatic-Mode-Testing-and-Release-Gates.md).

## Concrete validation matrix

| Case | Setup | Pass condition |
|---|---|---|
| Coupled aperture | One starburst + three iris ghosts linked; 6→8 blades | Related shapes update coherently; unlinked elements unchanged; undo works |
| Cross-screen motion | Source travels center→edges→offscreen→opposite edge | Correct ghost axis, smooth transforms/alpha, no sudden disappearing gate flare except by explicit design |
| Focal change | CineCamera zoom while moving source | Configurable focal response with no temporal popping; camera cuts reset histories |
| Occlusion | Opaque/masked blocker, partial cover, camera moving | Predictable fade and no frame-flash on view transitions |
| Lens dirt | Same camera-space dirt across two sources | Dirt doesn't duplicate unnecessarily or slide with each source; source illumination remains controllable |
| Quality/cost | 1, 8, 32 sources with variable element counts at 1080p | GPU, CPU, VRAM and quality documented; settings change cost without corrupting look |
| Product UX | Optical Flares-trained artist builds an original flare | Fast familiar authoring with native Undo/Redo, saved asset, preview and packaging |
| Translucency/fog spike | Translucent object and dense volumetric cloud | Record what actually works and explicitly document fallbacks; no silent false occlusion assumptions |

## Roadmap effect

- **R0:** package a minimal Glow/Streak source, source motion and depth occlusion, confirm HDR pipeline and timing.
- **R1:** define Lens Profile data and override policy alongside source / element schema; physical solver excluded.
- **R2:** editor adds optional Lens Profile pane and focal/source path preview without moving the core element stack.
- **R3–R4:** linked aperture & ghost behavior after analytic implementation demonstrates good fidelity and cost.
- **R5–R6:** evaluate with repeatable packaged performance fixtures and user usability testing; publish supported/unsupported cases.

**Strategic result:** avoid a false choice between a freeform artist tool and a realistic lens simulation. Glint's strength should be art-direction, optionally informed by lens physics, and always governed by real-time costs.

## Updated camera-first asset contract

A separate **Glint Lens** asset owns scene HDR highlight detection, image-field ghost/streak behavior, candidate mapping to Glint Flare templates, camera dirt, budgets and project/camera default handling. The **Glint Flare** asset remains the editable source-relative composition and optional linked lens optics. Both must compile into a shared GPU system. The exact precedence, visual authoring and performance implications are defined in [23 — Camera Lens Profile](23-Camera-Lens-Profile-and-UX.md) and [24 — Auto + Authored](24-Automatic-and-Authored-Rendering.md).

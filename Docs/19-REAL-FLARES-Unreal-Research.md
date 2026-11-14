# 19 — REAL FLARES: Direct Unreal Competitor Research

[← Documentation index](README.md) · [18 — Maxon](18-Maxons-Real-Lens-Flares-Research.md) · [20 — Glint direction](20-Hybrid-Lens-Design-and-Validation.md)

**Date:** 2026-11-12. **Evidence level:** Fab *seller-reported* features, not independently measured or validated. No proprietary code or licensed content was inspected. The product's seller links a Google Docs manual and public promotional video, but a full hands-on UI/renderer review remains a separate task.

## Product overview

[REAL FLARES | Optical effects Toolsuit (Fab)](https://www.fab.com/listings/7d4eddfd-4be4-4d6b-a9d2-07aa3c11d30c), seller TXC STOCK. Listing describes Version 2.0, supporting UE 5.3–5.8. Its existence proves that a custom Unreal flare editor and sprite/procedural RDG rendering are commercially offered, but **does not prove any particular GPU cost or visual superiority**.

## Seller-advertised features

| Subsystem | Public claim | Implication for Glint |
|---|---|---|
| Runtime render path | RDG pass; sprite and procedural elements | Validate the standalone UE 5.8 rendering route in R0; avoid assuming another plugin's architecture is correct for us |
| Flare editor | Editable layer/element authoring, manager and library | Dedicated editor is baseline functionality, not our differentiation |
| Types | Sprite, Ghost, Multi-Ghosts, Lens Dirt, Light Shafts, Chroma Arc, Multi-Streaks, Iris Glints, Glow — nine named types | Authoring depth and consistent controls matter more than raw type count |
| Assets | Advertises 400+ HDR 32-bit element textures, plus 200+ presets in v2.0 | Glint uses original procedural shapes and original content; optimize dependency footprint |
| Source hookup | Placeable source Actor or component and manager to attach to scene objects/lights | Include Actors and components **and** transient Actor-free Blueprint/C++ handles |
| Optical look | Lens profile emulation, antireflective coatings, chromatic aberration, UV distortion | Shared Lens Profiles alone are not novel; demonstrate coherent editable behavior |
| Visibility | Depth, offscreen/distance fading, LOD/culling | Measure camera-edge behavior and GPU source scaling |
| Special passes | Screen-space shafts; bump/refraction/shadowed dirt | Costly effects must have explicit quality toggles or be deferred |
| Material/atmosphere | Claims translucent occlusion color/intensity response and volumetric-cloud/fog response | Add separate proof tasks; do not imply opaque depth tests cover these |
| Integration | Blueprint, Sequencer, runtime `r.RealFlares*` quality/effect CVars | Native UE tools, data-driven sources, Quality Settings and real-time diagnostics |

The phrase *unlimited element count* is an authoring capability. It is not proof of unlimited or flat render cost; fragment overdraw, bandwidth, sort breaks, source count and texture dimensions still determine actual GPU work.

## Unknowns requiring hands-on evaluation

1. Does scene-linear HDR composite correctly with eye adaptation, bloom, tonemapping and different output transforms?
2. Which exact per-element parameters exist? Does dynamic behavior animate geometry, ghost movement and texture/shape, or only transforms/intensity?
3. Does its light response account for actual geometry overlap near partially obscured sources? How does translucency affect intensity/color?
4. Which volumetric-fog/cloud information is sampled and what are the failure conditions?
5. Does source occlusion handle camera cuts, TAA/TSR, dynamic resolution, split-screen, editor cameras and packaged builds?
6. What are CPU costs and incremental GPU timings at one source, eight detailed sources, and 32 small lights? Are per-source full-screen operations used?
7. How intuitive and native are the editor toolbar, undo/redo, Content Browser asset usage, dirty state, preview and detail customization?
8. Are camera-attached dirt and per-source dirt separate? Do offscreen sources contribute to projected artifacts intentionally?
9. What state is maintained at runtime: transient handles, Actors only, components only, or a mix?
10. Can individual layer shapes deform organically as light source moves (a Maxon-style strength), while remaining easily artist-directable (an Optical Flares strength)?

## Fair competitor study protocol

Use independent reference art, **not** copied seller presets. Compare identical camera paths and target looks: Natural Sun, Blue Anamorphic, Dirty Practical, Edge Gate, Sci-Fi Starburst and Transient Flash. Lock GPU, render resolution, screen percentage, RHI, renderer, bloom, exposure, TAA/TSR, view mode and source screen coverage. Record authoring steps and source UX separately from measured incremental GPU/CPU costs; do not infer speed from editor FPS. Test packaged builds.

**Important limitation:** Public product marketing is sufficient to establish *advertised feature parity*, not to assess actual code quality, precise property definitions, packaged performance, or reliability. This is an actionable research backlog rather than an unfair product review.

## Sources

- [Official Fab listing](https://www.fab.com/listings/7d4eddfd-4be4-4d6b-a9d2-07aa3c11d30c)
- [Linked RealFlares_Documentation (Google Doc)](https://docs.google.com/document/d/1TGnqy-JL5Dkh6zGO_rwWLSEGrwiJ_J9eYZ6lfjtHB4g/edit?usp=drive_link) (not exhaustively audited)
- [Official promotional video](https://www.youtube.com/watch?v=mTUsW4kFBBk) (visual reference, not performance evidence)

> **Latest:** [Glint 0.8.0 Dev08 — release and QA notes](86-Dev08-Release-Notes-and-QA.md), [source grouping/energy](83-Dev08-Source-Grouping-and-HDR-Energy.md), [icon and reset fixes](84-Dev08-Editor-Brushes-and-Reset-Arrows.md), [optical presets/performance](85-Dev08-Optical-Presets-and-Performance.md). **35 Glint automation tests registered; UE5.8.3 build and visual validation pending.**

> **Current source handoff:** [Glint 0.7.0 — Dev07 expanded authoring and editor fixes](80-Dev07-Release-Notes.md) lists this source candidate, known validation limits, and the 29-test suite. See [editor preview details](77-Dev07-Editor-Refresh-and-Preview.md), [optical library](78-Dev07-Optical-Library-and-Presets.md), and [QA checklist](79-Dev07-Manual-QA-and-Automation.md). The older v1 research chapters remain design targets, not declarations of complete features.

> **Current source handoff:** [Glint 0.6.1 — Slate row build fix](76-Dev06-Slate-Row-BuildFix.md) documents the most recent Glint-only compile correction, the 0.6 UI changes and the Windows validation still required. The older research chapters remain v1 product targets, not claims of implemented functionality.

> **Implementation status:** Development Pass 03 source is now included in the plugin. The 46 original planning/research documents describe target v1.0 behavior, not the feature completeness of this build. See [Dev 01 build and QA](47-Dev01-Validation-and-Manual-QA.md), [source audit](46-UE58-Source-Audit.md), [Gameplay Cameras plan](48-Gameplay-Cameras-Integration.md) and [renderer next pass](49-Renderer-Budget-and-Next-Pass.md).

# Glint — v1.0 Planning Documentation

> **Product name:** Glint (confirmed)  
> **Status:** Research specification plus Dev03 implementation; **v1.0 feature set is not complete**  
> **Updated:** 2026-11-12 (Froyok and five user-supplied lens flare research references audited)  
> **Target:** Unreal Engine 5.8.0–5.8.3, Win64 deferred renderer, packaged runtime and native Slate/editor authoring  
> **North star:** **The lens reacts to the scene automatically**, and creators can also hand-author any flare with Optical Flares-like detail.

## Read this first: automatic lens mode is part of v1

Glint is now a **camera-oriented lens system**, not only a flare attached to manually specified lights. Give a camera/project a **Glint Lens** asset and qualifying bright rendered pixels generate ghosting, anamorphic streaks and associated optical artifacts. GPU highlight extraction should also produce **bounded, stable source candidates** that drive detailed **Glint Flare** element compositions. Manual sources remain for hero, offscreen and explicit directional effects.

This direction **supersedes earlier references that deferred automatic image-based flares**. It is a product requirement; the exact GPU implementation and benchmark performance still require an Unreal 5.8 feasibility spike. See [21 — Vision](21-Automatic-Lens-Vision-and-Workflows.md) and [28 — Release gates](28-Automatic-Mode-Testing-and-Release-Gates.md).


## Latest research addendum: Froyok and uploaded papers

These papers are now part of the Glint design corpus. The new chapters distinguish **historical measured performance**, **ideas validated only in individual prototypes**, and **Glint features actually committed to v1**.

| Document | Purpose |
|---|---|
| [38 — Froyok Unreal custom flare](38-Froyok-Custom-UE-Lens-Flare-Audit.md) | RDG passes, independent HDR threshold, temporal stability, and source-modification caveat |
| [39 — Tile-based ghosts (2023)](39-Tiled-Ghost-Rendering-2023-Audit.md) | Overdraw reduction, hierarchy, primitive merging, actual GPU figures |
| [40 — Walch measured flares (2017)](40-Walch-2017-Measured-Lens-Research.md) | Photographic sampling, Bezier primitive fitting, compact GPU response curves |
| [41 — Meta capture reconstruction (2026)](41-Meta-2026-Capture-Reconstruction-Audit.md) | Camera-anchored learned flares, transfer, real per-light deformation cost and limitations |
| [42 — Reelfs optical primer (2013)](42-Reelfs-2013-Seminar-Audit.md) | Background and research lineage; not a new independent GPU algorithm |
| [43 — Architecture synthesis](43-Rendering-Architecture-Research-Synthesis.md) | Unified automatic field/candidates/authored sources and optional future response bake |
| [44 — Prototype gates](44-New-Research-Prototype-Gates.md) | Exact tests to run before committing further renderer architecture |
| [45 — Hullin et al. 2011 review](45-2011-Physically-Based-Paper-Review.md) | Previous paper review, now included in the full documentation corpus |

**Decision:** Automatic HDR field and source candidates remain required v1.0. Photography-derived lens capture, ML/3DGS flare models and optical tile-based ray rasterization are **research/options, not v1 dependencies**.

## Documentation map

| Document | Purpose |
|---|---|
| [01 — Product Vision](01-Product-Vision.md) | Camera-first product, two assets, three source paths, principles |
| [02 — v1.0 Scope](02-v1.0-Scope.md) | Must-have deliverables, exclusions and acceptance |
| [03 — Editor Design](03-Editor-UX.md) | Native authoring, preview, element stack, presets |
| [04 — Element Specification](04-Element-System.md) | Primitives, placement and compositing |
| [05 — Runtime System](05-Runtime-Architecture.md) | Modules, source APIs and per-view architecture |
| [06 — Renderer & Occlusion](06-Renderer-and-Occlusion.md) | HDR/occlusion integration, automatic pipeline extension |
| [07 — Performance](07-Performance-and-Scalability.md) | Budgets, scalability, auto + manual measurement |
| [08 — Testing](08-Testing-and-Validation.md) | UE native tests, image regression, packaged validation |
| [09 — Roadmap](09-Development-Roadmap.md) | Feasibility-first development milestones |
| [10 — Research & Decisions](10-Research-and-Decisions.md) | Sources, alternatives and open decisions |
| [11 — Optical Flares Reference Audit](11-Optical-Flares-Reference-Audit.md) | Original element families, features and uncertainty |
| [12 — Element Parameter Schema](12-Glint-Element-Parameter-Schema.md) | Shared and per-element controls |
| [13 — Dynamic Behavior & Triggering](13-Behavior-and-Trigger-System.md) | Trigger functions, time, deterministic behavior |
| [14 — Optics & Renderer Research](14-Optics-and-Rendering-Research.md) | Optical realism and image effects |
| [15 — Optical Flares-to-Glint Workflow](15-Optical-Flares-to-Glint-Workflow.md) | Familiar authoring in native UE tool |
| [16 — Technical Spikes](16-Technical-Spikes-and-Validation.md) | Integration prototypes and gates |
| [17 — Competitive Landscape](17-Competitive-Landscape-and-Differentiation.md) | Other plugins, claims vs evidence |
| [18 — Maxon Real Lens Flares](18-Maxons-Real-Lens-Flares-Research.md) | Optical simulation, camera and dirt behaviors |
| [19 — REAL FLARES Unreal](19-REAL-FLARES-Unreal-Research.md) | Advertised Unreal plugin feature audit |
| [20 — Hybrid Lens Design](20-Hybrid-Lens-Design-and-Validation.md) | Shared optics + authored element design |
| **[21 — Automatic Lens Vision & Workflows](21-Automatic-Lens-Vision-and-Workflows.md)** | **Camera-default auto system, workflows and product contract** |
| **[22 — HDR Highlight Detection](22-HDR-Highlight-Detection.md)** | **GPU bright-pass, region grouping, scoring, top-K, classes and sensitivity** |
| **[23 — Camera Lens Profile & UX](23-Camera-Lens-Profile-and-UX.md)** | **Glint Lens asset, project default, camera override, first-use controls** |
| **[24 — Auto + Authored Rendering](24-Automatic-and-Authored-Rendering.md)** | **Field/complex candidate sources, shared renderer, dedup and occlusion** |
| **[25 — Optical Response Research](25-Optical-Response-and-Image-Effects.md)** | **Full family of lens behaviors, approximations and optical LUT opportunities** |
| **[26 — Exposure & Temporal Behavior](26-Exposure-Temporal-and-Renderer-Compatibility.md)** | **Pre-exposure, camera changes, TSR, translucency, view isolation, comfort** |
| **[27 — Research Backlog](27-Research-Backlog-and-Technical-Decisions.md)** | **Ranked unanswered questions and decisions** |
| **[28 — Automatic Mode Release Gates](28-Automatic-Mode-Testing-and-Release-Gates.md)** | **New renderer proof plan, tests, benchmark fixtures and acceptance criteria** |
| **[29 — Source Audit](29-Automatic-Flare-Research-Sources.md)** | **Direct documentation/research links, confidence and remaining gaps** |
| **[30 — UE5.8 Postprocess Integration](30-UE58-Postprocess-Integration-Research.md)** | **Research-confirmed callback/blendable distinction, TSR resolution, HDR sensitivity and bloom decisions** |
| **[31 — Camera Workflow & Renderer Acceptance](31-Camera-Workflow-and-Renderer-Acceptance-Plan.md)** | **Project/camera/volume lens ownership, blend rules, R0 tests and first implementation contract** |
| **[32 — Perceptual Optimizations & Budget](32-Perceptual-Optimization-and-Render-Budget.md)** | **Low-resolution effects, optical LUTs, GPU budgets, LOD, tiling and pitfalls** |
| **[33 — Performance Experiments & Gates](33-Performance-Experiments-and-Gates.md)** | **Prioritized performance spikes, reproducible benchmark scenes, QA tests and release gates** |
| **[34 — Other Engines](34-Other-Engines-Real-Time-Lens-Flares.md)** | **Unity, CRYENGINE, Unreal, Godot, Three.js, Babylon.js: documented implementation patterns** |
| **[35 — Shipped Games](35-Shipped-Games-and-Visual-Design-Lessons.md)** | **Crysis 3, Killzone: Shadow Fall, camera/visor art direction and gameplay implications** |
| **[36 — Rendering Research](36-Rendering-Papers-and-Architectural-Lessons.md)** | **2011 physically-based, 2013 first-order transfer, 2023 tile binning, 2024 sparse polynomials** |
| **[37 — Decisions & Prototype Gates](37-Cross-Engine-Findings-and-Prototype-Gates.md)** | **Concrete Glint changes, priority, tests, and non-goals** |

## Recommended workflow

1. Create **Glint Lens** (`UGlintLensProfile`) and choose a starter lens; assign it as the **project default**. Eligible cameras now have automatic lens behavior without any Glint Source Actors.
2. Use the Lens editor to inspect HDR bright areas, source candidates, sensitivity, lens appearance, quality and camera behavior.
3. Create **Glint Flare** (`UGlintAsset`) to design custom elements with the familiar Glow, Streak, Iris, Multi Iris, Ring and trigger workflow.
4. Let bright regions drive candidate Glint Flare elements automatically; optionally attach a manually authored Flare asset to one world/gameplay source for precise art direction.
5. Glint's shared per-view GPU renderer composes automatic image-field effects, bounded automatic candidate elements, authored elements and camera-space dirt, subject to measured budgets.

## Non-negotiables

- Automatic camera-based HDR bright-region response and an artist-readable Lens Profile are **v1.0**; no manually selected source is required for normal operation.
- Art-directed flare element stacks and manually registered world/Blueprint sources are also supported, not replaced.
- Standalone plugin without Engine source modifications; must build, cook and package using UE 5.8.0–5.8.3 Win64 (actual verification required).
- Two named native Content Browser assets, dedicated Slate editors, native Details, undo/redo, dirty state, presets, dynamic triggering and familiar interaction.
- No synchronous GPU-to-CPU per-highlight readback, no per-pixel/Actor/per-element spawning; bounded GPU work and per-view histories.
- Source-visible HDR input, no self-feedback from Glint; correct pre-exposure, camera cuts and postprocess coexistence.
- GPU performance must be measured separately for detection, field, candidate, authored, dirt and composite.
- UE `Automation RunTests Glint` coverage, deterministic regression assets, real packaged smoke and clear feature matrices.
- Original source, shaders, assets, textures and presets; commercial products serve as *research references*, not redistributable material.

## Unverified and deferred

The actual UE5.8 plugin callback/resource setup, top-K GPU algorithm and per-view performance are **not implemented or verified**. Deferred: full physical ray tracing, native support for all RHIs/platforms, unrestricted hundreds of individually high-detail flare compositions, automatic understanding of offscreen lights, and exact commercial preset import. Basic automatic candidate extraction is *not* deferred.

## New pre-implementation findings (2026-11-12)

The documented UE5.8 `BeforeBloom` **post-process material location** is not also an explicitly named **Scene View Extension** callback. The correct RDG HDR access/composite scheme and use of the current view-aware callback must be proved before committing to the renderer. An `IBlendableInterface` object is also a possible route for Glint Lens overrides through normal Post Process Volumes, but custom per-view behavior remains untested. See [30](30-UE58-Postprocess-Integration-Research.md) and [31](31-Camera-Workflow-and-Renderer-Acceptance-Plan.md).

## First substantive implementation batch

**R0A + R0B + R0C + R0D:** minimal HDR integration → automatic bright-mask and field ghosts/streaks → stable GPU top-K candidate driving a couple of Glint elements → authored source and dedup → packaged 5.8 smoke with profiling. See [28](28-Automatic-Mode-Testing-and-Release-Gates.md).

## Performance-by-design update

The renderer should *fake costly optical detail when visually equivalent*: shared automatic highlight analysis and per-view dirt; source budgets with Full / Simplified / Field-Only treatments; mixed-resolution soft/sharp rendering; GPU-driven compact candidates; cooked lens response data; instanced element families; and predictable LOD. Avoid adding optional complexity (tile binning, async compute, dynamic quality controller) before a profiler shows it wins. See [32](32-Perceptual-Optimization-and-Render-Budget.md) and [33](33-Performance-Experiments-and-Gates.md).

## Latest cross-engine research

The research in [34](34-Other-Engines-Real-Time-Lens-Flares.md)–[37](37-Cross-Engine-Findings-and-Prototype-Gates.md) adds the generic Repeater authoring modifier, screen-edge deformation, source occlusion response research, brightness-pyramid optimization feasibility, stock UE double-flare diagnostics, source-peak preservation, optical simplification options and more precise benchmark gates. They **do not** change the core camera-first requirement or defer automatic image-based flares.

- [50 — Edge Sampling Regression and QA](50-Edge-Sampling-Regression-and-QA.md) — real UE5.8.3 observed boundary stretching, root cause, shader-only fix, and manual acceptance tests.

## Dev02 implementation and validation additions (November 14, 2026)

- [51 — Dev02 Automatic + Authored Runtime](51-Dev02-Automatic-and-Authored-Runtime.md)
- [52 — Dev02 Camera Transitions](52-Dev02-Camera-Transitions.md)
- [53 — Dev02 Shader Contracts](53-Dev02-Shader-Contracts.md)
- [54 — Dev02 Visual and Performance QA](54-Dev02-Visual-Performance-QA.md)

## Dev03 implementation additions (November 15, 2026)

- [55 — Dev03 GPU Source Selection](55-Dev03-GPU-Source-Selection.md)
- [56 — Dev03 Native Flare Editor](56-Dev03-Native-Flare-Editor.md)
- [57 — Dev03 Physical Cameras and Gameplay Cameras](57-Dev03-Physical-Cameras-and-Gameplay-Cameras.md)
- [58 — Dev03 Visual QA and Performance](58-Dev03-Visual-QA-and-Performance.md)
- [59 — Dev03 Contracts Release Gates](59-Dev03-Contracts-Release-Gates.md)
- [60 — Dev03 Release Notes](60-Dev03-Release-Notes.md)

## Development Pass 04 — Three-mode editor and source refinement

- [61 — Schematic, Rendered & Hybrid GPU editor](61-Dev04-Three-Mode-GPU-Editor.md)
- [62 — Element identification and editor UX](62-Dev04-Element-Identification-and-UX.md)
- [63 — Optics additions and source-edge behavior](63-Dev04-Optical-Extensions-and-Source-Behavior.md)
- [64 — Engine integration and shader contracts](64-Dev04-Engine-Integration-and-Shader-Contracts.md)
- [65 — Manual QA and performance](65-Dev04-Manual-QA-and-Performance.md)
- [66 — Development release notes](66-Dev04-Release-Notes.md)

## Development Pass 05 — Artist Defaults, Camera Guide and Reference Texture

- [67 — Dev05 Artist First Element Defaults](67-Dev05-Artist-First-Element-Defaults.md)
- [68 — Dev05 Outliner Toolbar and Interaction](68-Dev05-Outliner-Toolbar-and-Interaction.md)
- [69 — Dev05 Camera Frame and Offscreen Workspace](69-Dev05-Camera-Frame-and-Offscreen-Workspace.md)
- [70 — Dev05 Reference Texture Preview](70-Dev05-Reference-Texture-Preview.md)
- [71 — Dev05 Manual QA and Tests](71-Dev05-Manual-QA-and-Tests.md)
- [72 — Dev05 Release Notes](72-Dev05-Release-Notes.md)

## Glint 0.5.1 — UE5.8 editor build correction

- [73 — Build Fix 01: Drag-and-drop header](73-Dev05-BuildFix01-UE58-DragDrop.md) — UE5.8.3 `C1083` resolution, scope, and validation status.

## Glint 0.5.2 — Native Flare Editor Toolbar

- [74 — Native toolbar visibility and same-row layout](74-Dev05-BuildFix02-Native-Toolbar.md) — Restores missing preview buttons and gear on the far right of the existing Save/Browse toolbar; documents regression test and Windows QA.

## Glint 0.5.3 — ToolMenus toolbar compile correction

- [75 — ToolMenus Toolbar Fix](75-Dev05-BuildFix03-ToolMenus-Toolbar.md) — UE5.8 C2248/C2678 fix, editor-only API extension and tests.

## Dev07 additions (source candidate)

| Document | Purpose |
|---|---|
| [77 — Editor refresh and preview](77-Dev07-Editor-Refresh-and-Preview.md) | Undo/Redo notifications, compact gear, lower-opacity schematic, editor icon handling |
| [78 — Optical library and presets](78-Dev07-Optical-Library-and-Presets.md) | Thirteen additional types, source-aware shader shapes, Default/Subtle/Cinematic presets, budgets |
| [79 — Manual QA and automated tests](79-Dev07-Manual-QA-and-Automation.md) | 29-test command and focused visual/undo/performance checks |
| [80 — Dev07 source release notes](80-Dev07-Release-Notes.md) | Implemented changes, static checks, and remaining Windows validation |

- [Glint 0.7.1 — Automation budget fixture correction](81-Dev07-01-Automation-Budget-Correction.md)

## Glint 0.8.0 — Dev08 source grouping, optical quality and editor corrections

- [83 — Source grouping and HDR energy](83-Dev08-Source-Grouping-and-HDR-Energy.md)
- [84 — Editor brush icons and Lens reset arrows](84-Dev08-Editor-Brushes-and-Reset-Arrows.md)
- [85 — Optical presets and performance](85-Dev08-Optical-Presets-and-Performance.md)
- [86 — Release notes and QA](86-Dev08-Release-Notes-and-QA.md)

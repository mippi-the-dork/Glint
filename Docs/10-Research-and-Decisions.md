# 10 — Research, Alternatives, Risks and Decisions

[← Documentation index](README.md)

## Reviewed external sources

Research links below are reference material only. No source files, textures, presets or code from third-party projects should be copied into the distribution without explicit applicable licenses and review.

### Video Copilot Optical Flares — authoring reference

- [Optical Flares product page](https://www.videocopilot.net/products/opticalflares/)
- Officially describes a visual 3D lens flare studio, reusable individual lens elements, preset browser, custom textures, 3D lights, dynamic triggering, chromatic aberration, flickering, 3D occlusion and luminance tracking.
- **Lesson:** composition and reusable element presets are a more useful primary authoring abstraction than a collection of global lens-flare properties.
- **Game-specific change:** runtime sources, GPU scene depth, scalability, cooked asset data and packaging matter as much as editing ease.

### Epic Games Unreal Engine 5.8

- [Post Process Effects](https://dev.epicgames.com/documentation/unreal-engine/post-process-effects-in-unreal-engine): native image-based lens flare controls and existing postprocess behavior.
- [Scene View Extension callback](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/ISceneViewExtension/SubscribeToPostProcessingPass): **view-aware** API and deprecated older signature; the callback has rules around override output.
- [Post-processing pass enum](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/ISceneViewExtension/EPostProcessingPass): pass locations to evaluate experimentally.
- [Scene View Extension base](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/FSceneViewExtensionBase): supported abstraction for extending views.
- [Asset Definitions](https://dev.epicgames.com/documentation/unreal-engine/API/Editor/AssetDefinition/UAssetDefinition): native Content Browser asset integration (replacement for old asset actions).
- [Post Process Materials](https://dev.epicgames.com/documentation/en-us/unreal-engine/post-process-materials-in-unreal-engine): HDR and after-tonemap tradeoffs, pass costs and temporal behavior.
- [Visibility and Occlusion Culling](https://dev.epicgames.com/documentation/unreal-engine/visibility-and-occlusion-culling-in-unreal-engine): GPU scene depth and HZB reference, not a plug-and-play flare occlusion API.
- [Niagara Performance](https://dev.epicgames.com/documentation/en-us/unreal-engine/measuring-performance-in-niagara): particle sprite overdraw and screen coverage cost.
- [Unreal Performance Profiling](https://dev.epicgames.com/documentation/unreal-engine/introduction-to-performance-profiling-and-configuration-in-unreal-engine): Insights, stats and RenderDoc workflow.
- [Automation Testing](https://dev.epicgames.com/documentation/en-us/unreal-engine/automation-test-framework-in-unreal-engine): native unit, feature, editor, screenshot and plugin tests.

### Independent flare-renderer investigations

- [Froyok, Custom Lens-Flare Post-Process in Unreal Engine (2021)](https://www.froyok.fr/blog/2021-09-ue4-custom-lens-flare/). Ghost/halo/glare taxonomy, multiple render passes, artistic shader controls, practical GPU costs; implementation required engine modifications on its UE 4.25 target. The reported historical GPU cost does **not** forecast UE5.8 performance.
- [CustomLensFlaresUE GitHub](https://github.com/singinwhale/CustomLensFlaresUE): later demonstration based on that research; README explicitly includes engine patches for UE 5.7.4. Proof of architectural interest, but not a drop-in plugin for this project's no-fork requirement.

## Architecture alternatives considered

| Approach | Advantages | Issues | Decision |
|---|---|---|---|
| Niagara translucent particle per element/source | Native workflow, familiar VFX tooling | Sprite overdraw, per-system overhead, authoring not purpose-built, arbitrary multi-source batching harder | Not primary renderer |
| Post-process material only | Fast prototype, no C++ shader work | Limited procedural instancing, complex per-source data, many passes or per-source effects, precise control concerns | Debug prototype/fallback at most |
| Edit Unreal built-in image-based Lens Flare renderer | Close integration, access to renderer internals | Requires engine fork/patch and maintenance across UE patches | Not acceptable for v1 |
| RDG global shaders + Scene View Extension | GPU batching, custom primitives, plugin isolation | Integration depth/HDR/version stability needs real spike | **Preferred, gated by R0** |
| Separate translucent screen-space meshes | Easy early visuals | Significant overdraw and source/object management, harder postprocess integration | Not default |
| Full physically traced optical system | Theoretical fidelity | Cost, complexity, authoring burden | Beyond v1 |

## Critical risks

| Risk | Severity | Mitigation |
|---|---|---|
| Desired HDR/depth buffers unavailable at safe plugin callback stage | Critical | R0 integration proof, selectable fallback; no engine patch assumption |
| Excessive pixel fill on giant streaks | High | Analytic cheap shaders, LOD, resolution split, profiling per element |
| TAA/TSR smears sharp flares | High | Pass placement experiments, analytic AA, temporal history resets |
| Many sources require too many GPU depth samples | Medium/High | Source caps, 1-tap tier, sample count limits, GPU batching |
| GPU readback/per-frame CPU traces create stalls | High | No CPU readback in normal occlusion path |
| Preview diverges from packaged runtime | High | Shared shader path and screenshot parity fixtures |
| Source/world resource lifetime causes editor teardown crashes | High | Tests across PIE/world/shader/editor shutdown lifecycle |
| Translucent objects appear not to occlude | Medium | Document opaque/masked limitation; opt-in special-case overrides |
| Presets or third-party textures create licensing issues | High | Original sample presets/textures, user-imported content only |
| Plugin naming conflicts with engine/Marketplace assets | Low | Glint is confirmed; check marketplace availability later |

## Decision register

| ID | Decision | State |
|---|---|---|
| D01 | Data-authored Glint asset with dedicated native editor | Recommended |
| D02 | Component and actor-free runtime source paths | Recommended |
| D03 | Custom RDG GPU renderer, no Niagara or engine patch required | Preferred; must pass R0 |
| D04 | Layered element editor, not generic node graph | Recommended |
| D05 | Depth-aware opaque/masked occlusion and time smoothing | Required v1 |
| D06 | High-quality HDR pre-tonemap integration | Goal; exact callback undecided pending R0 |
| D07 | Tiered scalability with clear visual/CPU/GPU stats | Required v1 |
| D08 | Asset and example library using only original content | Required v1 |
| D09 | Confirmed name **Glint** and automation prefix `Glint.*` | Decided |
| D10 | DX12/SM6 desktop deferred, UE5.8 Win64 first | Proposed baseline; verify |

## Questions to resolve **through spikes** rather than speculation

1. Which UE5.8 per-view callback affords required scene depth, pre-exposure and working HDR compositing without engine modification?
2. Which source-depth sampling pattern remains stable with TSR and dynamic resolution?
3. Should simple source projection live CPU-side for efficient culling, and which work belongs in GPU VS/CS for throughput?
4. Do high-quality streaks require a separate full-resolution render path, or can procedural AA at half resolution suffice?
5. Which Unreal-provided editor asset category best fits the Content Browser, and can viewport Show menu integration be cleanly added without hijacking native flags?
6. What actual source/instance counts meet the target performance range on fixed reference hardware?
7. Which rendering paths besides Win64 desktop deferred can be certified and advertised after prototype testing?

## Next action

Implement the R0 renderer spike exactly as specified in [Roadmap](09-Development-Roadmap.md), and record compiler, renderer pass, profiling and packaged-game findings here before finalizing shader architecture.

## Additional Optical Flares research

The key expanded audit is [11 — Optical Flares Reference](11-Optical-Flares-Reference-Audit.md), with vendor-confirmed AE features distinguished from related Nuke documentation and independent reconstruction. Add to decision register:

| ID | Decision | State |
|---|---|---|
| D11 | 12 familiar visual families + Texture Sprite first-class in the Add Element palette | Proposed v1 requirement |
| D12 | Shared typed Dynamic Triggering system for all appropriate elements; visible red/heatmap preview | Proposed v1 requirement |
| D13 | Keep familiar Glow/Streak/Iris/Multi Iris naming but design native Unreal Slate editor | Decided UX direction |
| D14 | Screen-like blending cannot blindly use LDR formula on unconstrained HDR values | Requires renderer proof |
| D15 | Lens Dirt/Orbs uses camera/lens space illumination, batched per view | Proposed design, measure S4 |
| D16 | Per-source + per-camera state and deterministic shimmer/flicker, not shared mutable asset state | Required v1 |

## Automatic Lens Mode research — v1 scope correction, November 12 2026

The original source-driven architecture was intentionally conservative, but **automatic camera-based HDR flare response is now a primary product requirement**. Confirmed precedents: UE5.8 itself has image-based lens flares; Unity HDRP/URP generate flares from bloom-fed bright image regions; Video Copilot Optical Flares advertises Luminance Tracking with threshold/resolution; Flares OFX advertises top-K region ranking/area-weighted light generation. This does not establish any of their source code, performance or exact implementation in UE.

Adopted/research-gated decisions D21–D29, complete source audit and prioritized technical unknowns: [27](27-Research-Backlog-and-Technical-Decisions.md) and [29](29-Automatic-Flare-Research-Sources.md). Primary technical risks are renderer HDR pre-exposure integration, highlight segmentation/top-K, temporal stability, high dynamic range compositing and GPU fill/gather cost.

# 31 — Glint Camera Workflow and Renderer Acceptance Plan

[← Documentation index](README.md) · [30 — UE5.8 integration findings](30-UE58-Postprocess-Integration-Research.md) · [28 — v1 release gates](28-Automatic-Mode-Testing-and-Release-Gates.md)

> **Status:** product decisions proposed for the first implementation spike. Implementation and verification are pending.

## 1. Lens ownership and setup UX

**Default path:** Create a `Glint Lens` Content Browser asset, pick it as the **Project Default Lens** in `Project Settings > Plugins > Glint`, and activate automatic flares for game cameras without adding any source Actors.

**Override path:** Assign an optional camera-specific lens (through a small `Glint` component or another validated camera integration) when cinematics or character cameras need a distinct style. Camera overrides must never mutate the original lens asset or change unrelated cameras.

**Volume path to investigate:** UE's Post Process Volume and Component accept `IBlendableInterface` objects through `AddOrUpdateBlendable`; the interface is intended for objects blended by the post-process system. This provides a *possible integration point* for Glint lens overrides inside existing artist-authored volumes. Verify that a custom UObject receives the intended per-camera weight and that the Glint renderer can resolve the lens profile without unsafe cross-thread references. This is **not a proven Glint integration**. If it cannot work cleanly, fall back to a Glint-specific lightweight blend region built around existing native Volume interaction conventions. Avoid modifying `FPostProcessSettings` in Engine source.

**Docs:** [IBlendableInterface](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/IBlendableInterface); [APostProcessVolume::AddOrUpdateBlendable](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/APostProcessVolume); [Post Process Volume blending](https://dev.epicgames.com/documentation/unreal-engine/post-process-effects-in-unreal-engine).

## 2. Blend rules for lens assets

Blending numeric Lens settings between volumes is straightforward in principle; blending authored compositions with **different element counts, seeds and optical constructions** is not. A naive element-by-element interpolation is undefined and artist-hostile.

Proposed rules:

1. **Scalar blendables** (intensity, sensitivity, overall tint, quality caps within safe integer transitions) can use normalized volume/camera weights.
2. **Composition selection** is an explicit asset selection by priority. Optionally crossfade *rendered outputs* between different compositions only on quality tiers where extra rendering is acceptable and benchmarked.
3. **Camera overrides** have higher precedence than project defaults. Define how explicit camera overrides interact with weighted PP volumes; show the final source and weight in the debug display.
4. **No hidden global state:** source resolution is per `FSceneView`, not whichever camera rendered most recently.
5. **Transitions** require hysteresis when switching dominant compositions, to avoid rapid swaps near PP volume boundaries.

Editor diagnostics should display: `Project Default → PP Volume(s) → Camera Override → Resolved Lens`, with weighted values and the reason one composition won.

## 3. Acceptance fixtures and exact observations

| Scene / operation | Expected observation | Failure that blocks progression |
|---|---|---|
| Dark room, one bright emissive sign, no Glint actors | Flare emerges from HDR input using Project Default Lens | Only appears if a Light Actor is manually assigned |
| Toggle UE Bloom and native lens flares independently | Automatic Glint continues functioning and can be visually isolated | Depends on built-in bloom or silently alters native settings |
| Bright white wall, small headlamp, long LED bar | Area/contrast filters distinguish broad, point and elongated response | Hundreds of heavy iris stacks for a flat white wall |
| Exposure EV sweep, auto-exposure transition | Behavior matches published scene/camera/hybrid sensitivity mode | Random changes from pre-exposure numbers |
| Camera hard cut and world travel | Candidate history resets; no unrelated ghost drifting | History leaks across views or worlds |
| TSR 50%, TAA, native res, 16:9 and 21:9 | Correct UV, positioning, aspect and stable glints | Offset flare centers, jitter or distorted shapes |
| One source hidden by opaque foreground | Source-derived flare dims smoothly; automatic input follows visible HDR | Light shines through solid geometry incorrectly |
| Fog, particles, translucent glass | Record exactly which contributions are visible at chosen pass; disable unsupported options clearly | Claims of physically correct occlusion without proof |
| Editor / PIE / standalone cooked, DX12/SM6 | Same asset loads, shader cooking works, and callbacks render | Shader file unavailable in packaged game |
| Two simultaneous views with different profiles | No shared exposure/history/preset contamination | Cross-camera flares |
| Profile A → B via volume with different element counts | Deterministic choice or measured crossfade; never remap unrelated elements | Sudden crash or arbitrary element jumps |
| 100+ bright spots + authored hero source | Bounded full sources, shared field for others, no duplicated hero | Unbounded GPU work and unintended double flare |

## 4. R0 first-batch engineering contract

1. **R0A — Plugin / pipeline proof:** small runtime plugin, own shaders, shader directory registration, editor+packaged draw, before/after comparison of candidate HDR stages, valid RDG resource lifetime, no Unreal source edits.
2. **R0B — Automatic field:** bright-pass and ghost/streak response from emissive image alone, optional debug masks, native bloom on/off.
3. **R0C — Source candidate proof:** GPU downsample + bounded top-K/peak merge; a source descriptor drives basic Glow/Iris. Temporal candidate IDs and threshold hysteresis, at least in one camera.
4. **R0D — Hybrid proof:** one authored world source with depth occlusion and one automatic image source coexist; default overlap policy, debug why both were or were not drawn.
5. **R0E — View ownership proof:** rapidly switch project lens and optional camera override, reset on cuts, verify scope is per view. Test scene capture exclusion when disabled.
6. **R0F — Volume feasibility:** minimal `IBlendableInterface` lens override object in an existing Post Process Volume, or documented fallback if unsupported; prove weight semantics rather than assuming.

Do **not** begin the comprehensive element editor before R0A–D work; the R0E–F scope can be developed alongside the R1 foundation if the renderer needs further polish, but lens resolution must be decided before serializing authoring assets.

## 5. Tests and observability

- Core console: `Automation RunTests Glint`.
- Initial test groups: `Glint.Render.Integration.*`, `Glint.Math.Exposure.*`, `Glint.Auto.Detection.*`, `Glint.Auto.Temporal.*`, `Glint.Data.LensResolution.*`, `Glint.Editor.Lens.*`.
- Optional native view overlay: **Debug Highlights** showing bright-pass / candidate centers / brightness / candidate size / rejection reason / confidence / total cost.
- GPU timings: `Extract`, `Classify/TopK`, `Track`, `Field`, `Detailed Elements`, `Authored Elements`, `Dirt`, `Composite`.
- Capture actual test evidence with Unreal Insights/GPU Visualizer and packaged smoke screenshots, including camera cuts and moving highlights. Documentation should record engine build, RHI, screen percentage, view type, GPU, timings and known limits.
- Block v1 claims on real data; do not assume 0.6 ms total when auto detection and a full screen field are active.

## 6. Early product decision to lock

**Glint is a lens look for the camera, not an obligatory attachment on a light.** Project-default automatic flares are the first-use experience. Direct source-driven flares remain available for hero art direction and offscreen/directional needs. Users should not need to understand Engine pass names, exposure buffers or view-extension APIs to make a flare; those are implementation details behind a clear editor and debug views.

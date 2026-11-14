# 24 — Shared GPU Renderer: Automatic Field + Candidates + Authored Sources

[← Index](README.md) · [22 — Detection](22-HDR-Highlight-Detection.md) · [25 — Optical behavior](25-Optical-Response-and-Image-Effects.md)

> **Status:** Proposed pass topology for a standalone UE 5.8 plugin. HDR access, pass placement and scene texture lifetime need actual engine/cooked-build validation.

## Recommended per-view pipeline

```text
UE camera + scene color (+ exposure, depth, view constants)
          |
     Resolve active Glint Lens Profile (project / volume / camera / runtime)
          |
     HDR bright-pass and GPU reduction
          |                      \
  IMAGE FIELD BRANCH              CANDIDATE BRANCH
  - direct warped/reversed         - peak extraction
  - anamorphic streak             - region merge/top-K
  - spectral ghosts               - temporal matching
  - broad glow                    - element instance generation
          |                      /
          +---------------------+
                    |
            Authored source descriptors
            (world/component/Blueprint)
                    |
            conflict/suppression policy
                    |
           common renderable element packets
                    |
       HDR composite + lens-space dirt (per view)
                    |
             continue UE postprocessing
```

The render dependency graph should manage textures and buffer lifetimes; no renderer actor or one render pass per flare. The field branch uses bounded image operations; the candidate/author branch uses a shared instancing/batching path. **Do not detect Glint's own output** or another flare already composited into scene color, otherwise recursive brightening can occur.

## Field rendering: affordable many-to-one response

The image field is a reduced HDR representation of the qualifying highlights. From it, generate:

- ghost reflections by source-to-center UV remapping, including regular and mirrored orientations;
- warped/polar ghosts and chromatic displacement;
- streaks via anisotropic directional gather or optimized blur;
- lower-frequency glow/spectral smears;
- camera-wide lens dirt illumination from the accepted bright field.

The result should not be limited to nine hard-coded copied blobs or a single giant blur. The number of field ghost bands, tint, scale/axis distance, softness, lens response and aspect correction should be artist-editable in the Lens Profile. Start with a small finite set of field operators and benchmark individually.

Field passes should use a profile-controlled downscale/mip selection. Reusing an engine bloom pyramid could reduce cost but risks dependence on engine bloom settings/texture lifetime; the **fallback/baseline** is a Glint-owned HDR bright texture. If native engine bloom is disabled, Glint automatic mode should remain functional unless the user explicitly connects the two.

## Candidate rendering: complex Glint elements from automatically found pixels

A detected bright region becomes a synthetic source descriptor. It can reference a Glint Flare asset template; template element parameters are modulated by detected RGB, radiance, centroid, area, uncertainty, camera optic settings and source class. Sample mapping:

| Candidate | Suggested element priority | Downgrade under load |
|---|---|---|
| Sun-sized compact bright region | Glow, streak, multi iris, ring, radial glint | Reduce iris count and radial samples |
| Headlight or lamp filament | Streak, glow, ghosts, smaller iris | Render glow + streak only |
| Small specular flash | Sharp glint, tiny halo, restrained ghost | Glint only, then field only |
| Wide luminous doorway | Broad glow/warped field, weak ghosts | Field only |
| Long neon strip | Anamorphic streak, no many duplicate centers | Field only or single low-cost representative source |

The same Lens Profile can specify a single default template and optional class-specific overrides; do not make source classification a mandatory complex programming exercise in v1. Treat all per-class mappings as artist controllable with a **Default** mapping.

## Deduplicating explicitly authored and auto-generated flares

An authored source projected near a bright pixel can produce an automatic flare *and* a manually authored one. Glint should avoid silently doubling the result.

Proposed per-camera policy (artist-exposed):

1. `Prefer Authored` (**recommended default**) — remove or attenuate the nearest automatic candidate within a configurable screen-space footprint for the authored source, while preserving the broader field response where desired.
2. `Blend` — allow automatic plus authored contributions with optional total-intensity clamp.
3. `Automatic Only` — ignore authored sources for this lens/camera.
4. `Authored Only` — bypass automatic detection/render path for this camera.

Deduplication is approximate in screen space; two different lights can overlap in projection. Add a per-source override: `Suppress Auto Near This Source`, `Allow Auto`, `Authoritative`, plus suppression radius based on projected source size. For image-field processing, test a soft suppression mask for known manual sources; do not automatically remove whole light regions or adjacent unrelated highlights.

The field branch and candidate branch can also double-count *one automatic highlight*. The profile must control field/candidate weights separately; candidate mode can subtract/attenuate a region's contribution from the field ghost component or use a lower default field ghost response when full candidates are enabled. A shared brightness/energy normalization pass is worth prototyping before introducing more blend mode presets.

## Visibility and occlusion semantics

| Source | Visibility | Why |
|---|---|---|
| Automatic image pixel/region | Presence in eligible HDR scene signal, temporal confidence, source mask | Visibility is already encoded in the rendered image; a second world-depth test is invalid because world position is unknown |
| Authored world point | GPU scene depth vs projected 3D point plus soft sampling | Camera can know whether source is hidden by opaque/masked geometry |
| Authored directional | Far-field/depth-aware visibility approximation, configurable offscreen behavior | A directional light is not a finite world point |
| Authored screen position | Artist-defined fade/mask; optionally depth source if separately supplied | No implied 3D depth |

Translucent occlusion, fog/cloud attenuation and offscreen behavior differ by rendering stage; they remain separate explicit technical tests, not implied parity from a competitor. Image-derived flares cannot represent an occluded offscreen light, but authored sun/hero light paths can.

## Render pass, layer and UI rules

- Extract from **pre-Glint HDR** input; ideally after the scene lighting effects intended to contribute and before tone map. Check whether the selected pass already contains translucent emissive/VFX; do not advertise untested coverage.
- Composite in an appropriate HDR domain so source radiance and camera exposure produce consistent results; test UE's ACES-tonemap and alternate settings, bloom, TSR and dynamic screen percentage.
- UI/UMG is not a flare source; scene capture/render-target/multi-view handling may need opt-in to prevent expensive incidental camera passes.
- Respect editor view toggles and proper lifetime boundaries; no frame-to-frame resource state leaked across world teardown or camera cuts.

## Engine integration research

- UE5.8 API: [ISceneViewExtension::SubscribeToPostProcessingPass](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/ISceneViewExtension/SubscribeToPostProcessingPass), [EPostProcessingPass](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/ISceneViewExtension/EPostProcessingPass).
- Verify `FPostProcessingPassDelegateArray` and the 5.8 overload with `const FSceneView&`; older signature is deprecated. Build against the *actual installed* 5.8 SDK and test on 5.8.0–5.8.3.
- UE5.8 [Auto Exposure](https://dev.epicgames.com/documentation/en-us/unreal-engine/auto-exposure-in-unreal-engine) / [PreExposure](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FSceneViewStateInterface/GetPreExposure) before setting thresholds.

## GPU performance principles

- GPU-resident candidate detection and matching; no synchronous readback, no CPU Actor creation for automatic candidates.
- Fixed/bounded array capacities; fallback if overflow; no costly sort of every bright pixel on CPU.
- Batched instanced elements, texture atlas or bindless strategy after benchmarking; bounded per-family shader permutations.
- Single shared dirt response per camera; avoid full-screen blur for every flare.
- Early skip if auto disabled and no authored sources; with auto enabled but no bright pixels, use early GPU out/cull downstream work as practical (an initial tiny bright-pass still has a cost).
- Performance diagnostics break out detection, field, candidates, authored elements, temporal, dirt and final composite.

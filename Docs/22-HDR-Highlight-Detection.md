# 22 — HDR Highlight Detection and Automatic Source Extraction

[← Index](README.md) · [21 — Vision](21-Automatic-Lens-Vision-and-Workflows.md) · [24 — Hybrid pipeline](24-Automatic-and-Authored-Rendering.md) · [26 — Temporal/Exposure](26-Exposure-Temporal-and-Renderer-Compatibility.md)

> **Status:** Proposed compute/RDG design, **not** benchmarked or confirmed implementable through a shipping-safe UE5.8 plugin callback. Use the R0 technical spike to verify availability of the needed HDR input, view constants and render pass lifetime.

## The fundamental input is bright HDR *scene content*

Detect before tone mapping and before Glint's own contribution. Tone mapped white pixels do not retain the information needed to distinguish a slightly bright white wall from a very intense light source. The extraction signal should include visible HDR emissive and specular reflections that appear in scene color at the selected pipeline stage. Whether translucency, volumetric effects and temporal reconstruction are already represented depends on the injection point, so record this in the spike.

Unreal applies **pre-exposure** to scene color using view exposure; thresholding raw RGB without compensating for this may make a flare flicker or react unpredictably to eye adaptation. Study [UE5.8 Auto Exposure](https://dev.epicgames.com/documentation/en-us/unreal-engine/auto-exposure-in-unreal-engine) and [FSceneViewStateInterface::GetPreExposure](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FSceneViewStateInterface/GetPreExposure) before finalizing the numerical representation.

### Proposed detection pipeline

```text
[HDR Scene Color — before Glint]                 [Camera/Lens Settings]
                |                                        |
        [Correct pre-exposure]  <-------------------------+
                |
       [Threshold + soft knee + local contrast]
                |
        [Reduced-resolution bright pass]
                |
        +-------+---------------------------+
        |                                   |
[Image-space flare field]               [Candidate detector]
 ghost / reverse / warped / streak       tiled peaks + region stats
        |                                   |
        |                          [Merge + prioritize + top K]
        |                                   |
        |                          [Temporal source matching]
        |                                   |
        |                          [Glint element composition]
        +----------------+------------------+
                         |
               [HDR flare composite]
                         |
                  [UE tonemap]
```

## Candidate GPU algorithm: feasibility proposal

1. **Threshold:** Convert scene RGB to linear luminance (Rec.709-like weights only if confirmed appropriate for the actual working color space). Compute a smooth highlight mask above a selected threshold. Preserve weighted source RGB for color.
2. **Spatial reduction:** Evaluate the HDR input at roughly quarter/eighth resolution, experimenting with max preserving vs weighted average. A hot single-pixel sparkle must not vanish; a huge window of white must not become an undesired explosion of light.
3. **Tile statistics:** Each workgroup emits bounded maxima, weighted spatial centroid, energy, approximate radius/area, color and optional first/second spatial moments. Keep GPU buffers fixed/pooled.
4. **Peak suppression/region merge:** Merge nearby peaks that belong to one source; avoid treating an elongated neon strip as dozens of identical lightbulbs. A basic tile-based nonmaximum suppression with variable merge radius is the first implementation to evaluate; full connected-component labeling is an optional more expensive prototype.
5. **Scoring:** Rank candidates by a weighted combination of peak radiance, integrated brightness, local contrast, source area and user-authored response curves. Distinguish small strong sources from broad bright surfaces. Mark candidates as Point, Small Area, Elongated or Broad Area, with an Unknown fallback.
6. **Budget:** Keep only the strongest `K` complex candidates (example research presets might start at 4, 8, or 16; these are *not validated defaults*). Remaining highlights continue contributing to the low-cost image-space field.
7. **Temporal matching:** Match candidate centroid/extent/color to previous per-view GPU history with prediction/hysteresis; don't do GPU-to-CPU readbacks just to send instances back to the GPU. Reset or invalidate on camera cuts and resolution changes.
8. **Emit descriptors:** `UV, Radiance, Color, Area, Extent, Orientation, Classification, Confidence, PersistentID, Fade, LensTemplateIndex, SourceKind` feed the normal Glint element renderer.

**Key decision:** shared renderer data structures for automatic and manual candidates, but **different source visibility logic**. The automatic candidate is derived from visible pixels, so its source should not be independently depth-tested as if it were a known world position. A manual world source needs world-space depth/occlusion handling.

## Exposure and sensitivity policies

We need two user-facing modes and an optional combined safeguard:

- **Camera-relative sensitivity:** compare to the view's exposed HDR brightness. Results track what feels bright within the current exposure; practical for games, but can exaggerate modest lights after a dark adaptation.
- **Scene-referred sensitivity:** compare to a fixed scene radiance threshold after removing pre-exposure. Stable under camera exposure transitions; results can be weaker in stylized/non-photometric lighting.
- **Camera-relative plus minimum absolute brightness:** suggested starting default, pending usability tests. Require source radiance to exceed a minimum threshold AND allow relative brightness/local contrast to influence visibility. This prevents a uniformly gray dark scene from suddenly generating bright ghosts simply because exposure increased.

Expose artists to a **Sensitivity** control and a few display-friendly EV/stop-relative advanced parameters; retain numerical technical controls for developers. Do not describe arbitrary linear scene values as physical nits unless their unit conversion is actually defined and verified.

## Threshold and region parameters (proposed)

| Setting | Role | UX |
|---|---|---|
| Sensitivity | Master artist-facing detection response | Basic |
| Threshold Units | Camera-relative / Scene-referred / Hybrid | Advanced |
| Brightness Threshold | Minimum candidate luminance | Basic tooltip + advanced value |
| Soft Knee | Gradual onset near threshold | Advanced |
| Minimum Local Contrast | Exclude broad uniformly bright scene areas | Advanced |
| Minimum Highlight Size | Reject unstable isolated noise | Advanced |
| Maximum Highlight Size | Downweight/skip enormous broad sources | Advanced |
| Area Influence | How integrated area influences score | Advanced |
| Merge Radius | Group nearby pixels into one source | Advanced |
| Peak Suppression | Limit repeated candidates from one region | Advanced |
| Maximum Complex Flares | Top-K high-detail compositions | Basic |
| Minimum Lifetime/Hysteresis | Prevent source flicker from threshold crossings | Advanced |
| Color Sampling | Weighted HDR color vs neutral white vs artistic tint | Advanced |
| Exclusion Mask | Optional artist-authored screen/material masking path | Research |

## Extended versus point highlights

A hot pixel, a long neon tube and an open sunlit doorway should not be treated identically. Color/energy/area covariance provides useful approximations:

- **Point:** crisp glint/starburst/ghost.
- **Small area:** softer bokeh/ghost and broad glow.
- **Elongated:** strong streak response with a restrained ghost budget.
- **Broad area:** low-frequency bloom-like light scattering, fewer or no sharp independent ghosts.

Do not claim full object recognition. Only classify highlight geometry derived from the image. Very bright sun discs can require a manually authored directional source if exposure/sky materials don't produce a useful bright image region.

## Alternatives and research decision

| Approach | Advantage | Drawback | Recommendation |
|---|---|---|---|
| Sample bloom mip and warp | Efficient, proven screen-space precedent | Tied to UE bloom resource lifetime/settings; limited source identity | **Research** reuse; not a hard dependency |
| Glint-owned reduced HDR bright pass | Stable independent controls, better candidate stats | Extra downsample/reduction work and memory | **Baseline design** |
| Compute local maxima/top-K | Rich source identity and authored element compatibility | Merging and flicker are difficult; per-view buffers | **Prototype early** |
| GPU connected-component labeling | Better isolation for long/broad emitters | Complex GPU work and costly iterations | Optional advanced experiment |
| CPU pixel readback | Easy to inspect/debug | Synchronization/latency and scaling cost | **Reject as runtime default** |
| Discover Unreal lights instead | Easy source positions and metadata | Misses emissive/specular highlights and requires source semantics | Optional supplemental authored path only |

## Debugging and acceptance fixtures

Provide raw HDR luminance preview, threshold mask, selected peaks, merged regions, ranked sources, chosen full-detail candidates, rejected reasons, temporal IDs and the final field pass as selectable editor overlays. The debugging UI is part of the feature, not a stretch goal: artists must understand *why* a flare did or didn't appear.

Fixture sequence: dark hallway with one small light; dozens of car headlights; chain of long neon strips; reflective water; single-pixel sparkles; large bright white facade; sun partly behind leaves; warm-to-cool transition; exposure compensation sweep; camera sprint/cut; abrupt 16:9→21:9 viewport change. Candidate count, cost and stability should be measured in a packaged Win64 executable.

## Source precedents

- [Unity HDRP screen-space lens flares](https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4017.0/manual/shared/lens-flare/Override-Screen-Space-Lens-Flare.html): bright-image/bloom source, regular/reverse/warped/streak output.
- [Video Copilot Optical Flares features](https://www.videocopilot.net/products/opticalflares/): automatic luminance tracking with threshold and resolution.
- [Flares OFX pixel-driven](https://palmpixel.eu/flaresofx-documentation/openfx-integration/plugin-features/pixel-driven-flares/): threshold, per-region ranking, source area weighting and starburst-only downgrade.
- [Unreal emissive material](https://dev.epicgames.com/documentation/unreal-engine/using-the-emissive-material-input-in-unreal-engine): bright emissive feeds bloom.

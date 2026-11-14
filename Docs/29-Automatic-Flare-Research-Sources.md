# 29 — Source Audit: Automatic Flares, Optical Simulation and Unreal Integration

[← Index](README.md) · [10 — General research](10-Research-and-Decisions.md) · [27 — Decisions](27-Research-Backlog-and-Technical-Decisions.md)

Research gathered **2026-11-12**. These references establish external feature descriptions or technical precedents, **not** that Glint has implemented them, nor that another plugin's performance or internal algorithm has been inspected. Record a licensed hands-on investigation separately when one occurs.

| Reference | Confirmed useful fact | What it does **not** establish |
|---|---|---|
| [Epic UE5.8 Post Process Effects](https://dev.epicgames.com/documentation/unreal-engine/post-process-effects-in-unreal-engine) | UE's own Lens Flare is image-based; tints, threshold, bokeh and intensity | Custom procedural arbitrary element data or plugin callback viability |
| [Epic UE5.8 Add Post Process Volumes](https://dev.epicgames.com/documentation/unreal-engine/add-post-process-volumes) | Bright emissive can feed bloom and lens flares; practical editor settings | Safe access to internal bloom textures from a separate plugin |
| [Epic UE5.8 Auto Exposure](https://dev.epicgames.com/documentation/en-us/unreal-engine/auto-exposure-in-unreal-engine) | Eye adaptation, histogram and pre-exposure semantics | Per-highlight positions from global exposure histogram |
| [Epic UE5.8 PostProcess view extension](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/ISceneViewExtension/SubscribeToPostProcessingPass) | Postprocess extension callback; new overload with `FSceneView` | Actual render target availability at required stage until built |
| [Epic UE5.8 Cine Cameras](https://dev.epicgames.com/documentation/en-us/unreal-engine/cinematic-cameras-in-unreal-engine) | Filmback, focal length, f-stop, diaphragm blade count, squeeze | Lens flare response that UE creates automatically from these values |
| [Video Copilot Optical Flares](https://www.videocopilot.net/products/opticalflares/) | **Automatic Luminance Tracking** with threshold/resolution; artist element editor and Dynamic Triggering | Exact internal HDR region algorithm or cost of multi-highlight runtime use |
| [Video Copilot original announcement](https://www.videocopilot.net/blog/2010/01/optical-flares-online) | Luminance tracking was a core advertised feature since launch | Exact present-day per-object controls or multi-spot source handling |
| [Unity HDRP Screen Space Lens Flare](https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4017.0/manual/shared/lens-flare/Override-Screen-Space-Lens-Flare.html) | Extracts bright image regions from bloom pipeline; regular/reverse/warped/streak; image-screen effects and authored components can coexist | Direct availability of HDRP implementation in UE; equal look at same cost |
| [Unity URP Screen Space Lens Flares](https://docs.unity.com/en-us/engine/6000.7/manual/visual-effects/lens-flares/lens-flare/post-processing-screen-space) | Current Unity rendering path supports image-based bright-region flare without assigning lights | Algorithm details or UE-specific equivalents |
| [Flares OFX pixel-driven positioning](https://palmpixel.eu/flaresofx-documentation/openfx-integration/plugin-features/pixel-driven-flares/) | Region threshold, candidate ranking, top-K, starburst-only downgrade, area weighting | GPU-only implementation in Unreal or performance on game hardware |
| [Maxon Red Giant Real Lens Flares](https://www.maxon.net/en/product-detail/red-giant/vfx/real-lens-flares) | Lens model, optical movement/morphing, camera aperture/zoom, dirt simulation | Automatic arbitrary scene-pixel detection as the primary workflow |
| [Maxon background-illuminated Schmutz](https://help.maxon.net/rg/en-us/Content/html/06-Real-Lens-Flares_core-projections-schmutz.html) | Dirt can be lit from both explicit light and luminous background | Runtime GPU cost for a full dynamic UE scene |
| [REAL FLARES Fab listing](https://www.fab.com/listings/7d4eddfd-4be4-4d6b-a9d2-07aa3c11d30c) | Vendor advertises source-actor/component workflow, RDG sprite/procedural elements, depth/translucency/fog behaviors | Independently tested performance or equivalent automatic bright-pixel region detection |
| [Lee/Eisemann real-time lens flares (2013)](https://graphics.tudelft.nl/Publications-new/2013/LE13/PracticalReal-TimeLens.pdf) | Published first-order matrix approach for fast plausible lens ghosts | One-click plugin production readiness, measured UE5.8 latency |
| [Hullin et al. physically based flares (2011)](https://light.informatik.uni-bonn.de/papers/HullinEtAl-LensflareRendering-SIGGRAPH2011.pdf) | Optical simulation research including ghosts, coatings, aberrations | Low per-camera runtime cost at many bright highlights; link availability varies |
| [Efficient tile-based ghost rendering (2023)](https://www.sciencedirect.com/science/article/pii/S0097849323001486) | Tiled accumulation addresses ghost overdraw | That Glint must implement ray tracing or that method is required for v1 |

## Remaining evidence gaps

- Video Copilot's official Luminance Tracking demo confirms the feature, but its public HTML is video-centric; a hands-on UI audit is required for exact thresholds/defaults and multiple-light handling.
- REAL FLARES source-based plugin has substantial documented vendor features; this research did not examine a licensed build, source, profiler captures or its complete user manual. Avoid asserting it **cannot** do any specific auto-detection feature solely because the public listing emphasizes sources.
- UE5.8 `FSceneViewExtension` and HDR input extraction must be built/tested before claiming shipping support; 5.8.0–5.8.3 compilation and packaged D3D12/SM6 are the required proof.
- Optical papers are implementation inspiration, not licensed or redistributable shader/source material. Reimplement originally and evaluate dependencies/licenses before reuse.
- The above evidence does not prove that candidate extraction can supply complex procedural Glint elements for dozens of real-time sources within a specific GPU budget. That remains the top technical risk.

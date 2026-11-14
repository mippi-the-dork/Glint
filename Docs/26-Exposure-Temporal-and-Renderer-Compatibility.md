# 26 — Exposure, Temporal Stability, Camera Compatibility and Render Integration

[← Index](README.md) · [22 — Detection](22-HDR-Highlight-Detection.md) · [24 — Pipeline](24-Automatic-and-Authored-Rendering.md) · [28 — Tests](28-Automatic-Mode-Testing-and-Release-Gates.md)

## Problem: flare brightness and stability are view properties

With automatic detection, a camera's exposure, rendering resolution, temporal upscaling and filmback directly affect **which highlights exist** and how flares appear. A convincing still frame can nevertheless be unusable in a game if a tiny bright reflection repeatedly crosses the threshold and creates huge flashing ghosts.

## HDR and pre-exposure research

UE pre-exposure multiplies scene color to manage HDR range based on prior frame exposure. The same physically or artistically bright scene source can therefore show different stored RGB values across frames. Glint must derive its detection threshold in an explicitly defined domain and use exposure-aware conversion, rather than thresholding whichever input texture value is convenient.

Define a tested exposure policy in three spaces:

1. **Scene-referred normalized:** undo the view's pre-exposure multiplier to compare against a stable scene reference.
2. **Camera-relative:** compare brightness after current camera exposure semantics without relying on destructive tonemapping.
3. **Hybrid:** absolute minimum luminance with a contrast/camera-relative factor for art direction.

Measure with fixed scene radiance and varied manual EV100, auto exposure compensation, local exposure and sudden transitions; verify that artifacts follow the chosen policy. Unit-tested conversion logic plus image-golden tests are required. The engine's 64-bin histogram is designed for global auto-exposure estimation, **not** identifying individual bright spots, so do not use its global scalar as a replacement for Glint's spatial source extraction.

References: [UE5.8 Auto Exposure](https://dev.epicgames.com/documentation/en-us/unreal-engine/auto-exposure-in-unreal-engine), [FSceneViewStateInterface::GetPreExposure](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FSceneViewStateInterface/GetPreExposure).

## Temporal stability requirements

- **Onset:** a genuine bright flash may appear quickly; don't add universal slow fades that make gameplay feedback lag.
- **Hysteresis:** a candidate should have different enter/exit thresholds, so a reflection hovering near threshold doesn't appear/disappear repeatedly.
- **Confidence persistence:** optional short grace period for subpixel motion; avoid preserving ghosts after a source is fully blocked or camera teleports.
- **Spatial matching:** track a small number of candidates across frames by UV, size/color and expected movement; consider motion vectors if safely available and useful, otherwise use a bounded nearest-neighbor/weighted match.
- **Disocclusion:** if an opaque object suddenly reveals a bright region, allow its flare to appear with a controllable response while suppressing one-frame overshoot.
- **Camera cuts:** reset or explicitly remap previous candidate history, flare intensity and source IDs; no long streaks sliding from an unrelated previous camera.
- **Resolution changes:** reinitialize or correctly transform candidate coordinates and history when screen percentage / aspect / render target size changes.
- **Pause/Sequencer:** deterministic animation with an authored timeline or explicit time input; don't let temporal noise depend on an arbitrary last real-time frame.
- **Dynamic rendering:** test TAA, TSR and alternate AA paths; a source extracted before temporal reconstruction can alias, while one extracted after may inherit temporal ghosting. The correct stage must be measured.

## Important camera categories

| View | Required for v1? | Research/behavior |
|---|---|---|
| Editor level viewport | Yes | Native preview toggle and clear diagnostics; not dependent on asset tab being open |
| Game CameraActor / CameraComponent | Yes | Global lens profile plus camera override; packaged build |
| CineCameraActor | Yes | Focal length, filmback, f-stop, blades, anamorphic squeeze response if enabled |
| PIE / standalone packaged | Yes | Same source selection and quality logic, independent lifetime |
| Camera cuts / Sequencer | Yes (basic) | Reset history and permit lens asset changes; full Sequencer UI integration may grow |
| Split-screen | Validation target | Independent lens/exposure/candidates/history per view; no cross-view leaking |
| SceneCapture2D / render targets | Validation target | Default opt-in/off policy to avoid incidental expense; define resource lifetime |
| Movie Render Queue/Graph | Validation target | Multi temporal samples, camera cuts, output HDR/OCIO and no repeated candidate generation artifacts |
| Stereo / VR | Beyond v1 unless verified | Avoid eye mismatch, discomfort, and double flare/split calibration |
| Forward/mobile/path tracer | Beyond v1 unless verified | May lack required scene signals or stable injection stage |

UE5.8 CineCamera settings: [Cinematic Cameras](https://dev.epicgames.com/documentation/en-us/unreal-engine/cinematic-cameras-in-unreal-engine). Captures: [SceneCaptureComponent2D](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/USceneCaptureComponent2D). Movie Render Graph temporal samples/cuts: [Nodes](https://dev.epicgames.com/documentation/unreal-engine/movie-render-graph-nodes-in-unreal-engine).

## Postprocessing order and multiple effects

Test against HDR scene color, native UE lens flares, bloom standard/convolution, local exposure, depth of field, color grading, TAA/TSR, anamorphic lens distortion, vignette and tone mapping. Compatibility means producing sensible output and *not* double-writing a resolved render target; it does not mean Glint owns UE's entire postprocess chain.

The exact integration callback and HDR render resources are **unverified for this plugin**. The UE5.8 post-process view extension API is the likely extensibility point; prove the selected callback receives suitable textures on D3D12/SM6 without an Engine fork. Check the **newer overload** taking `const FSceneView&`, not the deprecated 3-parameter overload. [API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/ISceneViewExtension/SubscribeToPostProcessingPass).

### Avoid double flaring

Expose a native setting: `UE Image-Based Lens Flares: Do Not Change / Warn If Enabled / Assist Disable For This Camera`. Default to **Warn**, never silently change global project/volume settings. Show a short warning and link to the relevant lens flare controls when both are active. Glint's preview can independently toggle Native vs Glint contributions.

## Visual comfort and accessibility

Optical flares may affect visual clarity, aiming and fast movement. Include controls for maximum overall intensity, max flare screen coverage, flash onset, shimmer/flicker amplitude and a runtime intensity multiplier. A **Reduced Flashes** preset should tone down transient strobing and rapid brightness changes while retaining artistic style. Avoid unsupported medical safety guarantees; treat this as an accessibility/comfort feature with visual user testing.

## Fog, translucency and reflections

- Screen-derived flares represent what is visible in the sampled color image; if fog/translucency hasn't been composited when detection runs, the source will not reflect those effects correctly.
- Screen-derived specular highlights can flicker as surfaces/cameras move; local contrast and temporal hysteresis address this without requiring the geometry identity.
- An authored light can be behind translucent/fog media and needs an explicit attenuation model; depth alone is insufficient.
- Screen-space reflections and Lumen may contribute bright pixels differently under different camera angles and temporal settings. Verify rather than relying on metadata from scene actors.

## Candidate stability debug overlays

Display per-camera `Exposure / PreExposure / Detection Domain / Threshold / Candidate # / UID / Time Alive / Match Confidence / Source Class / FieldContribution / Dedup Reason / Final Intensity` and a breakdown of per-stage GPU cost. These can live in a native Glint viewport visualizer and inside the asset editor.

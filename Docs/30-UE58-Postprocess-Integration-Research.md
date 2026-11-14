# 30 — UE5.8 Postprocess Integration: Findings Before Implementation

[← Documentation index](README.md) · [26 — Exposure and temporal](26-Exposure-Temporal-and-Renderer-Compatibility.md) · [31 — Technical acceptance test](31-Camera-Workflow-and-Renderer-Acceptance-Plan.md)

> **Research date:** 2026-11-12. This report verifies documented Unreal Engine APIs; **none of the Glint rendering paths have been implemented, benchmarked, cooked or tested.** Confirm 5.8.0–5.8.3 behavior using installed engine headers/source and a real plugin.

## 1. Critical distinction: blendable locations versus view-extension callbacks

Unreal 5.8 documents `EBlendableLocation::BL_SceneColorBeforeBloom`, which places a **post-process material** before bloom. This is an especially interesting stage for Glint, because bright extraction can operate in linear HDR and Glint can be deliberately included in or excluded from subsequent bloom.

But the **Scene View Extension** `EPostProcessingPass` enum is **not an identical list of blendable locations**. The documented enum contains `BeforeDOF`, `AfterDOF`, `TranslucencyAfterDOF`, `SSRInput`, `ReplacingTonemapper`, `MotionBlur`, `Tonemap`, `FXAA`, `SMAA`, `VisualizeDepthOfField`; it does **not** explicitly contain `BeforeBloom`. Consequently, do not design Glint assuming a `BeforeBloom` SVE callback exists. Verify which existing callbacks provide a useful HDR input, how to insert the RDG work at the desired moment, and what alternative is needed for a genuine pre-bloom result.

**Integration rule:** prefer engine-supported callback/hook(s) with no engine fork, prove HDR input/output, TAA/TSR behavior, native bloom coexistence, and no writing to the input texture while sampling it. Separate the *detection* stage from the *composite* stage if required, but cap additional passes/resources and measure.

### API compatibility gotcha

`ISceneViewExtension::SubscribeToPostProcessingPass` has a deprecated **three-parameter** overload that Epic documents as no longer called. The current **four-parameter** overload includes `const FSceneView& InView`. Use the latter and test callbacks in editor, PIE and packaged builds. If the callback is the final pass and the `OverrideOutput` texture is provided, it must be written. Callbacks should register only when they produce GPU resources; no unnecessary work when Glint is disabled.

### Resolution is part of the design

UE's TSR documentation states `Scene Color Before Bloom` is at **display resolution** when using TSR/TAAU, whereas scene color before/after DOF is at **render resolution**. Candidate detection should typically operate on an explicit downsampled image, and source UVs should use a resolution-independent convention. Test source centering, aspect ratio, dynamic resolution, 1080p/1440p/4K, viewport resizing, editor screen percentage and both temporal and non-temporal AA.

**Sources:**
- [EBlendableLocation, UE5.8](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/EBlendableLocation)
- [EPostProcessingPass, UE5.8](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/ISceneViewExtension/EPostProcessingPass)
- [SubscribeToPostProcessingPass, UE5.8](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/ISceneViewExtension/SubscribeToPostProcessingPass)
- [Temporal Super Resolution, UE5.8](https://dev.epicgames.com/documentation/unreal-engine/temporal-super-resolution-in-unreal-engine)
- [Post Process Material User Scene Textures, UE5.8](https://dev.epicgames.com/documentation/unreal-engine/post-process-material-user-scene-textures-in-unreal-engine)

## 2. HDR thresholds: three different user expectations

Unreal's **pre-exposure** changes the numerical values stored in HDR scene color, using the previous frame's exposure. A fixed numerical threshold applied to whatever texture happens to be available will not produce a stable, intuitive lens response.

Glint should explicitly separate:

- **Scene-referred sensitivity:** a genuinely high scene luminance qualifies even when the camera's adapted exposure is low. Good for source-intensity consistency and authoring.
- **Camera-relative sensitivity:** a highlight qualifies relative to the camera's apparent brightness and exposure. Good for a cinematic camera response, but can make very ordinary lights flare in deep darkness.
- **Hybrid sensitivity:** a scene-referred floor plus camera-relative strength. This is the leading **proposed default** because it can avoid both bright snow generating thousands of flares and night scenes becoming inert. Its numerical curve needs visual evaluation.

The editor must provide understandable artist controls (e.g. `Sensitivity`, `Highlight Contrast`, `Minimum Source Size`, `Maximum Full Flares`) and an Advanced HDR section for the exact threshold domain. It must show how many candidates are rejected by each rule. This is a design proposal, not a verified correct physical model.

**Source:** [Auto Exposure and Pre-Exposure, UE5.8](https://dev.epicgames.com/documentation/en-us/unreal-engine/auto-exposure-in-unreal-engine).

## 3. Built-in bloom should remain independent

Epic describes low-resolution multi-scale bloom, pre-bloom materials, and an independent built-in image-based lens flare implementation. Glint should **not depend on UE's built-in lens flares or the bloom result as its only source**, since both can be disabled. Detection from the original suitable HDR scene buffer is the intended default.

Test two composite looks: (A) Glint contributes before bloom, with the resulting diffuse bloom; (B) Glint added after bloom but before tone mapping, preserving sharper authored rays. Compare photographs, overdraw and HDR color stability; choose the default from evidence. Warn when native UE lens flares are enabled, but never silently modify project or volume settings.

**Sources:** [Bloom, UE5.8](https://dev.epicgames.com/documentation/unreal-engine/bloom-in-unreal-engine), [Post Process Effects, UE5.8](https://dev.epicgames.com/documentation/unreal-engine/post-process-effects-in-unreal-engine).

## 4. Defining eligibility for an automatic flare

Qualifying pixels must become meaningful, bounded lens responses. They should not map directly to one complex Glint Flare per pixel or Unreal Actor. Use a downsampled HDR bright pass and prioritize candidates based on intensity, area, local contrast, extent, color and screen position. Use explicit non-maximal suppression / region merging and hard `Top-K` limits. Track visible candidates with stable IDs and hysteresis; avoid synchronous GPU→CPU readback.

The strongest full sources receive the full custom iris/ghost stack; weaker ones can contribute to one shared image-space field (a streak / scattering layer). **Long LED strips** and **white sky / snow fields** require different behavior from pinpoint headlights; optional area/aspect classification should be tested, not promised as universally accurate. An author should be able to debug mask → peaks → merged regions → selected candidates → field/full effects.

**Research precedent:** [Flares OFX Pixel-Driven Positioning](https://palmpixel.eu/flaresofx-documentation/openfx-integration/plugin-features/pixel-driven-flares/) documents region thresholding, source sorting, cap on rendered sources, and a starburst-only fallback.

## 5. Camera/view ownership and fallback behavior

Ensure all automatic data is scoped to the individual rendered **view**, not the game world or current editor viewport. Distinct views can have different exposure and lens profiles; their candidate lists, temporal histories and source IDs must not leak. Camera cuts should explicitly invalidate history. SceneCapture2D should be opt-in/off by default for predictable runtime cost; split screen and Movie Render Graph are validation cases, not assumed fully supported before a spike.

UE5.8's documented viewport-type activation mechanisms include different editor, PIE and game-primary cases. We should implement Glint-specific activation, independent of requiring the Flare Editor open, without imposing a dependency on Virtual Production Utilities.

**Source:** [Viewport-type Scene View Extension activation reference, UE5.8](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/VPUtilities/UVPRenderingBlueprintLibrary/GenerateSceneVie-).

## 6. Release boundaries

- Runtime: packaged Win64 deferred renderer, DX12/SM6 first; validate other RHI/renderers before advertising support.
- Editor: original native Glint Lens and Glint Flare asset editors; no standalone third-party-looking authoring UI.
- No Engine source edits or experimental Engine plugin dependencies required to ship.
- No frame-to-frame GPU readback stalls, thousands of transient Actors, or unbounded per-highlight passes.
- No promises about physically accurate lens behavior, volumetric/translucent attenuation or VR until measured.

**R0 exit:** show no-source automatic flare in a cooked executable, correct exposure/TSR behavior, bounded cost, and at least one candidate driving original Glint primitives. See [31](31-Camera-Workflow-and-Renderer-Acceptance-Plan.md).

## Evidence from Froyok's 2021 UE4.25 custom postprocess

Froyok's engine-source modifications illustrate why private renderer function signatures and exact passes should not be hardcoded for Glint distribution. The GPU work remains useful: independent source threshold, selective weighted HDR reduction, image-space ghosts, halo and procedural glare, multi-pass final composition. Their 0.65 ms custom 1080p result is not UE5.8 evidence.

**Do not infer** that an SVE callback fires at UE5.8's `BL_SceneColorBeforeBloom` material location. Verify the real callback and input. Build tests around per-view/render-thread ownership, shader packaging, viewport subrects and bloom/TSR-off paths. More: [38](38-Froyok-Custom-UE-Lens-Flare-Audit.md).

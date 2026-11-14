# 34 — Lens-Flare Approaches in Other Engines

[← Documentation index](README.md) · [35 — Shipped games](35-Shipped-Games-and-Visual-Design-Lessons.md) · [36 — Research](36-Rendering-Papers-and-Architectural-Lessons.md) · [37 — Decisions](37-Cross-Engine-Findings-and-Prototype-Gates.md)

**Status:** Public documentation audit, 2026-11-12. Describes documented capabilities, not source-level performance verification or claims that the approaches can be directly imported into Unreal Engine 5.8. Glint must use original code and assets. Engine documentation applies to the documented versions; vendor defaults can change.

## 1. Unity HDRP/URP — two methods are complementary

Unity documents two different flare workflows that may coexist in a scene:

- **Screen Space Lens Flare:** responds to emissive objects, bright visible image regions, and highlights. The volume override supports regular, reversed and warped flare ghosts, anamorphic streaks, chromatic aberration, bloom mip selection, sample count, vignetting, and independent effect multipliers. Automatic image behavior is attractive for automotive and reflective environments. It runs as image/GPU postprocessing rather than maintaining a world source for every highlight.
- **Lens Flare (SRP):** source-oriented reusable Data assets, with explicit source component, depth-based occlusion, optional environment occlusion, light-shape attenuation, offscreen support and screen-edge/distance curves. It provides precise artistic sources that cannot be inferred from a currently visible bright pixel.

**High-value findings for Glint:**

1. Keep Automatic and Authored modes complementary rather than making authored attachment mandatory. Hybrid mode should explicitly prevent double flare contribution around the same source.
2. Investigate sampling an existing **bloom mip pyramid** for image-driven ghosts: Unity exposes Bloom Mip Bias and selectable reduced streak resolution. In UE5.8 it must be conditional on resource availability, pre-exposure compatibility, and rendering stage; maintain Glint-owned extraction as an independent path, including with Unreal bloom disabled.
3. A clear cost/quality control for **streak resolution** is worthwhile. Unity documents half/quarter/eighth options, not only full/half.
4. Source-emitter shape matters. For authored spotlights, a directional/backside attenuation curve should avoid light appearing to flare through its own fixture.
5. A full Flare asset benefits from **repeated elements** (Uniform, Curve, Random), deterministic seeds, and nested reusable sub-assets. Expose a simple Repeater section with advanced per-instance overrides; compile to flat instances for the runtime.
6. Apply distance fading, source shape, screen-edge behavior and occlusion **before** expensive element expansion when possible.

### Unity authoring features relevant to Glint

| Documented Unity property/concept | Glint implication | Commitment |
|---|---|---|
| Image, Circle, Polygon, Ring and nested Lens Flare Data element | Native reusable element stack and group composition | v1 editor structure; exact type naming independent |
| Multiple Elements: Uniform, Curve, Random; size/color/rotation/position variation | Generic **Repeater** modifier rather than duplicating dozens of elements | v1 authoring capability |
| Lens Flare Data as an element, recursive with depth safety | Nested Glint Flare compositions must reject cycles and have finite cook flattening | v1 safety |
| Radial Edge Curve and distortion relative to center/source | Edge deformation as a shared behavior; not only element position | v1 basic + P1 higher-fidelity |
| Depth occlusion radius, offset, sample count, remap curve | Transparent and soft visibility policy, depth sample bias, artist response curve | v1 authored opaque baseline; advanced P1 |
| Environment occlusion: volumetric cloud/fog/water | Avoid claiming universal occlusion from scene depth alone | explicit compatibility matrix |

**Sources:** [Unity choosing lens flare type](https://docs.unity.com/en-us/engine/6000.7/manual/visual-effects/lens-flares/lens-flare/choose-a-lens-flare-type), [screen-space override reference](https://docs.unity.com/en-us/engine/7000.0/manual/visual-effects/lens-flares/lens-flare/reference-screen-space), [Lens Flare Data asset](https://docs.unity.com/en-us/engine/6000.5/manual/visual-effects/lens-flares/lens-flare/asset), [source component and attenuation](https://docs.unity.com/en-us/engine/6000.3/manual/visual-effects/lens-flares/lens-flare/srp-reference), [HDRP source occlusion](https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4017.1/manual/shared/lens-flare/lens-flare-srp-reference.html).

## 2. CRYENGINE — authored optics and partial visibility

Crytek describes an **element-based**, intentionally art-directable Optical Flare System. Important elements include Ghost, Multi Ghost, Glow, Chromatic Ring, Iris Shafts, Volumetric Shafts and Camera Orbs. The editor offers a library/tree, selected-element properties, live preview, and reusable groups. Position can be fixed to the light, mirrored about screen center, or anchored to the center; independent X/Y factors and perspective/size variation are available.

Most valuable difference from simpler flare systems: **Occlusion Interaction** is not merely a uniform brightness fade. Crytek documents *eclipse-like* responses in ghosts/orbs and an iris-shaft distribution response to partly occluded light. Its camera-orb/dirt features are separately described as potentially overdraw-heavy.

### What Glint should learn

- **Source-visibility fraction is not enough** for every shape. A small visibility distribution around the light can modify the shape of selected elements. Make a high-quality optional **Shape Response** mode for suitable element families; do not require it for all flare objects or all automatic image sources.
- Provide common **Position Axis**, **Perspective**, **Screen Edge Size/Intensity**, **Correct Aspect Ratio** settings. Be careful about screen aspect changes, FOV, anamorphic squeeze, render resolution and editor preview sizing.
- Distinguish **physical-looking ghosts and camera dirt** from **light shafts in the atmosphere**. Volumetric shafts and diffraction spikes are not the same optical event, even if both look like rays.
- Group properties should have clear inheritance/local-override markers, avoiding the opaque parent-child enable behavior Crytek notes in its manual.
- Treat large camera orbs as an **overdraw-sensitive** quality class; keep element area/texture coverage visible in statistics.

**Sources:** [CRYENGINE Optical Flare System](https://www.cryengine.com/docs/static/engines/cryengine-5/categories/23756816/pages/36868680), [Lens Flare Editor](https://www.cryengine.com/docs/static/engines/cryengine-5/categories/23756816/pages/36869088).

## 3. Unreal Engine's stock image-based flare — a useful baseline

UE5.8's Post Process documentation identifies a bright-image-based lens flare, threshold, bokeh texture and size, intensity, tint and position-specific tints. Epic warns that increasing bokeh radius increases cost proportional to radius squared. This reinforces the need to make Glint's cost independent of an artist casually creating giant expensive circles.

**Glint action:** keep a stock-Unreal comparison scene and measure two scenarios: Glint-only vs stock effect with matching sensitivity/style; and accidental double activation. Provide clear setup guidance and a diagnostic when built-in lens flares are simultaneously enabled. Never modify unrelated project post-process settings without consent.

**Source:** [UE5.8 Post Process Effects](https://dev.epicgames.com/documentation/unreal-engine/post-process-effects-in-unreal-engine?lang=en-US).

## 4. Godot — a lesson in lighting/dirt consistency

Godot's documented Glow effect is a screen image process with HDR thresholds and multiple blurred levels, plus an optional **glow map** used for camera-space lens dirt/scratches. This is an economical shared-camera solution that does not need one dirt sprite per source. Godot documents per-platform/per-renderer feature differences and upscaling tradeoffs.

**Glint action:** one camera-level dirt accumulation; tie it to flare illumination but expose independent controls for dirty camera optics vs broad bloom; document renderer capabilities rather than silently pretending the same path works on all platforms. Godot's Glow is not evidence of a complete built-in ghost/iris composer.

**Sources:** [Godot environment and post-processing](https://docs.godotengine.org/en/stable/tutorials/3d/environment_and_post_processing.html), [Environment class](https://docs.godotengine.org/en/4.6/classes/class_environment.html).

## 5. Three.js and Babylon.js — the inexpensive source-driven reference

Both offer classic lens sprites linked to an emitter with position along the flare axis. Babylon.js documents camera/light/mesh emitters and author-controlled flare-blocker meshes; Three.js exposes `LensflareElement` size, texture, color and normalized distance. They demonstrate how simple a workable authored system can be, but also underline what is missing: automatic highlight extraction, high-fidelity behavior and performance-aware advanced composition.

**Glint action:** maintain an inexpensive manually registered source path, with an explicit occluder/bias override useful for intentional cinematic art direction. Do not force all hero sources through image-based detection.

**Sources:** [Three.js Lensflare](https://threejs.org/docs/pages/Lensflare.html), [Three.js LensflareElement](https://threejs.org/docs/pages/LensflareElement.html), [Babylon.js Lens Flares](https://doc.babylonjs.com/features/featuresDeepDive/environment/lenseFlare/).

## Comparison and confidence

| Technique | Evidence | Our use |
|---|---|---|
| Automatic image-field lens ghosts | Direct Unity/Unreal documentation | v1 must-have |
| Reusable authorable source effects | Direct Unity/Crytek documentation | v1 must-have |
| Repeated nested elements | Direct Unity documentation | v1 editor behavior |
| Partial occlusion deforms flare response | Direct CRYENGINE documentation | Prototype, optional quality behavior |
| Existing Bloom pyramid reuse | Unity reference, **not proof of UE access** | UE5.8 investigation with fallback |
| Dirt from shared HDR/glow response | Direct Godot/Crytek documentation | v1 shared camera accumulation |
| Source-specific light-shape attenuation | Direct Unity documentation | v1 authored source control |
| Any engine's published performance is transferable to UE5.8 | **No** | Must benchmark our implementation |

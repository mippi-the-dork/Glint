# 44 — New Research-Informed Prototype Gates

[← Index](README.md) · [09 — Roadmap](09-Development-Roadmap.md) · [30 — UE5.8 API](30-UE58-Postprocess-Integration-Research.md)

## R0 — Correctness and packaged runtime FIRST

**R0.1 Unreal integration**: create a standalone, editor-loaded + packaged Win64 UE5.8 plugin with externally mapped HLSL shaders and no source patch. Prove a public callback can access a useful HDR input and composite a flare at a verified stage. Use the **four-argument per-view** `SubscribeToPostProcessingPass` where relevant, not the deprecated legacy overload. Exercise editor viewport, PIE and packaged game. Check bloom/TSR disabled permutations, input view rects and resizing.

**R0.2 Stable automatic bright mask**: move 1–2 pixel emissive lights across subpixel positions at 30/60/120 FPS and several screen percentages; compare naive downsampling vs weighted filtering, peak-preserving detection and soft thresholds. Capture side-by-side video and measured source energy/centroid jitter.

**R0.3 Image field**: independently generate a low-resolution ghost, chromatic offset and streak from HDR pixels with *no* Glint Source component; verify no bloom dependency and distinguish from native UE flares.

**R0.4 Camera candidate path**: GPU source candidates with bounded max, weighted peak/area, persistent IDs and no synchronous GPU->CPU readback; trigger at least one authored Glint element and keep it stable on camera pan.

**R0.5 Real-world source + occlusion**: explicit, directional and offscreen source behavior with camera-dependent depth visibility. Verify behind-wall suppression, edge response, camera cuts and actor-free API.

## R1–R2 — Editor/product foundation

- Native `Glint Lens` and `Glint Flare` Content Browser assets.
- Native Slate editor, live source drag, source angle preview, reference photo overlay, element stack and Details, transactions.
- Shared Lens Profile response controls with local element overrides; no ML or optics prescription required.
- Optional response curve/lookup interfaces that remain compact and serializable (do not implement large capture system yet).
- Clear data ownership per camera/view; render thread and editor lifecycle tests.

## R3 — Optical elements and cheap visual complexity

- Standard compact shapes, sprite masks, radial and streak families.
- Repeater modifier and deterministic seeds.
- Per-element optical-axis response; color/opacity/angle/focal zoom.
- Compare low-cost diffraction mask lookup to purely procedural starburst.
- Artist-facing behavior coverage and Optical Flares mental model, not identical vendor code or naming where unnecessary.

## R4–R5 — Experimental branches (only after baseline)

| Experiment | Proceed only if | Reject if |
|---|---|---|
| Tile renderer | Dense overlapping ghost benchmark is demonstrably overdraw/bandwidth-bound; tile total GPU ms wins including build | Light/small scenes regress or quality loss is too large |
| First-order optics | Observable improvement in ghost position/shape with negligible per-source overhead | Similar look can be attained with simple authored response curves |
| Measured response bake | Real lens capture reliably fits target element shape/behavior and artist sees a workflow benefit | Capture complexity or interpolation quality becomes impractical |
| 3DGS/learned flare | A specific captured hero lens provides fidelity impossible with cheaper representation and measured total runtime cost meets budget | Deformation networks consume ms per light or cannot fit rings |
| Reuse bloom pyramid | Accessible at correct point, deterministic with bloom toggle and stable with editor/TSR | Introduces a dependency on stock bloom, invalid resources or render order |

## Quantitative acceptance plan

- Report **total incremental GPU cost**, not a particular pass's cost. Compare no-Glint baseline and both Glint ON/OFF within the same frame capture.
- Record hardware, renderer, resolution, screen percentage, AA/upscaler, native bloom setting, HDR threshold mode, number of sources and worst-case overdraw.
- Include zero-highlight, single-pixel headlamp, emissive strip, many headlights, static sunlight, camera pan, offscreen light and multiple simultaneous views.
- For temporal stability, collect centroid path error, flare energy time series and visual capture at edges and subpixel motion.
- For deferred Win64 packaged validation, measure 1080p and 4K with two quality tiers rather than assuming the 2021 or 2023 paper's numbers transfer.
- Declare 3DGS/captured model experiment separate from core Win64 runtime; it is not v1.0 required work.

## Documentation and source hygiene

Use primary URL/citation for each research method and label results by reported hardware. Do not copy shaders, source files, photographs or reference assets into the commercial plugin without an explicit licensing review. Glint code/visual assets should be original or properly licensed. Maintain the sources as documentation, not runtime dependencies.

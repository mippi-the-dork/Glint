# 37 — Decisions, New Features, and Prototype Gates from Cross-Engine Research

[← Documentation index](README.md) · [34 — Engines](34-Other-Engines-Real-Time-Lens-Flares.md) · [35 — Games](35-Shipped-Games-and-Visual-Design-Lessons.md) · [36 — Research](36-Rendering-Papers-and-Architectural-Lessons.md) · [09 — Roadmap](09-Development-Roadmap.md)

## Summary

Glint's direction remains **camera-first automatic HDR response**, plus **Optical Flares-style authored elements and optional world sources**, in a standalone Unreal 5.8 plugin. New research sharpens the implementation and editor contract. It does **not** justify adding full physical lens ray tracing or delaying R0 for a sophisticated tile rasterizer.

## Decisions to adopt

| ID | Decision | Priority | Rationale / reference |
|---|---|---|---|
| CX-01 | Implement Generic **Repeater** modifier (uniform, curve, seeded random); nested reusable groups, bounded recursion/cycle diagnostics | v1 | Unity authored Data asset functionality; reduces busywork and compiles to instanced elements |
| CX-02 | Add screen-edge **Radial Deformation** on compatible ghosts, with aspect and anamorphic policy | v1 basic | Unity distortion and CRYENGINE sensor variations; important outside screen center |
| CX-03 | Keep separate **Automatic Image Field**, **Automatic Candidate**, **Authored Source** paths feeding one lens/compositor | v1 | Unity HDRP/URP confirms complementary use cases |
| CX-04 | Expose per-element **Source Visibility Response** with default Uniform Fade and optional Shape Response prototype | v1 baseline + P1 optional | CRYENGINE occlusion interaction; optical nature of source vs sensor artifact |
| CX-05 | **Bloom pyramid reuse** only as conditional optimization with an always-correct standalone Glint HDR extraction path | R0 | Unity documents bloom mip reuse, but no validated UE5.8 access/stage contract |
| CX-06 | Add a built-in stock UE flare **Double-Effect Diagnostic** and setup guidance | v1 | Both built-in image flares and Glint may be active at once |
| CX-07 | Record render-target formats/full-screen reads as part of every performance report | v1 | Guerrilla's bandwidth-focused post-processing findings; RDG costs not just ALU |
| CX-08 | Keep **physical optics baking** extensible in Lens asset data model; do not make physical simulation required | v1 architecture | 2011/2013/2024 research supports increasingly complex optics approximations |
| CX-09 | **No direct assumption of offscreen automatic detection**; authored sources can intentionally remain active offscreen | v1 | Screen-space methods lack offscreen source pixels |
| CX-10 | Make **Lens Context** presets and artist-readable camera optical personality central to first-use UX | v1 content | Crysis 3 visual language lesson |
| CX-11 | Per-camera work quality must respect **gameplay readability** and comfort controls | v1 | Avoid masking gameplay/UI and exposing users to extreme flashing |
| CX-12 | Tile-based rasterization remains a measured **conditional optimization** | post-R0 | Paper shows heavy-overlap win; basic flares may cost less as instanced quads |

## Critical automatic detection edge cases newly emphasized

### Preserve tiny bright sources

**Problem:** average-only HDR reductions can erase one-pixel highlights, so a distant light may produce no flare even when visually striking.

**Candidate solution:** during hierarchical bright-source reduction, retain multiple statistics (e.g. peak luminance, integrated energy, weighted centroid, pixel coverage/extent) rather than only average color. Compute importance from *peak and area/energy*; tune so an enormous bright wall does not outrank a pinpoint headlight by accident. Use GPU-only statistics.

**Test:** 1px/2px/4px emissive points at multiple resolutions, far headlight, and broad bright rectangular wall; compare stable response under TSR and screen percentage changes.

### Avoid splitting a broad light into dozens of candidates

**Problem:** a light strip or emissive screen can generate several tile maxima; naïve top-K fills all slots with one object.

**Candidate solution:** start with tile maxima plus overlap/non-maximum suppression, and compare with a reduced-resolution connected-region grouping approximation. Both must remain GPU-resident. Preserve separate neighboring headlights when visually distinct; ensure temporal candidate IDs don't switch every frame.

**Test:** two adjacent headlights, one long LED strip, scattered specular reflections, a large doorway and thousands of neon pixels.

### Use previous frame only for slow decisions

**Problem:** asynchronous extraction or candidate history can make long flares visibly lag the light on a fast pan.

**Candidate solution:** derive visible source motion from current-frame imagery when available; temporal hysteresis for *selection and opacity*, not a blanket frame-delayed rendering pose. Camera cuts reset histories. If source position must be lagged, evaluate whether reprojection/velocity can correct it, and document artifacts.

**Test:** camera whip pan, rapid light movement, auto-exposure change, camera cut, TAA/TSR, render scale change.

### Prevent self-feedback and duplicates

**Problem:** if source detection samples a scene after Glint is composited, the flare may detect itself. Bloom and Glint can exaggerate one another, and an authored source may overlap an automatic source.

**Policy:** capture HDR input before Glint contribution; retain normal tone-map/bloom behavior by validated stage; deduplicate authored vs automatic candidates where spatial/color matches are sufficiently reliable, with an artist override. Keep UE's stock flare separate and warn about simultaneous use.

## Prototype matrix, implementation order

| Gate | Experiment | Minimum pass criteria |
|---|---|---|
| E0 | Access HDR before Glint, independent of bloom setting | Correct scene signal, no self-feedback, editor/PIE/packaged identical basic operation |
| E1 | Brightness reduction: max vs mean vs peak+sum | Small highlights survive without broad false-positive flood; visually stable at changing resolution |
| E2 | Automatic field using HDR pyramid vs standalone mip generation | Measure end-to-end cost/compatibility; independent path always works |
| E3 | Source grouping: tile maxima/NMS vs approximate connected-region grouping | Stable distinct sources, strict per-view top-K, bounded GPU work, no sync readback |
| E4 | Draw full/simplified/field tier, 0/1/8/100/1000 highlight fixtures | Quality tier fades cleanly; no visible catastrophic popping |
| E5 | Partial finite-source occlusion using 4/8/16/32 samples vs uniform fade | Chosen mode reacts plausibly in foliage and edge transitions; cost controlled |
| E6 | Multiple elements vs generic Repeater instance compilation | Artist result remains editable; cycles rejected; runtime is flat/compact |
| E7 | Low/high-res buffers & R11G11B10F/FP16 where appropriate | Formats and stages verified for HDR energy, tint, alpha and blending; no regressions |
| E8 | Optional first-order optic-derived ghost placement vs hand-authored axis | Similar or improved visual plausibility for meaningful lenses; acceptable edit complexity |
| E9 | 1080p/1440p/4K, DLSS/TSR as accessible, HDR output, 16:9/21:9 | Crisp streaks, correct aspect and source motion, bounded full-pipeline cost |

**All tests must be recorded in packaged Win64 UE5.8** as well as editor and PIE. Record CPU, GPU per RDG pass, frame percentiles, resolution, hardware, HDR path, DLSS/TSR presence and visual regression captures. Automated `Automation RunTests Glint` tests should cover deterministic data, policy and shader dispatch constraints; manual/render tests should cover visibility and quality. Do not invent tests or performance claims before code exists.

## Deliberate exclusions

- No plugin-level claim to recognize arbitrary lights/materials from pixels; automatic classification works on *observed HDR image structure*.
- No demand to render physics-accurate rays for every source; physical-looking optics can be baked or artist-driven.
- No assumption that volumetric fog, water, raytraced translucency, VR, DX11 or mobile behave identically to opaque deferred Win64.
- No direct redistribution or reverse engineering of commercial engine/plugin code, presets, textures or patented lens prescriptions.

## Next engineering batch

**R0/early R1:** HDR integration + optional-independent pyramid baseline → stable automatic image field → source detection with peak/energy/area statistics → bounded GPU candidate selection → basic Glint Glow/Iris/Streak from the same lens profile. Include a simple source/threshold view and per-pass GPU timings *before* implementing complex lens physics or deep editor tooling.

Once R0 passes, progress with native Lens and Flare editors, generic Repeater, optical edge behavior and source visibility response. Optional complex optics and tile rasterization remain measurement-driven research.

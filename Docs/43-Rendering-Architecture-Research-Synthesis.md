# 43 — Glint Rendering Architecture: Research Synthesis

[← Index](README.md) · [38 — Froyok](38-Froyok-Custom-UE-Lens-Flare-Audit.md) · [40 — Walch](40-Walch-2017-Measured-Lens-Research.md) · [41 — 2026 model](41-Meta-2026-Capture-Reconstruction-Audit.md) · [44 — Gates](44-New-Research-Prototype-Gates.md)

> This is a **proposed Glint architecture** informed by source research. It is not a verified implemented feature or an assertion that any cited product contains this exact architecture.

## 1. Separate the four jobs

1. **Image analysis**: bright-pass extracted from correct HDR scene state, with pre-exposure compensation, stable downsampling and region classification.
2. **Source representation**: bounded persistent source descriptors (position, weighted color, peak/energy, shape, screen size, visibility, confidence, temporal ID). Can be automatic, explicit, directional or offscreen-authored. No Actor requirement.
3. **Optical evaluation**: Glint Lens Profile chooses how sources affect field response, authored flare elements, generated ghosts and linked aperture, with a uniform semantics for angle, source energy, zoom and optional dynamic triggers.
4. **Rendering/compositing**: share screen-space image field, batch detailed elements, mixed resolutions, optional prepared optics and one deliberate composite in HDR.

Critically, the **automatic field** is not the same as automatically generating a set of source identities: field effects are cheap over continuous image information; source candidates activate a bounded number of full compositions.

## 2. Candidate schema worth preserving

A GPU-side source record should support at minimum: view identifier; stable candidate ID; normalized screen position; projected source direction if available; energy (integrated HDR luminance); peak; approximate occupied area/aspect; weighted RGB/chromaticity; frame confidence; source category; occlusion/visibility policy; temporal state; authored-source association/dedup channel; priority. Fields can be compacted and changed after profiling.

Why this matters: Walch and the 2026 paper both emphasize that **source-camera geometry** controls ghost behavior, while Froyok demonstrates that noisy input can ruin the entire screen-space pipeline. The structure needs stable and expressive input without readback stalls.

## 3. Camera lens profiles can have THREE authoring representations

**A. Direct artist composition (v1 core)**

Optical Flares-inspired element stack, per-element transforms/appearance/behaviors and a common trigger model; compiled to dense runtime records.

**B. Optical approximation or pre-baked response (design readiness in v1)**

Compute dynamic ghost parameters using compact matrix/curve/lookup representations; optionally generate diffraction textures when the artist changes aperture/coating. Avoid expensive per-source ray tracing.

**C. Measured photographic response (post-v1 research)**

Capture light at several angles/apertures/focal lengths, fit a response surface, and evaluate it like other Glint properties. Decompose into Glint-native elements only to the extent their shape family allows; keep an optional opaque baked response for hard-to-fit artifacts.

All three should end in the same output description: image response plus bounded elements. **No authoring workflow should require an ML runtime.**

## 4. Proposed source-specific visibility policy

- **Automatically detected visible pixels:** eligibility derives from the actual HDR image; no extra line-trace cost required for these pixels.
- **Explicit world source:** combine camera/frustum, distance, GPU depth and optional transparency-aware attenuation; authored lights don't automatically count as visible.
- **Offscreen optical stray light:** a lens can still flare when strong light is outside the picture, but only if an **explicit source representation** supplies position/energy (or a future extended-frustum rendering method does). Avoid claiming ordinary scene-color detection can see offscreen emitters.
- **Fog/translucency:** define capability and fallback separately; do not guess visibility from a depth buffer that lacks relevant transparency data.
- **Camera cut:** reset temporal history immediately.

A source behind an opaque wall must not flare merely because its 3D light Actor exists. An artist can deliberately override this for a stylistic effect.

## 5. Four rendering technique tiers (internal, not obligatory exposed UI)

| Tier | Algorithm class | Expected purpose | Confidence |
|---|---|---|---|
| Field | Froyok/Chapman-like image-space ghosts, streaks, halo | Many weak/untracked highlights | Strong basis, untested UE5.8 integration |
| Elements | Batch procedural masks, sprites, line/streak instancing | Selected full authored/automatic sources | Strong engineering baseline |
| Approx optical | Precomputed diffraction/coating, first-order or fitted ghost response | Physically plausible behavior at bounded cost | Prototype, not committed |
| Rich optical | Deformed ghost response grids, tiled primitive accumulation, or offline fitted captures | Hero cameras/complex optical effects | Research, not default v1 |

## 6. Known rendering hazards

- Distinguish **stored pre-exposed HDR** values from scene-referred luminance and perceived brightness. Do not threshold in an accidental domain.
- Full-screen post-process pass placement relative to bloom, DOF, translucency, motion blur, TAA/TSR and tonemapping changes visual outcome.
- Small very bright highlights can alias through naive downsampling; preserve their peak while avoiding noisy flicker.
- Repeat sampling at low res can produce edge artifacts and visible UV inversion. Add edge masks and explicit border behavior.
- Keep independent per-view history and render resources; avoid singleton sharing results between the editor and game view.
- GPU output buffers from an RDG pass cannot be assumed to survive across frames without appropriate extraction/lifetime handling; temporal data needs explicit valid ownership.
- Per-source learned deformation may make a nominally 'one-pass' renderer expensive. Benchmark total shader and prepare cost, not raster stage only.
- Large optical ghosts incur fill-rate and memory-bandwidth costs; increase resolution only when visible.

## 7. Native editor consequences

- A Glint Lens asset should include a **Source Preview** panel (center, edge, offscreen, occluded, partial) and reference frame slot.
- An optional **Comparison** overlay for a photographed or captured flare should not be coupled to the renderer type.
- Treat *Appearance* and *Behavior* as consistent element groups, and expose shared linked lens properties with local overrides.
- Provide a native **Detection Debug** view: raw HDR, normalized luminance, filtered bright mask, candidate IDs/score, flare family/quality tier, final optical effects.
- A technical **Perf panel** should show full Glint GPU time, passes, candidate count, full-detail count, total drawn pixels, vs. tier-budget and average under camera pan.

## 8. No premature implementation commitments

The first hard gate remains plugin-only HDR access **and a packaged executable**, not physically simulated optics. Tile accumulation, ML gaussian splatting and complex capture pipelines should not become architectural prerequisites. The interface for optional response-bake assets can be planned now, but actual bake formats should be validated against profiling and authoring needs before being frozen.

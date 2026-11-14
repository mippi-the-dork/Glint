# 32 — Performance Illusions, Perceptual LOD, and Rendering Budget

[← Documentation index](README.md) · [07 — Performance & Scalability](07-Performance-and-Scalability.md) · [22 — HDR detection](22-HDR-Highlight-Detection.md) · [33 — Performance experiments](33-Performance-Experiments-and-Gates.md)

> **Status:** Architectural recommendations and experiments, **not implemented, measured, or guaranteed**. Keep the renderer capable of a visually faithful high-quality mode, then give simpler quality tiers equivalent visual *intent* without equal processing cost. Initial baseline is UE 5.8.0–5.8.3, Win64, DX12/SM6, deferred. Do not invent tested performance results.

## Performance doctrine

**Simulate the appearance of a lens, not every light path through glass.** Spend a limited per-view GPU workload on what the player can perceive. The biggest savings are often avoiding effects, pixels, uploads and passes rather than reducing ALU instructions. This is particularly true for automatic camera mode because scene brightness can create unbounded candidate counts unless we actively prioritize.

Seven rules:

1. **Shared work first.** One source extraction, lens profile evaluation and dirt response per view, not one copy per light.
2. **No unbounded multipliers.** Limit complex sources, elements, occlusion taps, composite samples and render-target area. Separately bound image-field work.
3. **Mixed resolution by appearance.** Soft energy at low resolution, thin or high-frequency detail at appropriate resolution. Do not force every element to full res.
4. **GPU stays on the GPU.** Compact automatic candidate buffers feed the element compositor without per-frame synchronous readback.
5. **Precompute stable optics.** Bake expensive, invariant shape and lens response properties in the editor/cook; keep changes to source position and intensity inexpensive.
6. **Temporal stability is more important than aggressive reduction.** A slightly cheaper effect that pops, ghosts or trails is the wrong tradeoff.
7. **Never hide meaningful art-directable properties.** Keep the Glint Flare asset authoritative. Quality tiers change its *evaluation*, not serialized source settings.

## A — Multi-resolution illusion (P0; v1 architecture)

| Content | Proposed evaluation | Rationale / guardrail |
|---|---|---|
| Broad glow, soft halos, low-frequency ghosts | 1/4 or 1/2 width and height | Soft gradients tolerate low-frequency accumulation; can create blocky color edges on over-sharp gradients |
| Lens dirt illumination, broad scattering | 1/4–1/8 width/height if quality allows | Camera-level, perceptually diffuse; evaluate texture detail vs dirt masks |
| Ordinary iris ghosts and sprites | 1/2 or full depending on screen area and edge sharpness | Aperture edges/texture patterns can reveal resampling; choose per element or group |
| Thin anamorphic streak, starburst spikes, sharp bokeh | Analytic/full-resolution or selective higher-res pass | Preserve thin-line stability; avoid low-res flicker from TSR/jitter |
| Bright-source detection/importance statistics | Dedicated reduced HDR reduction chain | Preserve small intense highlights with max/energy-aware reductions, not simple average alone |

**Pixel math**: for a 3840×2160 full-resolution buffer, half width × half height uses 25% the pixels; quarter width × quarter height uses 6.25%. These are pixel-count ratios, **not proportional measured GPU savings**. Cost can shift to upsampling, blending, bandwidth and overhead. At 4K, a full RGBA16F 8-byte-per-pixel surface is about 66.4 MB decimal; quarter width/height about 4.15 MB, ignoring mips, compression, alignment, transient aliasing and history. The cost of reading/writing matters as much as storage.

**Implementation requirement:** group by evaluation path, not by lens-object name alone. e.g. an authored Iris may require full-res near camera but half-res at low opacity. A crisp user-provided photographic texture must not silently become blurry solely because it's called an Iris.

## B — Optical response as inexpensive lookups (P1; design now, measure later)

- **Lens preset LUTs:** precompute or cook small response curves for aperture clipping, dispersion tint, center-to-edge deformation, intensity falloff, focal-length response, and ghost axis distance. A per-element response can evaluate from normalized source position, radial distance, angle, selected lens parameters, and time.
- **Sprite/analytic dual representation:** represent expensive diffraction, shimmer, caustic or multi-ring detail with source-independent texture atlases or generated procedural texture data; apply cheap per-frame transform, tint, and brightness. Preserve analytic elements for effects that need sharp dynamic geometry.
- **Shared bokeh/aperture texture:** many ghosts can sample one small aperture-mask texture with variations in transform, signed scale, and chromatic offsets rather than independently generating polygon cutouts.
- **Precompiled renderer descriptors:** authoring records are flexible data; a cooked/render-thread descriptor is compact, flattened, and organized by primitive family and texture source. Do not instantiate per-element UObjects, dynamic material instances, or actors to represent runtime fragments.
- **Avoid recomputing static optic math:** lens design is primarily static; source position and source appearance change every frame. Precompute stable optical geometry, preserve per-view dynamic response.

**Tradeoff:** LUTs are not free; texture fetch bandwidth, interpolation, permutations, streaming and update invalidation all matter. Keep optional lookup paths behind profiling rather than assuming every precomputation is a win. Papers on simplified optical transfer are research evidence, not product benchmarks.

## C — Automatic-highlight selection with an explicit budget (P0)

The high-value 'smoke and mirrors' trick is **every qualifying highlight contributes to the lens character, while only a few highlights receive expensive individual compositions.**

1. One GPU bright-pass produces a reduced HDR highlight representation. Where legally accessible and independent of bloom settings, consider reusing existing bloom textures; retain a Glint-owned fallback.
2. Tiled source analysis creates compact candidates (`position`, `radius/extent`, `energy`, `color`, `orientation`, `confidence`). Classify point, small-area, elongated and broad highlights; don't claim material/object recognition.
3. Rank using brightness, contrast, area and screen importance. Merge nearby peaks and suppress duplicates.
4. **Global camera budget + optional per-tile caps:** prevent one dense neon sign from consuming every premium slot; allow artist-marked priority overrides for manually authored sources.
5. Top ranked source candidates receive full procedural ghost chains. Middle candidates receive simplified starburst/iris groups. All remaining qualifying image content contributes to a cheap image-field response rather than nothing.
6. Introduce budget-boundary hysteresis, e.g. maintain existing slots until new candidates exceed a replacement margin for a period. **Example** experimental top-K values 4/8/16, not final defaults.
7. Automatic and authored sources feed the same element renderer where suitable, with suppression/dedup so authored sources do not create a second automatic flare for the same highlight.

The engine should evaluate **importance**, not merely `n flares`. A point source taking a small screen area can deserve a premium slot more than a giant bright wall. At the same time, don't let huge nearby flare geometry dominate the GPU merely because its source is bright.

**No CPU readback for source counts:** zero/low-source conditions may use GPU-driven indirect work or a bounded tiny pass; do not synchronize GPU → CPU every frame to decide whether the flare is enabled. UE5.8 has indirect compute support, but the exact RDG integration must be proven. `Off` and camera/feature-disabled states can bypass from known CPU-visible settings.

## D — Batching and rasterization (P0 for straightforward batching, P2 for tiles)

- Gather compatible primitive families for each view into packed GPU instance lists. Element descriptors include source index, pivot/axis, scale, orientation, tint/opacity, response curve index, texture atlas page and quality class.
- Minimize redundant setup and binding work. Do not assume a single shader for *all* element types will be optimal; shader specialization can reduce branch divergence, while too many permutations increase compilation and cache pressure.
- Texture atlases or arrays can reduce binding changes for compatible sprite families. Preserve custom imported images and streaming/cooking safety; don't require that every user texture be in one monolithic atlas.
- **Crop to useful pixels.** Use tightly bounded quads or scissor/tile-aware work for local ghosts; do not run expensive full-screen math for a tiny artifact. Broad streaks and global dirt have different coverage models.
- For many overlapping full-screen ghosts, a *tile-binned accumulation* path might reduce overdraw versus thousands of additive quads. It is a **research branch**, not mandatory first-implementation complexity. For few ghosts, conventional instanced quads may be faster.
- Pre-sort additive-compatible elements by renderer family/texture; preserve explicit ordering for noncommutative blend modes. Artists can rely on Screen/Alpha-style effects that must not be reordered incorrectly.

**Research precedent:** Bodonyi & Kunkli (2023), *Efficient tile-based rendering of lens flare ghosts*, investigated screen tiles and reduced ghost overdraw. The reported advantage is for that paper's optical ghost workloads, not a prediction for Glint's actual number of sources and element types.

## E — Fake complexity with composition (P1)

| Illusion | Cheaper approximation | When not appropriate |
|---|---|---|
| Dense Multi Iris chain | GPU-instantiate a seed ghost with weighted positions, scale, rotation and deterministic variation | All ghosts need unique custom textures/large geometry deformation |
| Realistic diffraction starburst | Small precomputed starburst texture or short analytic ray set | Need close-up changing aperture physics or extreme resolution |
| Optical chromatic scattering | 1D radial/spectral gradient lookup or modest RGB offset sampling | Premium cinematic where individual wavelength halos matter |
| Irregular shimmer | Animate small distortion/noise field or phase parameter, not rebuild all rays | The desired look depends on truly evolving fine geometry |
| Complex caustic | A warp/distortion atlas, a few response shapes, or precomputed optics LUT | High-detail reference/production rendering requiring precise caustics |
| Heavy multi-ring halo | One procedural ring shader with a response curve | Hard authored per-ring occlusion and nonadditive blend requirements |
| Large continuous dirt field | One per-camera dirt texture modulated by total lens-light energy | Different dirt masks per physical camera in split-screen; still one per view |

**Global dirt** should not iterate over every light for every pixel. Aggregate a small camera-level illumination field or weighted tint/brightness and use that once. This is more convincing than ten identical dirt cards moving with ten headlights.

## F — Temporal amortization without visible lag (P1)

Do *not* blindly evaluate all effects every 2nd/3rd frame; it creates noticeable latency, especially near the screen edge and with sudden camera cuts. Instead:

- Update source **position, visibility and base brightness each rendered frame** where practical.
- Reuse source identity and slowly varying candidate *classification* between frames, with camera-motion-aware matching, fade/hysteresis and a defined maximum latency.
- Compute expensive low-frequency lens deformation, noise evolution or caustic variation less frequently **only when** interpolation is stable and tested at high camera angular velocity.
- Use static generated noise and deterministic phase evolution rather than expensive new random state each frame.
- Reject or reset histories after camera cuts, discontinuous zoom, large viewport resize, history invalidation, or abrupt source reappearance.
- Temporal accumulation for soft elements must be validated against TSR/TAA, moving occluders, dynamic resolution, and color changes. History can *increase* ghosting, not just improve quality.

## G — Smart occlusion and source integration (P1)

- Automatic highlights are **already visible in the rendered image**, so they should not pay an additional world-space depth-occlusion pass that assumes an unknown 3D origin.
- Authored sources can use a graded hierarchy: center depth test → a few nearby weighted taps → optional richer visibility. Upgrade only for important sources or those near depth edges; keep upper bounds.
- Preserve stable partial occlusion when geometry sweeps across light. Avoid frame-to-frame reallocations, random sample jitter and hard threshold toggles.
- Don't claim that source depth tests reproduce translucent glass, fog or cloud attenuation. These require dedicated tests and separate feature gates.
- For authored hero sources, permit selective offscreen extension; screen-based automatic mode cannot invent an offscreen source invisible in HDR input.

## H — Scalability as artistic equivalence (P0)

Define four dimensions independently in the lens/profile, then choose automatic quality tier defaults:

1. **Field Quality:** reduced-resolution factor, sample count, number of warped/reverse/streak contributions.
2. **Candidate Quality:** max complex sources, simplified source slots, class-specific limits.
3. **Element Quality:** ray complexity, sprite sharpness, aberration samples, ring/deformation quality.
4. **Visibility Quality:** soft depth taps, temporal persistence, history quality, selected special cases.

**Off** bypasses work and releases transient state safely. **Low** retains the lens identity (tint, principal streak/ghost), not merely a global brightness reduction. **Medium** is the production baseline. **High** uses more premium candidates and sharper elements. The four axes give games control; artists should first see a compact `Quality: Automatic / Low / Medium / High` selector, with Advanced overrides rather than dozens of visible toggles.

**Degrade gracefully:** Full Composition → Reduced Ghost Set → Signature Element(s) → Image Field Only → Nothing for truly subthreshold pixels. Crossfade levels with energy-normalized intensity and hysteresis to avoid popping and apparent exposure shifts. Do not guarantee visual equivalence across every preset without testing.

## I — Dynamic budgets and frame pacing (P2 optional; instrumentation from day one)

- Start with **deterministic fixed structural budgets**, not an opaque frame-by-frame quality controller. Structural caps guarantee that worst-case source counts cannot explode geometrically.
- Measure each stage: HDR extraction/reduction; candidate grouping; source history; field; instanced elements; authored occlusion; dirt; composite.
- Later evaluate an **optional delayed GPU-budget feedback controller**: moving averaged GPU times, upper/lower hysteresis, a cooldown between tier changes, minimum acceptable quality and an explicit opt-out for cinematics/Sequencer.
- Don't use CPU frame time or instantaneous FPS to infer Glint GPU cost; Unreal's per-frame cost can be dominated by unrelated systems. Avoid quality changes during short transient spikes or editor viewport stalls.
- Multi-view and split-screen costs require per-view state plus an aggregate global ceiling; no cross-camera source or history contamination.

## J — Native editor and content optimizations (P1)

- Live previews should use shared runtime shaders but **quality-scaled render targets**. Idle editors should not render continuously if their lens inputs/time are static, while live preview motion remains responsive.
- Source visualizations, LUT regeneration, and texture thumbnails update only when their dependent data changes. Keep CPU-side element stacks compact and separate from render state.
- Cook/Derived Data Cache expensive static shape data where appropriate, but maintain reversible asset-authoring sources. Different runtime profiles should not require one giant permanently resident texture for every possible lens.
- Shader permutations must be bounded and compiled during packaging; cache/warm high-frequency PSOs and ensure no in-game editor-like compilation stalls.
- Where possible, use engine texture streaming/mips; minimize manual persistent GPU buffers. Never assume transient resource aliasing reduces total GPU *execution* time by itself.

## Things that SOUND fast but can backfire

| Temptation | Risk | Better default |
|---|---|---|
| Render everything at quarter resolution | Thin streaks shimmer or blur; upsample cost | Separate soft vs sharp paths |
| Recompute bright sources every 4 frames | Motion lag, missed flashes, popping | Per-frame lightweight detection, amortize stable expensive detail |
| Reuse the UE bloom buffer unconditionally | Wrong settings/lifetime, bloom disabled, HDR mismatch | Probe access, support independent Glint extraction |
| Read GPU source count on CPU to skip passes | GPU pipeline stalls | Fixed cheap reduction and/or GPU-driven indirect downstream work |
| Use one giant universal shader | Divergence, register pressure, permutation/compile cost | Group by compatible primitive families, profile variants |
| Texture atlas everything | Excess VRAM, streaming and updates, custom texture complexity | Compact atlases/arrays for common element families |
| Many tiny compute passes | Dispatch/barrier overhead outweighs saved math | Fuse work when it reduces total cost, but preserve useful RDG dependencies |
| Async compute by default | Resource contention/synchronization; poor overlap | Measure overlap using RDG Insights before enabling |
| Fake occlusion with hard center checks everywhere | Highly visible cuts at geometry edges | Soft sample only on high-importance/edge cases |
| Adaptive quality based on FPS every frame | Oscillation and unrelated CPU/GPU bottlenecks | Fixed caps first; delayed optional measured GPU controller |

## Required profiler instrumentation

Provide (subject to verified UE5.8 API) `RDG_EVENT_SCOPE`, `RDG_GPU_STAT_SCOPE`, and/or CSV profile scopes around identifiable Glint stages. A debugging panel should show: camera ID, render resolution, field resolution, detected regions, selected complex/simple candidates, culled candidates and why, number of primitive instances, atlas usage, render-target memory estimate, pass timing (where available), tier, and active renderer path.

No instantaneous microsecond timing displayed in a per-element Details panel unless it is an actually measured query, not a CPU-side estimate. UE5.8 RDG Insights provides insight into graph passes, resource lifetimes, merging and async overlap.

## Required guarantees and non-goals

- **Quality fidelity is opt-in**, not all-or-nothing. Artists can force hero compositions to High and give automatic background sources smaller caps, subject to documented project override rules.
- Glint does not override the game's native bloom/exposure settings silently and does not require native bloom to be active.
- No CPU per-pixel scan, no source Actor spam, no per-frame dynamic asset recompilation, no mandatory synchronous GPU readback.
- Per-view history must be isolated and invalidated correctly.
- v1 must demonstrate a stable, fixed-budget GPU pipeline. Advanced tiles, physical precomputation and feedback control may be promoted only after comparative benchmarks.

## References and implementation evidence

1. Epic, [UE5.8 RDG documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine) — pass culling, transient resources, profiling markers, RDG Insights.
2. Epic, [UE5.8 Post Process Materials](https://dev.epicgames.com/documentation/en-us/unreal-engine/post-process-materials-in-unreal-engine) — HDR vs LDR bandwidth, lookup count, cost of passes, and viewport UVs.
3. Epic, [UE5.8 Temporal Upscalers](https://dev.epicgames.com/documentation/unreal-engine/temporal-upscalers-in-unreal-engine) — resolution/viewport and pre-exposure concerns.
4. Epic, [UE5.8 PCG GPU](https://dev.epicgames.com/documentation/unreal-engine/using-pcg-with-gpu-processing-in-unreal-engine) — general GPU↔CPU transfer overhead (analogy, not a Glint implementation guide).
5. Epic, [UE5.8 Indirect Dispatch](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FComputeShaderUtils__DispatchInd-) — possible GPU-driven work in supported RHIs; integration unverified.
6. Unity, [HDRP Screen-Space Lens Flares](https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4017.4/manual/shared/lens-flare/Override-Screen-Space-Lens-Flare.html) — bloom-derived shared image field; reference, not engine parity proof.
7. Lee & Eisemann, [Practical Real-Time Lens-Flare Rendering (2013)](https://publications.graphics.tudelft.nl/papers/335) — first-order approximate optical transfer.
8. Bodonyi & Kunkli, [Efficient Tile-Based Rendering of Lens Flare Ghosts (2023)](https://www.sciencedirect.com/science/article/pii/S0097849323001486) — reduce ghost overdraw with tile binning.
9. [Real-time ray transfer for lens flare rendering using sparse polynomials (2024/2025 journal)](https://link.springer.com/article/10.1007/s00371-024-03625-7) — offline fit / runtime polynomial evaluation.

**Evidence standard:** citations establish feasibility of methods or Unreal systems; none establishes an achieved Glint speedup. Benchmark each architectural path in a packaged UE5.8 implementation before locking it.

## Added lessons from other engines and shipped games

- Unity documents sampling a Bloom mip pyramid for screen-space lens flares. Explore conditional UE5.8 reuse, but guarantee Glint-owned HDR reduction when Bloom is disabled or the texture is unavailable/incompatible. [Unity reference](https://docs.unity.com/en-us/engine/7000.0/manual/visual-effects/lens-flares/lens-flare/reference-screen-space).
- Guerrilla's *Killzone: Shadow Fall* postmortem illustrates the need to optimize postprocess **memory bandwidth and intermediate formats**, not only arithmetic/draw counts. Its historical PS4 timings are not Glint predictions. [Guerrilla document](https://www.guerrilla-games.com/media/News/Files/Valient_Killzone_Shadow_Fall_Demo_Postmortem.pdf).
- For candidate selection preserve peak + energy/coverage statistics. Downsampled **mean luminance alone erases tiny highlight sources**, while peak-only ranking favors single-pixel noise. Test those competing biases explicitly.
- CRYENGINE's camera-orb documentation warns about overdraw. Log approximate covered pixels/area for large ghost elements; a source's count alone is a poor cost proxy.
- Tile-based ghosts and high-order optics remain **measurement-driven second-generation paths**. See [36](36-Rendering-Papers-and-Architectural-Lessons.md).

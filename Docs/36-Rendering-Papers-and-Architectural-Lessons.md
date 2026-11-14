# 36 — Real-Time Lens-Flare Research: What the Papers Actually Establish

[← Documentation index](README.md) · [34 — Engines](34-Other-Engines-Real-Time-Lens-Flares.md) · [37 — Decisions](37-Cross-Engine-Findings-and-Prototype-Gates.md)

**Goal:** Research optical simulation that is convincing enough to guide authors while cheap enough to use in a UE5.8 game. Paper figures refer to each paper's own hardware, test scenes and implementation. **Do not use published milliseconds as Glint's GPU budget.**

## A — Hullin et al., 2011: physically based ghosts are feasible but complex

[Physically-based real-time lens flare rendering](https://publications.graphics.tudelft.nl/papers/508), ACM TOG (2011), Matthias Hullin, Elmar Eisemann, Hans-Peter Seidel and Sungkil Lee.

**Demonstrates:** interactive physically plausible lens ghosts with lens imperfections, chromatic and geometric aberration and anti-reflective coatings; has performance-quality controls and artistic extensions.

**Practical lesson:** A convincing physically rooted Lens Profile can describe *why* ghost colors, relative positions and distortions occur. However, constructing physical ray paths dynamically for every automatically detected bright region is not the v1 default. Treat the physical implementation as a reference/evaluation target.

**Glint action:** allow lens-type parameters, shared aperture, coating-inspired tint/attenuation and chromatic response to influence multiple child elements. Keep authored overrides so the designer can exaggerate a look.

## B — Lee & Eisemann, 2013: first-order optical transfer is a promising middle ground

[Practical Real-Time Lens-Flare Rendering](https://publications.graphics.tudelft.nl/papers/335), Computer Graphics Forum (2013), Sungkil Lee and Elmar Eisemann.

**Demonstrates:** a first-order/paraxial ray-transfer matrix approximating ghost position, size and color from optical system parameters. Compared with more complete physical methods, it trades accuracy (especially nonlinear deformation away from the optical axis) for much cheaper evaluation. The paper discusses angle/quality limits: its reported accuracy suffers for sufficiently large source incidence angles.

**High-value idea:** generate **initial ghost placements and shared behavior** from an inexpensive physically inspired model, then give artists direct per-element control. Optical transfer matrices are candidate **editor/cook-time** tools even if runtime uses plain source position plus compact transforms/curves.

**Do not overpromise:** a first-order model does *not* capture every edge deformation, lens defect or aspherical response. Glint must preserve independent artistic radial warps and sample/texture-driven imperfections for off-axis compositions.

## C — 2023: efficient tile-based ghost rasterization

[Efficient tile-based rendering of lens flare ghosts](https://www.sciencedirect.com/science/article/pii/S0097849323001486) (2023).

**Demonstrates:** a two-level tile buffer process and per-tile accumulation of ghost primitives; adaptive primitive merging reduces repeated rasterization/overdraw from many overlapping ghosts. The published method scales with increasing source/ghost complexity.

**Glint takeaway:** normal batches of sprite/analytic elements are our **baseline**; tile-binned accumulation is a **conditional second renderer** if profiling proves overlapping high-area ghosts are our dominant cost. A tiled path adds binning, memory/buffer management and more shader complexity, and could lose on small flare sets.

**Prototype comparison:** many wide ghost layers at 1080p and 4K, with identical HDR output, record raster overdraw, RT writes, binning cost, and end-to-end GPU time; don't benchmark the tile pass without its setup.

## D — Bodonyi, Csoba & Kunkli, 2024/2025: fast non-linear optical simulation exists, but is not free

[Real-time ray transfer for lens flare rendering using sparse polynomials](https://link.springer.com/article/10.1007/s00371-024-03625-7), published online October 2024, journal issue 2025.

**Demonstrates:** precomputed, locally fitted sparse polynomial ray-transfer approximations that better preserve ghost shape, attenuation, chromatic dispersion and blocked rays than naive polynomial approaches. The paper describes faster optics evaluation than analytical ray tracing. It also reports full flare examples taking roughly 1.84–2.01 ms at 1080p on an NVIDIA TITAN Xp for its test cases. Those figures make it **unsuitable as an assumed free or general-purpose v1 automatic path**.

**Glint lesson:** splitting expensive **lens design/precomputation** from inexpensive **source-dependent runtime evaluation** is useful. Sparse polynomial optics could support later high-quality hero lenses or editor-generated optical assets; it should not become a mandatory dependency or bottleneck for every automatically detected highlight.

### Candidate data strategy

Authoring Glint Lens Profile → compile source-independent optics → small transferable response tables or compact ghost descriptors → per-view GPU source response → shared rendered elements.

This keeps a path for future physical-informed lens profiles without placing a proprietary-looking physical lens GUI in front of basic creators.

## E — NVIDIA GPU Gems: visibility is a source question, not just an overlay

[GPU Gems, Chapter 29: Efficient Occlusion Culling](https://developer.nvidia.com/gpugems/gpugems/part-v-performance-and-practicalities/chapter-29-efficient-occlusion-culling) describes source-level visibility tests for classical lens flares and why an eye-space overlay cannot simply be depth-tested like a world object.

**Terminology caution:** the chapter describes a physically motivated source-visibility interpretation and binary visibility context; CRYENGINE later documents artistic *partial-occlusion shape response*. These do not necessarily contradict one another: the **source footprint** may be partly visible, changing the intensity/shape of a lens response, but a foreground tree should not literally cut holes through every camera-sensor ghost.

**Glint action:** make `Source Visibility` a separate intermediate quantity from `Ghost Shape Response`; test single-pixel point sources and finite-area disk/area sources separately. If Automatic mode observes the already-occluded scene, avoid applying a second opaque-scene visibility fade to the same pixels.

## F — Quantitative decision hierarchy

| Approach | Quality | Cost risk | v1 use |
|---|---|---|---|
| Author-controlled sprite/analytic lens elements | Flexible, artist-driven | Overdraw if unbounded | **Required** |
| Screen-space reversed/warped ghosts from an HDR pyramid | Good automatic field response | Additional texture samples and scene bandwidth | **Required** |
| First-order lens ghost placement | Physically inspired but misses nonlinear response | Low-to-moderate | R0–R2 experiment, potentially bake |
| Finite-area source occlusion response | Strong realism at foliage edges | Extra depth/mask samples | P1 prototype |
| Optical LUTs/curves | Reusable and inexpensive per source | Caching, interpolation, fetch overhead | Design in v1; compare performance |
| Tile-binned accumulation | Wins in heavy ghost overlap | Extra pass/setup and buffering | Conditional P2 |
| Sparse polynomial lens simulation | Detailed lens physics and shape changes | Meaningful runtime workload/precomputation | Post-v1 research / premium path |
| Full physical lens ray tracing every frame | High physical fidelity | Not suited to many auto candidates | Excluded v1 |

## Key questions to validate experimentally

1. Does the optical transfer approximation perceptually improve an authored lens profile, at the center **and extreme screen edges**?
2. Can editor-side baking retain complex ghost behavior while runtime only applies inexpensive transforms/tints/warps?
3. Are source visibility/shape changes stable when a tree branch crosses the source at low pixel coverage or during TSR jitter?
4. At what number/area of overlapping ghosts does tile binning overtake conventional instanced quads?
5. Does the full automatic system stay affordable when image-driven field ghosting and element-driven candidates are both active?

**Research status:** Validated in the cited papers; **not tested or implemented in Glint**.

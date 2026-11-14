# Glint Research Review — Physically-Based Real-Time Lens Flare Rendering (2011)

**Status:** Reference reviewed; design implications proposed, not yet experimentally validated in Unreal Engine 5.8  
**Source:** Hullin, M., Eisemann, E., Seidel, H.-P., and Lee, S. (2011). *Physically-Based Real-Time Lens Flare Rendering*. ACM Transactions on Graphics 30(4), Article 108. DOI: [10.1145/1964921.1965003](https://doi.org/10.1145/1964921.1965003).  
**Reviewed PDF:** `2010324.1965003.pdf` (user-provided)

## Assessment

**High value as a technical reference. Not appropriate to implement wholesale as Glint's default runtime path.**

The paper simulates optical ghosting in multi-element lenses using sparse rays, beam interpolation, and GPU rasterization. Unlike static additive sprites, ghost shape, color and deformation respond to optical design, light incidence, aperture, coatings, and spectral effects. The paper also explicitly discusses artistic modifications.

## Relevant techniques and exact paper locations

| Technique | Paper section / figure | Concrete finding | Glint application |
|---|---|---|---|
| Two-reflection ghost paths | §4.1; Figure 8–9; pp. 4–5 | Enumerate each pair of reflecting interfaces, usually omitting weaker higher bounce orders | Authoring/baking: derive a coherent family of optical ghosts from a lens specification |
| Sparse ray bundles + interpolation | §4.1; Figures 10–12; pp. 5–6 | Trace a sparse grid and interpolate contributions within rasterized cells instead of dense ray tracing | Research path for high-quality procedural ghost deformation, not the default automatic highlight renderer |
| Fourier aperture diffraction | §3.3; Figures 5–6; pp. 3–4 | Precompute a wavelength-aware starburst texture from aperture geometry; precompute ringing approximation | Build-time generation of shared starburst/ghost-ring response textures, with editable overrides |
| Coating spectral colors | §3.1; Figure 4; p. 3 | Lens coatings produce different color response per internal optical interface, depending on angle and wavelength | Shared lens coating palette and angle-dependent ghost tint (approximate with LUT/curves) |
| Bound rays before tracing | §4.2; p. 5 | Precompute the regions of input rays capable of reaching the sensor | Potential baked optical-model acceleration |
| Adaptive detail / brightest ghost LOD | §4.2; p. 6 | Increase tessellation only for spatially complex ghosts; rank and render the brightest ghosts | **Key v1 principle:** rank optical components by energy and perceptual importance, not fixed count |
| Exploit axisymmetry | §4.2; Figure 13; p. 6 | Compute one incidence orientation and rotate; optionally mirror | Bake/lookup reduction for compatible photographic lenses; do not assume it applies to all anamorphic models |
| Artist controls | §5; Figures 14–16; p. 7 | Unusual aperture shapes, dirt/scratches, imperfect symmetry, per-ghost coating colors | Native Glint Lens Profile with artistic override and preview |
| Runtime light detection | §7.1; p. 8 | They note identifying and tracking bright regions in footage by intensity threshold | Useful precedent, but **not** a complete GPU automatic source-selection system |

## Caution: reported performance and scope

- Section 6 and Figure 15 compare multiple photographic lens models on a **GTX 285**. High-quality and standard modes differ drastically; the figure includes rates as low as approximately **0.5–29 fps** high quality and **7.5–228 fps** standard, depending on lens system and aperture. Those are historical algorithm-performance results, not Glint frame-time predictions.
- Section 6 notes that discarding **20% of the weakest ghosts** gave about **20% speedup**, and discarding **40%** gave about **50% speedup** in their experiments. This establishes the importance of ghost-priority culling, but the numbers are not transferable as performance guarantees.
- Precomputation is not trivial for complex lenses: the authors report about **5 minutes** for a 142-ghost Nikon lens and about **20 minutes** for a 312-ghost Canon lens under their test settings. Glint must not make such computations part of routine property scrubbing.
- The method is optimized for **one distant point/directional source** (§7.2), whereas Glint Automatic must process multiple bright regions, area lights, emissive strips, reflections, and changing cameras. Accordingly, automated HDR source extraction and source budgeting remain separate problems.
- Their implementation used vertex/geometry/fragment shader stages (§4.3). In UE5.8, the rendering algorithm should be re-expressed using modern RDG/shader infrastructure, not ported literally.

## Follow-up work: newer research

1. **Lee & Eisemann (2013), [Practical Real-Time Lens-Flare Rendering](https://publications.graphics.tudelft.nl/papers/335).** Uses first-order ray-transfer matrices to cheaply approximate ghost position, size and color, but lacks the complex nonlinear deformations of the 2011 approach. **Recommended baseline optics experiment** for Glint.
2. **Bodonyi & Kunkli (2023), [Efficient tile-based rendering of lens flare ghosts](https://www.sciencedirect.com/science/article/pii/S0097849323001486).** Groups overlapping ghost contributions into screen-aligned tiles to reduce rasterization overdraw. **Profile-dependent candidate**, not mandatory at startup.
3. **Bodonyi, Csoba & Kunkli (2024 / 2025 volume), [Real-time ray transfer for lens flare rendering using sparse polynomials](https://link.springer.com/article/10.1007/s00371-024-03625-7).** Uses compact, locally fitted polynomial maps with sparse GPU ray evaluations for complex ghost shapes; published online 7 Oct 2024. Reports **2–5× speedup** vs analytical ray tracing in tested cases, but example *total effect* times around **1.84–2.01 ms** at 1080p on a TITAN Xp. **Candidate for selected hero/high-quality lenses**, not unrestricted auto mode.
4. **Original [2011 author project page](https://resources.mpi-inf.mpg.de/lensflareRendering/)** lists supplemental C-like pseudocode and describes a historical patent-pending status. **Perform licensing / patent diligence before reproducing code or assuming source redistribution rights.** The theoretical ideas can guide an independent implementation; licensing and patent status require separate verification.

## Glint design recommendations

### v1.0 must-have architecture

- Keep **two separate authored asset concepts**: a **Glint Lens** determining per-camera response and detection thresholds, and a **Glint Flare** describing editable element composition.
- Keep **automatic HDR highlight extraction** decoupled from **ghost synthesis**, so a cheap image-response path can cover many highlights and a bounded number of detected candidates can receive richer procedural flares.
- Support **per-ghost estimated importance/energy** and **quality tiers**. Fade or replace detail gracefully when budgets change.
- Support precomputed, asset-local **diffraction patterns, aperture masks, and optical response textures/curves**, but maintain live, low-latency editor preview.
- Preserve **artistic overrides** at the element level even where defaults come from a shared optical profile.
- Reserve enough flexibility for first-order matrices, baked LUTs, or sparse polynomials without embedding their computation into per-element authored logic.

### Proposed phased experiments

**E1 — Fast photographic ghost baseline:** Compare a traditional center-axis sprite chain against a first-order matrix-inspired placement solution for size/position/tint as the source moves and focal length changes. Evaluate animation and GPU cost; start with a single distant source.

**E2 — Procedural diffraction baking:** Generate a starburst from aperture shape at edit/bake time, then render at runtime as a shared texture; validate blade count, aperture changes, chromatic response, and tiny-image aliasing.

**E3 — Automatic multi-source scaling:** Test 1, 8, 32 and 100+ strong HDR regions. Apply full/simplified/image-only source tiers, keep all selection on GPU when feasible, and measure total frame cost, popping, and visual energy conservation.

**E4 — Hero lens nonlinear deformation:** Prototype selected ghost warping using precomputed response fields or sparse polynomial transfer only when visuals justify it. Track memory, bake time, editing feedback, and performance impact.

**E5 — Overdraw comparison:** On a stress scene, compare instanced ghost quads with a tiled accumulation approach. Adopt a new path only where a real GPU capture shows a net win.

## Bottom line

Use the 2011 paper as a **physical and perceptual design reference** and an inspiration for offline/baked optical profiles. Use the much cheaper 2013 matrix approach as the first optics prototype. Use contemporary polynomial and tiled methods as targeted upgrades if needed. **Do not attempt the full 2011 simulation for every automatically detected bright region.**

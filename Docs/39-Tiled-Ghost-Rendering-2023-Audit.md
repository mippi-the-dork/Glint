# 39 — 2023 Efficient Tile-Based Rendering of Lens Flare Ghosts

[← Index](README.md) · [32 — Perceptual Optimization](32-Perceptual-Optimization-and-Render-Budget.md) · [44 — Gates](44-New-Research-Prototype-Gates.md)

**Primary source:** Andrea Bodonyi and Roland Kunkli, *Efficient tile-based rendering of lens flare ghosts*, Computers & Graphics 115 (2023), pp. 472–483. DOI https://doi.org/10.1016/j.cag.2023.07.019. User-supplied complete 12-page PDF.

**Claim discipline:** Reported timings measure the paper's physically based ghost system; they are **not** evidence that arbitrary procedural flares can be rendered for those costs, nor guaranteed savings for Glint.

## Problem addressed

With a complex optical system, ghost after ghost is individually rasterized into the same image. Many overlap and many contain deformed ray-grid quads. Rasterizing each one separately wastes substantial bandwidth even when the contribution is faint. The paper organizes projected ghost primitives in **screen-aligned tiles** and computes all relevant contributions per pixel in a single tile traversal.

## Exact algorithmic building blocks

1. Enumerate relevant ghost paths and wavelengths; generally concentrate on two-reflection paths.
2. Trace coarse ray grids through lens surfaces to derive deformed ghost meshes.
3. Construct projected quads; discard quads with no contributing rays.
4. Merge adjacent quads that are sufficiently similar, while retaining detailed edges and deformations.
5. Build a **two-level tile hierarchy** (coarse assignment, dense assignment) to reduce primitive-to-tile work.
6. Traverse per-tile primitive lists, shade and accumulate relevant contributions for each output pixel.

See the paper's **Figure 6 (PDF page 5)** for the staged pipeline and **Figures 7–9 (PDF page 6)** for pruning, merging and tile hierarchy.

## Performance interpretation

- The authors report approximately **2×** rasterization improvement for simple cases and up to **5×** in their demanding comparisons, relative to their chosen baseline.
- Figure 1 (PDF page 2) gives **4.04 ms** for an Itoh lens and **12.79 ms** for a Nikon lens at **1920×1080** on an **RTX 3060**. These numbers show that high-fidelity optical ghost simulation is not automatically a low-cost game feature.
- Savings improve with higher overlap and more complex ghosts, but tile construction, primitive merging, and buffer access themselves cost GPU time.
- Reduced geometry from primitive merging trades accuracy for speed. Preserve silhouette and spectral details where the player can see them.

## Glint application

**Do not start with full optical ray-grid generation or a tile renderer.** Start with simple batched quads and half/quarter-res field effects, measure total time and overdraw in dense-night benchmark, then compare a compute tiled path only if actual ghost overdraw dominates.

If adopted later, shared tile infrastructure could also service non-physical authored ghost elements by converting them into bounded analytic primitives. That is an **engineering hypothesis**, not established by the paper. It may be worse than direct rasterization for sparse/glow-heavy scenes.

## Proposed tests

- Full resolution and 1/2/1/4 resolution; 1, 8, 16, 64, 128 flare elements.
- Few large overlapping ghosts versus many small non-overlapping sprites.
- Compare baseline instanced quads, batched groups, and prototype tile accumulation.
- Count GPU dispatches, rasterized pixels, memory traffic, tile-build ms, traversal ms and final composite ms.
- Validate visual differences around fine ring edges, chromatic features and source motion.

## Legal and code sourcing

The article is provided as open-access **CC BY** text. It refers to the **OpenLensFlare** project (BSD-2-Clause; https://github.com/csobaistvan/OpenLensFlare) for generating examples, which has its own dependencies and limits. No code, shader files or reference images are being incorporated into Glint by this design document. Independent implementation and any third-party notice/licensing review are separate tasks.

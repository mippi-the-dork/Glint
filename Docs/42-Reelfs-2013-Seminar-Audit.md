# 42 — Reelfs 2013: Physically-Based Real-Time Lens Flare Rendering Seminar

[← Index](README.md) · [14 — Optics](14-Optics-and-Rendering-Research.md)

**Primary source:** Jens Helge Reelfs, *Physically-Based Real-Time Lens Flare Rendering*, seminar, February 21, 2013. User-supplied 24-page PDF.

## Assessment

A useful tutorial/technical primer, primarily a survey and explanation of the earlier Hullin et al. 2011 work—not a novel independent algorithm. It covers ghosting vs glare, empirical sprites, Fresnel reflection, chromatic dispersion, anti-reflective coatings, Fraunhofer/Fresnel diffraction, realistic cameras and sparse ray bundles.

## Concepts most relevant to artists and engineers

- **Ghosts** from internal reflection are separate from **source-centered glare**. Artist-facing Glint should distinguish them rather than equate every effect with camera-space ghosts.
- Glare can itself contain **lenticular halos** and **ciliary corona/starburst** components: they have different apparent spatial behavior. This supports our separate Glow/Halo/Ring vs Starburst/Streak families.
- **Lens aperture mask** drives polygonal ghost shapes and diffraction behavior. A shared mask should be reusable between directly authored elements and optically linked lens elements.
- **Chromatic dispersion/coatings** influence brightness and color of ghosts, and can be represented by approximations or prepared texture/curve data rather than full spectral transport.
- **Sparse bundles and interpolation** are mathematically useful, but runtime costs depend strongly on the ghost complexity and degree of deformation.

## What this doesn't establish

- It doesn't validate the current UE5.8 postprocessing interfaces, plugin integration or packed game timing.
- It doesn't introduce a method to automatically detect and group many arbitrary HDR scene highlights.
- It doesn't justify a general-purpose ray-traced lens renderer as default v1 architecture.

## Recommendation

Keep as introductory optical reference and as vocabulary for Glint user docs. Use the primary 2011 and later work for algorithm and performance claims rather than double-counting this seminar as independent confirmation.

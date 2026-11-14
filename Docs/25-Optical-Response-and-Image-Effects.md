# 25 — Optical Response: More Research Beyond Flare Elements

[← Index](README.md) · [14 — Optics](14-Optics-and-Rendering-Research.md) · [23 — Lens Profile](23-Camera-Lens-Profile-and-UX.md) · [27 — Research Priorities](27-Research-Backlog-and-Technical-Decisions.md)

## A lens flare is not a single visual phenomenon

It helps to separate the **cause**, the **optical family**, and the **rendering method**. That avoids treating everything as an additive glowing sprite.

| Visual family | Physical / photographic cue | Proposed Glint technique | v1 direction |
|---|---|---|---|
| Veiling glare / scattering | Weak broad wash over contrast | Reduced-res bright-field convolution / soft gather | Required inexpensive option |
| Primary halo/glow | Source-centered scattering and sensor response | Source-weighted radial analytic response | Required |
| Internal ghosts | Reflections between glass interfaces | Axis-mapped procedural iris/texture elements; image-field warp | Required |
| Aperture image | Ghosts sharing polygonal iris profile | Shared aperture profile + independent tweaks | Required for supported iris types |
| Starburst/diffraction | Light interacting with aperture/obstructions | Procedural ray profile, optional baked response LUT | Required artistic approximation |
| Anamorphic streak | Lens-related anisotropy / scattering | Directional gathers + authored line elements | Required |
| Lens dirt (schmutz) | Surface grime illuminated by strong light | Camera-space texture modulated by bright-field energy/position | Required basic version |
| Chromatic ghosts | Wavelength-dependent reflection/refraction | Spectral offsets / channel weighting / tint gradients | Required artistic approximation |
| Cat's eye/vignetting | Oblique rays clipped by lens housing | UV-dependent ellipse/shape/brightness deformation | Research v1 if inexpensive |
| Gate/edge flare | Bright source grazing barrel/edge | Screen-edge/angle driven local element | Required via trigger; physical correctness later |
| Sensor streak/smear | Sensor readout or saturation artifacts | Optional stylized image-space streak | Deferred unless clear use case |
| Sensor dust / damage | Dust often appears via imaging/aperture effects | Camera-space pattern with suitable blur and highlight illumination | Research; distinguish from lens-front dirt |
| Lens distortion interactions | Flares distorted with image projection | Optics UV map and calibrated optical center | Validate in camera pipeline |

Some labels above describe an **artistically approximated** physical effect, not literal optical simulation; do not market them as traced optical physics.

## Particularly useful research findings

**1. Lens optics can be far cheaper than per-frame glass tracing.** Lee & Eisemann's 2013 *Practical Real-Time Lens-Flare Rendering* describes a first-order matrix approximation of optical ray transfer, trading physical nonlinearity for significant speed. This motivates investigating editor-authored/cooked optical-response coefficients, not copying their algorithm or claiming comparable benchmark performance in UE5.8.

**2. High-end tracing is not automatically the right runtime solution.** Hullin et al.'s 2011 research describes physically plausible lens flare interactions (coatings, aberrations, aperture) and acceleration strategies, but does not establish a free performance budget for dozens of game highlights. Use the underlying optics as reference for *what* to approximate.

**3. Heavy overdraw can dominate many-ghost rendering.** A 2023 research paper on tile-based ghost rendering proposes collecting contributions into screen tiles before raster accumulation to avoid costly per-ghost overdraw. A useful alternative to investigate if Glint's procedural stacks get dense; not mandatory until profiling identifies this as the bottleneck.

**4. One lens can be applied to multiple highlights.** The per-view surface response (dirt/streaks/veiling) should normally accumulate once and be shared, not calculated as a full independent full-screen pass for each light.

## Physical versus artistic controls

Provide a simple **Optical Style** group with Blend/Physical Influence, Aperture, Filmback, Focal Length Response, Chromatic Dispersion, Lens Coating Tint, Dirt and Edge/Oblique Response. Each source/element can retain `Linked` or `Local` override status; numerical controls must remain legible.

The shared profile may include an optional **response field** sampled as a function of source screen UV and focal length. Response channels could influence ghost centroid, 2D scale, rotation, deformation, RGB/dispersion, opacity and falloff. The simplest v1 response can be 1D curves or analytic formulas; small 2D LUTs are a validation candidate. Neither is equivalent to real physical lens prescriptions.

## Crucial distinction: bloom is not full lens flare

- **Bloom** generally spreads light locally around bright areas.
- **Ghosting** puts related images/artifacts at other screen positions determined by source/camera optics.
- **Diffraction or starbursts** add directional ray patterns.
- **Lens dirt** modulates light using camera-attached surface structure.

Unreal has existing bloom and image-based flare controls; don't duplicate them blindly. Glint must cooperate with UE bloom (including bloom disabled), expose per-family toggles, and test how optics appear in pre/post-tonemap HDR.

## Potential research projects after core feasibility

1. Capture a small, user-owned reference dataset: photography of different lenses against controlled point lights, moving across image center/edges, varied focal length and aperture. Use own images or properly licensed reference photos.
2. Fit simple UV-dependent response functions to observed ghost movement, size and deformation; compare to ordinary Optical Flares-style axis placement.
3. Study color response with HDR light temperatures, varied apertures and tonal mapping. Distinguish spectral coating artistic tints from inaccurate claims of physical calibration.
4. Evaluate precomputed PSF/diffraction patterns for different aperture counts, obstruction/irregularities and anisotropic squeeze, including cost of dynamic parameter changes.
5. Benchmark artistically similar output using procedural code, sprites/atlas, reduced-res image warp and optional compact optical LUTs.

## References

- [Lee & Eisemann, Practical Real-Time Lens-Flare Rendering (2013)](https://graphics.tudelft.nl/Publications-new/2013/LE13/PracticalReal-TimeLens.pdf) — study abstract/approach; verify local full text where accessible.
- [Hullin et al., Physically-Based Real-Time Lens Flare Rendering (2011)](https://light.informatik.uni-bonn.de/papers/HullinEtAl-LensflareRendering-SIGGRAPH2011.pdf) — note some public copies may be unavailable.
- [Efficient tile-based rendering of lens flare ghosts (2023)](https://www.sciencedirect.com/science/article/pii/S0097849323001486).
- [Maxon Real Lens Flares](https://www.maxon.net/en/product-detail/red-giant/vfx/real-lens-flares).
- [Maxon Schmutz, background illumination option](https://help.maxon.net/rg/en-us/Content/html/06-Real-Lens-Flares_core-projections-schmutz.html).

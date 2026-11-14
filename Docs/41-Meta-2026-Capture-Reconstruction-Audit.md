# 41 — Meta Reality Labs 2026: Lens Flare Removal and Reconstruction

[← Index](README.md) · [40 — Measured lenses](40-Walch-2017-Measured-Lens-Research.md) · [43 — Synthesis](43-Rendering-Architecture-Research-Synthesis.md)

**Primary source:** Tarun Yenamandra, Jonathon Luiten, Daniel Cremers and Nathan Matsuda, *Lens Flare Removal and Reconstruction*, arXiv:2609.39527v1, September 30, 2026. https://arxiv.org/abs/2609.39527. Complete user-supplied 20-page PDF.

## Distinguish two separate contributions

The research contributes **(A) a diffusion-based flare removal/data set workflow** for clean multi-view reconstruction and **(B) an explicit re-renderable flare model** learned from captured footage using Gaussian splatting. Part A is not itself an interactive lens-flare generator, and the paper does not claim a generic prompt-to-flare authoring editor.

The representation is highly relevant: flares are inherently properties of the camera image, not persistent objects in ordinary 3D world space. The authors use camera-anchored Gaussians, largely arranged along the line from projected light to the camera principal point, with learned deformation for positions, shapes, colors, opacity and asymmetry.

## What the novel model supports

- Reference image and multi-view capture can be decomposed into scene and flare components.
- A view-dependent, source-conditioned model reproduces how *captured* flares move and deform across the reconstructed viewpoints.
- Explicit flare Gaussians can be edited/scaled and transferred to new rendered scenes.
- The modeled source can be off-screen; pure image-threshold-only detection would not recover every offscreen source.
- The authors show improved robustness to imperfect light-source localization by learning position corrections and finer deformations.

**Important limitation:** A model fit to a capture is not a generally correct physical model of an arbitrary lens. In this work, off-screen/occluded light sources may be active based on projected position without general-purpose world-geometry visibility semantics. For Glint, separate **camera-lens stray-light policy** from geometry-occluded source eligibility rather than blindly emitting through walls.

## Performance: read the whole number, not the headline FPS

The paper's **single-light** 1920×1080 experiment on an **RTX 4090** uses approximately 10,965 flare Gaussians and 162,283 scene Gaussians:

| Measured part | Result |
|---|---:|
| Flare deformation network | **3.92 ms** |
| Combined scene + flare rasterization | **6.86 ms** |
| Scene-only rasterization | **6.74 ms** |
| Combined frame | **10.79 ms / 92.7 FPS** |
| Extra rasterization with flares | **~0.12 ms / ~1.8% of scene rasterization** |

**Interpretation:** low *additional rasterization* cost does **not** mean 0.12 ms total flare cost: deformation accounts for nearly 4 ms. The paper reports deformation around **3.9, 6.8, 11.1 ms for 1, 2, 3 lights**. That is incompatible with Glint's desired many-light gameplay default, even before accounting for a full UE scene. These are the authors' specialized 3DGS results, not an UE5 measurement.

## Explicit limits discovered in paper

- Ring-shaped flare representation is challenging because a Gaussian peaks at its center; a direct annular representation is preferable for ring/hoop elements.
- The method depends on capture and training, and is a representation for matching a captured signature rather than procedurally varying one artist-authored flare under any lighting condition.
- Source count grows per-light model cost.
- Training-stage diffusion and Gaussian fitting are not appropriate for v1 runtime work.

## Product/design implications

**Valuable for future Glint:** a Capture-to-Glint workflow, learned response maps, offline fitting, multi-view image matching, explicit source-screen-axis deformation, asymmetry from real captures, offscreen example coverage.

**Not adopted for v1 runtime:** per-source MLP deformation, 3D Gaussian splat renderer, AI flare-removal model, or general data-collection/training requirement.

**Potential future alternative:** distill a captured/learned representation into compact per-element curves, analytic shapes and texture response lookups. This bridges the research to our established Glint runtime architecture; it is our design hypothesis, not a result proved in the paper.

## Validation proposal if pursued

Compare photographic reference and Glint render across yaw/pitch, aperture and focal length; measure image similarity plus temporal consistency; maintain source identity and automatic/manual handling; require inference cost below the render-budget cap. Begin with a deliberately small set of angles and a few known cameras.

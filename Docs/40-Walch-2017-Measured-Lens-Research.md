# 40 — Walch 2017: Measured Lens Flare Prediction

[← Index](README.md) · [41 — 2026 capture-based model](41-Meta-2026-Capture-Reconstruction-Audit.md) · [43 — Architecture consequences](43-Rendering-Architecture-Research-Synthesis.md)

**Primary source:** Andreas Walch, *Lens Flare Prediction based on Measurements with Real-Time Visualization*, TU Wien diploma thesis, March 2, 2017 (97 PDF pages). Accompanying one-page poster is the same project, not independent evidence.

## What problem does it solve?

Accurately matching a real photographic lens requires information (especially proprietary coatings) that frequently cannot be obtained. Walch treats the physical lens as a **black box**: photograph how flares appear when a bright source changes position, fit a compact parameterization to the observed ghost shapes, and evaluate that parameterization rapidly for unseen intermediate camera/light configurations.

This is a direct conceptual basis for a future **Capture Lens Behavior** authoring workflow in Glint. It can reproduce the personality of a specific camera lens without an optical prescription.

## Research workflow

1. Dark, repeatable capture setup with a small bright source; a programmable pan head sweeps camera-light angle.
2. Collect photographs at known source/camera relationships and camera parameters (focal length, aperture).
3. Identify/follow related ghost components across neighboring captures.
4. Fit each ghost with a compact **Bezier-curve silhouette** and intensity/falloff parameters, distinguishing different clipping or curvature on left and right sides.
5. Optimize parameters against the image samples. The thesis describes a cost that considers both intensity and edges, with neighboring-frame fits as initialization.
6. Approximate parameter trajectories between samples using smooth polynomial functions; run the resulting model on the GPU.
7. Apply estimated flare magnitude as an **occurrence estimator** for camera/light arrangements.

The thesis's Chapters 5–8 are the critical engineering sections, especially Chapter 6 (primitive shape), Chapter 7 (fit) and Chapter 8 (polynomial interpolation). Poster summarizes the methodology.

## New Glint opportunities

- **Measured Lens asset/source**: store reference image samples and a compact trained response profile, separate from artist-authored Glint Flare composition.
- **Parameter-driven ghost deformation**: use radial source angle as input for ghost position, geometry, opacity, edge falloff and chromatic bias; store separate curves for different focal length/aperture samples.
- **Optional curve export**: fit captured behavior into standard Glint element parameters (where possible) for Optical Flares-like editing. Missing behavior would remain a baked response, not fictitiously recreated as a simple Iris.
- **Live similarity preview**: drag a light around the Glint editor and compare simulated flare to a photographic reference; add reference-image overlay.
- **Capture coverage/debug**: visualize measured angle/keyframe coverage and extrapolation regions; avoid convincing-looking but wildly wrong behavior outside recorded ranges.

## Important limits

- Capturing a single lens across all incident angles, zoom positions, aperture settings and light spectra can be expensive and inconvenient. Do not make photographic capture a prerequisite for basic authoring.
- The original study emphasizes primarily ghost shapes and source-camera angle, not universal extraction of every scatter, flare, glare and dirt phenomenon for all cameras.
- A point-source capture may fail to represent large area emitters; chromatic spectral and exposure calibration also need handling.
- The paper's demonstrations are measured-lens interpolation, not proof that a general desktop UE plugin can automatically derive editable assets from arbitrary photographs.
- Poster/thesis reported GPU real-time visualization with cost increasing with projected element resolution and count; do not infer modern per-camera timings from that result.

## Recommendation

Build Glint's authored element records and trigger/response curves to support **external response functions** later. Keep full capture, fitting and import as optional/post-v1 research. However, **add a photographic reference image overlay and source-position scrubber to the core editor UX** if they can be implemented cheaply.

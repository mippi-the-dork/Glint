# Dev03 — camera optics and Gameplay Cameras boundaries

The plugin still uses a per-rendered-view Scene View Extension and final `FPostProcessSettings` lens blendables. When `UGlintLensProfile::bFollowCameraAperture` is enabled, it applies an aperture ratio-derived size adjustment to authored elements after all lens blending and before RDG upload. It reads `View.FinalPostProcessSettings.DepthOfFieldFstop`, not a particular camera class, preserving standard/Cine/Gameplay Camera final-view support.

**Important limitation:** The `UGlintCameraBindingComponent` still owns explicit profile crossfades; this does not read Gameplay Cameras internal rig transition weights. UE5.8's Gameplay Cameras post-process blending does not necessarily interpolate custom weighted blendables. A native camera rig node and evaluator-level transition integration remain required before claiming fully native Gameplay Cameras authoring. The renderer and project can operate without enabling Gameplay Cameras at all.

QA: compare manual and automatic exposure; animate Cine Camera aperture; animate a Gameplay Cameras Lens Parameters Camera Node f-stop and enable Apply Physical Camera Exposure; observe opt-in size response; verify off-by-one camera cuts and simultaneous views.

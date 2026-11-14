# 48 — Gameplay Cameras Integration: Present Bridge and v1 Requirements

## Confirmed from uploaded 5.8 source

- `ULensParametersCameraNode` and `FCameraPose` carry physical lens properties.
- `FCameraNodeEvaluationResult::GetViewInfo()` exports to the same `FMinimalViewInfo` used by the main renderer.
- `UPostProcessCameraNode` supplies postprocessing to rig evaluation.
- `FPostProcessSettingsCollection::LerpAll()` uses `FPostProcessUtils::BlendPostProcessSettings`, documented as *not blending custom Blendable objects*. Therefore simply putting `UGlintLensProfile` in a rig's `WeightedBlendables` does not deliver seamless transitions.

## What Dev 01 provides

1. Glint renders at the final UE scene view. Camera rig composition is independent of renderer.
2. With a `Glint Lens` referenced by a Post Process Volume, any camera view covered by the volume receives the same lens response.
3. For player-controlled cameras, `UGlintCameraBindingComponent` may be attached to PlayerController. The `SetupViewPoint` hook reads the final controller setting, then injects the selected lens as a weighted blendable. This is **not** a native Camera Rig node and does **not** automatically track rig transitions.
4. Project Settings > Plugins > Glint selects a default gameplay lens. This currently follows the controller viewpoint hook; editor viewport and non-player render paths require volume or explicit future adapters.

## v1 first-class integration planned

- Create **Glint Lens Camera Node** with native Camera Rig asset authoring and editor presentation, without engine patches.
- Ensure the node communicates runtime lens settings **after** rig evaluation and across rig blends, without relying on `FPostProcessUtils::BlendPostProcessSettings` to blend custom Blendable objects.
- Evaluate whether Camera Variables or custom evaluation services can carry a **blendable numeric optical state** (intensity, sensitivity, chromatic shift, streaks) across transitions.
- Keep discrete profile composition selection and smooth numeric-property blending separate; select a policy for incompatible element stacks (cut vs crossfade).
- Support camera cuts and resets, focal length, f-stop, filmback and anamorphic squeeze.
- Validate multiple simultaneous Gameplay Camera users, split-screen and independent view state.
- Make adapter activation optional so projects without Gameplay Cameras compile and package normally.

## Important limitation

This doc is a source analysis and implementation plan, not evidence that first-class Gameplay Camera node support is complete. The next pass must prove the adapter with the user-provided UE5.8 `GameplayCameras` source and visual tests.

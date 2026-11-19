# 52 — Dev02 Camera Lens Crossfades

## Current implementation

`UGlintCameraBindingComponent` can be attached to a PlayerController and exposes:

- `SetGlintLens(NewProfile, NewWeight)` for an immediate assignment, preserving its previous Blueprint contract.
- `BlendToGlintLens(NewProfile, Duration, NewWeight)` for smoothstep crossfade of two lens profiles.
- `IsGlintLensBlending()` and `GetGlintBlendWeights()` to query state; `AdvanceGlintBlend` provides deterministic test progression.

The component only ticks during an active transition; its outgoing `UGlintLensProfile` remains GC-referenced until the transition completes. If the outgoing or incoming profile is null, the view extension resolves that side to Project Settings' default Lens. Calling a new blend during a running transition uses the previous destination as the new outgoing profile and does not build an unlimited profile chain.

`FGlintSceneViewExtension::SetupViewPoint` emits two independent `FWeightedBlendable` objects while a transition is active. Per-view blending then mixes numeric lens properties, and each authored element gets a normalized contribution from its profile. A bounded strongest-element selection avoids always privileging the outgoing lens when the per-view element budget is smaller than both stacks combined. This deliberately avoids assuming that `FPostProcessUtils::BlendPostProcessSettings` understands custom blendable objects.

## Gameplay Cameras boundary

This component works with Gameplay Cameras **when the camera's final view is evaluated through its PlayerController**; it is not attached to a Gameplay Cameras node or evaluator. It can be driven explicitly from Blueprint/C++ Camera Rig transition events today. Native Camera Rig node support, evaluator-to-Glint automatic lens binding, and independently-blended physical lens focal/aperture properties require a dedicated optional module and further tests against supplied Gameplay Cameras source.

Glint core runtime depends on Engine/RenderCore/Renderer/RHI, not on GameplayCameras. Preserving this boundary ensures a game can disable that plugin without losing automatic flares.

## Follow-on technical tests

1. Verify profile crossfade in PIE on standard `UCameraComponent` and active Gameplay Cameras evaluator.
2. Switch rigs mid-transition, blend between two different authored flare assets, and test rapid cancellation/retargeting.
3. Confirm the behavior with project-default lens and an unset outgoing lens, with no default lens, and with PPV blending in the same view.
4. Inspect the camera-frame lifecycle to avoid a half-frame mismatch between evaluated camera state and `SetupViewPoint` contributions.
5. Add native Gameplay Camera Rig graph node only when evaluator lifetime, build dependency boundaries, and transition semantics are confirmed.

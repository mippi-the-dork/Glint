# 46 — UE5.8 Renderer and Camera Source Audit (Dev 01)

**Source provenance:** User-supplied Unreal Engine 5.8 C++ headers, implementation files and ZIP archives: `PostProcessing.cpp`, `SceneViewExtension.h/.cpp`, `PostProcessLensFlares.cpp/.h`, `Scene.h`, `BlendableInterface.h`, `PostProcessMaterialInputs.h`, `ScreenPass.h`, `SceneView.cpp`, `PostProcessVolume.h/.cpp`, `GameplayCameras/Source/GameplayCameras/Private/Core/CameraNodeEvaluator.cpp`, `PostProcessSettingsCollection.cpp`, and `PostProcessCameraNode.cpp`. Engine source is **not** included in the package.

## Confirmed source behavior

1. `SceneViewExtension.h` provides the per-view `SubscribeToPostProcessingPass(Pass, View, Callbacks, bEnabled)` overload. The older non-view overload should not be used. It defines `MotionBlur` as the extension stage preceding `BL_SceneColorBeforeBloom`.
2. `PostProcessing.cpp`: `EPass::MotionBlur`, followed by the `EPass::PostProcessMaterialBeforeBloom` chain. It invokes `AddAfterPassForSceneColorSlice(EPass::MotionBlur, SceneColorSlice)` regardless of whether the engine MotionBlur pass itself is enabled. This is the plugin's chosen HDR stage.
3. `FPostProcessMaterialInputs` exposes the current scene-color slice, `ReturnUntouchedSceneColorForPostProcessing`, and an optional `OverrideOutput`. The initial GPU implementation defers rendering if an override target is active rather than incorrectly writing to the wrong output.
4. `BlendableInterface.h`: implements `IBlendableInterface::OverrideBlendableSettings(FSceneView&, float Weight)`. `FSceneView` holds a final post-process `BlendableManager`; POD copies can be pushed for render-thread consumption (UE5.8 API: `FBlendableManager::PushBlendableData` and `IterateBlendables`).
5. `Scene.h`: exposes `WeightedBlendables`, camera exposure, default native lens flares, and lens dirt as separate post-processing settings.
6. Gameplay Cameras `FCameraNodeEvaluationResult::GetViewInfo()` exports its evaluated `FCameraPose` and `PostProcessSettings` to `FMinimalViewInfo`. The underlying renderer is therefore shared.
7. Gameplay Cameras `FPostProcessSettingsCollection::LerpAll` calls `FPostProcessUtils::BlendPostProcessSettings`, which explicitly does **not** blend custom Blendable objects. We must not claim seamless Camera Rig lens transitions from `WeightedBlendables` alone.
8. `ScreenPass.h` is view-rectangle-aware and can cope with texture-array slices. `FScreenPassTexture::CopyFromSlice` may perform a GPU copy; the first implementation pays this cost and must measure it.

## The proposed first source-validated path

```text
Gameplay Camera / Cine Camera / regular camera
    ↓ camera pose and FMinimalViewInfo
PostProcess Settings + Glint Lens (IBlendableInterface)
    ↓ per-view POD FGlintLensBlendData
UE5.8 MotionBlur Scene View Extension callback
    ↓ HDR Scene Color (RDG)
4x4 peak-preserving HDR reduction → low-resolution ghosts/streaks
    ↓ HDR full-resolution compositing
Built-in UE bloom, color processing and tonemapping
```

## Limitations and required proofs

- Compilability/linkability and shader registration must be proven inside the installed UE 5.8.3 engine. Source inspection does not establish that the provided plugin compiles.
- GPU threshold interpretation must compensate correctly for `PreExposure` and camera exposure. The current first-pass shader has a *scene-buffer threshold* rather than an exposure-invariant physical brightness threshold.
- Check `PostProcessing.cpp` pass ordering with TSR/TAAU, translucency, and multiple view rectangles. Verify no double composite in secondary cameras.
- Profiling must isolate RDG copies, extract cost, ghost cost, composite cost and total bandwidth.
- Full automatic candidate reduction/top-K; authored sources; temporal identity; memory budgets; HDR clipping rules; partial occlusion; editor preview; Gameplay Camera rig transitions are not yet proven.

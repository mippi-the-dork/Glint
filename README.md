# Glint — Unreal Engine 5.8 Lens Flares

**Development pass 01 / 0.1.3 (Edge Sampling Fix 03)**. Prototype/source package, **not a finished v1.0 plugin or verified packaged binary**. Unreal 5.8.0–5.8.3, Windows 64-bit is the target; this package has not been built against the user's installed Unreal engine from this environment.

Glint is a standalone UE renderer extension that creates camera-lens artifacts from HDR scene pixels, with dedicated editable **Glint Lens** and **Glint Flare** Content Browser assets. Camera-specific configurations can use Unreal's weighted blendable system. The current rendering pass uses a simple low-resolution automatic image field, **not** the final candidate tracker or authorable element-stack renderer.

## Install

1. Unzip the **Glint** folder into `<Project>/Plugins/Glint/`.
2. Regenerate project files and compile `YourProjectEditor Win64 Development` in UE 5.8.
3. Enable **Glint** in Edit > Plugins if needed. Restart to allow shader registration.
4. In Content Browser, right-click and create **Glint > Glint Lens** and, optionally, **Glint > Glint Flare**.
5. For a level-wide automatic lens in editor and gameplay, place or select an unbound **Post Process Volume**. In **Rendering Features > Post Process Materials > Weighted Blendables**, add the `Glint Lens` asset with weight 1.0.
6. Alternatively, select a default **Glint Lens** in **Project Settings > Plugins > Glint** for gameplay cameras. For a camera-specific override attach **Glint Camera Binding Component** to a **PlayerController** and choose its lens asset. The `Set Glint Lens` Blueprint function can change the asset/weight during gameplay.
7. Put a bright emissive mesh in view. Adjust the Lens' **Highlights > Threshold**, **Overview > Intensity**, and **Optics** settings. Glint works independently of Unreal's built-in lens flare toggle and does not require light Actors.

### Runtime commands

```text
r.Glint.Enabled 1
r.Glint.Enabled 0
r.Glint.DebugBrightPass 1
r.Glint.DebugBrightPass 0
Automation RunTests Glint
```

## Current implementation

- UE5.8 per-view HDR hook: `ISceneViewExtension::EPostProcessingPass::MotionBlur`, after scene motion-blur stage and before the `BeforeBloom` material location.
- Three RDG global compute shader dispatches: peak-preserving 4×4 HDR bright extraction, image-based ghosts and horizontal streaks at quarter resolution, then HDR compositing.
- No CPU readback or per-highlight Actor allocation; no GPU work if a view has no Glint Lens blendable.
- Native `UGlintLensProfile` asset implements `IBlendableInterface`. Per-view lens data transferred into `FBlendableManager` as POD; renderer never touches a UObject.
- Native `UGlintFlare` asset with initial element definition schema. Element composition is **data-only in this pass**, not yet rendered.
- Camera and Post Process Volume blendables; project default lens and player-controller camera bindings intended to work with traditional and Gameplay Cameras when their final view is supplied by PlayerController.
- Native automation tests of asset state and controller binding. GPU visual behavior and shipping builds **are unverified**.

## Important limitations of the first pass

- **Gameplay Cameras:** Final view integration is available via a PlayerController camera binding. Native `Glint Lens Camera Node` asset-graph authoring and automatic camera-rig transition blending are **not yet implemented**. `FPostProcessUtils::BlendPostProcessSettings` does not blend custom weighted blendable objects, so an ordinary post-process node in a Camera Rig is **not sufficient** for seamless rig-to-rig lens transitions.
- **Highlight response:** Threshold currently operates on the stored pre-exposed HDR scene image. Physical exposure normalization and time-consistent source classification are next technical gates.
- **No bounded full-detail candidates:** The renderer is image-driven only in this pass. The top-K automatic candidate tracker, authored source rendering, multi-element compositing, occlusion, camera-aware diffraction, optical LUTs, and caching remain roadmap items.
- **Rendering features:** TSR, multiview/split-screen, stereo, SceneCapture, unusual HDR modes, resolution changes, packaged Win64 and render target override paths require manual validation. The first renderer uses a full-resolution HDR compute composite, which should be profiled before shipping.
- **Cooked default lens:** A soft reference in Project Settings may require explicit cooking / Asset Manager inclusion of the referenced asset; a Post Process Volume or hard-referenced camera component is safer for the first test.
- **Legacy Unreal lens flare:** Disable its intensity if you do not want both effects to appear simultaneously.

## Documentation

See [`Docs/README.md`](Docs/README.md) for the existing multi-chapter planning/research library and first source audit + test notes. The original research material has been retained; it describes intended v1.0 scope, **not what is already implemented**.

## Source policy

This archive contains original Glint plugin code, the plugin's own shader and documentation. It does not redistribute the Epic Games Unreal Engine source files supplied for research.

## Build Fix 01 (November 13, 2026)

- Fixed UE5.8 compilation failure in `GlintSceneViewExtension.h`: complete `UGlintLensProfile` definition is now included before `TStrongObjectPtr<UGlintLensProfile>` is instantiated.
- Updated `GlintModule.cpp` to use `FCoreDelegates::GetOnPostEngineInit()` for registration and removal, eliminating the two Glint-specific deprecation warnings.
- No changes to other plugins, Unreal Engine source, rendering algorithms, shaders, or runtime behavior are intended.
- This patch was inspected and statically validated here; **it has not been compiled against the Windows UE5.8 installation**.

The project-wide build may still fail due to other plugins. To validate Glint on its own, build Glint in a clean UE5.8 project with no unrelated project plugins enabled, then run `Automation RunTests Glint`.

## Shader Fix 02 (November 13, 2026)

- Fixed an editor startup fatal error in UE5.8 shader compilation for `FGlintCompositeCS` (`Glint.usf:13`, `BrightSize could not be bound`).
- `GlintCompositeCS` uses the shared HLSL global `int2 BrightSize` for quarter-resolution flare sampling. `FGlintCompositeCS::FParameters` now declares `SHADER_PARAMETER(FIntPoint, BrightSize)` and the RDG pass populates it from `SmallSize`.
- Checked each of the three Glint shader entry points against the C++ parameter structures for this class of mismatch; the other two already declare their required inputs.
- The shader file itself, rendering behavior, and unrelated Unreal plugins were not changed.
- Start the editor with the replacement plugin in place so Unreal can recompile the changed global shader metadata. Retry on the **unchanged** package won't help. It isn't necessary to clear Derived Data Cache to apply this fix.
- Static verification performed; actual shader compilation and runtime execution require testing in UE5.8 on Windows.

## Edge Sampling Fix 03 (November 13, 2026)

- Corrected all-edge stretching observed during actual UE5.8.3 viewport testing. Root cause: the automatic ghost/streak shader clamped out-of-bounds UV coordinates to the last bright texel, repeatedly sampling a bright edge pixel and drawing elongated bars.
- Replaced clamped sampling in `GlintSampleBright` with a zero-outside-image, bilinear-filtered texture lookup. The same helper serves ghost, chromatic and streak taps, so all boundaries receive the fix. The final composite also uses zero-valued texture taps instead of clamping its bilinear neighbors at the viewport edge.
- Removed border replication when the 4x4 HDR extraction cell extends beyond an odd-sized view. Only valid pixels contribute to each partial cell.
- No shader parameter layout or C++ render-pass changes. This avoids introducing a new global-shader binding mismatch. The existing three automated tests passed in the user's UE5.8.3 installation on 0.1.2; the new shader behavior still requires runtime visual validation.
- Run `Automation RunTests Glint` and sweep the sun or a bright emissive object across all four sides and the four corners. The intended streaks should remain finite; there should be no stretched edge rows, columns, or smear after the source exits the screen. See `Docs/50-Edge-Sampling-Regression-and-QA.md`.

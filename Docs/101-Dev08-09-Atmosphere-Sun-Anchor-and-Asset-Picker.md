# Glint Dev08.09: Atmosphere Sun Anchor & Blendable Class Picker

Version: **0.8.9**. Target: Unreal Engine 5.8.x on Windows 64-bit, editor and runtime. Replaces the complete Glint plugin. **C++ build, PCD3D_SM6 shader compilation, and live rendering are not verified in this sandbox.**

## Verified starting evidence

The user ran all **51/51** Glint 0.8.8 native automation tests successfully. The supplied `DebugBrightPass` screenshot shows the sun at the brightest compact region, but Glint tracks nearby cloud highlights instead. Looking away and back can produce 2, 6, or other numbers of tracked source slots. The native Post Process Volume > Rendering Features > Post Process Materials > Choose menu still lists `GlintLensProfile` next to `Asset Reference`. Those are unresolved **manual** failures; automation does not supersede them.

## Root cause and implementation

The 0.8.8 shader ranks HDR tiles and fills selected source slots by brightness, grouping, and recent history. Despite reducing compact grouping, there is no semantic rule tying any bright candidate to the actual sky sun. A small disk can still lose to another regional candidate, fail a relative score gate, or be grouped with an already tracked cloud. Camera cuts/revisits reset or alter the candidate set; source count is not a stable physical-light count. Further tuning cannot guarantee sun identity.

In 0.8.9, `FGlintSelectSourcesCS` now has a second source acquisition path, independent of tiled candidates:

1. Use `View.AtmosphereLightDirection[0]` and `View.AtmosphereLightIlluminanceOuterSpace[0]` from UE's `FViewUniformShaderParameters`. A default directional vector with zero actual atmospheric illumination is ignored.
2. Project the **direction** with the current view's unjittered world-to-clip matrix and w=0, treating the source as infinitely distant. The source center follows the exact camera projection; it does not follow a shifting cloud maximum.
3. Sample the filtered detection texture's preserved **raw HDR peak** channel near that screen location, compared with the surrounding detection-luminance ring. The measured raw peak must exceed both the configured source acquisition gate and the local luminous background. The visible HDR image, not just light presence, determines whether the sun is eligible.
4. When eligible, reserve slot 0 for the camera-projected sun and sample the same HDR source color path used by other sources. Other sources use slots 1–7. On a short occlusion with a matching previous sky slot, keep its exact projected position with decaying confidence instead of teleporting to a cloud.
5. Leave candidate reduction, automatic optics, authored flare asset schema, and the existing non-sky source detection untouched.

CVar `r.Glint.AtmosphereLightIndex`: **0** = primary sky light (default), **1** = secondary sky light. Change this when the sun is authored as Atmosphere Sun Light Index 1; only one index is tracked as a deterministic sun at a time.

CVar `r.Glint.AtmosphereSource`: **0** = disable anchored sun; **1** = require atmospheric light and current HDR validation (default); **2** = if a real atmospheric light is registered and projects onscreen, force the anchor even without enough HDR evidence, **for diagnosis only**. Do not ship a project with diagnostic mode 2 as its default because it can produce flares when a real sun is obscured.

**Important limitations:** This anchors **only atmosphere selected directional-light index (0 or 1; default 0)**, not all point/spot/emissive lights. It does not prove cloud occlusion quality, temporal smoothness of non-sky sources, or constant total source count as the camera changes. The 7×7 detection center and 8-sample comparison ring are intentional small fixed-cost samples in the already single-threaded source-selection shader. User-reported previous GPU capture timings remain the baseline: no performance claim is made without fresh GPU profiling.

## Post Process Materials class picker

`UGlintLensProfile` inherits `UDataAsset` and implements `IBlendableInterface` so existing profile assets are valid for Weighted Blendables. `NotEditInlineNew` cleared `CLASS_EditInlineNew` but user screenshots prove the **class picker still listed the type**. Version 0.8.9 adds `UCLASS(..., HideDropdown)` and a reflection regression assertion for `CLASS_HideDropDown`. This requests omission from property-window class choices without modifying the engine or converting the asset to a post-process material. The **asset reference picker must remain functional**; the native menu must be inspected live, because tests of class flags cannot prove the list is filtered correctly by this particular editor widget.

## Validation available here

- `Tools/CheckShaderContracts.py` validates shader parameter names, direct and helper call paths for each compute entrypoint.
- `Tools/CheckDev089Source.py` validates the atmospheric uniform buffer, projected source path, slot reservation, HDR brightness gate, class-picker flags, and new regression coverage.
- Historical `Tools/Check*.py` are retained.
- Native automation test `Glint.Source.Detection.AtmosphereSunHDRGate` tests sun-versus-background gate behavior and rejects missing atmospheric light.
- Updated `Glint.Editor.Lens.WeightedBlendableAssetReferenceOnly` checks the class visibility flag, but is **not a screenshot/paint test**.

## Required live acceptance — Unreal Editor 5.8

**Setup:** Close the Editor. Replace the entire `<Project>/Plugins/Glint/` directory with this ZIP's one top-level `Glint/` directory; preserve existing `.uasset` profiles in the project's Content Browser. Rebuild `ProjectHeavyWaterEditor` Win64 Development. Launch with the existing D3D SM6 renderer. If a C++ error or shader startup dialog appears, stop and send the complete error text rather than retrying indefinitely.

1. Open **Tools > Debug > Output Log**; enter `Automation RunTests Glint`. Target: **52/52 Success**. Report any failing test names and assertions.
2. Open the same level and unbound Post Process Volume with the existing Glint Lens Profile assigned through **Rendering Features > Post Process Materials > Asset Reference**. Keep your prior exposure, Lens thresholds, and view mode unchanged. In Output Log set `r.Glint.DebugBrightPass 1`, `r.Glint.DebugSources 3`, and `r.Glint.AtmosphereSource 1`. The real visible sun should consistently have a cross **at its disk center**, not at a nearby cloud. Look away, return, and repeat at least five times. Source count can vary for other highlights; the sun must remain consistent while visible.
3. For a direct A/B, use `r.Glint.AtmosphereSource 0` in the same view. The deterministic sun slot disappears; other HDR candidate behavior remains. Set `r.Glint.AtmosphereSource 1` again. If the new sun still fails to appear, use `r.Glint.AtmosphereSource 2` **briefly** while the sun is visible. This diagnostic bypasses the HDR gate but still requires an atmospheric light. If 2 locates the sun correctly while 1 does not, the problem is HDR gating. If 2 projects elsewhere or finds no sun and your directional light uses **Atmosphere Sun Light Index 1**, run `r.Glint.AtmosphereLightIndex 1` and repeat modes 1 and 2. A wrong projection after choosing the correct index suggests an engine view matrix or direction mismatch. Return to mode 1.
4. Rotate slowly by ~5°, then ~30°, then stop and reverse. With mode 3, the **sun** should move continuously with camera projection without jumping to a cloud. Non-sky sources can still shift; report them separately. Also view a partially cloud-obscured sun: it should fade briefly and not immediately spawn a flare on a cloud highlight.
5. In the Post Process Volume Details panel, expand **Rendering Features > Post Process Materials > Array**. Click **Choose**. `GlintLensProfile` should no longer appear as an inline-create class, while **Asset Reference** should remain. Select the existing Glint Lens Profile through Asset Reference, verify existing Lens values are intact and flares work. Send a screenshot if the inline type remains.
6. Solo/Delete icon paint is **not changed in this pass** and should not be assumed fixed. If still blank, note that separately. Source slots are only 8 wide; do not treat an arbitrary frame's total source count as a pass/fail without controlling the scene.
7. Restore `r.Glint.DebugBrightPass 0`, `r.Glint.DebugSources 0`, `r.Glint.AtmosphereSource 1`, `r.Glint.AtmosphereLightIndex 0` (unless your scene actually uses index 1). With identical camera/exposure/resolution and other settings, capture 3+ GPU profiles each using `r.Glint.Enabled 0` and `1`. Compare both total GPU time and the named Glint candidate and tracking passes.

## Next design work

If this deterministic sun anchor passes, investigate reliable scene-depth occlusion/volumetric-cloud visibility and a continuous image-based flare path for some automatic effects, while preserving precise authored source anchors. Other source counts will remain data dependent until a more complete temporal classification system is implemented. Keep this phase focused on correctness and profile cost before wider architectural changes.

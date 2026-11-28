# Glint 0.8.9 — HDR-Validated Sky Sun & Class Picker Fix

**Development snapshot — UE 5.8.x, Win64.** Your 0.8.8 native suite passed **51/51**, but live debug screenshots still show a visible sun without a source marker, unstable numbers of cloud/terrain candidates after camera rotation, and the extra inline `GlintLensProfile` entry inside **Post Process Materials > Choose**.

**0.8.9:** For the selected Sky Atmosphere directional light (index 0 by default; index 1 via `r.Glint.AtmosphereLightIndex 1`), Glint uses Unreal's existing per-view atmospheric light direction to project an **infinite-distance, unjittered sun anchor**, independently of tile candidates and region grouping. It only admits this anchor when the current HDR detection signal supports a visible bright disk; it fades a briefly obscured sun instead of immediately giving its slot to another bright region. Other emissive/reflected sources continue to use the 0.8.8 detector. `r.Glint.AtmosphereSource` defaults to 1 (HDR-validated); 0 disables; 2 is a deliberately **diagnostic-only** forced anchor. A direction without registered atmospheric illumination does not create a sun. Support for directional-light index 1 and generic lights is not implied.

The `UGlintLensProfile` class now uses `HideDropdown` as well as `NotEditInlineNew` to request that Unreal's class picker **not offer inline creation**. Existing `Glint Lens Profile` assets and asset references are retained. This has not yet been verified in the actual UE 5.8 Post Process Volume picker. No global editor customization, engine changes, or asset migration.

**52 native tests registered** (one new source-HDR gate test plus an updated picker reflection assertion). Static checks passing do **not** establish C++ or SM6 compilation, correct sky projection, suitable cloud occlusion, or that the native picker respects `HideDropdown`. [Dev08.09 implementation and manual acceptance](Docs/101-Dev08-09-Atmosphere-Sun-Anchor-and-Asset-Picker.md).

---

**Glint 0.8.7.3 SM6 startup hotfix:** Fixed a reserved HLSL variable (`centroid`) in candidate detection and restored the `[numthreads(1,1,1)]` attribute to the source-selection compute entry point. Unreal shader compilation still needs validation. See [Docs/99](Docs/99-Dev08-07-03-SM6-Candidate-Source-Selection-Fix.md). Previous Glint 0.8.7.2 API fixes and 50 registered tests are retained.

**Glint 0.8.7.2 UE 5.8 API hotfix:** Replaced the deprecated no-AA projection accessor in camera reprojection; the previous user build succeeded with only this Glint C4996 warning. No flare behavior or shader changes. See [Docs/98](Docs/98-Dev08-07-02-UE58-NoAA-Projection-API.md). Run `Automation RunTests Glint` after rebuilding; 50 tests registered.

---

# Glint

> **Development snapshot 0.8.8 (UE 5.8.x):** Compact HDR-source grouping, bounded reprojection correction and direct Slate action images. Requires Unreal/SM6 compilation and manual rendering validation. See [Dev08.08 handoff](Docs/100-Dev08-08-Compact-Grouping-and-Camera-Stability.md). 0.8.7 — HDR Sun Peak Protection, Camera Motion & Native Asset Picker

**UE 5.8.x / Win64, complete source plugin.** Your Glint 0.8.6.1 passed **47/47** Unreal automation tests, but manual QA showed that the sun was frequently excluded even though it was visible in the filtered HDR view, camera rotation produced jerky optical movement, action buttons remained visually blank, and the PP Weighted Blendables picker offered unwanted inline profile creation.

**0.8.7 updates** candidate collection to preserve both the best coherent region and the raw brightest HDR peak per tile; prioritizes compact peaks, and reprojects previous tracked source UVs through unjittered camera matrices **before** matching. It disables inline construction for Glint Lens assets and switches Solo/Delete to direct native `SButton.Text("S"/"X")` for the time being. The full engineering/QA handoff is in [Docs/96](Docs/96-Dev08-07-Peak-Camera-Reprojection-and-UX.md).

**50 native tests registered; C++/SM6 compilation, GPU performance, actual viewport results, and Slate visibility remain unverified for this package.** This pass doubles the number of candidate texels consumed by the tracker; test its GPU cost. The native Unreal image-based flare avoids discrete source tracking, so this is a targeted improvement, not a claim of full parity. Replace the complete `Plugins/Glint` folder, rebuild, run `Automation RunTests Glint`, then compare the sunny scene, camera rotation, and editor controls. No engine edits, no Probe dependency, no intended asset migration.

---

# Glint 0.8.6 — Coherent HDR Detection & Source Challenger Tracking

**UE 5.8.x / Win64 / complete source plugin.** In the user's previous 0.8.5.1 run, all **43 native tests passed**, but manual screenshots confirmed the Solo/Delete images were still invisible and some source positions wiggled while bright clouds/ground frequently beat the visible sun. 0.8.6 adds a *separate low-resolution spatial HDR detection filter*, source-region ranking and centroids, a relative new-source gate, and sustained (including nearby-region) challenger replacement when slots are full. The original HDR scene and raw peak brightness remain unblurred. Solo/Delete now use visible font glyphs **S / X** instead of the unreliable brush-backed action images. Mode `r.Glint.DebugSources 4` shows the filtered detection signal alongside tracked crosses.

**47 registered native tests (four new); Windows C++ compile, SM6 shader execution, actual visual quality, Slate paint, and GPU cost are NOT yet verified for 0.8.6.** No engine edits, no dependency on Probe, and no asset migration. **Reinstall the complete `Plugins/Glint` folder and restart Unreal** to reset the expanded 8×3 source history. Run `Automation RunTests Glint` and see the [full 0.8.6 handoff and manual QA](Docs/94-Dev08-06-Coherent-HDR-Source-Detection.md). Source-only validation: `python Plugins/Glint/Tools/CheckShaderContracts.py` and `python Plugins/Glint/Tools/CheckDev086Source.py`. The previous 0.8.4.1 GPU capture remains a one-frame reference only; this revision may cost *more* GPU time due to its added filter.

---

# Glint 0.8.5 — Source Identity, HDR Tint, and Editor Recovery

**UE5.8 Windows source plugin.** This pass addresses stationary-camera source hopping, erratic confidence-state changes, HDR source-color sampling, and missing Solo/Delete outliner icons. Two added native automation tests cover stationary source identity and HDR confirmation. The `ReleaseHysteresis` test now has an explicit float tolerance. New debug mode `r.Glint.DebugSources 3` colors sources by persistent slot index. **43 native tests are registered; UE compilation and native execution of 0.8.5 are not yet verified.**

**Measured pre-update GPU reference:** one Glint-off frame was 6.0827 ms; a Glint-on frame was 7.2362 ms. The identifiable Glint passes totaled 0.9419 ms, about 87% of that in candidate finding and source selection. A single captured frame pair is a diagnostic reference, not a stable benchmark or a claimed post-update gain.

**QA:** `Automation RunTests Glint`, `python Plugins/Glint/Tools/CheckShaderContracts.py`, `python Plugins/Glint/Tools/CheckDev085Source.py`. Full changelog, limitations, profiler findings, and explicit manual acceptance: [0.8.5 development handoff](Docs/92-Dev08-05-Temporal-Source-Identity.md).

---

# Glint 0.8.4.1 — Shader startup fix

**UE5.8 Windows source plugin.** Fixes the `FGlintGhostCS` SM6 startup compile error in 0.8.4: `streak` is now declared in the enclosing function scope, rather than inside the legacy automatic branch. No optical algorithms or editor behavior were changed. All **41** existing native tests are retained; the most recent user-confirmed native run was **37/37 on 0.8.3**. Compilation and execution of 0.8.4.1 in UE5.8 remain unverified.

**Validation:** `python Plugins/Glint/Tools/CheckShaderContracts.py`, `python Plugins/Glint/Tools/CheckDev0841ShaderScope.py`, and `python Plugins/Glint/Tools/CheckDev084Source.py`. Detailed notes: [0.8.4.1 shader hotfix](Docs/91-Dev08-04-01-Shader-Scope-Fix.md).

---

# Glint 0.8.4 — Stable HDR Sources & Source-Relative Automatic Optics

**UE5.8 Windows source plugin.** The user last verified **37/37 native tests on 0.8.3**. This update adds four tests (**41 registered**); UE5.8.3 compile, GPU and Slate acceptance remain pending.

**New:** source acquisition/release hysteresis, stable previous-source slots, local HDR-weighted candidate centroids, temporal HDR color/brightness smoothing and source-position dead zone. The default automatic effect no longer resamples inverted copies of the sky: it draws source-relative optical rings, discs and bounded anamorphic streaks. The previous automatic image warp remains an explicit diagnostic (`r.Glint.LegacyAutomaticField 1`, default `0`). Solo/Delete glyphs are recentered within their 24px buttons. `r.Glint.DebugSources 2` gives confidence-colored source markers (magenta/amber/cyan). The new Lens Profile controls are **Source Hysteresis** and **Source Position Dead Zone**.

**QA:** `Automation RunTests Glint`, `python Plugins/Glint/Tools/CheckShaderContracts.py`, and `python Plugins/Glint/Tools/CheckDev084Source.py`. See [Glint 0.8.4 implementation, limitations, and detailed manual testing](Docs/90-Dev08-04-Source-Stability-and-Automatic-Optics.md). Test with your normal temporal upscaler and separately at native resolution; all GPU and native test results require real UE5.8.3 verification.

---

# Glint 0.8.1 — Dev08 Manual QA Corrections

**Previous validated version:** 35/35 native tests passed on Glint 0.8.0. **New package:** 36 registered tests, expanded shader contract audits; Windows UE5.8.3 compilation and GUI/GPU confirmation pending.

**What changed:** `r.Glint.DebugSources 1` now paints strongly contrasted magenta plus signs with black outlines at each tracked bright source, at the final image resolution rather than into quarter-resolution flare. The Solo and Delete actions draw their own Slate line geometry instead of relying on brush resources that repeatedly appeared blank. Lens Profile numeric and Boolean rows include an explicit Reset-to-Class-Default control beside the native value editor. No authored element properties, scene-source selection, source grouping, or energy limiting math changed.

**Performance findings (single uploaded GPU `.profViz`):** named Glint passes add up to approximately **0.2775 ms**, and the captured GPU frame was approximately **7.43 ms**. This does **not** explain a 65-versus-85 FPS difference on its own; make controlled captures with `r.Glint.Enabled 0` versus `1` and debug overlays disabled. See [Dev08.1 release notes and QA](Docs/87-Dev08-01-Manual-QA-Hotfix.md).

---

# Glint 0.8.0 — Dev08 Source Grouping, Optical Quality and Editor Fixes

**Previous validated version:** the user ran **32/32 native Glint tests successfully on 0.7.2**, and camera-motion flare dropout improved. **New version:** 35 registered Glint automation tests; Windows build, live GPU behavior and visual UI validation pending.

**Glint-only changes:** Fix Solo/Trash icon resource paths; attach Lens reset arrows to the existing Details rows; add Lens `Source Grouping Radius` (default `0.125`) to suppress multiple tile maxima from a single broad highlight; add `Authored Energy Limit` (default 6, zero disables) to smoothly control overexposed authored stacks without changing automatic ghosts; early out of source element GPU evaluation when safely outside the element's analytic bounds; more natural ring intensity and dust layout; and type-specific Cinematic creation presets for all 26 optical types (existing assets unchanged).

**Install:** Replace `<Project>/Plugins/Glint` only, rebuild `ProjectHeavyWaterEditor` Win64 Development against UE5.8.3, then run `Automation RunTests Glint`. To check shader parameter contracts, use `python Plugins/Glint/Tools/CheckShaderContracts.py`. Compare a detailed 26-element flare with Source Grouping Radius `0` vs `0.125`, and Authored Energy Limit `0` vs `6`; test colored emissive spheres, camera movement, the Solo/Delete icons and per-property reset arrows.

**Documents:** [Dev08 release/QA notes](Docs/86-Dev08-Release-Notes-and-QA.md), [source grouping and energy](Docs/83-Dev08-Source-Grouping-and-HDR-Energy.md), [editor corrections](Docs/84-Dev08-Editor-Brushes-and-Reset-Arrows.md), [optical presets/performance](Docs/85-Dev08-Optical-Presets-and-Performance.md), and the [complete documentation index](Docs/README.md).

---

**Glint 0.7.2 / Source Motion Stability + Lens Profile Reset Arrows.** The 0.7.1 working renderer and editor are retained. GPU source tracking now retains the last valid HDR source color and decays confidence through brief detection gaps, with motion-adaptive position smoothing and reset on camera cuts; `Source Persistence` is exposed under Lens Profile > Source Detection (default `0.65`, range `0`–`0.95`). A Glint Lens Details customization adds native per-property reset arrows using the C++ class defaults, so edited settings can be restored without recreating the asset. The source history texture is now `8×2` (positions/confidence and cached HDR color); no CPU readbacks. Added `Glint.Source.Tracking.MotionAndDropoutPersistence` and `Glint.Editor.Lens.PerPropertyResetDefaults` native automation cases (32 tests registered). **UE5.8.3 compilation, shader execution, and UI behavior are pending validation**. See [0.7.2 technical notes](Docs/82-Dev07-02-Motion-Stability-and-Lens-Resets.md).

---

**Glint 0.7.1 / Dev07 test correction.** The user validated 28/29 native automation tests in UE5.8.3. `Glint.Data.Flare.ElementBudget` failed because it authored 30 elements but expected the new 32-element runtime limit. The test now supplies 40 elements, verifies the runtime truncates to 32, preserves the first and last eligible entries, clamps size, and leaves all 40 editable elements in the Flare asset. No runtime/editor rendering code or shaders changed from 0.7.0. The combined 26-element Cinematic screenshot shows visually excessive accumulated effects; source clustering, more discriminating highlight selection and type-specific Cinematic defaults are future work, not included here. Windows test rerun pending.

# Glint — Unreal Engine 5.8 Lens Flares

**Development Pass 07 / Glint 0.7.0 (Editor Refresh + Expanded Optical Library).** Complete Glint source plugin targeting UE5.8.0–5.8.3 Win64. **29 registered tests; Windows compilation and GPU/Slate validation pending**. The user's previous 0.6.1 suite returned 22/23 passed; the single failure was due to an undersized 16-element test fixture, and Dev07 now exercises the actual 32-element cap.

**Dev07 changes:** fixed editor Undo/Redo refresh; 26 source-aware element types (up from 13), Default/Subtle/Cinematic per-type presets, 32-element per-lens and per-view budgets, more type-specific schematic guides, compact Settings gear with configurable schematic opacity, near-black neutral gray canvas, real icon assets in square drag/Solo/Delete controls, insertion-line feedback for reordering, single-closure circles and an editor-only blue source marker instead of an implicit glowing white source. These changes preserve the source-aware renderer, reference-image backdrop, camera binding and Gameplay Camera compatibility of prior passes.

**Key documentation:** [Dev07 editor UX](Docs/77-Dev07-Editor-Refresh-and-Preview.md), [new optical types](Docs/78-Dev07-Optical-Library-and-Presets.md), [test and manual QA plan](Docs/79-Dev07-Manual-QA-and-Automation.md), [release notes](Docs/80-Dev07-Release-Notes.md).

**Installation:** Replace only `Plugins/Glint`, rebuild against UE5.8.3, then in the Unreal console run `Automation RunTests Glint`. Optional source checks: `python Plugins/Glint/Tools/CheckShaderContracts.py` and `python Plugins/Glint/Tools/CheckDev07Source.py`. Neither static script substitutes for Windows compilation or live GPU validation.

---

**Development Pass 06 / Glint 0.6.1 (Slate Row Build Fix)**. Complete Win64 source plugin for Unreal Engine 5.8.0–5.8.3. The user verified **22/22 Glint 0.5.x native tests passing** and supplied an authored flare screenshot. The ToolMenus editor toolbar integration and 23rd test require a fresh UE5.8.3 build and visual verification.

Glint provides a GPU HDR-driven lens flare renderer plus Glint Lens and Glint Flare assets. The native Flare editor has Schematic, GPU Rendered and Hybrid previews, with editable optical elements. Gameplay Cameras can reach the shared rendered-view integration path, but a dedicated Camera Rig node and automatic rig transition synchronization remain open.



## 0.6.1 — UE5.8 Slate row syntax correction

- Fixed `C2059` in `GlintFlareEditorToolkit.cpp` at the element-row Delete button: a nested `SBox` / `SButton` / `SImage` Slate declaration had one fewer closing square bracket than opening brackets.
- Retains the 0.6.0 preview-pane toolbar strip, neutral-grey schematic background, square Solo/Delete controls, drag-over reorder highlighting, circle seam adjustment, and the 24-element per-lens authored budget.
- Build log confirmed the Glint runtime DLL linked, while GlintEditor stopped at this C++ syntax error.
- **Validation:** balanced the relevant Slate expression and checked archive integrity; UE5.8.3 compilation and visual/automation tests must still be run locally. No other plugins changed.

## 0.6.0 — Editor polish and authored element budget

- Added Schematic/Rendered/Hybrid/Settings to a dedicated right-pane strip, independent of the top asset toolbar.
- Changed square icon actions and drag-over row feedback; switched the schematic workspace to neutral greys.
- Closed schematic circle paths with a small overlap to reduce visible line seams.
- Raised `FGlintLensBlendData::MaxElementsPerLens` from 12 to 24; `r.Glint.MaxElements` remains capped at 24 per blended view.
- The existing 13 element types are unchanged; new families remain future work.

## 0.5.3 — UE5.8 toolbar compile correction

- Replaced the 0.5.2 workaround that accessed private `FAssetEditorToolkit::ToolbarWidgetContent` (C2248) with the supported `GenerateCommonActionsToolbar(FToolMenuContext&)` override. This calls the base method to preserve Save/Browse and appends the Glint mode buttons/settings gear as a native `ToolMenus` widget entry in the trailing toolbar section.
- Eliminated `SWidget`/`SHorizontalBox` mismatched comparisons (C2678) and reworked `Glint.Editor.Toolbar.SameRowAndRegeneration` to validate the actual `UToolMenu` entry and idempotency.
- Added the editor-only `ToolMenus` module dependency. No renderer, flare elements, shader, or other plugins were changed.
- **Verification:** this code passed static source/package checks but has not compiled or run inside UE5.8.3 here. The 23rd test must run on your Windows installation; toolbar placement needs a visual check.

## 0.5.2 — Native toolbar visibility correction (superseded by 0.5.3)

- Fixed the missing **Schematic / Rendered / Hybrid / Settings** buttons in the Flare asset editor. UE5.8's default standalone editor toolbar displayed Save/Browse, but Glint's `AddToolbarWidget` controls were missing in the user's 0.5.1 screenshot.
- An attempt to wrap the native Slate toolbar was introduced here but proved incompatible with UE5.8's private member access; superseded by 0.5.3's ToolMenus extension.
- Added `Glint.Editor.Toolbar.SameRowAndRegeneration` automated coverage. The suite now contains **23 registered tests**; the previously validated 22 tests are unchanged.
- Authored elements and shaders were **not modified**. The supplied GIF opened as a single still image in this environment, so we are not claiming validation of animation.
- Scope: **Glint only**. This package requires UE5.8.3 compilation and visual verification on Windows.

## 0.5.1 — UE5.8 editor compile correction

- Fixed the `GlintFlareEditorToolkit.cpp` include path for Slate drag-and-drop operations: `Framework/DragDropOperation.h` → `Input/DragAndDrop.h`. The existing `DragAndDrop/DecoratedDragDropOp.h` remains.
- The 0.5 editor features, shader pipeline, and tests are otherwise unchanged. Only Glint is modified.
- User's UE5.8.3 build log: `Glint` runtime DLL linked; `GlintEditor` compilation stopped on this include. A fresh build must confirm there are no subsequent Glint editor errors. Unrelated project errors are out of scope.

## Install and verify

1. Close Unreal. Replace the entire `<Project>/Plugins/Glint/` folder, not just individual files.
2. Regenerate project files and build `<Project>Editor` Win64 Development with UE5.8.3.
3. Launch the editor, open a Glint Flare asset, and run `Automation RunTests Glint`. This branch registers **23 tests** (including the toolbar ToolMenus entry).
4. Run `python Plugins/Glint/Tools/CheckShaderContracts.py` to independently verify the six compute shader declarations.
5. Follow [the detailed manual QA checklist](Docs/71-Dev05-Manual-QA-and-Tests.md).

## New in 0.5

- Native toolbar controls for **Schematic / Rendered / Hybrid / Gear**, alongside standard asset actions.
- **Resizable left outliner + selected-element Details**, right visual workspace. Search, drag/drop reorder grip, per-row Solo (supplied Focus-style icon), visibility and trash; no Up/Down/Remove toolbar buttons.
- **Right-click and drag** moves the synthetic source, including outside the camera image. Exposure and White/Red/Green/Blue preview-light controls are in the Gear menu.
- Every element starts with **type-specific placement/scale/appearance** from `UGlintFlare::MakeDefaultElement`, instead of a generic `AxisPosition=-1` template.
- **Shimmer, Sparkle, Caustic** added for 13 authored element families. Seven type-conditioned shape controls are packed to the source-driven shader and GPU editor preview.
- **Camera Frame Guide:** the inner image is visually distinct from the larger offscreen authoring workspace; elements beyond the camera frame remain editable. Extremely distant handles are represented at the workspace boundary.
- **Reference screenshot texture:** Gear > Camera background (Texture2D). Import a screenshot as a Texture2D asset, select it, and Glint places it underneath its transparent GPU flare in Rendered/Hybrid. The inner frame follows the reference texture's aspect ratio, and the choice is persisted **per Flare in editor preferences**, not inside the cooked Flare asset.

**Important limitation:** The selected reference texture is currently a visual backdrop. It does not yet feed Glint automatic source detection, because a screenshot usually lacks the HDR/exposure information used by the runtime. The editor's synthetic light remains movable separately from the texture.

## Testing quick start

In an unbound Post Process Volume, assign a Glint Lens to Weighted Blendables and give it a Detailed Flare. Use colored emissive spheres, with console commands:

```text
Automation RunTests Glint
r.Glint.Enabled 1
r.Glint.Automatic 0
r.Glint.Authored 1
r.Glint.SourceTracking 1
r.Glint.DebugSources 1
r.Glint.DebugSources 0
```

For the editor, verify all three modes, element defaults, shape Details, drag/drop reorder + Undo/Redo, Solo, reference image selection/clearing and restoration, and source motion beyond each frame edge. Test both portrait and wide textures.

## Validation and boundaries

Previous 0.4.0 passed 16 UE automation tests; the user verified all 22 Glint 0.5.x tests before the new toolbar test was added. This branch registers 23 tests, and 0.6.1 has not been compiled or GPU-tested in UE5.8.3 here; the user's prior build log confirmed only the runtime module linked. No Unreal Engine modifications. Only Glint code and assets are in the archive. The README and [full documentation index](Docs/README.md) contain technical and design references.


### 0.8.3 development diagnostics

`r.Glint.DebugFlareOnly 1` shows Glint over pure black while retaining scene-HDR source detection. `r.Glint.FlareResolutionDivisor 1/2/4` switches full/half/quarter flare-buffer resolution (default 2). With **Source Driven Authored** off, authored elements are disabled; automatic processing stays independent. The original image-field authored approximation is available for comparison via `r.Glint.LegacyAuthoredField 1` only. See `Docs/89-Dev08-03-Authored-Isolation-and-Flare-Diagnostics.md`.

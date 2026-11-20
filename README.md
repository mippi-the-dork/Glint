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

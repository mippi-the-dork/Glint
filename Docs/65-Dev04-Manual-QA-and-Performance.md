# Dev04 — Manual QA, Rendering and Performance

## Before starting

Close Unreal, replace the **entire** `Plugins/Glint` folder with the archive, regenerate project files, rebuild `ProjectHeavyWaterEditor Win64 Development` in UE5.8.3, and launch. The global shader `FGlintPreviewCS` compiles on editor startup; report any `PCD3D_SM6` binding error with the C++ log and shader path. Do not clear the entire DDC unless evidence warrants it.

Run `Automation RunTests Glint`; **16 tests** are expected if this branch builds. Capture the complete output.

## Flare editor acceptance

1. Create a Glint Flare asset; double-click and verify the **Schematic, Rendered, Hybrid** controls, list, and one-element Details.
2. Rendered: look for a GPU glow/ghost shape over a dark background. Hybrid: same rendered shapes under schematic markers. Schematic: no GPU background, only guides and markers.
3. Choose `Opposite Ghost` from list. Confirm selection is highlighted in list, in schematic/hybrid marker and in selected Details. Click the corresponding marker; check reverse selection. Try overlapping elements at same axis and screen offset, clicking repeatedly to cycle.
4. Move an element with the mouse and verify `Screen Offset` updates (and persists after Save/reopen). Change Size/Type/Color/Intensity numerically and confirm the GPU preview refreshes. Test Undo/Redo on both drag and Details edits.
5. Click **Solo** on one element: only its contribution should remain in Rendered and Hybrid; asset is not dirtied by Solo. Click again to restore full preview. Toggle visibility: the asset *should* become dirty, and that element should no longer contribute in game or editor.
6. Search by label, then by `Hoop`/`Multi Iris`/`Spike Ball`. Add each type, duplicate, reorder and remove. Reset filter and check list state.
7. Drag the source with Shift, move the X/Y sliders, adjust exposure, switch preview source between W/R/G/B and confirm reactions. Switch modes repeatedly. Close and reopen the asset: preview mode/position/exposure should persist.

## Runtime acceptance

1. Recreate the 0.3 three-emissive-sphere test with `r.Glint.Automatic 0`, `r.Glint.Authored 1`, `r.Glint.SourceTracking 1`.
2. Add the three new optic types; verify they appear in the camera and the editor GPU preview. Compare fallback using `r.Glint.SourceTracking 0` (different visual character expected).
3. Move each bright source to all screen edges and corners; assert no long rectangular streaking. `SourceEdgeFade = 0` should disable intentional fade; try `0.08`, then `0.25` and compare. SourceGain=0 suppresses source-driven authored optics while automatic mode can still remain active.
4. Exercise camera cuts, two views/split screen if available, viewport resizing, standard Camera Component, Cine Camera, and a Gameplay Camera-controlled view. Profile expensive source × element cases.
5. Turn off the Post Process Volume or its Glint Lens: editor GPU preview may still work, but there should be no new runtime flare effect.

## Known limitations

The editor GPU preview is a synthetic **one-light** simulation, not a live SceneCapture. It cannot validate HDR luminance threshold, automatic source selection, occlusion, offscreen lights, or tonemapping. The runtime remains an early render prototype with bounded source tracking; a future pass will add actual scene/live camera preview and deeper optical models. Native Camera Rig authoring remains not implemented. No 5.8.3 Win64 compilation or hardware test can be performed in the source-generation environment.

# Dev03 — UE5.8 validation checklist

## First: startup

- Replace old Glint fully, rebuild editor; verify five new/retained global shaders compile on `PCD3D_SM6` without binding errors. This is a new shader release and startup must be verified before use.
- `Automation RunTests Glint`: twelve native CPU data/binding/reference tests. A pass does **not** prove the GPU shader is correct.

## Visual baseline

- Test `r.Glint.Automatic 1` and `r.Glint.Authored 1` with your existing bright desert scene. Compare 0.2 baseline edge behavior, watch for stretching in all four corners.
- Test `r.Glint.DebugBrightPass 1` (HDR extraction), then `0` (normal output).
- Test `r.Glint.Automatic 0`; `r.Glint.Authored 1`; `r.Glint.DebugSources 1`: bright crosses should stay near actual bright sun/emissive surfaces when Lens.bSourceDrivenAuthored is true and the Lens has a Detailed Flare.
- Try two separated emissive spheres, overlapping spheres, a long bright panel, and a moving point. Evaluate candidate separation, loss/reacquisition and source order switches. Compare `r.Glint.SourceTracking 0` to isolate source-aware authored behavior.
- Pan source through each viewport border/corner and resize viewport between 16:9 and ultrawide. Look for UV distortions, history reuse, ghost seams, ghost bars and animation discontinuities.

## Artist workflow

- Double-click Flare, select each of seven types in list, drag placement; verify the selected element's `ScreenOffset` updates in Details and in level preview after save. Add, duplicate, remove, move up/down; Undo and Redo each action. Save/reopen and confirm offset/element order persists.

## Cameras and exposure

- Verify Post Process Volume, CameraComponent, CineCamera, gameplay camera rig, hard camera cut, crossfade, two local players and nonstandard screen percentage. Try manual exposure and auto exposure. Follow Camera Aperture should be opt-in and should not change v0.2 visuals when disabled.

## Performance gates

- Record `profilegpu` timings at 1080p, 1440p and ultrawide with authored 0/4/12/24 element blends, max sources 1/4/8, and debug off. Compare each RDG `Glint.*` event with source tracking toggled off.
- Do not assert a GPU budget until measured. At high authored × source counts, cost can become problematic despite the bounded source list.
- After full visual pass, test Win64 packaging/cooking and PIE/standalone. No non-Windows compatibility implied.

# 54 — Dev02 Visual and Performance QA (UE5.8.3)

## Required setup

Use a bright sun with clouds, small emissive cubes and a moving camera. Retain the same exposure settings for comparative tests. Prepare a Post Process Volume with the Glint Lens and a Glint Flare asset containing several enabled elements. Avoid enabling Unreal's legacy Lens Flare intensity while comparing results.

## Acceptance sequence

1. **Boot and compiler:** Rebuild editor with v0.2 shader, launch. No global shader binding error or crash. Inspect Output Log for `Glint` warnings.
2. **Native automation:** `Automation RunTests Glint` should discover seven tests: `Glint.Data.Lens.Defaults`, `Glint.Data.Flare.Count`, `Glint.Data.Flare.RuntimePacking`, `Glint.Data.Flare.ElementBudget`, `Glint.Data.Lens.RuntimeSanitization`, `Glint.Camera.Binding.SetLens`, `Glint.Camera.Binding.LensCrossfade`.
3. **Bright pass:** `r.Glint.DebugBrightPass 1`; compare Sun, clouds and emissive sources while adjusting `Threshold`, `PeakMix`, `MaxContribution`. Move camera; check energy and stability at resized viewports and nonmultiple-of-four dimensions.
4. **Automatic only:** `r.Glint.DebugBrightPass 0`, `r.Glint.Authored 0`, `r.Glint.Automatic 1`; sweep the sun across all four sides and corners. Confirm no elongated rows/columns, and adjust `Ghost Count` and `Streak Length` to see effects.
5. **Authored only:** `r.Glint.Authored 1`, `r.Glint.Automatic 0`; assign a new default example Glint Flare to the Lens. Confirm the source glow, mirrored ghost, iris and horizontal streak. Change each element's type/axis/size/color/intensity and verify the result changes. Try Ring, Halo and Starburst as well.
6. **Combined:** `r.Glint.Authored 1`, `r.Glint.Automatic 1`; verify both systems contribute and toggle their CVars independently. `r.Glint.MaxElements 0` should remove authored contributions; `4` and `24` should respect budgets.
7. **Exposure:** Toggle manual vs auto exposure; check that an otherwise fixed scene does not cause sudden threshold popping. Test camera cuts and fast changes in exposure. Report any discrepancy with HDR debug image plus view settings.
8. **Camera:** With a PlayerController binding, call `BlendToGlintLens(LensB, 2.0)` after first assigning LensA. Expect a smooth crossfade; repeat with Gameplay Cameras driving the PlayerController final view. Test both with and without PPV.
9. **Multiple views:** Test PIE, editor viewport, standalone game, aspect ratio/viewport resize, HDR display mode and TSR; confirm no cross-view contamination.
10. **Performance:** GPU profile at 1080p and 4K with no Lens, automatic only, authored 4, authored 12, and `r.Glint.MaxElements 24`. Record times for `Glint.Extract`, `Glint.AutomaticAndAuthored`, and `Glint.CompositeHDR`. Compare cost with/without legacy UE flare and bloom.

## Reporting

Include screenshots for any visual mismatch and the Output Log for crashes or GPU validation errors. The next development pass should not claim source detection, offscreen occlusion, authored camera nodes, or shipping readiness based solely on data automation successes.

# 47 — Dev 01 Build, Manual QA and Tests

## Initial build / installation

1. Use UE 5.8.0–5.8.3 with Win64 and an editor project with C++ build support.
2. Copy the whole `Glint` folder to `<Project>/Plugins/Glint` (do not copy Epic source into the plugin).
3. Regenerate Visual Studio project files and build the project editor in `Development Editor | Win64` configuration. Capture the **first C++ compiler error**, not only MSB3073.
4. Open the editor and check **Plugins > Rendering > Glint**. Content Browser right-click should show **Glint Lens** and **Glint Flare** in Glint asset category.
5. In Project Settings > Plugins > Glint, assign a default lens; validate that gameplay cameras receive it. For editor viewport coverage, use an unbound Post Process Volume with Glint Lens in `Weighted Blendables`.

## Automated tests

Run in UE console or Session Frontend command input:

```text
Automation RunTests Glint
```

Expected registered definitions:
- `Glint.Data.Lens.Defaults`
- `Glint.Data.Flare.Count`
- `Glint.Camera.Binding.SetLens`

These are data tests. They do **not** prove that the shaders render or that packaged content works.

## Visual test matrix

| ID | Scene | Expected result | Gate |
|---|---|---|---|
| GL-V01 | Entirely dark level | No visible flares; no GPU work without active lens asset | R0A |
| GL-V02 | Single emissive white quad; no Light Actor | Automatic flare contribution without a source actor | R0A |
| GL-V03 | One-pixel distant emissive highlight | Peak-preserving downsample should retain candidate energy | R0A |
| GL-V04 | Animated emissive intensity 0 to 30 | Flares respond without engine bloom dependence | R0A |
| GL-V05 | Fixed emissive, vary auto-exposure EV | First-pass limitation: thresholds can drift; record screenshot for fix | R0A open |
| GL-V06 | Fast camera pan through thin neon lights | Check shimmer/flicker; temporal stabilization not yet implemented | R0B open |
| GL-V07 | 1080p, 1440p, 4K; TSR on/off | Compare cost, view-rect alignment, distortion and sharpness | R0A open |
| GL-V08 | Split-screen and SceneCapture | No leaks, hard errors or duplicate effects | Compatibility open |
| GL-V09 | Project default lens | Applied to player-controlled camera in PIE; cooked asset reference verified | Camera proof |
| GL-V10 | Gameplay Cameras rig with binding component on PlayerController | Glint responds to final evaluated camera | Gameplay proof |
| GL-V11 | Two Gameplay Camera Rigs with distinct lens profiles | Crossfade is **not implemented**; expected missing feature | Next pass |
| GL-V12 | Packaged Win64 build | Shader compile and automatic result without editor dependency | Release gate |

## Performance capture

For each scene: record total GPU frame time and individual `Glint.Extract`, `Glint.FieldGhosts`, `Glint.CompositeHDR` events, use RenderDoc or Unreal Insights/RDG Insights as appropriate. Compare enabled vs disabled (`r.Glint.Enabled 0`). Test `r.Glint.DebugBrightPass 1` to isolate the threshold mask.

## Debug output

- `r.Glint.Enabled 0/1`: globally disable/enable the plugin renderer.
- `r.Glint.DebugBrightPass 0/1`: show only the low-resolution bright extraction.

## What to report back

Compiler error with relevant log lines; UE patch version; screenshot with threshold and intensity; 1080p GPU pass timings; whether the Glint Lens asset is present in weighted blendables; results of `Automation RunTests Glint`.

## Glint Shader Fix 02: launch-time global shader compile (2026-11-13)

The first UE5.8 editor launch with Dev01 Build Fix 01 failed during global shader compilation:

```text
FGlintCompositeCS: Shader parameter BrightSize could not be bound to FParameters
```

The composite shader reads `BrightSize` to transform the full-resolution view into the quarter-resolution flare texture. This symbol was included in `Glint.usf` but omitted from `FGlintCompositeCS::FParameters`. The corrected class declares it and initializes it to `SmallSize`. Extract and Ghost stage parameter contracts were also inspected.

**Validation required:** close Unreal, replace the complete `Glint` plugin with v0.1.2, rebuild and reopen. Confirm no `FGlintCompositeCS` shader error, then run `Automation RunTests Glint` and the visual checks above. Shader compilation success does not certify rendering behavior or performance. Avoid retrying without replacing the plugin.

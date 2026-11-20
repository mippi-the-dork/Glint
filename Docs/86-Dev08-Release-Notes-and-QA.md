# Glint 0.8.0 — Dev08 notes and test gates

**Scope:** `Plugins/Glint` only. No Unreal Engine changes or other plugin changes.

**Validated baseline:** The user reported all **32 Glint 0.7.2 native automation tests passed** and camera-motion flare dropout improved. The user still reported invisible Solo/Delete icons and missing Lens Profile reset arrows. The 26-element Cinematic composite looked highly overexposed and duplicated around the sun.

## Implemented

- Fixed PNG resource naming in the native Glint Slate style.
- Switched Lens property reset customization to modify existing Details rows rather than creating duplicate ones.
- Added `SourceGroupingRadius` to Lens Profile → Source Detection, and `AuthoredEnergyLimit` to Lens Profile → Composition, sanitized and blended through the per-view GPU pipeline.
- Added soft authored HDR energy compression while retaining independent automatic rendering.
- Added a conservative early per-element bounding check in HLSL.
- Retuned all 26 Cinematic *creation* presets by family (saved assets unchanged).
- Added deterministic intra-cell dust jitter and subtle stable ring modulation.
- Added tests: `Glint.Source.Selection.RegionalGrouping`, `Glint.Source.Rendering.AuthoredEnergyShoulder`, and `Glint.Editor.Outliner.IconFilesResolve`. **35 total registered**, the previous 32 retained.
- Updated `Tools/CheckShaderContracts.py` for the new GPU shader bindings.

## Verification and limitations

Static shader contract check and Glint source sanity checks pass at development time. These checks DO NOT substitute for UE5.8.3 Win64 compilation, startup shader compilation, RenderDoc/RDG runtime profiling, automation test execution, or a visual QA pass. The Lens reset UI in particular must be verified in the actual asset editor.

## Suggested console test

```text
Automation RunTests Glint
r.Glint.Automatic 0
r.Glint.Authored 1
r.Glint.SourceTracking 1
r.Glint.DebugSources 1
```

Use the same sun and colored emissive spheres as 0.7.2. Check single-element and all-element compositions; compare grouping radius 0/default and energy limit 0/default, run sustained camera rotations, verify camera cuts/edge fades, observe GPU time. Confirm Solo/Delete icons and reset arrows in a Lens Profile asset. See [source grouping](83-Dev08-Source-Grouping-and-HDR-Energy.md), [editor fixes](84-Dev08-Editor-Brushes-and-Reset-Arrows.md), and [optical presets](85-Dev08-Optical-Presets-and-Performance.md).

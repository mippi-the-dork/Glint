# Dev04 — Shader and Plugin Technical Contract

## New runtime interface

`GlintPreviewRenderer.h` provides `FGlintPreviewRequest` and `QueueGlintPreviewRender(UTextureRenderTarget2D*, const FGlintPreviewRequest&)`. Only plain C++ value arrays and an editor-owned render target enter the render command; no flare assets or other UObjects are captured into the render-thread callback. The resource pointer comes from `GameThread_GetRenderTargetResource`, and on the render thread is wrapped as an RDG external texture. `UGlintLensProfile::PackElement` is shared by asset runtime packing and editor preview packing.

`FGlintPreviewCS` declares `FlareElements`, `PreviewOutput`, `PreviewSize`, `BrightSize`, `PreviewSourceUV`, `PreviewSourceColor`, `PreviewExposure`, and `PreviewElementCount`, all referenced in `GlintPreviewCS` / its shared optical function. `BrightSize` is necessary because that shared function uses it to preserve aspect ratio. Dev04's `FGlintGhostCS` gains `SourceEdgeFade` and `SourceGain` bindings.

## Regression protections

`python Tools/CheckShaderContracts.py` checks six global shader entry contracts (five original plus preview) and zero-outside-screen sample helpers. `Automation RunTests Glint` includes the 12 earlier tests and four added tests: `Data.Flare.PreviewPacking`, `Editor.Selection.OverlapCycle`, `Editor.Preview.SoloVisibility`, and `Editor.Preview.SelectionProxy`. The tests exercise value conversion and real editor selection/packing helpers, not just nonfunctional mock versions. They **do not** execute the global shader pipeline; runtime visual QA is mandatory.

## No engine fork

No engine source has been changed and Glint remains two modules, Glint Runtime and GlintEditor. The editor needs Slate, UnrealEd, PropertyEditor and its preexisting runtime module; the GPU preview entry point is implemented in Glint Runtime so it shares the same shader code and cooking path. All experimental camera paths remain additive; no GameplayCameras dependency is required to render Glint.

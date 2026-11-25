# Glint 0.8.6.1 | SM6 shader compilation hotfix

## Problem observed

The user's UE 5.8 editor build completed but project startup failed when compiling `FGlintCompositeCS` on `PCD3D_SM6`. Shader `Glint.usf` line 936 declared an `int2` variable named `point` in the new debug-source mode 4. In HLSL, `point` is a reserved interpolation modifier, so DXC reported `modifiers must appear before type`, `expected unqualified-id`, and `expected expression` in the following texture read.

## Fix

- Rename the mode-4 sampling location variable from `point` to `DetectionPixel` and update the `DetectionTexture.Load` expression.
- Leave filtered HDR detection, source selection/tracking, flare rendering, and the mode-4 visualization mathematics unchanged.
- Add a static source regression gate that rejects the reserved `int2 point` declaration in this path.
- Advance the plugin manifest to 0.8.6.1. The build/cook/SM6 runtime remains unverified in this environment.

## Verification and acceptance

The static shader contract and previous source regression checks must pass locally. This does *not* replace DXC SM6 compilation. Install the entire plugin, restart UE 5.8, let global shaders compile, and run `Automation RunTests Glint` (47 expected). Re-run the 0.8.6 manual source-stability, sun acquisition, debug-source mode 4, and Solo/Delete controls acceptance tests.

This is a narrow corrective hotfix; **no engine files or other plugins changed**.

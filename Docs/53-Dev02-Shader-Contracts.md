# 53 — Dev02 Shader Contracts and Startup Safety

The v0.1.0–0.1.2 startup shader crash was caused by a C++/HLSL mismatch (`BrightSize` missing from the composite parameter structure). Treat parity checks as a release gate whenever modifying shader code.

## Shader entries and required parameter names

- `GlintExtractCS`: `HDRScene`, `BrightOutput`, `SourceMin`, `SourceSize`, `BrightSize`, `BrightThreshold`, `KneeWidth`, `PeakMix`, `MaxContribution`.
- `GlintGhostCS`: `BrightTexture`, `GhostOutput`, `FlareElements`, `BrightSize`, `GhostStrength`, `StreakStrength`, `ChromaticShift`, `GhostSpacing`, `StreakLength`, `GhostCount`, `ElementCount`.
- `GlintCompositeCS`: `HDRScene`, `FlareTexture`, `SceneOutput`, `FullSize`, `SourceMin`, `SourceSize`, `BrightSize`, `FlareIntensity`, `DebugBrightPass`.

`FlareElements` HLSL is `StructuredBuffer<FGlintRuntimeElement>` with two float4 fields. The C++ declaration uses `SHADER_PARAMETER_RDG_BUFFER_SRV` and the payload has a static test checking its 32-byte size. All shaders must compile for `PCD3D_SM6`, and successful C++ compilation alone is not sufficient.

## Regression prevention

The previous edge-stretching regression was caused by clamping out-of-bounds bright samples. `GlintLoadBrightOrBlack` returns black, and `GlintSampleBright` bilinearly interpolates with black beyond the texture edges. Authored element samplers must use **only these helpers**, not direct texture loads with clamped UVs.

Automation tests cover data shape and budget but cannot verify shaders execute, that profiles blend correctly in live camera rigs, or that edge rendering looks correct. Run the manual suite in chapter 54 after each changed `Glint.usf` revision.

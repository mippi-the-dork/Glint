# Glint 0.8.7.3 | SM6 Candidate + Source Selection Startup Fix

## User-reported startup failure (UE 5.8 / PCD3D_SM6)

`FGlintFindCandidatesCS` compilation failed on `Glint.usf:260-267` because `centroid` is an HLSL interpolation modifier and cannot be used as a variable identifier. `FGlintSelectSourcesCS` additionally failed because its `[numthreads(1,1,1)]` attribute had drifted above the helper `GlintReprojectSourceUV` rather than immediately preceding the compute entry point.

## Glint-only fix

- Candidate reduction calls the local position `regionCenterUV`, preserving the original weighted-region math and compact-peak interpolation.
- Place `[numthreads(1,1,1)]` immediately before `void GlintSelectSourcesCS(uint3 id : SV_DispatchThreadID)` (not on the reprojection helper).
- Update `CheckDev087Source.py` to expect the new identifier and add `CheckDev0873ShaderGuards.py`, validating every Glint compute entry point has exactly one directly attached `[numthreads]` declaration and rejecting reserved interpolation modifiers used as variable names.
- Preserve all other 0.8.7 source detection, camera reprojection, and editor code, and both UE5.8 matrix API updates.

## Validation and handoff

- Source-only static checks and ZIP integrity can be run outside Unreal. **These are not proof that the shader compiles on PCD3D_SM6**.
- The latest user build of **0.8.7.2 C++ succeeded**. Its SM6 startup failed with four shader compiler errors; 0.8.7.3 addresses their source causes.
- **50 native automation tests** remain registered. Last user-verified run: **47/47 on 0.8.6.1**.
- Restart Unreal with the updated shader and verify startup, then `Automation RunTests Glint`.
- Manual acceptance still open: sun selection vs cloud/ground highlights, camera rotation smoothness, Solo/Delete button contents, Glint Lens reference picker, and GPU performance.
- No changes to other plugins, no engine source modifications, and no Probe dependency.

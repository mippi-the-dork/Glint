# Dev03 — shader contracts and release gates

`Tools/CheckShaderContracts.py` statically checks five compute shader parameter structures: Extract, FindCandidates, SelectSources, Ghost and Composite. It checks consistency of declared names, shader entry points and presence of zero-outside-image sampling. Unlike UBT/ShaderCompileWorker, it cannot verify shader compilation or GPU semantics. Release requirements include successful startup SM6 shader compile, `Automation RunTests Glint`, no edge artifact regressions, source debug overlay showing eligible highlights, and a GPU timing comparison.

Source packing is three float4 records per element (48 bytes), with per-lens max 12 and per-view cap 24. Source history is eight float4 records stored in a pooled 8×1 GPU texture, keyed by view state. It is reset on camera cuts/viewport resize and pruned for stale views.

Gameplay Cameras: full production native rig node/evaluator and physical optics validation remain open, despite the shared per-view rendering integration being present. No UE6 assumptions are needed for this UE5.8 production target.

# Glint 0.8.4.1: FGlintGhostCS shader startup hotfix

## User report

Project failed at launch during global shader compilation for PCD3D_SM6:

```
/Plugin/Glint/Private/Glint.usf:670:33: error: use of undeclared identifier 'streak'
    : ghost*GhostStrength + streak*StreakStrength;
```

## Cause and correction

Glint 0.8.4 introduced a diagnostic-only legacy rendering branch in `GlintGhostCS`. A `float3 streak = 0.0;` declaration was placed inside `if (UseSourceAutomatic == 0u)`, but the selection of the automatic contribution after that block references `streak`. The variable is out of lexical scope.

Move `float3 streak = 0.0;` to the same scope as `ghost` near the top of `GlintGhostCS`. Leave the accumulation inside the legacy conditional. This preserves both rendering branches and initializes the variable when the new source-relative path is selected.

**Scope:** Only `Glint.usf`, version metadata, this documentation, README, and the new static scope guard changed. There are no renderer math, asset, or editor behavior changes.

## Validation and limitations

- Run `python Plugins/Glint/Tools/CheckShaderContracts.py` to audit declared C++/HLSL entrypoint bindings.
- Run `python Plugins/Glint/Tools/CheckDev0841ShaderScope.py` to catch the accidental redeclaration inside the legacy branch.
- Run `python Plugins/Glint/Tools/CheckDev084Source.py` for 0.8.4 structural checks and 41 registered tests.
- **Not compiled with UE 5.8 in the packaging environment.** The actual startup shader compile in UE is still required.

## Manual acceptance

1. Remove the old `Plugins/Glint` folder and install this complete package into `Plugins/Glint`.
2. Build the project Editor target and launch UE 5.8 (SM6). The `FGlintGhostCS` undeclared identifier failure must be absent.
3. Run `Automation RunTests Glint` (41 tests expected).
4. With automatic flare enabled, verify both the default `r.Glint.LegacyAutomaticField 0` and diagnostic `r.Glint.LegacyAutomaticField 1` render without triggering a shader compile crash.

Do not treat a passing Python shader contract check as a proof of shader compiler acceptance; the previous check did not model local variable scope.

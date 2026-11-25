# Glint 0.8.5.1: UE5.8 view-matrix API compatibility

## Trigger

The user's November 22, 2026 ProjectHeavyWaterEditor Development Win64 build compiled and linked both `Glint` and `GlintEditor`, but emitted one Glint warning at `GlintSceneViewExtension.cpp:519`:

`C4996: FViewMatrices::GetInvViewMatrix: Use GetViewToWorld() instead`

The project build was not successful overall for reasons outside Glint; those are out of scope here. No Glint compilation errors were reported.

## Change

Replace `View.ViewMatrices.GetInvViewMatrix().ToQuat()` with `View.ViewMatrices.GetViewToWorld().ToQuat()` when sampling the unjittered world-space camera rotation used to calculate source-tracking motion. These are the inverse view/world transforms for the same rotation purpose; no change to the temporal algorithm, history schema, shader parameters, or source thresholds is intended.

Update the 0.8.5 source checker to assert use of the current accessor and absence of the deprecated call. Keep 43 registered native tests; this patch introduces no new runtime tests.

## Validation boundary

- Glint runtime/editor C++ compile + link: confirmed by the provided **pre-fix 0.8.5** build log.
- Patched 0.8.5.1: source and static check validation performed outside Unreal.
- A fresh **post-fix** UE5.8.3 build and SM6 shader compilation are still required.
- The full 43-test automation run, stability inspection, and Solo/Delete icon manual inspection remain pending.

## Next action

Install 0.8.5.1 and rebuild to confirm the C4996 warning disappears. Run `Automation RunTests Glint` in Unreal and continue the 0.8.5 manual acceptance plan in `Docs/92-Dev08-05-Temporal-Source-Identity.md`.

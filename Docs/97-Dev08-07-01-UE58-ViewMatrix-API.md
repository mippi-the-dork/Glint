# Glint 0.8.7.1 | Unreal Engine 5.8 world-to-view matrix compatibility

## User build evidence

The user's UE 5.8 ProjectHeavyWaterEditor build compiled and linked **both Glint modules** in 0.8.7. The only Glint diagnostic was warning `C4996` in `GlintSceneViewExtension.cpp:582`: `FViewMatrices::GetViewMatrix` is deprecated; Unreal instructs callers to use `GetWorldToView()`.

The overall project build did **not** succeed because of errors outside Glint. No claims about a successful whole-project build or 0.8.7 native automation run are made.

## Fix

In `GlintSceneViewExtension.cpp`, construct `CurrentNoAAViewProjection` with:

```cpp
const FMatrix CurrentNoAAViewProjection =
    View.ViewMatrices.GetWorldToView() * View.ViewMatrices.GetProjectionNoAAMatrix();
```

This replaces one deprecated API call and preserves the existing matrix multiplication order, no-AA projection, camera-history and GPU tracking behavior. The earlier inverse matrix accessor replacement `GetViewToWorld()` is retained.

The Glint 0.8.7 source-check script now asserts that this call uses the supported Unreal Engine 5.8 API and the deprecated call is absent.

## Verification status and next work

- Python/static source checks and archive integrity are evaluated when packaging this hotfix.
- Unreal C++ build, PCD3D_SM6 shader compilation and 50 registered Glint native automation tests require user validation.
- The outstanding Glint manual acceptance remains: sun preference, rotation stability, S/X icon visibility, and asset-picker behavior. No unrelated plugin or engine files changed.

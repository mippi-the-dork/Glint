# Glint 0.8.7.2 | Unreal Engine 5.8 no-AA projection API compatibility

## Latest verified build evidence

The user's `ProjectHeavyWaterEditor` Development Win64 build **succeeded**, including Glint runtime and editor C++ compilation and linking. One Glint `C4996` warning remains in `GlintSceneViewExtension.cpp:582`: Unreal Engine 5.8 deprecated `FViewMatrices::GetProjectionNoAAMatrix()` and advises `GetViewToClipNoAA()`.

## Glint-only change

`CurrentNoAAViewProjection` now uses:

```cpp
const FMatrix CurrentNoAAViewProjection =
    View.ViewMatrices.GetWorldToView() * View.ViewMatrices.GetViewToClipNoAA();
```

This preserves the unjittered world-to-clip multiplication order and existing temporal camera source reprojection. No shader code, source-selection math, rendering behavior, assets, editor UI, or other plugin files were intentionally changed.

The `CheckDev087Source.py` contract and new `CheckDev0872Source.py` both reject the deprecated matrix accessors.

## Validation and outstanding acceptance

- Source-only Python checks and ZIP integrity should pass when packaged. These are not a substitute for actual Unreal compilation.
- **50** native Unreal Automation Framework tests remain registered; the last user-verified suite is **47/47** in Glint 0.8.6.1. Run `Automation RunTests Glint` on 0.8.7.2.
- Windows C++ compile of this specific accessor, SM6 shader execution, and manual acceptance still require user validation.
- Manual Glint follow-ups remain: sun selection, smooth camera motion, Solo/Delete button rendering, and Post Process Volume asset picker behavior.

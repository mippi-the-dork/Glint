# 51 — Dev02 Automatic + Authored Field Renderer

## Purpose and status

Development pass 02 of Glint implements an initial **authored flare element render path** while preserving the 0.1.3 shader edge fix. It is a low-resolution image-field technique, **not** physically correct per-source optical path tracing or the eventual top-K candidate renderer. Changes are authored for UE 5.8.3 source APIs and await actual Windows compilation and GPU validation.

## Runtime flow

1. A PPV, camera blendable or PlayerController binding supplies one or more `UGlintLensProfile` assets.
2. On the game thread `UGlintLensProfile::MakeRuntimeData` copies scalar settings and up to 12 active authored `FGlintElement` entries into a POD `FGlintLensBlendData`. The render thread receives no asset pointers.
3. In `FGlintSceneViewExtension::AddGlintPass`, weighted blendable entries resolve numeric properties (weighted mean) and collect authored elements (per-item profile weight, later normalized). At most 24 elements are uploaded to the GPU, adjustable by `r.Glint.MaxElements`. When the budget is exceeded, the highest weighted-intensity contributions are retained across profiles, rather than simply keeping the first profile's elements.
4. The existing 4×4 HDR extract shader retains isolated bright sources and supports configurable `PeakMix` and `MaxContribution`. `Threshold` is multiplied by `FSceneViewStateInterface::GetPreExposure()` if available.
5. A quarter-resolution shader evaluates the adjustable automatic ghost/streak field and a loop over the authored elements. Elements are read from an RDG structured buffer with two `float4`s per element; no CPU or GPU individual highlight list is constructed.
6. The existing composite shader combines the low-resolution effects into the pre-tonemapped scene color. UV taps outside the field return black, not replicated edge pixels.

## Authoring mapping

The runtime element buffer is 32 bytes per item:

| Data | Content |
|---|---|
| `ColorStrength` | RGB color; alpha component stores nonnegative element intensity |
| `Geometry` | Element family number; optical-axis position; relative size; rotation radians |

Type order in `EGlintElementType` must match the shader's 0–6 dispatch: Glow, Halo, Ghost, Starburst, Streak, Iris, Ring. The axis convention is **+1 = stays on original highlight**, **-1 = mirrored about screen center**; values close to zero are clamped in the inverse mapping for numerical stability, rather than behaving as true center-locked elements. That center case requires a future source-aware representation.

Glow uses a 5-tap soft field, Halo and Ring use eight circular samples, Ghost samples the transformed HDR field, Starburst uses crossed pairs, Streak uses a rotated line, and Iris combines source and radial responses. These are visual approximations, not optical ray tracing. When many highlights are present every one contributes to the field.

## Costs, scopes and guarantees

- Added cost scales with `viewport quarter-resolution pixels × enabled authored element taps`, not with number of Actor lights. Upper bound is 24 elements per view; a single asset contributes at most 12.
- CVar `r.Glint.Authored 0` skips authored accumulation; `r.Glint.Automatic 0` independently disables automatic ghosts and streaks. Both off returns untouched Scene Color except during bright-pass debug.
- RDG structured-buffer upload is per rendered view; not per highlight, no CPU readback.
- Future work: sparse/highlight-candidate generation, top-K ranking, stable projected source coordinates, temporal source matching, bokeh textures, occlusion, multi-resolution near-sun path, optimized post-process composition and graphical editor.

## Visual correctness risks

- Screen-space field ghosting affects all bright scene pixels and can create ghost clouds. Use the `PeakMix`, `Threshold` and contribution cap to manage this until source candidates exist.
- Manual bright-pass inspection is needed to establish how pre-exposure compensation behaves in every camera and view class.
- Gaussian-like ring kernels are approximate; quarter-resolution small textures can appear pixelated.
- Axis positions close to zero are approximations and should not be used for critical visual content.

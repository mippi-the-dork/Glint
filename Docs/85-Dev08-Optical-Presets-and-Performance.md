# Dev08 — Per-family Cinematic presets and analytic performance

The user's 26 individual Cinematic screenshots showed overly perfect rings, oversaturated veiling, repetitive star streaks, and regular dust. The prior **universal Cinematic multiplier** enlarged each type by 1.3× and increased intensity 1.5× regardless of optical role.

## Authored starting presets

`UGlintFlare::MakePresetElement()` retains the unchanged serialized enum and the existing Default/Subtle options. **Newly created Cinematic** presets now select behavior by optical family: soft luminous components expand more gently, the Veiling Glow has a smaller/controlled veil, rays have restrained sharpness, rings widen rather than becoming harsh circles, and dust/shimmer/caustic are restrained and less repetitive. Saved flare elements are not silently rewritten.

## Analytic geometry changes

The source-aware shader uses a stable, subtle angular brightness variation for circular/perimeter optics, and the Dust Cluster now jitters each candidate within its cell with deterministic hashes rather than a uniform grid of cell centers. There is no time-seeded noise to flicker when the camera is stationary.

For most elements the shader now does a conservative source-centered bounds reject **before rotation/trigonometry**. This optimization still needs GPU profiling with max source and element budgets. Existing camera image sampling keeps the validated zero-outside behavior.

## Acceptance

Run the same single-source 26-family visual matrix the user used with Cinematic. Confirm (a) lens families are still distinct, (b) rings have softer optical variation, (c) dust placement is less regular, (d) Veiling Glow no longer dominates, (e) the new all-on composition is more coherent, and (f) automatic HDR behavior and colored emissive highlights remain valid. Compare the same saved 0.7.2 Flare assets to prove no automatic migration of existing authoring.

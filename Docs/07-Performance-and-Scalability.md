# 07 — Performance and Scalability

[← Documentation index](README.md)

## Performance philosophy

A gorgeous 2 ms effect can be unacceptable for 120 FPS; a limited but efficient 0.3 ms effect may be much more useful to a game. Optimize from **GPU milliseconds and screen pixels**, not only element count. An enormous translucent streak can cost much more than many tiny ghosts due to pixel fill and blending.

## Proposed reference scenarios (targets, not measurements)

Lock reference PC GPU/driver/CPU, viewport resolution, exposure, scene and RHI before using these as release gates. These are **initial aspirations** to revise after the renderer spike.

| Scenario | Example | Initial incremental GPU target |
|---|---|---|
| Authored only, idle | Automatic disabled, no authored sources | Near-zero incremental GPU work; bypass flare passes |
| Automatic idle | Auto enabled, no qualifying bright pixels | Bounded reduced HDR detection work; costly field/candidate composition skipped |
| Single hero | 1 source × up to 24 mixed elements, 1080p | ≤ 0.30 ms, median |
| Authored-only gameplay | 8 visible sources × 10 elements, 1080p | Original aspiration ≤ 0.60 ms median, ≤ 0.80 ms p95 — **not** a total automatic pipeline budget |
| Automatic gameplay | Bright highlights + field + bounded candidate sources, 1080p | **Determine after R0 GPU profiling**; do not publish guessed target |
| Stress | 32 visible sources × 12 elements, 1080p | No pathological spikes; degrade gracefully under budget |

**Authored-only CPU target:** typical gameplay preparation at or below **0.15 ms median** on a defined reference CPU. All numbers are provisional and hardware-dependent. Do not advertise as achieved until measured in a cooked build. GPU targets will be renegotiated if a better documented tradeoff exists.

## Performance mechanisms

- **Batch sources by view**, group compatible element shaders/textures, upload compact instance arrays.
- Reuse a **shared texture atlas** where appropriate; avoid texture switches and one-off dynamic material instances per element.
- **Early source cull:** behind camera, too distant, too small, too far offscreen, disabled, occluded below threshold (when safe), low priority.
- **Per-view maximum visible sources** with hysteresis to avoid pop when priority ranks shift.
- **Element LOD:** remove tiny ghosts, collapse rich starbursts into simpler shapes, reduce aberration and soft occlusion taps, avoid unneeded texture sampling.
- **Render resolution:** lower resolution for diffuse/glow elements, preserve fine streak sharpness with a full-res or analytic path as needed; compare real cost/quality instead of assuming half-res is always faster.
- **Skip work:** no dirty-lens pass or authored depth work if no enabled outputs require it. However, auto mode with zero registered sources must still evaluate a bounded HDR bright mask, then early-out expensive downstream work if no qualifying highlights.
- **Cache/static preparation:** preprocess asset geometry and curve LUTs, avoid allocating or rebuilding static asset data per tick.
- **Texture streaming:** sensible asset references and atlas dimensions; no runtime blocking loads.
- **No CPU GPU synchronization:** no per-flare readbacks or tracing by default; use GPU depth sampling for source occlusion.

## Scalability tiers (proposed)

| Tier | Occlusion | Elements | Rendering | Recommended use |
|---|---|---|---|---|
| Off | None | 0 | No flare work | User setting / performance fallback |
| Low | 1 depth tap | Critical/core only | Simplified and scaled | Low-end desktop |
| Medium | Reduced soft taps | Moderate | Mixed resolution | Default game |
| High | Higher-quality soft taps | All within budgets | Higher-quality streaks/artifacts | Hero/cinematic |

Tier controls should be both **project-configurable** and overrideable through UE scalability or console variables with documented precedence. Separate *visual authored intensity* from *quality tier* so low quality doesn't just make everything dim.

## Metrics

Report per view or aggregated with clear labels: number of registered sources, projected sources, occlusion-visible sources, rendered elements, GPU draw calls, texture switches, render-resolution mode, occlusion taps, CPU update time and GPU pass timings.

Define **actual render cost** using GPU profiling (`stat gpu`, Unreal Insights GPU tracks where available, RenderDoc); compare identical scene with plugin On vs Off. Measure in **packaged Development or Shipping-compatible test configurations**, not editor FPS alone.

## Benchmark methodology

1. Define machine and driver, UE 5.8 patch, RHI, AA/TSR, output resolution, exposure/PP settings, and test map.
2. Warm up shaders, source assets and streaming; capture stable frames, avoiding first-frame PSO compilation.
3. Hold camera/visible geometry constant for On vs Off comparison; use fixed seeds and stable source movements.
4. Record CPU/GPU median, p95, p99, minimum FPS only as secondary context; collect 1,000+ representative frames where practical.
5. Repeat at 1080p, 1440p and 4K, varying source count, screen coverage, occlusion samples, texture complexity and quality tier.
6. Repeat with dynamic resolution on/off and TSR on/off; compare screen-space stability.
7. Benchmark draw-heavy neon scenes and black/dark scenes. Make sure no-sources scenario is measured explicitly.
8. Include a real representative gameplay map, not only an empty benchmark stage.

## Performance acceptance

Do not ship based only on benchmarks that hide overdraw, avoid occlusion or render one tiny source. If project quality targets are missed, first reduce pass count and fill, then offer explicit quality fallbacks. Record the actual results in a release benchmark table.

## Automatic mode cost accounting (v1.0)

Measure `HDR threshold/reduction`, `tile maxima / region merge / top-K`, `per-view temporal IDs`, `field ghost/streak passes`, `automatic full element instances`, `authored instances`, `shared lens dirt`, and `HDR compositing` separately. Field cost scales primarily with the target texture footprint and gather count, whereas detailed candidates scale with top-K and element complexity. Keep **hard source caps** and a downgrade path from full ghost arrays → simple starburst/glow → image field only.

Revised benchmark groups: Off; Auto On/zero bright pixels; Auto Field Only (1 and 100+ highlights); Auto Candidate (1,8,16+); Authored Only; Hybrid with overlap; native Bloom enabled/disabled; no UI-triggered false positives. Use the complete fixtures and method in [28](28-Automatic-Mode-Testing-and-Release-Gates.md). The original authored source cost estimates are **provisional goals only** and cannot be advertised as Glint automatic performance.

## Perceptual optimization architecture — 2026-11-12 addition

**See [32 — Performance Illusions, Perceptual LOD, and Rendering Budget](32-Perceptual-Optimization-and-Render-Budget.md) for the technical specification and [33 — Experiments and Gates](33-Performance-Experiments-and-Gates.md) for adoption criteria.** These documents extend this budget without changing any *unmeasured* targets into promises.

Baseline architecture commitments: **mixed-resolution soft/sharp paths; shared per-view highlight extraction and lens dirt; GPU-only bounded top-K candidate descriptors; instanced compatible element families; fixed structural budgets; graceful energy-aware quality LOD; observable per-pass GPU costs; deterministic history/camera-cut invalidation.** Tile-binned accumulation, async compute and dynamic GPU time feedback are research branches, not initial mandatory complexity.

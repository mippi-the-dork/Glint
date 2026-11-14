# 33 — Performance Experiments, Instrumentation, and Release Gates

[← Documentation index](README.md) · [32 — Performance Illusions](32-Perceptual-Optimization-and-Render-Budget.md) · [28 — Automatic Lens Release Gates](28-Automatic-Mode-Testing-and-Release-Gates.md)

> **Status:** Proposed experiments and pass/fail gates for the *future* UE5.8 implementation. Targets marked illustrative must be replaced with real numbers from fixed hardware. Tests relying on a GPU render must not be reported as passing merely because data-only tests pass.

## Experiment hierarchy: prove simple wins before complex ones

| ID | Candidate optimization | Compare against | Measurements | Gate to adoption |
|---|---|---|---|---|
| **P0-01** | Soft half/quarter resolution | Full-resolution glow/dirt | GPU ms, VRAM/transient peak, edge/gradient alias | Significant reduction without visible blur/popping in normal gameplay motion |
| **P0-02** | Sharp high-res path separated from soft | One full-res all-elements pass | Pixel stability, resolution, GPU ms | Thin rays retain shape and TSR stability |
| **P0-03** | Fixed global top-K and simpler fallback | Unlimited full compositions | GPU ms slope vs highlights, visual identity | Bounded work at 1, 8, 64, 256 bright regions |
| **P0-04** | Compact GPU candidates with no readback | CPU-readback prototype (debug only) | CPU/GPU latency, hitches, pass count | No render-thread/game-thread stall in normal path |
| **P0-05** | GPU instancing by primitive family | Individual draw for each primitive | CPU submission, GPU ms, texture binds | Lower total cost; preserve ordering and per-element appearance |
| **P0-06** | One shared dirt response | Per-source dirt overlays | GPU ms, overall intensity, screen-area invariance | Comparable authored look without source-linear dirt cost |
| **P0-07** | Empty/disabled paths | Full pipeline every view | Idle GPU ms, CPU ms | Entirely disabled is near-zero; empty auto retains only bounded detection |
| **P1-01** | Small optical response LUT | Analytic shape/calculation | GPU ms, bandwidth, look parity | Lower cost after texture lookup and cache costs |
| **P1-02** | Starburst/shimmer atlas | Procedural multi-ray shading | Visual quality and GPU ms at 1080/4K | Faster for relevant sizes; fine shapes don't smear |
| **P1-03** | Adaptive authored occlusion taps | Constant expensive multi-sample | Occlusion edges, GPU cost | Partial occlusion still stable; hard cap on taps |
| **P1-04** | Stable limited-rate optical evolution | Every-frame full evolution | Lag, flicker, quality, GPU ms | No noticeable latency in sprint/cut/zoom cases |
| **P2-01** | Tile-binned ghost accumulation | Instanced quad blending | Overdraw, GPU ms at low/mid/stress counts | Faster at realistic source mixes; fallback for small counts |
| **P2-02** | Async compute extraction | Synchronous RDG compute | Net frame time, barriers, overlap | Improves actual GPU critical path, not just individual pass timer |
| **P2-03** | Adaptive measured-budget controller | Fixed caps | p95/p99 frame times, tier changes, popping | Better sustained behavior with stable visual quality |
| **P2-04** | Reusing native bloom resources | Own Glint bright-pass | GPU ms, correctness with bloom disabled and custom PP | Correct in every supported rendering mode and materially cheaper |

**P0** = design/implement in first runtime architecture; **P1** = carry schema hooks and test before v1 release where feasible; **P2** = research branch, not v1 mandatory.

## Reproducible benchmark scenes

1. **Dark control:** fixed exposure, no intense pixels. Validates reduced auto detection and bypass.
2. **Single star:** one small intense emissive point. Validates point preservation across downsampling.
3. **Neon stress:** 10/50/100 elongated emissive strips and small point highlights. Tests grouping, top-K stability and bounded cost.
4. **Specular carousel:** moving shiny props with transient single-frame highlights. Tests temporal tracking and missed flashes.
5. **Hero cinematics:** one authored complex Glint Flare with 24+ element records and high-resolution glints.
6. **Overlap:** eight world-authored light sources matching eight automatic highlights; dedup must remove duplicates without fading intended look.
7. **Occlusion sweep:** source partially obscured by a moving opaque object, then by masked/translucent material (record supported differences).
8. **High coverage:** giant glow, diagonal streak, dense iris chain; tests worst-case fill/overdraw at 1080p/1440p/4K.
9. **Camera cuts and motion:** 180-degree turn, rapid pan, zoom, exposure jump, aspect ratio change, dynamic resolution.
10. **Multi-view:** two different cameras/split-screen where supported; no cross-view history/candidate leaks.

## Capture method

- Baseline: compare identical frames with plugin entirely `Off`, then `Automatic Idle`, `Field Only`, `Candidate Only`, `Authored Only`, `Hybrid`, `All High`.
- Record build, exact UE5.8 patch, Windows/driver, GPU, CPU, power profile, RHI, view resolution, render resolution/screen percentage, TSR/TAA setting, bloom and exposure settings, camera path and Glint tier.
- Warm PSOs/streaming/shaders. Repeat in packaged **Development** executable; retest representative cases in Shipping config where telemetry is possible. Editor FPS is not sufficient.
- Use RDG GPU scope timers, `stat gpu`, Unreal Insights, RDG Insights / RenderDoc. Inspect resource lifetime and aliased peak memory separately from GPU time.
- For each condition, record median, p95, p99, worst-case spikes, CPU submission/update time, GPU pass cost and total *delta* vs disabled. Use 1,000+ comparable frames if practical.
- Confirm zero shader compilation, synchronous GPU readback, map loading or asset streaming spikes in warm steady-state tests. Log cold-start/warmup separately.
- Quality evaluation must include moving camera videos and images, not only static reference frames. Record source-selection pop count, thin-streak shimmer, ghost trailing, exposure jumps, shape drift, and occlusion edge artifacts.

## Fixed structural budget contract

The following are **experimental starting points, not shipping defaults**: Premium Candidate slots 4/8/16 across low/medium/high; single camera budget on simple candidates perhaps 8/16/32; soft fields at quarter/half/half width & height, with an independent full/half thin-detail pass. Actual values come from P0 tests. Make these caps overridable by project scalability while preserving obvious artist-visible behavior.

`Glint` should always be able to explain why a candidate received Full / Simplified / Field-Only / Rejected treatment. Rejection causes include below threshold, area/contrast score, duplicate, occluded authored source, outside screen policy, and budget cap. These labels are part of debug diagnostics, **not** an excuse to expose dozens of controls in the everyday Lens editor.

## Automated test candidates

Proposed native automation families (subject to implementation):

- `Glint.Perf.Budget.TopKStable` — repeatable ranking with budget margins and no unwanted slot thrash.
- `Glint.Perf.Budget.ClassLimits` — point/area/elongated sources each consume only configured classes.
- `Glint.Perf.LOD.TransitionEnergy` — preserve approximate integrated appearance across LOD transitions and fade smoothly.
- `Glint.Perf.LOD.SharpElementRouting` — thin streak/iris edge selects correct shader/target path.
- `Glint.Perf.RenderPacket.Grouping` — compatible draw family grouping without changing nonadditive compositing order.
- `Glint.Perf.Candidate.GPUOnly` — runtime source contract does not require per-frame synchronous readback.
- `Glint.Perf.ViewIsolation` — histories, candidate buffers and caps independent per view.
- `Glint.Perf.Temporal.Invalidation` — cut, resize, zoom discontinuity reset temporal caches correctly.
- `Glint.Perf.Disabled.ZeroPasses` — known-disabled path does not schedule Glint rendering.
- `Glint.Perf.Empty.BoundedDetection` — auto no-highlight still performs only permitted bounded detection.
- `Glint.Perf.LUT.CookDeterminism` — compiled lookup data stable between runs and source asset edits update cook output.

**Automation tests cannot prove GPU milliseconds on every user machine.** A separately measured packaged benchmark is the acceptance gate for actual performance and quality; native tests prove correctness and resource/control-flow invariants.

## Decision record expected after each spike

```
Hypothesis:
Scene and hardware:
Baseline architecture and enabled effects:
Candidate architecture:
Measured median/p95/p99 GPU delta:
Pass breakdown and peak transient memory:
Visible quality differences (still + motion):
Unacceptable edge cases:
Decision: Adopt / Defer / Reject
Next required experiment:
```

## Explicit release blockers

- Automatic mode cost scales without bound with source count or frame resolution because there is no functioning cap/fallback.
- Per-frame GPU→CPU synchronization in the normal detection path.
- A source that oscillates around a budget threshold visibly pops at normal camera speeds.
- Lens identity collapses at Low so that changing quality also substantially changes artistic tint/strength for no optical reason.
- TSR, exposure, viewport resizing or camera cuts produce persistent artifacts in required configurations.
- No credible stage-by-stage GPU profiling and no packaged-build benchmark.
- Editor authoring data and runtime data disagree so presets lose their intended look at different tiers.

## Evidence links

See [32 — References](32-Perceptual-Optimization-and-Render-Budget.md#references-and-implementation-evidence) and [07 — Benchmark Methodology](07-Performance-and-Scalability.md#benchmark-methodology).

## Cross-engine performance experiments

Add to the required benchmark scene list:

1. Pixel-sized intense emitter vs large area wall: compare peak, mean, and energy/coverage reductions at 1080p and dynamic resolution.
2. Two closely spaced headlights versus one elongated LED bar: confirm candidate grouping does not collapse or duplicate everything.
3. Bloom enabled/disabled, differing postprocess thresholds and camera exposure: independent Glint extraction must remain correct.
4. 40 wide overlapping iris ghosts: compare baseline instanced quads against potential tile-binning only if baseline overdraw is dominant.
5. R11G11B10F / RGBA16F intermediate passes on reference GPU: measure total pipeline time and HDR/compositing correctness rather than only memory footprint.
6. Built-in Unreal lens flare enabled simultaneously: editor detects double effect and setup docs state how to disable one system.
7. Foliage silhouette across a bright finite source: compare uniform fade with optional source-mask-driven deformation, record temporal stability and cost.

See [37](37-Cross-Engine-Findings-and-Prototype-Gates.md) for criteria.

## New paper-based caution

The 2023 tile paper reports 4.04 ms and 12.79 ms at 1080p on RTX 3060 for two optical configurations. The 2026 capture reconstruction paper reports 3.92 ms **flare deformation**, 6.86 ms **scene-plus-flare raster**, and 10.79 ms **combined frame** on RTX 4090 for one light; only the additional rasterization was ~0.12 ms. These numbers make **total incremental cost** the only appropriate Glint benchmark. No ML or physical ray-grid renderer belongs in v1 unless it dramatically beats simple image-field and quad baselines. See [39](39-Tiled-Ghost-Rendering-2023-Audit.md) and [41](41-Meta-2026-Capture-Reconstruction-Audit.md).

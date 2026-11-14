# 28 — Automatic Mode: Native Tests, Performance and v1.0 Gates

[← Index](README.md) · [08 — General QA](08-Testing-and-Validation.md) · [16 — Original spikes](16-Technical-Spikes-and-Validation.md) · [22 — Detection](22-HDR-Highlight-Detection.md)

## Revised engineering milestones

| Milestone | Deliverable | Release gate |
|---|---|---|
| **R0A — HDR injection** | Packaged minimal UE5.8 plugin with HDR bright-pass visualization, scene source, test output and correctly managed RDG textures | Read scene HDR at useful stage; correctly handle pre-exposure; no engine source edits |
| **R0B — Automatic field** | Actual bright emissive/specular image creates simple ghost + streak without a source component | Works in editor, PIE, cooked Win64 with UE bloom on **or off** and native flares independently switchable |
| **R0C — Automatic candidates** | GPU peak/region detector emits bounded source descriptors driving Glow/Iris or Glow/Streak composition | Stable tracking/top-K; no game-thread readback or Actors; performance measured |
| **R0D — Authored integration** | World + directional source share one renderer with automatic sources | Occlusion correct for authored; dedup policy demonstrable; no doubled hero light |
| **R1 — Assets and APIs** | Native Glint Lens + Glint Flare assets; project default, camera override and tested runtime resolution | Artist can enable auto by assigning lens asset once; no Actor setup required |
| **R2 — Native lens editor** | Automatic controls and candidate diagnostic overlays; separate detailed elements edit mode | Correct save/dirty/undo; clear provenance of affected values |
| **R3–R4 — Fidelity/behavior** | Approved element families, optics/filmback, spatial behavior, time and exposure stability | No critical temporal or camera artifacts in fixtures |
| **R5 — Perf/scalability** | GPU timings per branch, tested tiers, early-out, capacity guardrails | Meets a documented hardware-specific GPU envelope in representative scenes |
| **R6 — Release** | Cooked examples, native automation suite, asset docs and supported configuration matrix | All required gates green; no unsupported compatibility assertions |

## V1 required functional criteria

- [ ] Create `Glint Lens` in Content Browser, select as Project Default; an eligible game camera automatically shows flares from bright scene pixels **without registered Glint sources**.
- [ ] Threshold/strength/max-prominent controls function separately and respond immediately in viewport and editor preview.
- [ ] Bright emissive material and specular highlight generate appropriate automatic response. A white unlit wall or broad bright area can be constrained independently by area/contrast controls.
- [ ] Simple image field produces visible warped/ghost/streak look without requiring top-K detailed candidates.
- [ ] Automatic candidates can drive at least a meaningful procedural Glint Flare element stack and are bounded by active lens quality settings.
- [ ] Auto + authored sources coexist, with a default suppression policy and an explanation/debug display of overlaps.
- [ ] Camera-specific Lens Profile can override the project default; CineCamera optical data can be optionally linked where supported.
- [ ] HDR threshold semantics behave as documented under auto exposure, manual exposure and editor exposure overrides.
- [ ] Per-camera history resets on cuts/resize/world teardown; no previous camera ghost bleeds into new view.
- [ ] Editor and cooked builds have no crashes, resource leaks or synchronous GPU readback dependency.
- [ ] Clean automatic and authored opt-out; users can intentionally choose Glint, native UE lens flares, or both, with warnings against doubled output.

## Native Unreal automation families

Run using the UE console: `Automation RunTests Glint`.

| Prefix | Automated coverage |
|---|---|
| `Glint.Data.Lens.*` | Asset defaults, references, override scopes, cooked serialization, no cyclic asset refs |
| `Glint.Math.HDR.*` | RGB-luminance domain, exposure conversions, threshold soft knee, weighted source color and finite safety |
| `Glint.Auto.Detection.*` | Synthetic masks, tile peaks, merge radius, area/centroid scoring, source class, top-K cap and tie determinism |
| `Glint.Auto.Temporal.*` | Enter/exit thresholds, matched ID stability, source movement, split/merge, camera cuts, viewport resize |
| `Glint.Auto.Routing.*` | Automatic field/candidate source mapping, dedup with manual source, priorities, source-type occlusion rules |
| `Glint.Render.*` | Buffer bounds, shader resources, per-camera data isolation, RDG pass safety, no-resource bypass, HDR composite |
| `Glint.Editor.Lens.*` | Asset edit/undo/redo, project/camera override visualization, debug state, property mode editability |
| `Glint.Perf.*` | Fixture setup integrity, budget caps, GPU timing marker instrumentation, tier transitions |

GPU image-level integration tests should supplement CPU/unit test families with captured deterministic HDR maps, DX12/SM6 reference output comparisons (tolerance-based for GPU floating-point differences), and packaged game visual checks.

## Fixture matrix

| Fixture | Expected | Failure to detect |
|---|---|---|
| One emissive bulb in dark room | One dominant source, appropriate ghosts and streak | Requires manually assigned Actor; false dark-flare |
| Glossy metal rotating under light | Short dynamic highlights, smooth temporal behavior | Flashing 20 duplicate flares |
| Long LED bar | One broad/streak response | Dozens of unmerged iris arrays |
| Night city, 100+ lamps | Stable bounded detailed candidates, cheaper field response | O(Nlights×Nelements) blowup |
| Sun behind leaves | Auto flare follows visible bright region, fades correctly | Wrong hard point-source depth occlusion |
| White sky / snow field | Contrast/size filtering works | Screen completely washed out without override |
| Camera EV sweep | Behavior follows chosen threshold policy | Uncontrolled exposure flicker |
| Camera hard cut/teleport | Clean new source identities | Stale ghosts drifting from old frame |
| Native bloom disabled | Glint automatic still works (own bright-pass baseline) | Hidden bloom dependency |
| Native flares enabled | Warn/compare/suppress only if user chooses | Silently doubled/postprocess corruption |
| Manual source on glowing light | Hero flare and sensible automatic suppression | Duplicate bright flare |
| Scene without eligible highlights | Very low bounded detection cost and no expensive composition | Unconditional heavy full-screen ghost passes |
| Two cameras/views | Independent lens profiles/candidate IDs | Cross-camera contamination |
| Window resize + dynamic resolution | Aspect-correct axis and stable source UV | Misregistered flare positions |
| Dense foliage/translucency/fog | Document actual rendered signal behavior | Overstated occlusion support |

## GPU benchmarks: provisional methodology, NOT performance claims

Do not blindly reuse the original proposed 0.6 ms cost for **eight manually registered sources** as a complete auto budget. The automated detector and image-field passes add a screen-space baseline.

Measure on named reference hardware, at 1080p, 1440p and 4K, recording **mean, median, p95 and worst-frame** timings after warmup. Compare:

1. `Glint Off` baseline.
2. `Auto On / No Bright Pixels` — bright-pass overhead and early-out.
3. `Auto Field Only` — 1 bright region and a screen full of many bright regions.
4. `Auto Candidate Only` — 1, 8, 16, 64 candidate inputs; cap values clearly labeled.
5. `Auto + Authored` — 8 explicit sources, some overlapping auto candidates.
6. `Camera Dirt Only`, `Quality Tier Low/Med/High`, `TAA vs TSR`, and native UE bloom enabled/disabled.

Separate GPU timing markers for `DetectionThreshold`, `Reduce/Peaks`, `CandidateMerge/TopK`, `TemporalMatch`, `FieldGhost`, `FieldStreak`, `CandidateElements`, `AuthoredElements`, `Dirt` and `Composite`. Capture RenderDoc/Unreal Insights/GPU visualizer evidence, VRAM usage, buffer allocations and draw counts. Record *actual numbers* before approving v1.0 targets. Proposed performance acceptance is no unbounded work, no pathological frame spikes and an explicitly agreed total cost on representative scenes/hardware.

## Native manual UX test

An experienced Optical Flares artist and a novice Unreal lighting artist should each be able to:

1. Create and assign one Glint Lens asset at project default.
2. Produce attractive automatic flares in a provided night scene without modifying individual lamps.
3. Make the threshold stricter and identify which lights stop producing flares.
4. Dial in a custom iris/streak style in the familiar element editor and see automatic candidates adopt the look.
5. Add one authored hero flare and prevent double result.
6. Save, close, reopen, cook, package and observe the same look.

Record actual steps/obstacles; avoid declaring the tool intuitive without testing.

## Targeted technical gate added after API research

Before relying on a Scene View Extension for every pass, prove *which* documented UE5.8 callback actually sees the required HDR color and supports correct output. `BL_SceneColorBeforeBloom` does not imply a matching SVE callback. Use the current `SubscribeToPostProcessingPass(Pass, const FSceneView&, Callbacks, bIsPassEnabled)` overload. Record source/render and display resolution, bloom/TSR composition, RHI and packaged results. Add the custom-`IBlendableInterface` per-camera weight test and lens-profile switch fixture described in [31](31-Camera-Workflow-and-Renderer-Acceptance-Plan.md).

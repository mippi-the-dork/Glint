# 01 — Product Vision

[← Index](README.md) · [21 — Automatic Lens](21-Automatic-Lens-Vision-and-Workflows.md)

## Problem

Unreal Engine has built-in image-based lens flares and bloom, but little control over a camera's complete, reusable optical personality. Other workflows require manually assigning a flare to each light. Artists need both a **set-it-on-the-camera automatic lens effect** and a **fully visual Optical Flares-style element editor** that works within real-time game budgets.

## Desired end result

**Glint is an Unreal-native, runtime-efficient lens studio.** A lens profile makes bright emissive, specular and other HDR regions generate consistent flare effects for that camera automatically. When a light needs special treatment, designers and artists author an element composition and explicitly register it as a source. Both use the same camera lens characteristics and common renderer.

## Product model

| Owner | Responsibility | User |
|---|---|---|
| **Glint Lens** asset/editor | Camera-based detection/sensitivity, optical response, presets, intensity, automatic source caps, lens dirt and overrides | Lighting / technical art / cinematics |
| **Glint Flare** asset/editor | Authored elements, individual transforms, color/shape, dynamic triggering, group/preset manipulation | VFX / technical art |
| **Source registration / Blueprint** | Optional explicitly placed or transient hero light control; world position/intensity/lifetime | Game design / gameplay programming |
| **GPU renderer** | Extract highlights from HDR, build image-space field, candidate/top-K, depth occlusion for manual world sources, per-view composition, quality/budget | Rendering / tools engineering |

## Primary workflows

### A. Make the entire camera use Glint (primary)

Create Glint Lens asset → assign in Project Settings > Glint or directly override on Camera/CineCamera → select lens preset → set Sensitivity and Intensity → enter PIE or view level → bright scene regions automatically generate flares → open lens editor diagnostics to refine candidates.

### B. Design a custom lens style

Inside Glint Lens, adjust common aperture, focal and chromatic response; modify the field ghosts, streaks, camera dirt and the detailed auto-candidate template. Use original lens presets; choose linked/global vs local properties consciously.

### C. Author a custom flare composition

Create Glint Flare asset → open native visual editor → add Glow/Streak/Iris/Multi Iris/Ring etc. → refine direct placement and Dynamic Triggering → preview source motion, exposure, occlusion and edge fade → save. Use as the automatic candidate template or assign to an explicit source.

### D. Create a special hero light

Add a Glint Source component to a world Actor/light or spawn an Actor-free transient source from Blueprint/C++; choose Glint Flare composition and optional overrides. The active camera lens still governs global optical style unless art directs otherwise. Use a visible dedup setting if the same source also creates bright scene pixels.

### E. Adapt appearance per camera or environment

Use a project default lens and optional camera override/Glint Lens Volume (subject to feasibility). Camera-specific optical characteristics and lens styles transition by known precedence; filmback/focal zoom may optionally drive optical changes.

## Design principles

1. **The camera is the default owner** of lens behavior, not an Actor attached to every lamp.
2. **Familiar element editing:** Optical Flares users should know where to find glow, streak, iris, multi iris, color, transforms and triggers.
3. **Native Unreal UI:** dedicated CB asset types, Slate, Details, undo/redo, selection color, clean mode-dependent parameter visibility.
4. **Explainable automatic detection:** debug overlays show why bright regions qualify and how cost tiers simplify results.
5. **GPU budgets must be explicit:** screen-space work is bounded even in neon cities and reflective water.
6. **One renderer, multiple source pathways:** image field, automatic candidates, authored sources are separate inputs, not separate effects.
7. **Camera correctness:** explicit exposure, HDR and temporal behavior, independent view states.
8. **No engine fork or runtime external SDKs.** Original assets/presets only.
9. **Protect gameplay clarity:** artistic strength and visual-comfort controls should be exposed independent of renderer quality.

## Success metrics

- With a single project lens assignment, an artist sees automatic flares on emissive/specular regions in an eligible game view without manually adding any flare Actor.
- Within a short first-use session, the artist can tune sensitivity so dim regions stop flaring while strong lights remain.
- An Optical Flares user can produce recognizable Glow + Streak + Multi Iris compositions and have the automatic candidates use their look.
- In dense highlight scenes, full-detail candidates remain bounded; lesser highlights get cheaper field response or none.
- Camera cuts, exposure changes and TAA/TSR do not cause major wrong ghost trails or severe source popping.
- A scripted hero flare can override automatic behavior without accidental double brightness.
- Complete v1 runs in packaged UE5.8 desktop builds with real GPU benchmarks and native tests.

## Research status

These are **design/acceptance intentions**, not claims of implemented Glint functionality. See [22](22-HDR-Highlight-Detection.md) for the GPU proposal, [23](23-Camera-Lens-Profile-and-UX.md) for editor/asset contracts, and [28](28-Automatic-Mode-Testing-and-Release-Gates.md) for feasibility gates.

# 35 — Shipped Game Lessons: Flare as Part of the Art Direction

[← Documentation index](README.md) · [34 — Engines](34-Other-Engines-Real-Time-Lens-Flares.md) · [37 — Decisions](37-Cross-Engine-Findings-and-Prototype-Gates.md)

**Scope:** Only attribute implementation details to shipped games where publicly documented. Many visually spectacular game lens flares have no published technical breakdown; visual appearances alone are not evidence of specific shader algorithms. Glint should not use third-party textures, project presets or game screenshots as distributable assets without appropriate licensing.

## Crysis 3 / Crytek — a camera-lens language, not just brightness decoration

Crytek's leadership emphasized the in-game importance of anamorphic flare as part of *Crysis 3*'s sci-fi visual language, including the implied visor between the player and the world. CRYENGINE's public technical documentation describes artist-driven element trees, per-element position behavior, glow, multighost, chromatic rings, iris shafts, and occlusion interactions. Those documents establish plausible technical context, though not proof that each setting shipped unchanged in every Crysis 3 scene.

**Product lesson:** **Lens Context** is a strong authoring primitive. A camera is not always a neutral human eye. It can be a security camera, scope, helmet visor, racing windshield, anamorphic film camera, surveillance feed or fantasy lens. The asset should encode this intent so effects remain coherent across every light in the game.

**Glint implications:**

- Ship original starter **lens archetypes** (Clean, Cinematic, Anamorphic, Sci-Fi, Visor/Dirty) for stylistic learning, not copied commercial presets.
- Keep custom element authoring because real-time games use flare as intentional composition and directional storytelling.
- A camera-based Lens Profile makes changes consistent as the camera moves from sunlight to spotlights without hand-authoring every source.
- Do not infer scientific correctness from a deliberately exaggerated artistic style.

**Sources:** [PC Gamer interview with Crytek's Rasmus Højengaard](https://www.pcgamer.com/crytek-lens-flares-e2809cvery-importante2809c-to-crysis-3/), [CRYENGINE Optical Flare System](https://www.cryengine.com/docs/static/engines/cryengine-5/categories/23756816/pages/36868680).

## Killzone: Shadow Fall / Guerrilla — integrated art direction with explicit post-process cost

Guerrilla's *Killzone: Shadow Fall* technical demo postmortem lists **extensive artist-driven lens flares** among its post-process features. It reports contemporaneous bandwidth-focused optimizations to post-processing: smaller HDR formats for some operations, more careful access patterns and combining some compositing work to avoid extra full-screen re-reads. Those are **historical PS4-demo findings**, not evidence of any specific Glint-compatible flare shader or a modern 2026 DX12 speedup. The PDF contains broad implementation and performance material, but does not provide enough detail to infer the exact flare object design or algorithm.

**Product lesson:** Look at the *whole* post-processing pipeline, not flare shaders in isolation. Spending 0.2 ms to optimize flare ALU but adding an avoidable full-resolution HDR copy can be a net loss.

**Glint implications:**

1. Add a bandwidth/resource metric to every benchmark: transient target pixel format, lifetime, read/write count and full-screen passes, along with GPU time.
2. Compare multi-pass soft/sharp paths to a shared final composite when the UE5.8 extension stage allows it.
3. Evaluate formats such as R11G11B10F for **compatible positive HDR accumulation**, but confirm alpha, blending, numerical range, signed effects and intermediate accuracy. Do not globally replace RGBA16F by rule.
4. Keep artist controls coherent across levels; presets, quality-tier degradation and warm/cold lens response should not suddenly make the game's visual language inconsistent.

**Sources:** [Guerrilla's postmortem PDF](https://www.guerrilla-games.com/media/News/Files/Valient_Killzone_Shadow_Fall_Demo_Postmortem.pdf), [Guerrilla image-quality presentation overview](https://www.guerrilla-games.com/read/taking-killzone-shadow-fall-image-quality-into-the-next-generation-1). **Caveat:** The extracted PDF transcript lists the preceding items, but web screenshot retrieval was unavailable; use the original document for final visual audit.

## General game-design and production rules

- **Gameplay priority:** A hero flare must not obscure critical aiming targets, interactables, subtitles, warning indicators or necessary silhouettes. Lens Profile should support a global maximum flare contribution and a reduced-impact accessibility style; optional gameplay override inputs can lower effect weight for precision aiming and UI-heavy interactions.
- **Context-sensitive styles:** Visor vs clean camera can be independent Lens Profiles, blended on compatible scalar parameters and crossfaded between incompatible element sets, not silently interpolated as if the underlying structures match.
- **Responsive cues:** Flare movement can help communicate source position and camera motion, but excessive delayed/smeared transitions undermine spatial readability.
- **Light authenticity:** Automatic mode should not generate full photogenic flare chains from broad white walls, UI, or every exposure-adjusted surface. Use source classification and region-size response.
- **Allow exaggeration:** Graphic sci-fi flare styles have legitimate artistic uses; physical realism should be one influence rather than a creative restriction.
- **Performance is part of art direction:** Presets should carry authored **importance** and graceful simplified variants. Users design the look; the engine makes detail decisions per view.

## What we cannot confidently claim from public records

- That a named commercial game uses a particular modern Unreal-style RDG implementation, compute candidate extraction, or specific optical ray-transfer approximation.
- That publicly quoted console frame times represent flare-only GPU timings.
- That a visually similar effect demonstrates automatic luminance tracking rather than authored point sources.

Use games primarily to derive product/artistic requirements; use papers and engine implementation spikes to choose the actual algorithms.

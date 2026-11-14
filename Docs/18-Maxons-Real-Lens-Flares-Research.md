# 18 — Maxon Red Giant Real Lens Flares: Research

[← Documentation index](README.md) · [19 — REAL FLARES](19-REAL-FLARES-Unreal-Research.md) · [20 — Design Implications](20-Hybrid-Lens-Design-and-Validation.md)

**Research date:** 2026-11-12. **Status:** Verified against Maxon's public documentation and release notes, not a hands-on implementation audit. Older manual pages may not include recently added controls. Numbers below are vendor-specific, not Glint specifications.

## Core design distinction

Maxon's Real Lens Flares derives much of its result from a simulated optical design. Changing source angle, lens glass, aperture or sensor can cause ghost reflections to move, deform, merge, change brightness and chromatic character together. This is not merely a list of independent 2D sprites. For Glint, this suggests shared, parameter-driven relationships *in addition to* direct element editing, not replacing it.

## Authoring and interface

- **Preset browser:** thumbnails, search, groups for lens prescriptions, stylized flares and individual core projections.
- **Projection strip:** editable components and lens-generated reflections. Selection reveals granular controls; projections can be hidden. The legacy manual limits some Maxon projection counts (Light Ball one, Glow Ball up to three, Halo up to three, Ring one, Single Spike up to three, Gate Flare one, Schmutz one, Starburst one). These are *not* Glint design limits.
- **Lens pane:** interactive lens, aperture and image-plane representation. Selecting physical constituents displays relevant inspector settings.
- **Inspector:** contextual light-source, global flare, aperture, lens-element, image-plane and core-projection fields.
- **Preview:** lens effects on black or composited imagery, interactive source positioning, preset experimentation.

## Documented parameter groups

| Area | Maxon documented behavior / controls | Glint lesson |
|---|---|---|
| Light | Temperature, brightness, angular size, filter; different light source kinds | Source profiles drive color/brightness/style; artist override remains possible |
| Global | Scale, angle, exposure, procedural noise evolution, quality (legacy Fast/Production/Best/Extreme) | Consistent master controls, distinct from per-element appearance |
| Aperture | F-stop 0.9–22; 3–17 blades; curvature, twist, notch and notch angle; diffraction and irregularity; f-stop-associated rotation/curvature | Shared aperture shape controlling linked ghost iris and starburst |
| Glass element | Front/rear AR coating properties, thickness, refractive index, chromatic dispersion; affects reflections, not all core projections | Linked ghost tint, chroma, irregularity; do not claim optical accuracy unless modeled/tested |
| Image plane | Sensor format/size/position; changes reflected-ghost distribution | Camera and lens profile controls affect positioning and scale |
| Anamorphic | Anamorphic factor and appropriate optical distortion | Shared squeeze and streak behavior |
| Zoom | July 22, 2026 Red Giant 2026.5 added continuous/animatable focal zoom and aspherical support, plus 44 new lens presets | Animate with camera FOV/focal length, not just rescale final image |
| Lens dirt | Schmutz texture, defocus, illuminated source/background, aperture-dependent shape, influences starburst/reflections; Oct 14, 2026 release added user still/animated textures | Lens-surface effects are separate from source-space ghosts and may modify other elements |

Maxon's legacy quality tiers document wavelength sample counts 3, 7, 15 and 20. These are Maxon's specific render modes, **not** a Glint implementation target.

## Core projection vocabulary

**Light Ball** (visible light disc) differs from **Glow Ball** (soft halo) and behaves differently under occlusion. **Starburst** derives geometry from aperture diffraction. **Halo** and **Ring** create colored circular artifacts. **Single Spike** offers direction/spread/unidirectional behavior. **Gate Flare** creates barrel-edge effects from lights just outside the frame. **Schmutz** may be hidden as a direct projection but still influence lens reflections and starbursts. Lens-derived reflections are layered components distinct from core projections.

## What we should adapt

1. A **shared Lens Profile** that modifies applicable elements coherently, with local override per element and transparent effective-value UI.
2. An **optional lens schematic** separate from the default Optical Flares-like layer stack: novice users should not need optics knowledge.
3. Source motion preview, camera zoom preview, aperture-change preview, and isolated projections; static still comparisons cannot evaluate morphing.
4. User-defined **light source looks** (LED, tungsten, fluorescent, fire, etc.) and lens-surface dirt that remains camera-attached.
5. Explicit save/undo behavior. The older Maxon guide describes preset selection resetting unsaved tweaks: Glint should not silently discard edits.

## What we should not promise in v1

Full lens prescriptions, runtime ray traced glass reflections, exact emulation of specific commercial lenses, full spectral wave optics and manufacturing-level physical accuracy. Instead, test parametric lens-inspired approximation with art-directed overrides. All final content, textures, presets and UI should be original.

## Primary sources

- [Maxon product overview](https://www.maxon.net/en/product-detail/red-giant/vfx/real-lens-flares)
- [Designer introduction](https://help.maxon.net/rg/en-us/Content/html/03-Real-Lens-Flares_lens-flare-designer.html)
- [Projection strip](https://help.maxon.net/rg/en-us/Content/html/09-Real-Lens-Flares_projection-strip.html)
- [Inspector](https://help.maxon.net/rg/en-us/Content/html/08-Real-Lens-Flares_inspector-pane.html)
- [Detailed lens/aperture pane](https://help.maxon.net/rg/en-us/Content/html/10-Real-Lens-Flares_lens-pane.html)
- [Core projections](https://help.maxon.net/rg/en-us/Content/html/05-Real-Lens-Flares_core-projections.html)
- [Schmutz manual](https://help.maxon.net/rg/en-us/Content/html/06-Real-Lens-Flares_core-projections-schmutz.html) (older limitations superseded by 2027.0)
- [Red Giant 2026.5 July 22 release](https://support.maxon.net/hc/en-us/articles/29456656067740-Red-Giant-2026-5-0-July-22-2026)
- [Red Giant 2027.0 October 14 release](https://support.maxon.net/hc/en-us/articles/13764887933596-Red-Giant-2027-0-0-October-14-2026)

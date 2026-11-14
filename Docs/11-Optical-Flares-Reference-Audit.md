# 11 — Optical Flares Reference Audit: Objects, Controls, Workflow

[← Documentation index](README.md) · [12 — Glint parameter contract](12-Glint-Element-Parameter-Schema.md) · [13 — Dynamic behaviors](13-Behavior-and-Trigger-System.md)

> **Purpose:** Research what makes Video Copilot **Optical Flares** familiar to an experienced artist, without asserting that a similarly named Glint control is an exact reverse-engineered clone. This is a **reference analysis**, not a specification of the closed-source vendor's internals.
>
> **Evidence labels:** **[Official AE]** Video Copilot After Effects product page / screenshot; **[Official Nuke]** Video Copilot's closely related *Nuke* product documentation (differences explicitly identified); **[Independent]** third-party observations and screenshots; **[Glint proposal]** our own new behavior/API; **[Verify]** exact vendor detail not established from accessible official source.

## 1. Why the Optical Flares workflow succeeds

The author repeatedly iterates between a **live visual preview**, **Object Stack**, **per-object property editor**, and **preset/element browser**. The result is a composition of separately authored objects; the flare's convincing complexity emerges from overlap and relative motion. The original advertises a large detachable preview, 12 lens-object types, reusable single objects, shareable presets, object rename/reorder, Show Background, copy/paste, duplicate, and undo/redo. Dynamic triggering creates motion without ordinary keyframes. [Official AE]

### Verified interface layers

| Layer | Optical Flares evidence | Familiarity requirement for Glint |
|---|---|---|
| Overall effect controls | AE effect panel: Position XY, Center Position, Brightness, Scale, Scale Offset, Rotation Offset, Color, Color Mode, Animation Evolution, positioning/source, foreground layers, flicker, custom layers, render mode; some options visible in the Video Copilot installation screenshot | Global-level Glint asset and per-source overrides; visible separation of edit-time asset controls vs runtime source controls |
| Preview | Live flare, draggable position, preview brightness and preview scale, optional guides/background, pop-out preview | Live runtime-quality preview, mouse drag source, 16:9/4:3/9:16 guides, compare-to-scene mode; preview-only controls must NOT silently modify the asset |
| Object Stack | Rows for each object; selection, intensity/scale overview, Hide, Solo, delete; duplication, rename and reorder | Readable native Slate tree, thumbnail per element, quick brightness/scale, solo/visibility/lock/group and drag reordering |
| Object Properties | Selected object's detailed controls and type-specific settings | Native Details-like sections: Appearance, Shape, Placement, Color, Dynamics, Distortion, Quality |
| Lens Objects Browser | Pick individual object types and saved reusable elements | Add Element gallery with visual previews and recent/favorites |
| Preset Browser | Preview a complete flare before application, categories/user presets, save/share/combine | Dedicated Glint Content Browser assets plus embedded library of **Glint presets** and **element presets** |

Video Copilot's Nuke version preserves the Stack/Preview/Editor/Browser principle, but adds capabilities not present in AE (e.g. **uniform offset scale of flare elements** and **multiple 2D flare positions** per its official comparison). Do **not** attribute all Nuke features to Optical Flares AE. [Official Nuke]

## 2. Twelve-element family audit

**Important:** Video Copilot officially confirms *12* lens-object types but its accessible public product description does not provide a definitive per-property list. The object names and visual roles below are derived from original screenshots and an independent reconstruction/illustrated catalogue. For less-visible types (Caustic / Lens Orbs), verify exact naming and specific original control behavior against a licensed AE installation before claiming one-to-one parity. **This uncertainty does not prevent implementing the visual capability in Glint.**

| Reference name / family | Recognizable visual outcome | Common distinguishing controls to research / recreate in Glint | Relative placement | Glint v1 priority |
|---|---|---|---|---|
| **Glow** | Smooth, concentrated or diffuse bright disk around source | Radius/core radius, soft falloff, color ramp, inner/outer intensity, gamma/falloff power, asymmetry | Usually attached to source | Required |
| **Iris** | Individual circular/polygonal/photographic aperture reflection | Blade count, aperture roundness, feather, rings, shape/texture, internal gradient, chroma | On center axis, at free offset | Required |
| **Multi Iris** | Multiple ghosts, usually distributed on the line crossing center | Instance count, spread/range, scatter, seed, per-child size/brightness/rotation variation, polygon/texture shape | Source-to-center axis | Required |
| **Streak** | Horizontal, diagonal or cross-axis anamorphic line | Length, width, falloff, taper, softness, end spread, asymmetry, angle, stretch | Usually source-aligned; can be offset | Required |
| **Shimmer** | Dense, organic, changing radial starburst | Complexity/density, irregularity, length/brightness variation, phase/evolution, speed | Usually source-aligned | Required |
| **Glint** | Fine, sharp radial spikes | Spoke count, width, rays length, angular distribution, size/brightness randomness, flicker | Source-aligned or relative to axis | Required |
| **Spike Ball** | Dense petals / chunky radial rays | Count, length, width, softness, fan angle, random seed, per-ray animation | Usually source | Required |
| **Sparkle** | Spark dots or fragmented rays, micro-highlights | Quantity, dispersion, fragment size, color randomness, movement/evolution | Source or reflection axis | Required |
| **Ring** | Complete or partial luminous ring | Radius, thickness, feather, angular completion, color distribution, chroma, rotation | Source, center or reflection axis | Required |
| **Hoop** | Large curved chromatic reflection/arc | Radius, curvature, spectral gradient, start/end angle, taper, orientation, fade | Usually axis-relative | Required |
| **Caustic-like reflection** | Asymmetric, sometimes concentrated curved refractive highlight | Distortion, focus/aberration, thickness, completion, gradient/texture, axis rotation | Often offset near center/edge | Required visual family; exact original name **[Verify]** |
| **Lens Orbs / on-lens artifacts** | Large, soft particles or dust/bokeh responding to bright flares | Texture/density, scale, edge shape, lens-space placement, source illumination, blending | Fixed lens space, lit by moving sources | Required feature family; exact original object taxonomy **[Verify]** |

**Do not confuse a Glint object named `Glint` with the Glint plugin as a whole.** In the Add menu present **Glint (Star Rays)**, with `Glint` as the serialized/display type, so search remains familiar without ambiguity.

### Important conceptual separation

- **Glow vs bloom:** Glow is an authored flare element, not a replacement for UE's scene-wide bloom. They may interact, but must remain independently controllable.
- **Iris vs Multi Iris:** One element vs a generated *group of visually distinct aperture ghosts*. Multi Iris must optionally allow expansion/baking to editable individual children; keep compact procedural form by default.
- **Ring vs Hoop:** Ring is primarily concentric geometry; Hoop is an incomplete, often displaced optical arc with orientation and spectral variation. Both can use a shared shader family with different defaults.
- **Shimmer vs Glint vs Spike Ball:** Related radial geometry with different distribution, softness and animation. Share a rendering primitive/kernel where possible, but keep familiar artistic presets/types.
- **Lens Orbs/Dirt vs ghost chain:** Dirt is *lens-fixed* and illuminated by the flare; ghosts are *optical reflections* that change position with source movement.

## 3. Common controls — evidence versus Glint interpretation

| Control family | Evidence | Glint implementation decision |
|---|---|---|
| **Brightness / Scale** | Shown on object stack and effect-level screenshots [Official AE] | Distinct intensity and geometric scale, both inheritable from group/asset and optionally from runtime source |
| **Colorize** | Independent reconstruction notes single color, global tint, gradient, spectrum, gradient offset/wrapping [Independent] | `Global`, `Solid`, `TwoColor`, `Gradient`, `Spectrum`, `TextureColor`; color management in scene-linear HDR |
| **Transform** | Vendor calls out distance, offset, custom translation [Official AE] | Axis Distance, X/Y Offset, Stretch, Aspect, Rotation mode; explicit units and source-center guide |
| **Dynamic Triggering** | Vendor shows edge flare-up, size changes, preview of activation area [Official AE]; source docs demonstrate From Border [Official Nuke] | Position/occlusion/time/facing-driven response curves with fast preset templates and trigger visualization |
| **Circular Completion** | Independent reconstruction describes partial arc/ring control on appropriate objects [Independent] | Arc Start, Arc Sweep, Edge Feather, orientation; hide only for types that support it |
| **Variation / Shimmer** | Vendor advertises self-propelling shimmer with amount/speed, individual source flicker seed [Official AE] | Deterministic motion and per-source seeds; optional per-element seed override |
| **Global animation** | AE panel shows Animation Evolution, Rotation Offset, Flicker [Official AE screenshot] | Global evolution time and explicit source motion bindings; asset animation can run without Sequencer |
| **Texture shapes** | Official Nuke docs: Iris/Multi Iris shape -> Texture -> texture library [Official Nuke] | Direct `UTexture2D` reference in asset; procedural/texture toggle where relevant; import remains UE-native |
| **Blend/composite** | Nuke UI shows Blend Mode `Add`; independent implementation discusses Add and Screen [Official Nuke/Independent] | Additive + carefully specified Screen-like mode, with HDR pipeline equivalence tests; do not assume AE Screen math transfers unchanged to HDR |
| **Chromatic aberration** | Official AE product supports color fringing [Official AE] | Cheap per-element spectral fringe primarily; more expensive full-frame distortion optional/not default |
| **Lens texture** | Vendor illuminates dust/scratches with source [Official AE] | Screen/lens-space dirt atlas/texture processed once per view where feasible |
| **Occlusion** | Vendor advertises alpha/luminance occlusion on foreground layers [Official AE] | Depth-aware per-source visibility in 3D world; developer-facing modes for Translucent/Masked/Custom Depth |

## 4. Behaviors to learn precisely during an Optical Flares hands-on validation session

The **reference validation checklist** must capture for each object type: default object appearance; all visible property names; units, min/max and negative ranges; color modes; rotation/axis relation; whether completion is exposed; hide/solo preview vs saved state; whether a texture can replace shape; behavior at center, border, outside frame; and how behavior reacts to globals. Record **UI control semantics**, not proprietary rendering code or vendor asset data.

### Suggested 15-minute artist exercise

1. Create a blank flare, add a Glow, drag source from left to right, record which properties change automatically.
2. Add a Streak, set fixed 0-degree orientation, then set axis-aligned orientation; drag source through center and across screen edge.
3. Add one Iris and a Multi Iris; vary count/spread/seed, observe predictable relative movement.
4. Add Shimmer and Spike Ball; compare static complexity with animated evolution, note distinct randomness patterns.
5. Add partial Ring and Hoop; rotate the source and distinguish world rotation from lens-axis rotation.
6. Set Dynamic Triggering `From Border`, toggle trigger preview, use nondefault brightness and scale offsets; confirm falloff semantics.
7. Save one element preset, copy it to another flare, then save a complete flare preset; reopen and confirm identical behavior.
8. Turn on chromatic aberration and lens texture, test brightness/exposure changes, note any divergent color behavior.

This is a **design parity** session, not an automated import/conversion plan.

## 5. Verified research sources

- [Video Copilot Optical Flares for After Effects — official features](https://www.videocopilot.net/products/opticalflares/): 12 core types, object stack capabilities, textures, dynamics, 3D lights, shimmer, flicker, presets.
- [Video Copilot Optical Flares object descriptions — official announcement](https://www.videocopilot.net/blog/2010/01/object-descriptions): establishes a 12-object reference PDF was published. Linked ZIP is currently not usable through this research environment; original property names remain unverified.
- [Video Copilot Optical Flares Nuke — official cross-version differences](https://www.videocopilot.net/products/opticalflaresnuke): vital for not conflating AE with Nuke.
- [Nuke: Creating a Lens Flare from Scratch](https://video-copilot.gitbook.io/opticalflaresnuke/using-optical-flares-for-nuke/creating-a-lens-flare-from-scratch): stack workflow and Iris/Multi Iris texture shape.
- [Nuke: Dynamic Triggering](https://video-copilot.gitbook.io/opticalflaresnuke/using-optical-flares-for-nuke/dynamic-triggering-and-automatic-animation-of-lens-objects): `From Border` and observable actions.
- [Independent real-time reconstruction study](https://wiki.nuaj.net/index.php?title=Lens_Flares): useful taxonomy, parameter families, Screen blending research; **not** canonical vendor documentation.
- [Video Copilot installation guide (Windows)](https://d3jddrml5mymee.cloudfront.net/assets/public/ckfinder/userfiles/files/Install%20Guides/Installation_WINDOWS_Pro%20Presets%20and%20Pro%20Presets%202.pdf): historic AE global effect control screenshot (names may vary by plugin version).

## 6. Research conclusions

1. Matching **mental model and iterative workflow** is more important than cloning every UI widget or low-level shader.
2. Glint must preserve immediately recognizable **element names**, **stack/presets workflow**, and **per-element dynamics**.
3. The existing 7-family first draft under-scoped the authoring experience: Glint v1 should present **12 familiar families plus Texture Sprite**, implemented through fewer shared GPU primitives where possible.
4. Glint can exceed the reference where UE is stronger: runtime sources, per-camera visibility, scene depth occlusion, Blueprint & Sequencer, scalability, GPU timings, native assets.
5. Exact per-element parameter parity is a **follow-up verification task**; do not claim 1:1 Optical Flares reproduction or ability to load OFP presets.

## Additional confirmed original feature: Automatic Luminance Tracking

Video Copilot's [Optical Flares product page](https://www.videocopilot.net/products/opticalflares/) and [original release announcement](https://www.videocopilot.net/blog/2010/01/optical-flares-online) explicitly advertise **Luminance Tracking**, with **threshold and resolution** controls, generating lens flares from bright regions in footage. This directly supports Glint's camera-driven auto mode and should become a dedicated hands-on reference test. The official feature video is [Luminance Tracking](https://www.videocopilot.net/products/opticalflares/features/luminance_tracking). These sources **do not** establish the exact region algorithm, default parameter values, or simultaneous source limit.

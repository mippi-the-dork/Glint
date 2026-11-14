# 13 — Glint Behavior System: Dynamic Triggering, Animation and Optical Motion

[← Documentation index](README.md) · [12 — Parameter schema](12-Glint-Element-Parameter-Schema.md) · [15 — Workflow](15-Optical-Flares-to-Glint-Workflow.md)

> **Status:** Proposed Glint v1 design. The **Dynamic Triggering** concept and `From Border` example are documented by Video Copilot. All implementation formulas and extended inputs below are Glint-specific decisions unless stated otherwise.

## 1. Core promise

An artist can make a complex lens flare that changes naturally as the source travels across the viewport **without writing Blueprint logic or authoring keyframes**. Behavior lives inside the same element used by the editor, runtime and packaged build.

**Optical Flares reference behavior**: edge blooms; iris scale changes; fading near center; color shifts around image border; built-in shimmer/flicker. Video Copilot's Nuke documentation specifically shows `From Border` and a red preview of the trigger region. The AE product page confirms trigger areas can be previewed and elements animate without keyframes.

**Glint extension**: allow multiple drivers per property, 3D facing, distance, per-source occlusion and exposure when available; but ship **single-driver quick setup** as the default UI so the tool isn't overwhelming.

## 2. Data model

Each element has a base parameter set and an optional `TArray<FGlintBehaviorBinding>` serialized in the asset. A Binding has:

- `TargetProperty`: stable typed property identifier (Brightness, Opacity, Scale, Rotation, ColorMix, RayLength, Thickness, ArcSweep, etc.), not unrestricted reflection/string lookup at runtime.
- `Driver`: enum, e.g. From Border / From Center / Source Angle / Occlusion / Time / Flicker.
- `DriverParameters`: threshold distances, falloff, reference point, optional angles, mode and inversion.
- `Response`: Linear / Smoothstep / Curve (sampled LUT), with curve min/max or response start/end.
- `Operation`: Add, Multiply, Override, Min/Max (type-permitted), and a stable evaluation order.
- `Influence`: 0..1 and `Enabled`, optional easing/response blend.
- `EditorPreview`: trigger-region overlay toggle is **preview-only**, not an asset render property.

```
Source & Camera -> Projection -> Driver values (0..1)
                                    |
                          Response and Easing
                                    |
                      Apply to authored Base Values
                                    |
                    Per-element resolved render record
                                    |
                           GPU geometry & composite
```

Always calculate the base before modifiers. Preserve authored base values even when a driver is saturated. At runtime, compile bindings into compact typed structs/parameter IDs; no per-pixel arbitrary property searches, no running Blueprint every frame for each element.

## 3. Trigger drivers, semantics and artist use

| Driver | Driver signal (proposal) | Example | v1 |
|---|---|---|---|
| **From Border** | 0 in center safe area; rises near closest frame edge; can remain active just beyond frame | Streak flares in as a source exits frame; Iris grows | Must |
| **From Center** | 1 near optical center, smoothly falls to 0 with distance | Brightest halo when light is near center; less glare at edges | Must |
| **From Source** | Distance from source position to element position, normalized | Ghost brightness decreases as it separates from source | Must |
| **Angle Around Center** | Source-to-center angle mapped to degrees | Orient a rainbow arc differently at the top/bottom | Must |
| **Occlusion** | Per-source 0..1 visibility computed from current depth with history smoothing | Whole flare dims behind foreground geometry | Must |
| **Source Distance** | World-space range normalized by near/far values | Smaller lighthouse flares as camera moves away | Must |
| **Source Facing** | Dot of source forward and camera-facing direction, clamped | Spotlight lens flare only visible near its forward cone | Must |
| **Time / Evolution** | Per-source continuous time | Animate shimmer rays and rotating lens texture | Must |
| **Flicker / Noise** | Seeded continuous noise or pulses | Slight irregular source energy and glint twinkle | Must |
| **Source Intensity / Exposure** | Explicit source binding; optional physical photometric value | Bright vs dim lights feel consistently different | Must, with well-defined override |
| **Gameplay Attribute** | Externally provided named scalar | Reactor charge controls a multi-iris intensity | Possible v1 if simple API; more advanced binding later |
| **Camera Motion** | Angular and positional camera velocity | Additional flare smearing at whip-pan | Post-v1 unless cheap and stable |
| **Custom 2D texture mask** | Artist-specified screen-position lookup | Complex asymmetric trigger regions | Post-v1 unless existing driver model handles efficiently |

### Normalizing border triggers

The trigger's distance must be calculated against **the view rectangle** (not the full desktop window or editor chrome). Define an authorable border width measured as a fraction of the **reference short edge**. For stable 21:9 and vertical previews, use comparable physical relative distances. Support near-edge and offscreen bands. A visual overlay draws the actual evaluation mask and numeric value at cursor position.

Avoid `min(x, 1-x, y, 1-y)` without aspect correction: in ultra-wide view it creates unintended trigger activation. Account for orthographic/cine camera center offset, split-screen and cropped projections.

### Driver rules

- Source *behind camera*: never allow an unbounded mathematically mirrored flare to flash full brightness unless an explicitly selected Offscreen Behavior requests it. Apply a source-behind-camera gate before user behaviors.
- Occlusion: multiply **source visibility** before local behavior unless a behavior explicitly uses occlusion as its own driver. Prevent contradictory double attenuation.
- Offscreen: preserve axis ghost movement while source is offscreen within a tunable margin; terminate efficiently outside the maximum authored extent.
- Source cut/reuse: time smoothing histories must reset on camera cuts, teleport or handle recycle; otherwise previous brightness leaks into new frames.

## 4. Response formulas and determinism

For a normalized driver `u = clamp((x-a)/(b-a),0,1)`:

- **Linear:** `w=u`.
- **Smoothstep:** `w=u*u*(3-2*u)`.
- **Custom curve:** `w=LUT(u)` sampled from baked authoring curve, not live UObject graphs.
- **Invert:** `w=1-w` prior to response if explicitly selected.
- **Multiply example:** `Brightness_final = Brightness_base * lerp(1, 2.5, w)`.
- **Additive example:** `Rotation_final = Rotation_base + 35deg*w`.
- **Override example:** `Opacity_final = lerp(Opacity_base, target_opacity, w)`.

Set clear limits when a target is **unsigned/positive** (Brightness, Scale, Opacity); artist-specified negative reductions should clamp rather than invert or produce NaNs. Angle interpolation must respect wrapping conventions and not suddenly rotate a hoop 360 degrees.

All randomness is derived from `(asset seed, element GUID, runtime source seed, child index)`. Time-based smooth variation uses deterministic indexed noise; simple camera pause / replay / sequence playback must reproduce equivalent output for an equivalent time input. The renderer should evaluate common drivers **once per source/view**, then each element's selected bindings without per-element timers.

## 5. Three examples we should ship as ready-to-use behavior presets

### A. Cinematic anamorphic edge streak

**Base:** Streak, fixed horizontal rotation, fine width, blue-white tint, long length.

1. Add `From Border -> Brightness`, multiplier **1.0 to 2.0** when approaching frame edge.
2. Add `From Border -> Length`, multiplier **1.0 to 1.35**.
3. Add smooth `Occlusion -> Brightness` to dim when blocked.
4. Keep source-aligned Glow softer than Streak to avoid bright bands dominating full image.

**Expected:** An animated bright light produces recognizable optical 'flare-up' at frame margins without hand-animated keyframes.

### B. Moving multi-iris ghost chain

**Base:** Multi Iris with six ghosts from `Axis Distance 0.2 to 2.3`, stable seed, slight angular/scale variation.

1. Set Color Mode Spectrum, low saturation.
2. Enable rotation Follow Axis on selected irises, leave a few fixed.
3. Add `From Center -> Scale` subtle growth (or reverse, according to look).
4. Use grouped source occlusion, and a low-opacity lens dirt response.

**Expected:** Ghosts move at different apparent speeds across the center line, not as a rigid sprite group.

### C. Science-fiction beacon with animated diffraction

**Base:** Glow + Glint + Spike Ball + thin Hoop.

1. Glint `Flicker` on Ray Length with low variance and smooth speed.
2. Spike Ball `Time/Evolution` subtly changes petal distribution without re-randomizing every frame.
3. Hoop `Source Facing -> Brightness` reduces glare when beacon points away.
4. Source Blueprint drives one `Intensity` override for alert state (without editing asset).

**Expected:** Same Glint asset can appear as subdued idle beacon or vivid alert using a simple runtime override.

## 6. UI/UX: quick controls first, curve complexity second

At the foot of each applicable element's Details, show a **Dynamic Triggering** section:

- **Add Trigger** button and a concise row `From Border → Brightness`, with toggle, strength, curve preview and gear for advanced settings.
- **Preview Trigger** button overlays a transparent red/heatmap mask on preview, shows current driver value and live parameter resolved value.
- **Quick Presets:** Flare Up at Edge, Fade at Center, Scale at Border, Shimmer, Flicker, Occlusion Fade.
- **Advanced mode:** response graph, combine order, remapping ranges, individual channel targets, custom curves, input inversion.
- **No separate node graph required.** A stacked property-response list is faster and more legible for the typical use cases; optionally explore a Behavior Graph post-v1 for very complex logic.

Modifying drivers while preview is paused must still recompute when scrub/drag changes source position; time-only drivers pause. The UI must label **authored value** and **resolved preview value** separately, not edit the resolved result accidentally.

## 7. Temporal and camera edge cases

| Case | Required behavior |
|---|---|
| Very low frame rate | Time motion tied to elapsed time, no fixed per-frame rotation increment |
| Pause / photo mode | Explicit pause policy; ability to freeze motion without hiding sources |
| Camera cut | Reset occlusion/exposure smoothing appropriate for new camera |
| TAA/TSR and dynamic resolution | Stable pixel footprints and legal jitter compensation; verify in R0 |
| Two cameras / split-screen | Independent driver state per camera and source, no cross-view fade data |
| Source offscreen | Continue visible ghosts within allowed margin; no arbitrary cut-off at source edge |
| Source behind camera | Stable projective suppression and history reset, avoid large mirrored flares |
| Source world destroyed | Registry handle invalidated; no source memory leaks or residual ghost |
| Direct light and local light | Light intensity/facing obey binding mode and physically meaningful range mapping |
| Camera near/far clips | Stable projection; do not normalize invalid near-plane homogeneous coordinates |

## 8. Renderer and test requirements

- Evaluate base/driver bindings on game/render side as appropriate, but never block GPU with per-source readbacks or force game thread from scene render callback.
- Pack response arrays and LUTs with bounded memory, pre-calculate constant outputs on asset save/cook where possible.
- Hard-cap ray/ghost count per element and per view independently of visible UI slider soft ranges.
- `Glint.Behavior.Border/Center/Angle`: test representative values and aspect ratios.
- `Glint.Behavior.MultipleBindings`: order, additive vs multiply and deterministic stable output.
- `Glint.Behavior.SeedReplay`: identical time/seed yields equivalent flare, no global PRNG dependence.
- `Glint.Behavior.SourceIsolation`: same asset on two moving lights yields independent offsets, intensities and occlusion.
- `Glint.Behavior.CutsAndTeleports`: cut resets; no one-frame flashes.
- `Glint.Editor.TriggerPreview`: overlay corresponds to actual driver values, including on 21:9 and 9:16.

## 9. Reference sources

- [Video Copilot official AE feature page — Dynamic Triggering, Shimmer, Flicker](https://www.videocopilot.net/products/opticalflares/)
- [Video Copilot official Nuke Dynamic Triggering article](https://video-copilot.gitbook.io/opticalflaresnuke/using-optical-flares-for-nuke/dynamic-triggering-and-automatic-animation-of-lens-objects)
- [Independent Optical Flares reconstruction — global modifiers and circular completion](https://wiki.nuaj.net/index.php?title=Lens_Flares)

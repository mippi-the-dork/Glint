# 12 — Glint Element Parameter Schema (v1.0)

[← Documentation index](README.md) · [11 — Optical Flares reference](11-Optical-Flares-Reference-Audit.md) · [13 — Behavior](13-Behavior-and-Trigger-System.md)

> **Status:** Proposed native UE5 authoring and runtime schema. **All values/defaults here are Glint design proposals**, not a claim about Optical Flares' exact numeric defaults. Range limits and performance caps will be tuned against rendering prototypes.

## 1. Core authoring contract

All **Glint** types implement the same set of applicable **Element Core** fields. Specialization changes visible Shape parameters and kernel choices, not the meaning of Brightness, Scale, Placement or Dynamics. This is essential to making Optical Flares users comfortable and to making Blueprint overrides predictable.

| Property | Suggested UI type | Semantics / constraints |
|---|---|---|
| `Name` | Editable text | Unique within group for clarity; stable underlying GUID independent of name |
| `Type` | Icon + label | Immutable for a placed element by default; explicit Convert Type action if compatible |
| `Enabled` | Eye / checkbox | Serialized render enabled state; Solo is preview-scoped by default, never persisted accidentally |
| `Locked` | Lock icon | Stops direct manipulation, does not disable rendering or Details inspection |
| `Brightness` | Slider + digit field | Nonnegative linear multiplier; above 1 valid; zero means invisible |
| `Opacity` | Slider | 0–1 artistic coverage (not interchangeable with Brightness in HDR) |
| `Color Mode` | Enum: Global/Solid/Two-Color/Gradient/Spectrum/Texture | Only meaningful modes exposed; Global multiplies by source tint unless opted out |
| `Color / Gradient / Spectrum` | UE Color Picker + native gradient editor | Scene-linear values stored; authored color supports values beyond display range for HDR when appropriate |
| `Blend Mode` | Enum | Additive, Screen-style, Alpha/Soft Add; supported modes gated by renderer technique and tested HDR behavior |
| `Scale` | Numeric percent / factor | Common uniform scale with no change to axis positioning; supports beyond 100% |
| `Aspect` | Numeric ratio | 1 = normal; explicitly correct viewport aspect ratio. Offer Stretch X/Y as separate options |
| `Offset X/Y` | Two numerics + drag handle | A predictable viewport-independent unit (reference pixels at 1080p, converted appropriately) |
| `Axis Distance` | Slider/digit + line handle | 0 source; 1 camera optical center; 2 mirrored beyond center, negative and >2 valid |
| `Rotation` | Degrees with native digit scrubbing | Angle offset, interpreted by Rotation Mode |
| `Rotation Mode` | Enum | Fixed Screen / Follow Source–Center Axis / Source Facing / Camera Roll / Custom Input (available by type) |
| `Pivot` | 2D numeric/preview handle | Overrides element's geometric center; does not alter optical axis |
| `Chromatic Fringe` | Enable, strength, direction | Per-element cheap spectral splitting; disabled default |
| `Seed` | Numeric and randomize button | Explicit deterministic authoring seed; runtime derives independent child seed per source |
| `Dynamics` | Collapsible responders | Zero or more typed, deterministic response blocks, evaluated after base values |
| `Quality Importance` | enum Low/Normal/Hero | Guides per-element LOD; does not arbitrarily change authoring data |

**UI rule:** A property whose selected mode does not use it must be hidden or read-only with an explanatory tooltip. Do not show plausible editable controls that silently have no effect.

### Stored units and UI units

- **Position:** Source and center use normalized full-view coordinates; artist offsets default to **1080p-reference pixels** at fixed reference height, not raw current render pixels. Tooltips display normalized values when Advanced Mode is enabled.
- **Scale:** A common factor plus type-specific physical size, allowing Glow radius and Streak length to remain comprehensible.
- **Rotation:** Degrees in editor, radians/shader representation internally; canonical clockwise/counterclockwise convention must match actual preview gizmo.
- **HDR:** Store color and brightness as well-defined scene-linear values; show regular LDR color picker controls with an optional HDR intensity value where meaningful.
- **Aspect:** Define behavior under 16:9, 21:9, 9:16, split-screen, dynamic resolution, and cine camera filmback. Nonuniform stretch is an explicit artistic choice.
- **Ranges:** UI slider min/max are usable ranges only; numeric entry may allow wider validated bounds. Document useful values, warnings and hard shader caps separately.

## 2. Type-specific appearance controls

These are proposed Glint v1 controls. All types also inherit appropriate common controls above.

| Element | Specific exposed controls | Render primitive / quality strategy |
|---|---|---|
| **Glow** | Radius, Inner Radius/Core Size, Softness, Falloff Exponent, Bright Core, Outer Tint, Elliptical Stretch, optional Lens Halo | Analytic bounded quad or selected LUT; avoid full-screen quad for small glows |
| **Iris** | Shape (Circle/Polygon/Texture), Blades, Blade Roundness, Edge Feather, Inner/Outer Radii, Internal Rings, Aberration/Fringe, Texture, Texture Rotation | Single analytic or textured quad; aperture corners must remain coherent with rotation |
| **Multi Iris** | Count, Start/End Axis Distance, Spacing (Uniform/Jitter/Curve), Size Distribution, Rotation Variation, Brightness Variation, Scatter X/Y, Seed, Individual Override List | One generator producing multiple batched iris records; compact authoring remains canonical |
| **Streak** | Length, Width, End Taper, Softness, Falloff Curve, Fan Spread, Asymmetry, Gradient Along Length, Rotation Mode | Bounded analytic quad / limited segment fan where needed; long thin bounds carefully clipped |
| **Shimmer** | Ray Complexity, Angular Distribution, Inner/Outer Radius, Length Irregularity, Width Irregularity, Evolution, Undulation Amount, Speed, Random Seed | Bounded radial procedural kernel or cached geometry; deterministic temporal motion |
| **Glint (Star Rays)** | Ray Count, Distribution, Width, Length, Sharpness, Length Randomness, Brightness Randomness, Phase, Offset Angle | Narrow spike triangles/analytic ray kernel; allow crisp hard diffraction or soft glints |
| **Spike Ball** | Spoke Count, Petal Width, Petal Profile, Inner Radius, Max Length, Length Distribution, Angular Variation, Core Softness, Evolution | Batched/petal geometry or procedural polar kernel, density-aware LOD |
| **Sparkle** | Spark Count, Fragment Size, Radial Spread, Fragment Angle, Opacity Variation, Color Variation, Random Seed, Spark Evolution | Quads or packed radial generated element data; batch independently of per-spark UObject |
| **Ring** | Radius, Thickness, Feather Inner/Outer, Arc Start, Arc Sweep, Completion Feather, Polygon Sides, Gradient Around Circumference, Chromatic Edge | Analytic annulus; arc support without pre-baked textures |
| **Hoop** | Radius, Arc Start/End, Arc Tilt, Ellipticity, Curvature/Focus, Taper, Spectral Gradient, Axis Alignment, Brightness Asymmetry | Arc analytic shader or tessellated screen-space strip; shape predictable under aspect changes |
| **Caustic** | Focus, Distortion, Asymmetry, Curl/Curve, Spread, Softness, Fringing, Spectral Palette, Completion, Light Distribution | Procedural polar mapping with quality caps; expensive distortion path opt-in |
| **Lens Orbs / Dirt** | Lens Texture, Dirt Mask, Density, Scale, Clarity/Blur, Illumination Radius, Falloff, Local Color Response, Rotation, Tile Mode | Shared lens-space composite per view; no per-light full-screen pass |
| **Texture Sprite** | Texture asset, Texture Channel, UV Transform, Crop, Filter, Scale, Orientation, Animation Mode/Frame, Colorization, Premultiplied Alpha policy | General bound quad; UE texture soft reference/cooking verified |

### Iris and Multi Iris distinction

An Iris is one reflection **instance**. Multi Iris is one **authoring object** that expands to `N` instances with deterministic per-index variation. The user should see and control the generated ghosts while selecting Multi Iris, but need not manage `N` child UObjects. Provide **Expand to Individual Irises** as an explicit destructive-but-undoable conversion, not automatic on selection.

### Lens dirt special case

Lens Orbs/Dirt is not just another movable ghost. A lens texture stays in camera/lens coordinates while the source illuminates it. It must visually respond to source location and brightness without independently moving with the source. In the stack, it should carry a clear **Lens-Space** tag and have visibly different Placement controls. This can remain a special renderer path while appearing in the same authoring stack.

## 3. Color and blending semantics

### Color modes

- **Global:** inherits the authored overall Glint tint and optionally source light color.
- **Solid:** independent single color, optionally multiplied by the global tint.
- **Two Color:** radial or longitudinal interpolation between two colors, with color-space-correct blending.
- **Gradient:** a `FRichCurve`-like editor color ramp, placement axis selectable (radial, around ring, along streak, across width).
- **Spectrum:** spectral rainbow stylization with Hue Offset, Reverse, Repetitions, Smoothness, Saturation; implement distinct effects from physically based chromatic aberration.
- **Texture Color:** use texture RGB with tint/brightness applied according to documented policy.

### Blending restrictions

- **Additive** is defined in scene-linear light. It may push pixels above 1, intentionally; tone mapping then shapes highlight rolloff.
- **Screen** is conventionally `1-(1-src)*(1-dst)` for unit-interval values. **Do not naively apply that math to unconstrained HDR radiance**: >1 values can produce incorrect/nonmonotonic results. Choose an explicit bounded artistic screen-like mode and validate the result under exposure; shader implementation gated by R0.
- **Alpha/Soft Add** respects texture edges and subdued artistic looks. Establish straight vs premultiplied alpha rules, especially for texture import.
- Store **Opacity** and **Brightness** separately. Under Add, opacity may weight radiance; under other modes it also affects coverage/composition.
- **Stack order** matters for noncommutative blend modes, lens-dirt illumination and groups; batch/reorder only where the operator is provably order-independent. Group order remains artist-visible.

## 4. Placement mathematics and manipulators

Let source screen location be `S`, optical center `C` (possibly lens shift / off-center projection), and axis distance `d`. Define `P = S + d(C-S) + O` with normalized offset `O`.

- `d=0`: element tracks source position.
- `d=1`: element stays at optical center (except authored offset).
- `d=2`: element mirrors source across optical center.
- `d<0` and `d>2`: allowed for expressive lens reflections and offscreen optical paths.
- `AxisAngle = atan2(S.y-C.y, S.x-C.x)`; `Rotation = Base + AxisAngle` only for axis-following modes.
- `OpticalCenter != (0.5,0.5)` is supported for shifted camera lens projections where feasible; avoid hardwiring center-of-viewport as the optical center.

**Gizmos:** Drag a source marker, one selected element's center handle, and optional axis-distance handle. `Shift` constrains edits to the optical axis; `Ctrl` enables optional snapping; `Alt` drag duplicates only if it follows UE conventions. All drag edits use editor transactions and Escape cancel. Never steal UE viewport mouse cursor when Glint edit mode is not active.

## 5. Default values and presets strategy

Use artist-meaningful defaults, not an empty, invisible element: a Glow is visibly soft; a Streak is thin and clearly horizontal; a Multi Iris gives five varied ghosts; a Ring is a partially transparent halo; and Shimmer is recognizable at default size. Store these defaults in the **asset factory/element defaults**, with one source of truth shared by new Element and Add menu previews.

Define three independent reuse layers:

1. **Element preset:** a single element including its dynamics and textures.
2. **Group preset:** related elements with group overrides, useful for e.g. four glints and their collective color.
3. **Glint asset:** full composition and global behavior, used directly by runtime sources.

User preset thumbnails are generated by the real rendering path in a controlled background; cache thumbnails, do not render every preset live while scrolling.

## 6. Required tests

- `Glint.Data.ParameterDefaults`: each added type has visible, valid defaults, compatible enum names and correct serialized values.
- `Glint.Data.ParameterGating`: inactive modes hide/disable irrelevant controls (no silent noop setting).
- `Glint.Math.AxisDistance`: 0/1/2, negative and large factors; custom optical center; mirrored source.
- `Glint.Math.Aspect`: 16:9, 21:9, 9:16, shifted filmback and subview/split-screen.
- `Glint.Data.MultiIrisStableSeed`: count + seed reproducible across save/cook and independent sources.
- `Glint.Render.ColorSpaces`: linear input, gamut, exposure, additive, screen-like fallback, opacity, texture alpha.
- `Glint.Editor.ElementTransactions`: drag, Escape cancellation, undo/redo, duplicate, expand group, save/reload.
- `Glint.Render.ElementBounds`: huge streaks and hoops don't allocate full-screen primitives unnecessarily.

## 7. Decisions needing R0–R2 measured proof

Final numerical limits, spectral kernel quality, Screen-like blending definition in HDR, procedural vs texture paths per type, dirt lighting order, color ramp representation and live preview rendering integration. The **artist-facing terminology and serialized semantic intent** should remain consistent regardless of renderer implementation.

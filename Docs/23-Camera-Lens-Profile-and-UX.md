# 23 — Glint Lens Profile: Asset Schema, Camera Precedence and Native UX

[← Index](README.md) · [21 — Automatic vision](21-Automatic-Lens-Vision-and-Workflows.md) · [22 — Detection](22-HDR-Highlight-Detection.md) · [24 — Hybrid](24-Automatic-and-Authored-Rendering.md)

## Main design decision

Use **two complementary first-class Content Browser assets**:

- `UGlintLensProfile` — **Glint Lens**: describes the optical behavior of a *camera*, automatic highlight detection, image-space response, lens dirt, lens response, quality and source-template mapping.
- `UGlintAsset` — **Glint Flare**: an editable stack of source-relative visual elements (the existing proposed Optical Flares-familiar composition). Authored source components can reference it directly; automatic highlighted candidates may also use its elements through the active Lens Profile.

This split makes automatic mode legible without sacrificing the familiar element editor. A Lens Profile may contain a simple embedded default composition for easy onboarding, but should reference shared `Glint Flare` assets when reuse, nested overrides or richer editing is needed. Maintain hard-reference/cook rules and prevent cyclic dependencies.

## Where Glint becomes active

1. **Project Settings > Glint > Default Lens:** One reference applies automatically to eligible camera views, including the editor level viewport preview when enabled; no actor needs adding to every level.
2. **Optional Glint Lens Component** on CameraActor/CineCameraActor or Pawn camera: overrides the global lens profile for that camera. A component is a convenient *override*, **not** a requirement for auto operation.
3. **Glint Lens Volume**, only if validated worthwhile: defines a lens profile within a bounded world region, with blend radius/priority/weight analogous to familiar Post Process Volume semantics. Don't claim existing Post Process Volumes natively store plugin-specific properties.
4. **Runtime/Sequencer override:** camera-selectable lens state plus intensity/sensitivity/disable controls; explicitly define blend and cut transitions.

### Proposed precedence

`Project default → active Lens Volume(s) → camera component override → temporary cinematic/gameplay override`. Higher priority wins where a property is explicitly overridden. Avoid ambiguous equality: use explicit priority, deterministic tie-breaking and clear effective-value readback. A camera can opt out of automatic mode while still accepting explicitly authored sources if configured to do so.

Lens profile resolution must run per actual `FSceneView`/view context, not via `GetPlayerCameraManager` only: editor views, split-screen and captures can have different cameras. Respect world/editor preview enablement and view families; document tested support rather than promising all view types.

## Lens asset editor layout

Use native UE Slate dockable asset editor tooling, `AssetDefinition`, Details customization, toolbar commands, transactions, asset dirty/save state, native UE selection/blue accents, and Content Browser asset creation.

Suggested tabs:

| Tab | Goal | Essential contents |
|---|---|---|
| **Overview** | Immediately get pleasing effects | Preview, starter presets, Enable Automatic, Sensitivity, Overall Intensity, Max Prominent Flares |
| **Highlights** | Control what counts as bright | Threshold, source area/contrast, merging, classification, exposure policy, source ranking |
| **Optics** | Control lens personality | Aperture, ghost color/deformation, response by focal length, coatings-inspired artistic parameters, anamorphic orientation |
| **Elements** | Build the flare composition | Reuses familiar Glint Flare element stack/editor with group/duplicate/solo/visibility/selection |
| **Camera Surface** | Dirt/dust/scratches | Single shared lens-space texture response, illuminated by entire scene/selected highlights |
| **Quality** | Keep gameplay stable | Field resolution, max detailed candidates, ghost count, render resolution, fallback policies |
| **Preview & Diagnostics** | Explain behavior | Scene presets, camera path, HDR display, threshold mask, candidate IDs/rejection, color-space test |

The lens editor should open to **Overview**, not a dense property wall. An Optical Flares user can jump into Elements and recognize Glow/Iris/Streak/Multi Iris workflows immediately. A lighting artist can ignore the element stack until needed.

## Basic UI controls (mock specification)

```text
GLINT LENS — Vintage Anamorphic

[Automatic Flares: ON]    Source Mode: [Automatic + Authored]

Lens Preset      [Vintage Anamorphic v]    [Save As New]
Sensitivity      [---------●-----]         Visible threshold readout
Flare Strength   [------●--------]
Ghosts           [ON]  [Subtle v]          Image-space + candidate behavior
Streaks          [ON]  [Anamorphic v]
Lens Dirt        [ON]  [Inspect...]
Max Prominent    [8]                      Complexity budget, not total highlights

[Preview scene v] [Animate camera] [Show Highlight Candidates]
[Preview HDR] [Threshold Mask] [Field Only] [Candidates Only] [Final]
```

### Property meaning and grouping

- **Sensitivity** changes which bright regions qualify, **not** the brightness of already accepted flares. Its tooltip should state which detection parameters it drives.
- **Flare Strength** adjusts rendered intensity, **not** source detection.
- **Max Prominent Flares** limits full procedural per-source compositions; other bright highlights may still contribute field streaks/ghosts.
- **Source Mode** controls input eligibility, not which lens composition is used.
- **Focal Response** determines how lens optics react to FOV/camera geometry; default expected to use available CineCamera settings when safe.
- Expose variables only in modes where they operate. Use `EditCondition` or native Details visibility rather than allowing parameters to look editable but do nothing.

## Cinematic lens integration

UE5.8 `CineCameraActor` exposes filmback sensor dimensions, focal length, F-stop/aperture, diaphragm blades and squeeze factor. These are meaningful inputs to an optional link:

- Aperture blades and curvature-like parameters affect ghost iris shape and starburst.
- Focal length and filmback affect framing and chosen ghost dispersion/stretch curves.
- Anamorphic squeeze informs lens-space distortion, but should not automatically duplicate/override artistic anamorphic settings.
- Sensor shift/crop and constrained aspect ratio require separate optical-center mapping tests.

**Authority toggle:** `Use CineCamera Lens Properties` ON/OFF at profile and/or camera override. Show effective property and source in tooltips. An ordinary CameraComponent still works by using project/profile fallbacks.

See [Cine Camera Actor UE5.8](https://dev.epicgames.com/documentation/en-us/unreal-engine/cinematic-cameras-in-unreal-engine).

## Presets and documentation

Ship original lens profiles: **Natural / Clean Digital / Vintage Glass / Anamorphic / Sci-Fi / Warm Film**. Their exact parameter values are authored original assets, tuned and benchmarked against the same fixture scenes. Store cost tier metadata in profile assets and verify cooks. No proprietary presets imported from Video Copilot/Maxon/REAL FLARES.

Add a new `Glint Lens` property tooltip set, 10-minute first-use guide, team workflow (set global lens once; override for hero cameras), and a migration note: replacing UE's built-in image-based lens flare visually requires manually managing that engine effect to avoid double results, but Glint must not globally alter an existing project's postprocess settings without user choice.

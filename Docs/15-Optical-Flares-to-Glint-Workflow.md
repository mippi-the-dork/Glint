# 15 — Optical Flares to Glint: Artist Workflow and Native UE5 Editor Design

[← Documentation index](README.md) · [03 — Editor UX](03-Editor-UX.md) · [13 — Behavior](13-Behavior-and-Trigger-System.md)

> **Product goal:** An Optical Flares artist should recognize *what they can do and why* in their first minute; a UE5 artist should immediately recognize *how to do it*. This document intentionally adapts familiar concepts into UE's asset editor instead of visually cloning proprietary application UI.

## 1. The first 60 seconds

1. Content Browser → Add → choose **Glint** under an appropriate existing VFX/Materials category (final placement verified against UE5.8 menus); result is a **Glint Asset**, not a generic Data Asset chooser.
2. Double-click; the native asset editor opens with **Preview** in center, **Elements** on left, **Details** on right, and an accessible **Element/Preset Browser** tab or bottom shelf.
3. Press **Add Element → Glow** or **Streak**; the result appears immediately in Preview with a relevant default. The new element is selected in Stack and its parameters appear in Details.
4. Drag the source marker in the Preview: axis ghosts and dynamic responders move; regular UE drag/scrub fields are available for precise values.
5. Save. Add a **Glint Source** component to an existing Actor/light or choose Blueprint **Spawn Glint at Location**; assign asset. Visible in level editor, PIE, packaged builds.

The author should not need to create a material graph, spawn 12 actors, or understand Render Graph to produce a polished flare.

## 2. Proposed editor anatomy

```
┌ Glint / GL_SciFiBeacon                         [Save] [Undo] [Redo] [Stats] ┐
│ [Add Element ▼] [Add Group] [Preset Browser] [Play/Pause] [Preview Source] │
├────────────────┬──────────────────────────────────┬───────────────────────┤
│ ELEMENT STACK  │            LIVE PREVIEW          │ DETAILS               │
│ Search...      │                                  │ Glint (Star Rays)     │
│ ● Glow         │  axes / optical center /         │ Appearance            │
│ ● Streak       │  selected element handles /      │ Placement             │
│ ▾ Ghosts       │  trigger heatmap if toggled      │ Shape                 │
│   ● Multi Iris │                                  │ Color                 │
│   ● Ring       │  [Drag Light]                    │ Dynamics              │
│ ● Glint        │                                  │ Quality               │
│ ● Lens Dirt    │                                  │                       │
├────────────────┴──────────────────────────────────┴───────────────────────┤
│ PRESETS / ELEMENT LIBRARY:  Natural   Anamorphic   Sci-Fi   Custom           │
│ [Thumbnail] [Thumbnail] [Thumbnail] [Thumbnail]                            │
└───────────────────────────────────────────────────────────────────────────┘
```

Use UE's existing asset editor framework, `FTabManager` dockable panels, `SListView`/`STreeView`, Details customization, `FScopedTransaction`, Asset Definition, factory, asset registry, style icons, content browser references and keyboard shortcuts. Save/dirty/undo should behave exactly like other native Unreal assets.

### Left: Element Stack

- Thumbnail + icon + name; optional compact **Brightness** and **Scale** numbers just as in the Optical Flares mental model.
- **Eye**, **Solo**, **Lock**, **Delete** icons with tooltips; no ambiguous unlabeled state.
- Drag reorder; keyboard multi-select, duplicate, copy/paste between assets, group/ungroup, enable/disable, solo selected, rename, and find.
- Each group holds inherited tint, intensity, visibility, optional transform and quality priority; collapsing group should preserve selection feedback.
- **Solo is a preview diagnostic**, not a permanent hidden state; mark separately from serialized Enabled.
- Show warning/cost hint on elements with expensive full-screen behavior (e.g., Dirt / large Shimmer).

### Middle: Preview

- This is a **real rendering preview** of the final renderer, not an approximated Slate painting of flare shapes.
- Source marker, optical-center marker and dashed source-to-center axis. Selected element shows position, scale and rotate handles; other elements are not cluttered with gizmos.
- Toggle black/checker/level scene/background texture, exposure preview, neutral reference camera, aspect guides, HUD source and focus mode.
- **Drag Light** adjusts test source position only, never mutates saved flare asset unless explicit source-position authoring mode is selected.
- **Preview Trigger** renders actual driver mask/heatmap and live values; **Preview Occlusion** uses a test blocker plane and can preview soft fade.
- Hover source: display source screen coordinate and projected behind/offscreen status. Hover element: show `Axis Distance`, resolved brightness and current visual cost when Stats enabled.
- Optional popout preview in a separate native editor tab/window; preserve editor's docking system.

### Right: Details

- Selected element title plus type icon; common property ordering identical across types.
- Sections: **Appearance**, **Placement**, **Shape**, **Color**, **Dynamic Triggering**, **Distortion**, **Advanced Quality**.
- Every number uses UE native numeric entry with digit drag/scrub, reset arrow, units, sensible slider clamp and tooltips.
- Shape-specific fields hidden unless relevant. Choosing Texture Shape displays `UTexture2D` picker and UV controls; choosing Polygon Shape displays blades/roundness instead.
- **Global Parameters** are selected via a distinct top Stack row, exactly analogous to familiar global-vs-object mental model.
- Display resolved preview value beside authored base only when dynamics are enabled; never overwrite authored values due to preview camera movement.

### Bottom / right: library

- Tabs for **Glint Presets** (whole Glint assets), **Element Presets**, **Groups** and **Textures**.
- Search/filter by natural, cinematic, sci-fi, anamorphic, photographic, low-cost; favor categories/tags native to UE and an optional plugin-only favorites state.
- Asset thumbnails are generated from a controlled Glint source + renderer, not preview screenshots imported from a vendor.
- **Add Element** can insert a preset into stack; **Apply Glint Preset** replaces composition after undoable transaction; **Merge Preset** adds selected groups/elements.
- Asset reuse should be clear: **Copy Values** vs **Reference Shared Element Preset** only if latter is implemented with explicit update semantics. No implicit shared changes.

## 3. Familiar names, UE-native behavior

| Optical Flares language | Glint UI | Why |
|---|---|---|
| Lens Objects | Add Element / Element Library | Artist sees expected palette, Unreal sees named asset actions |
| Object Stack | Element Stack (left dock) | Familiar composition hierarchy with native UE tree/list interaction |
| Global Parameters | Global (top Stack item) / Asset Settings | Keeps cross-element controls visible |
| Options window | Dedicated Glint Asset Editor | Avoids a modal external-feeling UI |
| Preview / Preview Position | Live Preview / Drag Source | Explicitly distinguish preview source from authored asset |
| Object Brightness | Brightness | Retain term, not obscure substitutes like EnergyGain |
| Distance / Offset | Axis Distance / Position Offset | Clarify with on-screen source/center axis |
| Dynamic Triggering | Dynamic Triggering | Keep exact concept/name for transition; give quick presets |
| Evolution | Animation Evolution | Preserve familiar entry point; enable playback/scrubbing |
| Lens Texture | Lens Dirt / Lens Texture | Same function, clearer static lens-space meaning |
| Screen/Add | Blend Mode: Screen-like/Additive | Familiar, with realistic HDR constraints disclosed |
| User Presets | Glint Assets / Element Presets | Integrate with Content Browser and cooking |

## 4. Five UX requirements specific to this plugin

1. **Visual-first but precise:** preview manipulation always has a matching numeric control; numeric edits always update preview immediately.
2. **No unpredictable mode switches:** if Add Element changes selection, the right panel must show that element, while play state and preview camera remain intact.
3. **Native Unreal input:** standard undo/redo, mouse capture, Escape cancel, right-click context menu, Ctrl+C/V, Content Browser soft refs, transaction-safe reordering.
4. **Controls show their effects:** no placeholder color swatches, no unresponsive disabled-but-editable property sections, no element setting that only works in an undocumented rendering mode.
5. **Performance in authoring:** a complex flare shouldn't make the editor sluggish. Preview quality and runtime quality can be reduced independently only through clearly labeled **Preview Quality** and **Runtime Scalability**.

## 5. User workflow acceptance exercises

**New Optical Flares user entering Glint:** Can make a Glow + Streak + five ghosts + dynamic border flare-up and place a light in under ten minutes from a quickstart. Can identify which Glint UI control matches Brightness, Scale, Axis Distance, Color and Dynamic Triggering without reading a technical manual.

**New UE5 user:** Can create a Glint Asset via Content Browser, modify it with standard Details controls, attach a source to SpotLight, save/reopen and package. No reliance on Blueprint-only editor authoring.

**VFX artist recreating an optical reference:** Can load a user-provided reference frame as a **preview-only** background, adjust selected elements with a center-line guide and compare movement through a scripted test path, without importing proprietary presets.

**Gameplay designer:** Can create three beacon flare instances using one Glint Asset and expose only runtime intensity and tint overrides. The asset stays unchanged as gameplay changes source intensity.

## 6. Screenshot and tutorial needs

- Quickstart: 1-minute add and place; 5-minute anamorphic; 10-minute ghost chain.
- Element dictionary with **each type rendered alone** and common vs unique controls highlighted.
- Dynamic Triggering gif/video: source moves from center to border, red mask and resolved brightness shown.
- Position guide: Source / Center / Mirrored axis factors 0/1/2 with camera aspect variations.
- Performance lessons: a very large Glow and unbounded Dirt can be expensive despite a small element count.
- Troubleshooting: no visible source, light behind camera, occluded by sky/geometry, double flare due to native Lens Flare, exposure washed out, unsupported view path.

## 7. Accessibility / usability

- Tooltips explain outcome and preview handle, not merely restate label.
- Strong selection highlight using UE5 native selection blue; distinguish enabled, solo, selected and grouped.
- Icon-only actions must have descriptive tooltips and visible focus states.
- Use native scaling/high-DPI; test 125/150/200% UI scale and narrow editor width.
- All artist-important controls reachable from keyboard with searchable Add menu and numeric editing.
- Source preview motion can be paused, limiting automatically flashing/moving imagery while authoring.

## 8. Feature deferrals

A node-based optics compositor, exact OFP import, automatic photo-to-flare fitting, unbounded full-resolution connected-component segmentation beyond the budgeted GPU auto-detection system, fully physical glass-stack tracing and timeline curve editor for every low-level element are **not necessary** for v1. Favor fast, understandable response curves + real-time game bindings.

## New Optical Flares familiarity: Luminance Tracking / Automatic Lens

Experienced Optical Flares users may remember its **Luminance Tracking** feature. Glint's lens-side **Highlights** panel should expose similarly recognizable threshold and resolution concepts, complemented by game-focused GPU candidate caps, stabilization, source-area response and a debugging preview. Keep the original direct element/flare editor layout, but provide a native **Glint Lens** asset that applies the composed behavior automatically to scene brightness without attaching sources. See [21](21-Automatic-Lens-Vision-and-Workflows.md) and [23](23-Camera-Lens-Profile-and-UX.md).

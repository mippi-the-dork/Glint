# 03 — Native Editor UX and Workflows

[← Documentation index](README.md)

## UX recommendation

Use a **layered element composer** rather than a general node graph. The user is arranging independently renderable optical artifacts, not building a freely connected computation graph. Preserve the Video Copilot mental model of Preview, Stack, per-object Editor, and Preset Browser, but implement each as native dockable Slate panels. A node graph would introduce friction and waste canvas space for this domain.

## Native integration

- Create via Content Browser: **FX (or the engine's appropriate existing asset category) > Glint**; do not add an unnecessary top-level category if a native Effects/VFX category is available.
- `UAssetDefinition` / `UAssetDefinitionDefault` and asset factory for creation/launch; custom Slate asset editor using Unreal standard tabs, toolkit commands, menus, toolbars, Details, asset picker and transactions.
- Glint is a dedicated asset type with an internally editable data representation; users are not sent through **Data Asset > choose class**.
- Standard UE dockable tabs, dark styling, selection blue, keyboard accelerators, content picker thumbnail previews and asset dirty/save affordance.
- Settings menu should navigate directly to the plugin's section in Project Settings or Editor Preferences, not the settings home page.

## Default layout

```text
┌───────────────────────────────────────────────────────────────────────┐
│ Glint Asset | Save | Undo/Redo | Add Element | Play | Preview | Stats   │
├──────────────────────┬──────────────────────────┬─────────────────────┤
│ ELEMENT STACK        │ LIVE FLARE PREVIEW       │ SELECTED DETAILS    │
│ search/filter         │                          │ responsive to type │
│ - Glow                │ grid, center, source     │ Color, size, ratio  │
│ - Streak              │ on-axis handles, guides  │ Falloff, blending  │
│ - Ghost Group         │ reference overlay        │ Animation/occlusion│
│   - Iris              │                          │                     │
│   - Ring              │                          │                     │
│ + Add                 │                          │                     │
├──────────────────────┴──────────────────────────┴─────────────────────┤
│ PRESET LIBRARY / ELEMENT LIBRARY / DIAGNOSTICS (dockable tab)          │
└───────────────────────────────────────────────────────────────────────┘
```

Panels should be resizable, dockable, hideable and remembered across sessions using editor layout persistence. Make no assumptions about a user's resolution or DPI.

## Editor layout behavior

### Element stack

- Rows show type icon, friendly label, visibility eye, solo, lock, color swatch where useful.
- Drag/drop order and group; insert via searchable Add Element menu; context menu: duplicate, copy, paste, group, remove, reset.
- Multi-selection can move/scale shared values, with mixed-value display handled honestly.
- No two separate controls for the same action unless each has a distinct documented scope.

### Preview

- **Runtime-faithful** generated flare, not a static custom editor-only recreation.
- Grid/center axes, source dot, drag handles, optional guides/ghost axes, light-to-center line, zoom and pan.
- Adjustable test source on/offscreen and depth. Test edge fade, occlusion cover, intensity, camera-relative changes, aspect ratio and animation.
- Preview background options: black, checker, image/reference, neutral scene, controlled depth occluder; allow previewing without a level.
- A/B comparison, full-screen preview and pause/scrub animation; clear preview-only state distinct from saved asset properties.
- Reference image is an editor-only authoring aid, **not** silently cooked into each runtime asset.

### Details

- Sections named for actual concepts: Appearance, Placement, Shape, Color, Distortion, Animation, Quality, Overrides; hide irrelevant sections per element type.
- Native color picker, curve editing, numerical scrubbing, eyedropper only where valid, reset-to-default, tooltip units/ranges.
- Collapse non-applicable advanced settings and indicate when one parameter overrides another.
- Provide source/preset-level intensity scale without making each element permanently dependent on an individual Actor.

### Preset browser

- Visual thumbnails for complete Glint assets and reusable **element** definitions.
- Search/type filters, recently used, favorites and categories/tags; drag asset into stack to copy elements with provenance/optional source link.
- Saving an element preset and saving a whole Glint asset are separate, clearly named actions.
- No opaque bundled proprietary image/textures; create original sample presets/art.

### Native save behavior

- Every authoring action is transactional; Ctrl+Z / Ctrl+Y or native Editor redo command; Esc cancels active drag and restores prior state.
- Dirty mark appears only when persistent asset data changes. Preview simulation changes should not dirty the asset.
- Preview edits should not secretly change runtime placement bindings.
- All tabs/selection states survive editor hot reload or reopen where practical; asset source of truth is serialized.

## Key user journey acceptance tests

1. New Glint in Content Browser; double click; default Glow appears with clear visible preview.
2. Add Streak and Iris; drag their axis handles; change size/color; Undo/Redo; Save; reopen; result matches.
3. Duplicate a group; copy elements to another Glint asset; copy/paste preserves exact authored state.
4. Solo selected element, hide a group, lock element; these actions are readable without guessing.
5. Pick an Actor source and see the exact same Glint composition inside a PIE viewport.
6. Change viewport aspect ratio; reframe preview without distortion or surprising offscreen movement.

## Deliberate exclusions

No full general-purpose node graph in v1; no editor-specific visual feature absent at runtime; no custom aesthetic overhaul of native Unreal controls just to imitate a third-party product.

## Optical Flares parity extension

The [artist workflow crosswalk](15-Optical-Flares-to-Glint-Workflow.md) supersedes any sketch here that omits the 12-element palette, quick Brightness/Scale fields, source-to-center axis, global parameters or trigger heatmap. A clear distinction between **preview-only source position**, **authored element placement**, and **runtime source overrides** is required. Triggering and selected shape fields must be visible in the same Details system rather than a separate third-party feeling options popup.

## Automatic lens editor and camera-first entry point (2026-11-12)

The default onboarding editor is now **Glint Lens**, with Overview (Sensitivity/Intensity/Max Prominent Flares), Highlights (threshold, size, contrast and GPU extraction), Optics, Elements, Camera Surface, Quality and Diagnostics. The existing Flare/element editor described above remains intact as an authoring experience for complex source-relative compositions, but it is **not required** to make a camera lens automatically flare. The editor should support switching from a Lens asset to its candidate Flare template without losing layout/state.

Native controls should show **effective value and ownership** for project default, camera override, volume override and linked vs local lens properties. Settings that do nothing for the selected source mode must be hidden or disabled with a reason. In the level viewport, offer Glint display toggle and a dedicated `Show Highlight Candidates` debug visualization showing bright mask, merged peaks, source IDs and suppressed candidates. See [23 — Camera Lens Profile & UX](23-Camera-Lens-Profile-and-UX.md).

## Cross-engine authoring additions

- **Repeater** (inspired by documented Unity SRP functionality): for compatible elements choose Count, Spread, Distribution (Uniform/Curve/Seeded Random), color/size/rotation/intensity variation and Seed. Advanced users may expand and locally override generated entries; no uncontrollable hierarchy recursion.
- **Screen Edge** group: deformation curve, brightness and size response; obey camera aspect/anamorphic policy and avoid using raw screen pixels as an artist's only unit.
- **Source Visibility** group: Uniform Fade default; disabled/independent and optional Shape Response for families where an occluded source silhouette influences shape. Show exactly which behaviors are supported per element.
- **Lens Context presets**: Clean, Cinematic, Anamorphic, Sci-Fi, Visor/Dirty; informative rather than enforcing optical realism.
- **Diagnostic**: warn if the editor-preview camera or active Unreal Post Process Volume also has stock image-based lens flares enabled, to prevent misleading double output.
- Refer to [34](34-Other-Engines-Real-Time-Lens-Flares.md), [35](35-Shipped-Games-and-Visual-Design-Lessons.md) and [37](37-Cross-Engine-Findings-and-Prototype-Gates.md).

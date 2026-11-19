# Glint 0.5.0 — Manual QA and Automation

**Compilation and UE 5.8.3 GPU execution pending.** Static checks do not substitute for launch-time global shader compilation.

## Automated checks

Run `Automation RunTests Glint` in Unreal. Version 0.5.0 registers **22** tests: retains all 16 from 0.4.0, adds two camera-frame/offscreen tests, two image backdrop/aspect tests, and two authoring/shape-default tests. Verify the run list and investigate any failures independently.

## Required editor checks

1. Create new Glint Flare; add each of the 13 types and verify the starting placement, type-specific properties, appearance, label and preview reflect its family.
2. Change an existing element's Type; verify it gains sensible default behavior but a deliberately customized Label stays.
3. Verify outliner above selected Details in left panel, with main preview on right. Resize splitters.
4. Drag element grip to another row; verify stable ordering, selected/solo index follows, Undo and Redo. Verify Solo icon changes from outline to solid. Verify trash Remove and Undo.
5. In Schematic and Hybrid, drag an element. Verify left-click selection by itself doesn't dirty the asset. In Rendered, hidden handles must not accidentally capture clicks.
6. Right-drag light source across each edge and into authoring margin; verify camera-frame guide, marker accessibility, and continued correct source behavior.
7. Import widescreen and portrait screenshots as Texture2D, assign each from Gear > Camera background. Verify the inner camera frame and GPU output follow texture aspect without distortion, and reference appears under the optics in Rendered and Hybrid only.
8. Clear the reference texture, reopen the asset and verify the per-asset setting persists. Selecting a reference must **not** dirty the Flare asset or create a hard runtime dependency.
9. Change exposure and source colors in Gear. Confirm visual preview updates and all modes share one state.
10. Use the original emissive red/green/blue sphere scene in the main viewport; verify automatic HDR extraction, source tracking and zero-outside-frame edge sampling remain correct.
11. Verify `r.Glint.Automatic 0`, `r.Glint.Authored 1`, `r.Glint.SourceTracking 1`, and all on/off combinations to compare source-driven and image-field paths.
12. Observe GPU time with RenderDoc/Insights at several source/element budgets; no performance budget claims until measured.

## Known limits

Camera screenshot backdrops are LDR-style visual references, not HDR environment simulations. Native Gameplay Camera Rig node and live scene editor capture are not completed in this pass. New element types have best fidelity in source-aware mode; fallback field mode remains approximate. Runtime Win64 Cook/package and GPU performance still need direct verification.

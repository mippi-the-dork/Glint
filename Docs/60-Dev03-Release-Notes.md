# Glint 0.3.0 — Dev03 Release Notes

**Scope:** Glint only. Full plugin source (not a released Fab build). Windows compilation and GPU shader launch still require validation.

## Renderer

- Added parallel HDR tile-source detection and a bounded ranked GPU source list.
- Added per-view, camera-cut-aware temporal source-location matching without CPU GPU readback.
- Rendered authored Glint Flare elements relative to individual detected highlight positions.
- Preserved v0.2 image-based automatic effects and their zero-outside-viewport edge sampling.
- Added `r.Glint.DebugSources`, `r.Glint.SourceTracking`, and `r.Glint.MaxSources` performance control.
- Added independently editable `ScreenOffset` for each authored element.
- Added optional view-aperture coupling for authored element size.

## Editor

- Added dedicated native Slate Flare asset editor with optical-axis placement canvas.
- Added element list, add/duplicate/remove/reorder, canvas selection and drag position editing, plus native Details and transactions.
- Canvas is a schematic preview, not a GPU live preview.

## Tests and verification

- Expanded native automation from seven to twelve tests; new tests cover source selection, limits, temporal matching/camera cuts, source settings, and f-stop response.
- Expanded shader contract checks from three to five compute entry points.
- Static source, packaging, and documentation checks pass in the delivery environment; actual Windows UBT compilation and shader runtime are unverified.

## Remaining high-priority work

Native Gameplay Camera Rig graph node and automatic rig blend-state synchronization; GPU-backed live preview; robust connected-region grouping and source identities; source occlusion and offscreen light support; optical element quality improvements; 1080p/1440p/4K GPU benchmarks, multi-view/camera-cut validation, Win64 cook/packaging.

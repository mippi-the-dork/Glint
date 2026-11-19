# Dev03 — GPU source localization and temporal matching

## Implemented data flow

`Glint.Extract` downsamples the pre-exposed scene's valid per-view HDR rectangle 4:1, retaining a peak/mean mixture from 4×4 tiles. `Glint.FindSourceCandidates` sends one group of 8×8 GPU lanes per 32×32 bright buffer tile. Each lane scans 16 bright pixels and 64 lanes reduce to one maximum per tile. Candidate data is UV.xy, pre-exposed brightness, validity.

`Glint.SelectAndTrackSources` scans the **small tile-maximum texture**, chooses no more than eight bright points with minimum UV separation, and matches each to a previous per-view GPU texture record. It smooths position with Lens.SourceSmoothing. Camera cuts, resolution changes and new view states reset the history; the history is pooled and extracted at the end of the RDG graph, not read by the CPU.

`Glint.AutomaticAndAuthored` reads tracked sources and evaluates each authored element around `0.5 + (sourceUV-0.5)*AxisPosition + ScreenOffset`. Its seven preliminary shapes are analytic and screen-relative; the procedural ghost/streak effects remain an image-wide field until they are replaced by physically consistent source-relative optics.

## Limits

This is **per-frame local image-space detection with short-term position matching**, not robust object tracking. Tile extrema can represent different peaks on subsequent frames, especially in wide cloud highlights. It does not detect hidden or offscreen lights. Screen-separated sources can compete for a maximum of eight slots. No deterministic runtime GPU tests yet. Brightness is compensated with view pre-exposure; severe exposure changes, TSR/camera cuts and multi-view behavior require visual tests.

## Performance

- `r.Glint.SourceTracking 0`: legacy authored image field.
- `r.Glint.DebugSources 1`: source positions rendered as crosses.
- `r.Glint.MaxElements 4` and Lens.MaxSources 2: reduced authoring work.
- Observe `Glint.FindSourceCandidates` and `Glint.SelectAndTrackSources` in `profilegpu` / RDG event timings. The element shader loops over source × element pairs, so this stage needs performance gates before high-end optical models are added.

# Glint Development Pass 05 — Camera Frame vs Workspace

## Separate coordinate systems

The larger editor canvas is an **authoring workspace**; the inset bordered rectangle is the **camera's actual rendered image**. The camera frame preserves the active GPU render target aspect ratio, including when the artist selects a reference texture with a different width/height ratio. Offscreen optic positions use the same UV scheme (coordinates can fall below 0 or above 1) and can be inspected in the space around the image.

The preview/source UVs and optical parameters remain unbounded in asset data. Mouse-draggable handles are constrained to the *outer workspace* only. Out-of-workspace positions display selectably clamped directional handles; optical coordinates aren't serialized as clipped values merely because the workspace doesn't display them.

The screenshot background and GPU rendered optical effect are cropped to the camera rectangle. Schematic uses a clean camera guide; Hybrid retains the actual result with editable optical overlays. Right-drag source positions cross the camera edge and respond accordingly.

## Aspect ratio and GPU safety

When the artist selects a reference Texture2D, the camera frame aspect and render target aspect are set together. A one-time render-thread flush occurs only when the preview resolution changes, avoiding release of a render target still referenced by a queued GPU pass. No repeated resize occurs per frame. Default with no reference is 768×480.

## Tests

`Glint.Editor.Preview.CameraFrameAndOffscreen`, `Glint.Editor.Preview.OffscreenHandlesRemainSelectable`, and `Glint.Editor.Preview.ReferenceTextureAspect` cover geometry and hit-testing contracts. Still verify mouse interactions at extreme aspect ratios manually.

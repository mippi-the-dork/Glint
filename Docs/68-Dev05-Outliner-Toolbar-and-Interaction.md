# Glint Development Pass 05 — Editor Workflow

## Layout

The editor is divided into a resizable **left column** (Elements outliner above Selected Element Details) and a resizable **right column** (the visual canvas). The existing native asset Save/Browse toolbar receives Schematic, Rendered, Hybrid, and a preview settings gear instead of duplicating a large toolbar above the viewport. The buttons are appended to the native toolbar; their exact right-edge placement depends on the host editor's toolbar layout.

## Element rows

Each row contains a drag grip, supplied Focus-style Solo icon at the left, persistent visibility checkbox, clickable label/type, and a trash icon at the far right. Drag a grip and drop on another row to reorder by stable insertion (not Swap). Selection and Solo track the element as it moves. Duplicate, Remove, and Toggle Visibility are transactional. Solo only affects the editor's GPU element packing and never changes the saved element state.

## Canvas

- **Left click/drag** on an element to edit its screen offset in Schematic or Hybrid; selection alone doesn't dirty the asset.
- **Right click/drag** moves the preview source, including into the off-camera workspace. No exposed X/Y sliders.
- **Rendered** has no invisible draggable element handles. **Hybrid** shows selectable overlays on the GPU output.
- The settings gear contains exposure, source color channels, and background Texture2D selection.
- Schematic, Rendered, and Hybrid share source position, selection, and per-asset preview preferences.

## Future

A full undo/redo history for editor-only preview source actions isn't necessary: source dragging changes only editor state. GPU thumbnail/element hover picking and interactive rig-node animation remain open.

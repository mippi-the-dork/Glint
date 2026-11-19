# Dev04 — Element Identification and Editor UX

## Three persistent modes

- **Schematic:** no GPU texture shown, with exact editable optical-axis positions, handles and construction lines.
- **Rendered:** GPU-authored optical shapes without schematic overlays, clickable using the shared element center mapping.
- **Hybrid:** GPU rendering under the same schematic markers and selection handles.

The selected element is shared between the canvas, element list, and native Details panel. The Details panel now edits a **single transient `UGlintElementDetailsProxy` object**, so it no longer forces users to expand and search the entire `Elements` array. On change, the selected element is copied back to its authored `UGlintFlare` with an Unreal transaction and dirty notification.

## Fast navigation

The list supports text search over element name or type, one-click asset visibility, and **Solo** preview. Solo does not modify `bEnabled` or dirty the package; it only changes which element contributions are packed for the editor GPU preview. Visibility changes do modify the actual flare asset, are undoable, and affect runtime and editor identically. Repeatedly clicking overlapping markers cycles the candidate elements rather than always selecting the first. Hidden elements are excluded from hit testing.

The canvas supports drag-to-place via screen offset, Shift+drag for positioning the *preview light*, colored selected/hovered handles, and element-type-aware symbols. The preview toolbar controls source X, Y, exposure, and simulated white/red/green/blue light. Mode/source/exposure preferences are stored per flare in editor user configuration; they are not part of runtime asset serialization.

## UX notes

The source preview is intentionally separate from the runtime lens asset. This means artists can compare an element's response to a sample red, green or blue light without changing how the flare plays in the game. Clicking on empty space clears selection. Choosing an element in the list refreshes the selected-object Details panel; selecting in the schematic does likewise. Avoid relying on color alone to indicate selection.

## Known QA risk

The selected-element proxy is an editor-only intermediate. Verify Undo/Redo when changing numerical Details and when dragging; the source asset is the actual authored data. Verify editor closing during source drag and rapidly switching the three modes. A dedicated selection history/refresher is a possible follow-up if the proxy's details become stale after Unreal's transaction undo.

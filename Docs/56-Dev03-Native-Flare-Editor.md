# Dev03 — native Slate visual flare editor

The `GlintEditor` module registers custom Flare asset actions. Double-clicking a `UGlintFlare` now opens `FGlintFlareEditorToolkit` with a native `SGlintFlareCanvas`, a selectable element list and standard `IDetailsView`. The preview source is an intentionally fixed UV location (0.74,0.39), with a center/optical-axis crosshair; it is **a schematic placement editor**, not GPU flare output.

Artist actions: click an element or list row to select; drag on canvas to modify normalized `ScreenOffset`; add seven element families using the + menu; duplicate, remove and move items. All mutating actions use `FScopedTransaction` with `Modify()` for Unreal Undo/Redo. Numeric properties, enable flags and color edits remain in Details.

Renderer contract: `FGlintRuntimeElement` is now three float4 vectors (48 bytes): ColorStrength, Geometry and Offset. `MakeRuntimeData()` clamps both offset components to [-1,1] and the compiled shader reads `element.Offset.xy`. Existing Flare assets default offset to zero.

Next UX work: selected-element Details instead of whole array, live RDG preview, asset thumbnail, curves/effect triggers, texture slots, viewport controls and persistent workspace layout.

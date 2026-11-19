# Glint 0.5.1 — Build Fix 01 (UE5.8.3 Drag-and-Drop Header)

## Scope

Only Glint source, the plugin descriptor, and documentation. No engine or other plugin files were modified.

## Reported failure

The user’s ProjectHeavyWaterEditor build stopped in `GlintFlareEditorToolkit.cpp:24` with `C1083: Cannot open include file: Framework/DragDropOperation.h`. The Glint runtime module had compiled and linked successfully in that build, but the editor toolkit compile could not continue past its include list.

## Fix

Use `#include "Input/DragAndDrop.h"` (declares `FDragDropOperation` in UE5.8 SlateCore). Retain `#include "DragAndDrop/DecoratedDragDropOp.h"` (defines Glint's `FDecoratedDragDropOp` base class, from UnrealEd). `GlintEditor.Build.cs` already lists `SlateCore` and `UnrealEd`, so this requires no module dependency change.

Epic API references:

- https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/SlateCore/FDragDropOperation
- https://dev.epicgames.com/documentation/unreal-engine/API/Editor/UnrealEd/FDecoratedDragDropOp?application_version=5.8

## Validation

- Glint shader parameter contract check: pass (6 compute entries, 55 bindings).
- Static patch check: the invalid header is absent; the valid header is present.
- Full ZIP integrity and plugin descriptor JSON: pass.
- **Unverified**: UE5.8.3 C++ rebuild, shader execution, and 22 registered automation tests (the previous build stopped early in the editor toolkit).

## Next validation

1. Replace the entire `Plugins/Glint/` folder with 0.5.1.
2. Rebuild ProjectHeavyWaterEditor. Ignore unrelated plugin diagnostics when assessing Glint, but the overall project build can still fail because of those unrelated errors.
3. If the editor opens, run `Automation RunTests Glint` and test the native outliner reorder interaction.
4. If another Glint error appears after the header correction, capture the full compiler diagnostic with file, line, and call context.

## Next work

Validate drag/drop outliner, three-mode preview, background Texture2D preview, optics defaults and shapes against the installed UE5.8.3 editor before extending authoring further.

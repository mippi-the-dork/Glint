# Glint Development Pass 05 — Per-Type Element Creation

## Why this changed

A newly created optical element must be recognizable immediately; requiring an artist to know the typical optical-axis position, intensity, size, and color of each element makes the editor difficult to learn.

## Implementation

`UGlintFlare::MakeDefaultElement(EGlintElementType)` is the single source of truth for defaults used by both the Content Browser Flare factory and the Add Element menu. A newly created **Glow** appears at the source; a **Ghost** appears across the optical center; a **Streak** appears on the source with anamorphic stretch; an **Iris**, **Ring**, **Hoop**, and **Multi Iris** occupy their own optical-axis positions. Starburst, Spike Ball and Sparkle start with different ray counts and sharpness. The three new element families **Shimmer, Sparkle, Caustic** have their own profiles.

Type changes in Selected Element Details apply new type defaults. The tool preserves a deliberate custom label but refreshes automatic labels. All authored values remain editable and saved; the factory only defines starting values.

## Per-type properties

Runtime packing now uses five `float4` fields (80 bytes): color/intensity, type/axis/size/rotation, offset, shape (falloff, ring thickness, aspect ratio, ray count), and detail (ray sharpness, secondary strength, detail frequency). Properties are shown conditionally where their underlying shape family uses them. These seven shape controls are consumed in the source-driven analytic GPU shader and editor preview. The legacy image-field fallback is still a simplified approximation and does not provide complete shape fidelity for the three newest families.

## Tests

`Glint.Data.Flare.TypeSpecificDefaults` verifies every family has useful defaults and tests key positioning differences. `Glint.Data.Flare.ShapeParameterPacking` checks the new GPU payload fields. `Glint.Data.Flare.ElementBudget` continues to guard the authored rendering cap.

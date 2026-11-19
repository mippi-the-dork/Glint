# Glint 0.7.1 — Automation Budget Fixture Correction

## Observed UE5.8.3 results

- User ran `Automation RunTests Glint`: 28 passed, `Glint.Data.Flare.ElementBudget` failed.
- The new `Glint.Data.Flare.LensBudget32` passed.
- Failure was from **undersized test input**, not demonstrated render truncation: `GlintAutomationTests.cpp` created 30 elements and required 32 from `MakeRuntimeData()`.

## Change

- Replace the literal 30-element fixture with `MaxElementsPerLens + 8` elements, forcing a real over-budget case.
- Assert exactly `MaxElementsPerLens` elements are packed, assert size sanitization, and assert the first and last in-budget elements survive.
- Assert the Flare retains every authored element without truncation.
- Update descriptor and README. Runtime packing, editor widgets and shaders are untouched.

## Visual observations (not addressed in this test-only patch)

The screenshot with all 26 Cinematic elements shows extreme accumulated intensity and stacks of repeated circular, aperture and streak shapes. The current GPU source picker samples tile-local maxima and applies only distance-based source suppression, not connected-region merging. Testing `Max Sources=1` with a single Ghost/Ring/Disc is an isolation step; genuine source grouping, perceptual source prioritization, energy distribution and more optical Cinematic presets are separate work.

## Verification

- Run `Automation RunTests Glint.Data.Flare.ElementBudget` then `Automation RunTests Glint`.
- Static checks and packaging are not equivalent to the native UE5.8.3 automation run.

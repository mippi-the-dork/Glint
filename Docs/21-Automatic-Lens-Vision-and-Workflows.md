# 21 — Automatic Camera Lens: Product Vision and Workflows

[← Index](README.md) · [22 — Detection](22-HDR-Highlight-Detection.md) · [23 — Lens Profile](23-Camera-Lens-Profile-and-UX.md) · [28 — Acceptance](28-Automatic-Mode-Testing-and-Release-Gates.md)

> **Decision (2026-11-12):** Automatic camera-driven flares are a **core v1.0 requirement**, not a post-v1 feature. This supersedes the deferral in earlier drafts of documents 02, 16, and 20. Manual/actor-attached sources remain supported, but are not required for a working camera lens.

## Product statement

**Glint defines how a camera lens responds to bright imagery.** Artists select a lens style and sensitivity; as the camera sees sufficiently bright scene content, the renderer automatically creates optical effects. Assigning a Glint flare to a placed light/Actor is **optional**, for deliberate hero control or cases the image-based system cannot infer.

A new project can use Glint without adding a special flare Actor to every scene. A camera's automatic lens should produce meaningful results for bright emissive signs, exposed lightbulbs, reflective metal, water sparkle, headlights and the visible sun. Actual results depend on whether the rendered scene has bright HDR pixels; a purely invisible light component with no visible source or specular response does not automatically produce a flare.

## Three complementary source paths

| Path | Input | Artistic use | Cost model |
|---|---|---|---|
| **Automatic — image field** | All sufficiently bright rendered pixels | Glow response, smear, anamorphic streaks, reverse/warped ghosts, camera-wide dirt | Few reduced-resolution image passes, largely independent of number of highlights |
| **Automatic — highlight candidates** | Bounded list of grouped high-brightness image regions | Full Glint composition with Iris, Multi Iris, Ring, radial elements, distinct ghost paths and individual Dynamic Triggers | Top-K active sources × chosen element detail, GPU-only records |
| **Authored** | Explicit registered light/world/directional/Blueprint source | Sun outside frame, reliable hero moment, specially designed stylization, source-local constraints | Existing per-view source registry and batching |

A fourth **Hybrid** selection is a workflow preset combining automatic field/candidates plus authored sources; it is not a separate renderer.

## Why two automatic techniques?

A simple image-space pass can spread/reproject highlights efficiently across the image, but has little concept of a stable source identity or an individually editable procedural iris. Source extraction can drive Optical Flares-like element stacks, but naively turning thousands of hot pixels into thousands of complete flares is too expensive and aesthetically chaotic. Glint should use both paths, with lens-profile budgets and a shared source-to-render contract.

**Scope statement:** v1.0 must provide credible automatic streak/ghost behavior and bounded automatically driven flare compositions. Do not claim arbitrary numbers of independently rendered complex ghosts. The final candidate extraction/quality-tier algorithm is a technical gate, not a proven implementation.

## The new first-run experience

1. Create **Glint Lens** in the Content Browser (dedicated asset, not generic Data Asset).
2. Choose a starter lens (Natural 35 mm, Vintage, Anamorphic, Clean Digital, or Stylized); all presets are original.
3. In **Project Settings > Glint**, assign the default lens profile; automatic mode begins in eligible views without source setup. Alternatively, add `Glint Lens` to a Camera/CineCamera for an override.
4. Move around the level viewport: headlights, reflections and emissive surfaces generate flares when bright enough.
5. Open the lens asset editor and drag **Sensitivity**, **Strength**, **Max Prominent Flares**, **Ghost Style**, and **Streaks** while the scene preview updates.
6. Toggle **Highlight Candidates** visualization; red/gray markers explain rejected regions, ranked candidates, simplifications and suppression near authored sources.
7. For a critical scripted source, optionally create a **Glint Flare** asset and register it via component or Blueprint. Automatic and authored visuals cooperate.

## Lens and flare assets must have separate meanings

**Glint Lens** (`UGlintLensProfile` proposed): camera behavior and automatic detection. It holds lens optics, global glow/streak/ghost recipes, sensitivity/exposure behavior, budget, camera dirt and the mapping from detected sources to flare compositions. It may reference one or more Glint Flare templates.

**Glint Flare** (`UGlintAsset`, existing): source-relative authored composition using the Optical Flares-style element stack. It is the thing applied to a hero source and can be selected as the look for automatic candidates. Flares can inherit from the active lens profile or intentionally override it.

Both should appear as native asset types under a sensible Glint Content Browser grouping. Do not confuse users with two assets both called only “Glint”; retain plugin name **Glint** while displaying clear **Glint Lens** and **Glint Flare** asset names.

## Artist experience: the lens is the camera, not the world

- Lens dirt and surface scratches stay in **lens/camera space**; ghosts and glints move according to detected source positions; streak orientation follows the lens optical profile.
- The default lens changes with camera cuts or blended location overrides, not simply with the world light a source happens to be attached to.
- A flare's apparent optical behavior can depend on CineCamera focal length, aperture blade count, squeeze factor and filmback, with user-defined links and overrides.
- An authored source and an automatic source should look like they come from the **same lens** by default. Overrides remain possible.
- Users can disable Glint for a camera, gameplay view, editor preview or capture without deleting assets.

## The expected use cases

| Scenario | Expected automatic result | When authored is useful |
|---|---|---|
| Sun through moving tree canopy | Visible disc creates a prominent flare; leaf occlusion shapes the source without per-leaf raycasts | Source persists slightly past screen border; specific sky/sun direction |
| Night street with dozens of lights | Broad streak response from all prominent pixels; only most intense highlights receive complex iris/ghost arrays | One dramatic headlight on a cutscene vehicle |
| Emissive sci-fi panels | Flares from genuinely bright emitted pixels, regardless of Light Components | Force an effect even if emissive is deliberately dim |
| Metallic/specular glints | Small, short-lived highlights, with reduced visual complexity and anti-flicker logic | Designer wants specific reward sparkle |
| Water reflection | Many small highlights contribute to an aggregate flare/streak field; no mass spawning of full complex compositions | One cinematic glint following a scripted track |
| A luminous sky or white wall | Size/contrast controls prevent the entire screen from becoming an intense flare | Explicit art direction to produce a washout |
| Player flash grenade | Screen-derived brightness responds naturally; gameplay-controlled intensity safety clamp | Exact timed lens flare animation and camera exposure override |

## Important negative guarantees

- **Bright pixels, not every Unreal Light Component:** image-derived flares occur only for bright rendered regions, including specular and emissive, not unseen invisible lights.
- **No automatic offscreen knowledge:** image-based detection cannot know an offscreen sun/source; authored directional sources can provide optional edge response.
- **No magic isolation of UI:** process an HDR scene signal *before* game UI and Glint's own compositing; prove the exact extraction location.
- **No unlimited complex instances:** budgets are mandatory; lower-ranked highlights contribute cheap field effects or no effect.
- **No per-frame GPU-to-CPU readback:** GPU detection and candidate processing remain on the GPU.

## Research basis

- [Epic UE5.8 image-based lens flares](https://dev.epicgames.com/documentation/unreal-engine/post-process-effects-in-unreal-engine)
- [Video Copilot Optical Flares (automatic luminance tracking and threshold/resolution)](https://www.videocopilot.net/products/opticalflares/)
- [Unity HDRP screen-space lens flare](https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4017.0/manual/shared/lens-flare/Override-Screen-Space-Lens-Flare.html)
- [Flares OFX pixel-driven flare positioning](https://palmpixel.eu/flaresofx-documentation/openfx-integration/plugin-features/pixel-driven-flares/)

These are feature/workflow references. Glint's algorithm, thresholds, defaults, budget and native integration are **proposals to validate** in UE5.8.

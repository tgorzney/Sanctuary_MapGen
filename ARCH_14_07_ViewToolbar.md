[← ARCH index](ARCH.md) · [§14 ARCH_14_PreviewOverlayLayering](ARCH_14_PreviewOverlayLayering.md) · SanGen ARCH §14.7. **Only the ARCH Expert writes this file.**

### 14.7 View toolbar — replaces "Regenerate," one popup / two non-crossing sections
"View" opens a click-to-open popup (not hover — hover-close would fight a drag-reorder gesture),
`ImGui::BeginPopup("ViewLayersPopup")` rendering two independent `DraggableList` calls (the same
widget `LayersTab` already uses for GeoLayers) separated by a static section label:
- **"Terrain (composited)"** — `PreviewCompositeSettings::fieldLayers`.
- **"Overlays (screen-space)"** — `overlayLayers`.

Terrain rows carry their own blend-mode `Combo_UI` (`PreviewCompositeSettings::fieldLayers`'s
real GPU blend-equation switch into the composite shader — unchanged by this ruling). Overlay
rows carry an **opacity slider** instead (§14.2, §14.13 item 5, closed) — there is no
per-overlay-layer blend-equation switch; every overlay layer shares ImGui's one global blend
equation. Reorder is real *within* each section; **a row cannot cross sections** — true
interleaving (a marker rendering "under" a terrain layer) is rejected outright: it is not
renderable without either re-baking markers into the texture (the exact bug this whole redesign
kills) or rebuilding `PreviewComposite` into an interleaved multi-target compositor, and a control
that *looks* interleaved but isn't would violate the WYSIWYG law by showing an order that is not
the real render order. Mechanism: the two `DraggableList` renders use different drag-payload
identifiers so cross-section drops structurally fail to match — no new validation code needed. No
new widget; straight reuse.

**"Regenerate" is retired from the primary toolbar.** `Pipeline::PreviewDriver` already
auto-derives refresh tier from parameter hashes (`NotifyParametersChanged()`); a manual full-regen
button is the exact anti-pattern that system exists to replace. `MapCanvas::
RequestRegeneration()` and `PreviewDriver::RequestMapUpdate()` are currently **two rival trigger
paths** — per hit-list #3's "retire rival toggles" (applied here to a UI-level trigger, not a
compute backend), these **must collapse to one call path.** Keep exactly one debug/System-panel
affordance calling `RequestMapUpdate()` directly, for the one legitimate manual case
`PreviewDriver`'s own docstring already names ("a change no parameter hash can see: a resize, a
recipe reload, new stratum art") — not on the View toolbar.

⚠️ **R2 self-inconsistency found in the source document, flagged rather than silently resolved.**
R2's own "ARCH rulings (this round)" item 4 reads the fieldLayers/overlayLayers unification
question as still-open ("not ratifiable as scoped... route back to UI Expert"). A later section of
the *same document* ("View toolbar") states the UI Expert's dedicated pass already ran and records
the two-section/no-crossing design as "**Confirmed by human**." R2's own "Consolidated ❓ open
items" list (item 1) was never updated to drop this after that later resolution — it still reads
"sent back to UI Expert for a dedicated pass (in progress)." This ARCH ruling treats the later,
explicitly human-confirmed text as authoritative (the strongest ratification signal anywhere in
the document) and treats the two-section/no-crossing design above as **closed law**, not open —
but the inconsistency itself is recorded here rather than quietly picked one way.


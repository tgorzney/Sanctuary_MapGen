[← ARCH index](ARCH.md) · [§18 ARCH_18_SantpFootprintIngestion](ARCH_18_SantpFootprintIngestion.md) · SanGen ARCH §18.2. **Only the ARCH Expert writes this file.**

### 18.2 Determinism ruling — ingested footprint data may drive generation only via a `PARAMS` bake, never a live read

**Overrides a prior session's "Visual-class, display-only" framing.** The human has confirmed
directly that footprint data MUST be able to influence generation — the concrete case is reading a
prop's or unit's real footprint so scatter can place adjacent instances without overlap. A ruling
that ingested data can never reach a PROC stage is rejected outright. This reopens the determinism
question `DESIGN_SantpFootprintIngestion_R1.md` §6.2/Q8 raised and left for ARCH: Constitution §4
requires a map be fully reproducible from recipe+seed alone; two users on different game installs
(different patch, different branch, modded) must not silently produce different maps from an
identical `.sanmap`/recipe.

**Evaluated, not rubber-stamped: the design's own starting hypothesis said "baked into the map's own
recipe/DATA." That phrasing conflates two Constitution §1 layers that must not be conflated here —
the ruling below names exactly one of them.** `DATA` is defined as the *computed output* of
generation, regenerated from `PARAMS`+seed every run, with every field owned by exactly one writing
PROC stage that is itself a pure, re-runnable function of its declared inputs. A footprint value is
not computed by any PROC stage — it is authored/ingested input. Writing it into `DATA` would either
(a) require some PROC stage to "write" a value it does not compute, violating the one-writer/pure-
function invariant, or (b) require `DATA` to persist across regenerations independent of `PARAMS`,
which breaks "DATA is regenerated from PARAMS" outright. **The correct target is `PARAMS`, not
`DATA`.** `PARAMS` is exactly Constitution §1's "adjustable settings (the recipe) … what the
`.sanmap`'s SanGen-owned schema v3 sections serialize" — the layer Constitution §4's "settings+seed"
phrase refers to. Once a footprint value is a `PARAMS` field, it is transmitted with the recipe,
serialized in the `.sanmap`, and reproducible on any machine with no game install at all — which is
precisely the property needed.

**The mechanism, ruled:**

1. **Resolution is a discrete, human-triggered authoring action — never an implicit step inside
   generation itself.** A designer (or an explicit "resolve footprints" action in the ingestion UI,
   ticket 91's territory) looks up a template's real footprint from the IO-layer ingestion result
   (`Io::WorldFootprintSizeTable`/`TemplateIngest_IO`, still `Visual`-class, still IO-only, still
   fully optional and fully degradable per §4.4 of the design) and copies the resolved scalar(s)
   into the relevant `PARAMS` type as an ordinary authored field. This is a one-shot bake, not a
   live binding. **Generation itself (the PROC pipeline, triggered by Generate or by any dirty-hash
   recompute) must never re-query the ingestion result or the game install.** If it did, two runs on
   the *same* machine at different times — after a game patch changed a template's footprint between
   them — would silently diverge, which is the identical failure mode one keystroke away, just
   collapsed onto a single machine instead of two.
2. **PROC/scatter code reads only the baked `PARAMS` field, never `Io::WorldFootprintSizeTable`, and
   never any type owned by the ingestion pipeline.** This is what keeps the Exact/Deterministic
   chain (Constitution §4, ARCH §4.6) closed over pure recipe data — the baked number is, from
   PROC's point of view, indistinguishable from any other designer-typed spacing constant. The exact
   accuracy-class tag on the scatter-spacing calculation itself (`Exact` vs `Accurate`) is a
   Generator/Compute Optimization Expert call, out of scope here; what this ruling fixes is that
   whichever class it gets, its *input* is deterministic by construction.
3. **The baked field remains an ordinary, hand-editable `PARAMS` value after baking — never a
   read-only mirror of the ingested source.** Constitution §8's total-tweakability law applies
   unconditionally: a designer must be able to override a baked footprint just as freely as any
   other authored constant, including on a machine with no game install and no memory of where the
   original value came from. The bake is a convenience that fills in a starting value; it creates no
   ongoing dependency and no provenance obligation.
4. **An un-baked field is not an error state.** A `PARAMS` field a designer never resolved against a
   real install keeps whatever default it already has today (`STEP58`'s `kDefault*FootprintSize`
   class of constants) — an ordinary tunable, not a "missing data" condition. Ingestion never
   becomes mandatory for generation to proceed, on any machine, ever (`DESIGN_SantpFootprintIngestion_R1.md`
   §6.1's "SanGen must remain fully usable with no game install" stands unmodified by this ruling).
5. **The IO-layer ingestion pipeline's own accuracy-class tag does not change.** `Io::WorldFootprintSizeTable`
   stays `Visual`-class exactly as `STEP58_WorldFootprintSizeTable_IO.md` ships it, because its only
   *direct* consumer remains the preview's icon-LOD sizing. What changes is that a **second**,
   human-mediated path now also exists — copy a resolved value into `PARAMS` — and that second path
   is what is permitted to eventually influence generation, precisely because crossing it converts
   the value from "live external lookup" into "recipe data."

**Consequence for the proposed ticket sequence.** Tickets 85–88 (`LuaTableEvaluate_SYS`, the source
scanner, the dialect parser, the disk cache) are unaffected by this ruling — none of them touches a
PROC consumer, and §18.1 alone governs their legality. **Ticket 89** (the ingestion orchestrator)
must not be dispatched with a design that wires `Io::WorldFootprintSizeTable`/`TemplateIngest_IO`
directly into any `PROC`/scatter code path — this ruling is binding on that ticket's shape. A future
ticket that adds the actual "bake into `PARAMS`" authoring action (the `PARAMS` field, its wire
mapping, and the UI trigger) is new work this ruling authorizes but does not itself specify — the
Format Expert and Generator Expert's call, cross-referencing this section and
`DESIGN_SantpFootprintIngestion_R1.md`.

# Work-Order — Step 11: fix `stratumLayers` appearance export and build its missing importer

*Constitution §7. Executor: SanGen Coder. Implements `SANMAP_FORMAT_SPEC.md` Correction 13
verbatim — a bug-fix/completion of already-existing IO wiring, not a new schema section (every
field maps 1:1 onto an already-format-native `stratumLayers[9]` key, confirmed field-for-field
against `SanMap.Types.cs::Stratum`).*

## Root problem
`BuildStratumLayersJson` (`MapExporter_Recipe_IO.cpp`, confirmed by direct read) writes
`layer["albedo"/"normal"/"mask"]["path"]` as hardcoded empty strings for every stratum, on every
export, unconditionally — never the real `stratum.appearance.albedoTexturePath`/etc. `tileSizeFar`
reuses `stratum.tileCount` (the NEAR tile size) instead of `appearance.farTileCount` — a bug, not a
placeholder. Six more appearance fields (`tileSizeTriplanar`, `tileSizeFarTriplanar`,
`normalScale`, `normalScaleFar`, `normalFarNearBlend`, `heightFarNearBlend`) are never written at
all. `farColorRemap` is never written. **No importer for `stratumLayers` exists anywhere** — grep
confirms zero matches for `stratumLayers` in `MapImporter_Recipe_IO.cpp` or anywhere in `src/io/`.
Net effect: a stratum's appearance never round-trips through a `.sanmap` today, export or import.

## Target files
- `src/io/MapExporter_Recipe_IO.cpp` — `BuildStratumLayersJson`: fix all the writes below.
- `src/io/MapImporter_Recipe_IO.h`/`.cpp` — new `ReadStratumLayersJson` (no importer exists to
  extend; this is a wholly new reader, the mirror of the fixed builder).
- `src/io/MapImporter_IO.cpp` — **RESOLVED (Format Expert confirmed):** `stratumLayers` is a
  top-level sibling key, exactly like `areas`/`armies`/`atmosphere`/`SlopeDefaults`. Wire
  `ReadStratumLayersJson(document, outRecipe)` into `ParseSanmapJsonText` unconditionally, in the
  same top-level block those readers already run in (before the `mapGeneratorData` gate) — do NOT
  put it alongside `ReadStrataSettingsJson`, which correctly stays inside the gated
  `generatorData`-derived section for the unrelated `mapGeneratorData.Stratums` blob.
- `src/params/StratumAppearance_PARAMS.h` — **delete `diffuseRemapColor`** (dead, self-
  contradictory field per the spec: the file's own header comment already says the preview base
  color is `tintRed/Green/Blue`, not this field, and the (unchanged, already-correct) export
  mapping bears that out — `diffuseRemap` writes from `tintRGB`, never from `diffuseRemapColor`,
  so this field would round-trip nothing even after this ticket lands. Keeping it would create a
  second, competing color source for the same shader key. `farColorRemapColor` is NOT this defect
  — keep it, it has no competing scalar field and is the correct sole consumer of `farColorRemap`).
- `src/ui/StratumsTab_Appearance_UI.cpp` (no separate `.h` — confirmed, this translation unit
  includes `StratumsTab_Draw_UI.h` instead) — delete the "Diffuse Remap" swatch row
  (`DrawRemapColor("Diffuse Remap", appearance.diffuseRemapColor, ...)`, confirmed at
  `StratumsTab_Appearance_UI.cpp:65-66`). Do not touch `DrawPreviewBaseColor`/`tintRed/Green/Blue`'s
  own swatch — that one is correct and stays.
- `src/ui/StratumsTab_UI.h:60` — delete `RealtimeToggle diffuseRemapColorToggle;` from
  `StratumRowState` (the toggle paired with the swatch above — confirmed this is where it actually
  lives, NOT in `StratumsTab_Appearance_UI.cpp`).
- `src/params/Stratum_PARAMS_Test.cpp:57` — delete the `Check(appearance.diffuseRemapColor[channel]
  == 1.0f, ...)` default-value assertion (confirmed the only other live reference to the field;
  deleting the field without this leaves a compile error).

**This UI/test deletion is the one UI touch this ticket's ratifying spec explicitly calls for** —
unlike every other ticket this session, which excluded UI wiring; here, deleting a genuinely dead
control IS the correction, not scope creep.

## Layer & accuracy class
IO/BRIDGE + one narrow, spec-mandated UI deletion. Accuracy class: Exact.

## Backend policy
CPU only.

## ARCH rules invoked
- `SANMAP_FORMAT_SPEC.md` Correction 13 — binding, implement verbatim.
- Constitution §6 — cardinality mismatch handling (see below): loud logged warning, never silent
  truncation, never a hard refusal (this is recipe content, not a version gate).

## Solution — export fixes (`BuildStratumLayersJson`)
```
layer["albedo"]["path"]       <- stratum.appearance.albedoTexturePath      (currently always "")
layer["normal"]["path"]       <- stratum.appearance.normalTexturePath     (currently always "")
layer["mask"]["path"]         <- stratum.appearance.compositeTexturePath  (currently always "")
layer["tileSize"]             = { x: stratum.tileCount, y: stratum.tileCount }         (unchanged, correct)
layer["tileSizeFar"]          <- appearance.farTileCount                  (currently reuses tileCount — BUG)
layer["tileSizeTriplanar"]    <- appearance.triplanarTileCount            (currently never written)
layer["tileSizeFarTriplanar"] <- appearance.farTriplanarTileCount         (currently never written)
layer["normalScale"]          <- appearance.normalScale                   (currently never written)
layer["normalScaleFar"]       <- appearance.farNormalScale                (currently never written)
layer["normalFarNearBlend"]   <- appearance.normalFarNearBlend            (currently never written)
layer["heightFarNearBlend"]   <- appearance.heightFarNearBlend            (currently never written)
layer["diffuseRemap"]         = { r: tintRed, g: tintGreen, b: tintBlue, a: 1.0 }  (unchanged — correct;
                                                                             NOT appearance.diffuseRemapColor,
                                                                             which is being deleted, see above)
layer["farColorRemap"]        <- appearance.farColorRemapColor[4]         (currently never written)
layer["maskRemapMin"/"Max"]   — already correct, unchanged (ARCH §7.2 item 10, already shipped)
```

## Solution — the new importer (`ReadStratumLayersJson`)
Per index `0..8` of `document["stratumLayers"]`:
```
Params::Stratum::appearance.albedoTexturePath     <- layer["albedo"]["path"]
Params::Stratum::appearance.normalTexturePath     <- layer["normal"]["path"]
Params::Stratum::appearance.compositeTexturePath  <- layer["mask"]["path"]
Params::Stratum::tileCount                        <- layer["tileSize"]["x"]   (y ignored — Params::Stratum
                                                                                 has no anisotropic tile field)
Params::Stratum::appearance.farTileCount          <- layer["tileSizeFar"]["x"]
Params::Stratum::appearance.triplanarTileCount    <- layer["tileSizeTriplanar"]
Params::Stratum::appearance.farTriplanarTileCount <- layer["tileSizeFarTriplanar"]
Params::Stratum::appearance.normalScale           <- layer["normalScale"]
Params::Stratum::appearance.farNormalScale        <- layer["normalScaleFar"]
Params::Stratum::appearance.normalFarNearBlend    <- layer["normalFarNearBlend"]
Params::Stratum::appearance.heightFarNearBlend    <- layer["heightFarNearBlend"]
Params::Stratum::tintRed/tintGreen/tintBlue       <- layer["diffuseRemap"]["r"/"g"/"b"] (alpha dropped —
                                                                                            Stratum has no
                                                                                            tint-alpha field)
Params::Stratum::appearance.farColorRemapColor[4] <- layer["farColorRemap"]["r"/"g"/"b"/"a"]
Params::Stratum::maskRemapMinimum/Maximum[4]      <- layer["maskRemapMin"/"Max"]["x"/"y"/"z"/"w"]
```
Use the existing typed `JsonPrimitives_IO.h` accessors throughout (total/degrade-gracefully per
Constitution §6 — a missing/wrong-typed sub-key leaves that one field on its default, never aborts
the whole layer). `stratumLayers[9]` is a fixed-size format invariant (`STRATUM_COUNT = 9`,
confirmed); a document with a different array length is a **loud, logged warning**, not a silent
truncation and not a hard refusal — same cardinality-mismatch posture already established for
`StratumGenerationSettings`-adjacent corrections.

## Explicit out-of-scope
- **`layer["name"]`** currently writes a generated placeholder (`"Stratum " + index`) instead of
  `stratum.appearance.name`. Real, but explicitly NOT part of this correction per the ratified
  spec text ("flagged for a future pass, not fixed here") — do not fix it opportunistically.
- **`importedMaskMode`/`bEnabled`** (`Params::Stratum`) — flagged in the spec as having no
  ratified format home once the doomed `mapGeneratorData.Stratums` blob is eventually deleted; an
  open follow-up, explicitly not this correction's job. Do not invent a home for them here.
- **`StratumGenerationSettings`** (Correction 12 — soil physics + the `SlopeDefaults` mechanism's
  IO shape) — separate ticket, `SlopeDefaults`' PARAMS-side mechanism already shipped (Step 10);
  this ticket does not touch that.
- **`mapGeneratorData.Stratums` deletion itself** — that blob still exists and is still written/
  read elsewhere (`BuildStrataSettingsJson`/`ReadStrataSettingsJson` in the Layers IO files); this
  ticket does not delete it, only fixes the separate, already-live `stratumLayers` section.

## Acceptance test
Extend the round-trip fixture (`MapImporter_IO_Test.cpp`) with a `Params::Stratum` carrying
non-default values across every `StratumAppearance` field (real texture paths, distinct far/
triplanar tile counts, non-default normal/height blend values, a non-default `farColorRemapColor`)
and non-default `tintRed/Green/Blue`, asserting exact survival through export→import — this is a
genuinely new check, since no prior test exercised `stratumLayers` round-tripping at all. Assert a
`stratumLayers` array of the wrong length produces a logged warning, not a crash or silent
truncation. Confirm `diffuseRemapColor` no longer exists anywhere in `src/` (grep) — this requires
touching `Stratum_PARAMS_Test.cpp:57` and `StratumsTab_UI.h:60` per "Target files" above, not just
the field definition and its swatch. Full `SanGenV2` build stays clean.

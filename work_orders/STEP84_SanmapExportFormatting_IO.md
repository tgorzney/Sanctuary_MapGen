# STEP84 — `.sanmap` exporter OUTPUT FORMATTING: the ratified emission contract

**Layer:** IO. **Domain:** `.sanmap` serialization *presentation* — key order, indentation,
float digit strings, line endings, escaping, trailing bytes. **Not** document *content*
(no key is added, removed, renamed, or re-valued by this ticket).
**Accuracy class:** N/A (byte-level text emission; no computation).
**Backend policy:** N/A — pure CPU string assembly, once per export. Does not touch
`Dispatch_SYS`, no SIMD, no GPU handle.

**Depends on:** nothing. **Blocks:** nothing. Safe to schedule at any point.

---

## Root problem

Nothing in SanGen specifies what the `.sanmap` exporter's *output text* must look like.
`MapExporter::BuildSanmapJsonText` ends in a bare `document.dump(indent)`
(`src/io/MapExporter_Recipe_IO.cpp:33`) and the only knob anywhere in the tree is
`MapExportOptions::jsonIndentSpaceCount = 4` (`src/io/MapExporter_IO.h:54`). Everything
else — key order, float digit strings, line endings, unicode escaping, trailing bytes —
is whatever nlohmann happens to do by default. Nobody has ever written down what it
*should* be, so nobody can tell a regression from a preference.

**The human's ruling:** SanGen's exporter must produce correct, readable output matching
the formatting convention of real shipped game maps, and the untouched
`Pandemonium Isthmus.sanmap` is the reference SanGen should eventually match.

### The incident that surfaced this — and its correction

A prior session round-tripped the live map through Python `json.load` /
`json.dump(indent=4)` during a test. It was reported as having changed "key order,
whitespace and float style." **That report is wrong, and the correction matters more than
the original claim.** Measured this session against the restored original and the
preserved counter-example:

```
937326  Pandemonium Isthmus.sanmap                                    (restored original)
964969  Pandemonium Isthmus.sanmap.backup-2026-08-21_python-reformatted
        delta = 27643 bytes ... = exactly the file's 27643 lines

md5 (original, raw)            cd587b33a93c95a5da3eb8ccb8d754ea
md5 (original, CR stripped)    cd587b33a93c95a5da3eb8ccb8d754ea
md5 (reformatted, CR stripped) cd587b33a93c95a5da3eb8ccb8d754ea
diff <(tr -d '\r' <orig) <(tr -d '\r' <reformatted)  ->  EMPTY
```

The two files are **byte-identical except that every `\n` became `\r\n`.** One extra byte
per line, 27643 lines, 27643 bytes. Python's `json` module reproduced this map's key
order, indentation, spacing, integer/float discrimination, and every one of its ~24,000
float digit strings *exactly*; the only damage was Windows text-mode newline translation
in the `open(..., 'w')` call.

Two consequences, both load-bearing for this ticket:
1. **The reference's formatting is fully characterized and mechanically reproducible** —
   §1 below states the recipe. This is not guesswork.
2. **Line-ending discipline is the single highest-value guarantee here**, because it is
   the one that actually broke a real file, and it is invisible in every diff tool that
   normalizes newlines.

> **⚠️ Standing constraint, restated for the Coder.** No `.sanmap` file on this machine,
> and nothing under `E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\`, may
> be written, moved, deleted, or "normalized" by any part of this work. Those files are
> READ-ONLY inputs. This ticket's test fixture is a **new, small, checked-in file under
> the repo** (§7) — never the live map, never a copy written back over one.

---

## 1. The reference formatting, characterized empirically

All measurements taken this session against
`E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap`
(937,326 bytes, 27,643 lines), read-only.

| # | Property | Measured value |
|---|---|---|
| F1 | Indentation | **4 spaces per level.** Zero tab bytes in the file. Leading-space histogram is exactly `{0, 4, 8, 12, 16, 20, 24}` — no odd widths, max depth 6. |
| F2 | Key/value separator | **`": "`** — one space after the colon, always. Zero occurrences of `":` followed by a non-space. |
| F3 | Line endings | **LF only.** Zero `\r` bytes. |
| F4 | Trailing newline | **None.** File's last three bytes are `{ }` `\n` `}` — it ends on `}` with no terminator. |
| F5 | Trailing whitespace | **None.** Zero lines match ` +$`. |
| F6 | Arrays / objects | **One element per line, always.** Zero inline non-empty containers. Empty containers are `[]` / `{}` with no interior newline (1 `[]`, 17 `{}`). |
| F7 | Integral floats | Printed **with `.0`** — `"sunRA": 87.0`, `"fadeDistance": 128.0`, `"waterWindSpeed": 0.0`. |
| F8 | Genuine integers | Printed **bare** — `"width": 2048`, `"waterLevel": 78`, `"faction": 0`, `"heightmapResolution": 2049`. The distinction is real and carried through the file. |
| F9 | Exponent form | Used, lowercase `e`, explicit sign, ≥2 exponent digits: `"y": 4.8791442e-08`, `"z": -6.087084e-09`. 36 occurrences, all inside `props`. |
| F10 | Encoding / escaping | **Pure ASCII** (zero bytes ≥ 0x80). Zero backslash escapes anywhere. Forward slashes in strings are **unescaped** (`"Environment/Pandemonium/Props/…"`, 44 occurrences) — i.e. not `\/`. |
| F11 | Top-level key order | **Authoring order, with human-readable divider pseudo-keys.** See §1.1. |
| F12 | Float digit strings | **Mixed — two incompatible producers.** See §1.2. |

### 1.1 Key order (F11) — the divider keys are the tell

The document's top level is not alphabetical and not schema-derivable. It is one tool's
insertion order, punctuated by eight comment-shaped pseudo-keys that carry no data:

```
line 2:     "-- Map Global": "Settings",
line 12:    "-- Water": "Settings",
line 24:    "-- Height Blend": "Settings",
line 28:    "-- Lighting": "Settings",
line 64:    "-- Environment": "Settings",
line 108:   "-- Stratum": "Settings",
line 6215:  "-- Army": "Settings",
line 6314:  "-- Prop": "Settings",
```

These exist **only** to group the keys that follow them. Their entire meaning is
positional. Any writer that reorders top-level keys destroys them semantically while
leaving them syntactically valid — the worst possible failure mode, because the file still
loads.

Nested order is inconsistent by container kind, which is itself informative:
- **Dictionary-valued** containers are sorted lexicographically: `areas.PlayableArea`
  emits `height, width, x, y`; each `armies.ARMY_NN` emits `alloys, energy, faction,
  groups`; `markers.Alloys` emits `resource, transforms`. Consistent with a
  `std::map`/`SortedDictionary`-backed writer.
- **Struct-valued** containers are declaration order: `sunTint` emits `r, g, b, a` — not
  alphabetical.
- **And the same logical struct is emitted in two different orders in the same file.**
  Census of the key immediately following each `"rotation": {`:
  ```
  304  "w"      (markers block — w, x, y, z)
  1180 "x"      (props block   — x, y, z, w)
  ```
  This is decisive: **the reference file was written by at least two different producers**
  and is internally inconsistent. It is not the output of one rule.

### 1.2 Float digit strings (F12) — three distinct number populations

Fractional-digit histograms split cleanly by region:

```
markers (lines 731-6214):   1 digit x2731,  6 x12, 10 x3, 13 x27, 14 x262, 15 x4   -- bimodal
props   (lines 6315-27641): 1..20 digits, broad, peaks at 5, 7, 9 and again at 13, 16, 17
e-notation:                 0 occurrences before line 6315;  36 after
```

Testing representative literals (shortest-round-trip double of the parsed value vs.
shortest-round-trip double of the same value narrowed to `float` and re-widened):

| Literal in the reference | Where | `float`-narrowed and re-widened prints as | Same? |
|---|---|---|---|
| `30.615638732910156` | `waterDepth` | `30.615638732910156` | **yes** |
| `78.72360229492188` | marker `position.y` | `78.72360229492188` | **yes** |
| `44.5151672` | prop `position.y` | `44.515167236328125` | **no** |
| `1053.07251` | prop `position.x` | `1053.072509765625` | **no** |
| `0.387778044` | prop `scale.x` | `0.38777804374694824` | **no** |
| `4.8791442e-08` | prop `rotation.y` | `4.8791441997764196e-08` | **no** |
| `1103.367933379766` | prop `position.x` | `1103.367919921875` | **no** |

So the file contains three populations:
- **(P1)** double-shortest of a widened float32 — `30.615638732910156`. A `float` value
  held in a `double` and printed shortest.
- **(P2)** float32-shortest — `44.5151672`, `0.387778044`, `4.8791442e-08`. A `float`
  printed by a *float-aware* shortest algorithm (.NET `float.ToString("R")` shape).
- **(P3)** genuine doubles that are not float32 at all — `1103.367933379766` does not
  survive a round trip through `float`.

**The unifying property that made the Python round trip byte-exact:** every one of these
literals is already *shortest-round-trip for the double it parses to*. Parse-to-double
then print-shortest-double is the identity on all three populations. Narrow to `float`
anywhere in the middle and P2/P3 break.

---

## 2. What SanGen emits today

Verified by reading the actual write path end to end.

| Fact | Evidence |
|---|---|
| JSON library is **nlohmann/json v3.11.3**, pulled by `FetchContent`, not vendored into the tree | `CMakeLists.txt:30-34` (`URL …/releases/download/v3.11.3/json.tar.xz`) |
| Document type is **`nlohmann::ordered_json`** — insertion-ordered, *not* the `std::map`-backed default `nlohmann::json` | `src/io/MapExporter_Recipe_IO.cpp:22` |
| Serialization is **`document.dump(indent)`** with `indent = 4` | `src/io/MapExporter_Recipe_IO.cpp:32-33`; `src/io/MapExporter_IO.h:54` |
| `dump()`'s remaining three parameters are **left at their defaults**: `indent_char = ' '`, `ensure_ascii = false`, `error_handler = strict` | nlohmann `json.hpp:20574-20577` |
| Bytes reach disk through **`std::ios::binary`** — no CRLF translation | `src/io/FilesystemPrimitives_IO.cpp:33`, called from `src/io/MapExporter_IO.cpp:23` |
| Floats go through **Grisu2** (`to_chars`), round-trip-guaranteed | `json.hpp:18816-18825` |
| Grisu2 appends **`.0`** to integral values | `json.hpp:17906-17909` (`// Make it look like a floating-point number`) |
| Exponent is **lowercase `e`, explicit sign, ≥2 digits** | `json.hpp:17953` + `append_exponent`, `json.hpp:17835-17874` |
| Exponent thresholds are **`kMinExp = -4`, `kMaxExp = digits10` (15 for double)** | `json.hpp:18013-18021` |
| Every `.sanmap` float field is read into a C++ **`float`** | `src/io/JsonPrimitives_IO.h:24-28` — `destination = parent[key].get<float>();` |
| Unrecognized top-level keys are bagged in a **plain `nlohmann::json`** (i.e. `std::map` — **alphabetically sorted**) | `src/io/UnknownImportBag_IO.h:25` |
| …and re-emitted **last, nested under one `UnknownImport` key** | `src/io/MapExporter_UnknownImportMerge_IO.h:22-27`; called at `src/io/MapExporter_Recipe_IO.cpp:30` |
| SanGen's own top-level key order is fixed by four sequenced helpers | `src/io/MapExporter_DocumentAssembly_IO.cpp:16-98` |

---

## 3. The concrete delta

**Already correct, for free, today — nine of the twelve properties:**

| Property | Why it already matches |
|---|---|
| F1 4-space indent | `dump(4)` + default `indent_char = ' '` |
| F2 `": "` separator | nlohmann's pretty-print mode |
| F3 LF endings | nlohmann emits `\n`; `std::ios::binary` write does not translate it |
| F4 no trailing newline | `dump()` appends none |
| F5 no trailing whitespace | nlohmann never emits any |
| F6 one element per line, `[]`/`{}` for empties | nlohmann's pretty-print mode |
| F7 `.0` on integral floats | `json.hpp:17906-17909` |
| F9 `e-08` exponent shape | `append_exponent`, byte-for-byte the same shape |
| F10 unescaped `/` | nlohmann does not escape forward slash |

These are **unspecified and therefore unprotected**, not wrong. F3 in particular is
currently correct by a single un-commented `std::ios::binary` flag that any future
refactor could drop, reproducing the exact incident that opened this ticket.

**Genuinely different — five items:**

- **D1 — Top-level key order does not match, structurally.** SanGen emits its own fixed
  order (`fileVersion, mapVersion, SanGenVersion, name, credits, width, length, height,
  heightmapResolution, hasWater, waterLevel, waterDepth, deepWaterDepthMin, shader,
  heightTransition, fadeDistance, fadeStartDistance, stratumLayers, …`,
  `MapExporter_DocumentAssembly_IO.cpp:16-98`). The reference's order is different, and
  interleaves the eight `"-- X": "Settings"` dividers.
- **D2 — The divider keys are relocated and re-sorted.** None of the eight `-- X` keys is
  in `KnownTopLevelSanmapKeys()` (`src/io/Sanmap_KnownTopLevelKeys_IO.cpp:14-68`, checked
  key by key), so import bags all eight, and export re-emits them alphabetically sorted
  (`std::map`) inside a nested `UnknownImport` object at the end of the document. Their
  positional meaning — their only meaning — is destroyed. **The file still loads**, which
  is what makes this worth a ticket.
- **D3 — P2/P3 float literals are re-spelled.** Every float field is narrowed to C++
  `float` on import (`JsonPrimitives_IO.h:26`) and re-widened on export, so
  `44.5151672` comes back as `44.515167236328125`. Semantically identical for a
  float32 consumer; a large byte delta. P1 values round-trip exactly.
- **D4 — At least one integer becomes a float.** The reference has `"waterLevel": 78`;
  `Params::Water::waterLevelMaximum` is a `float` (`src/params/Water_PARAMS.h:10`) and
  `document["waterLevel"] = recipe.water.waterLevelMaximum;`
  (`MapExporter_DocumentAssembly_IO.cpp:34`), so SanGen emits `78.0`. Same class of
  delta applies to any other format-integer field SanGen stores as `float`. `height` is
  already handled correctly (`std::lround` to `int`,
  `MapExporter_DocumentAssembly_IO.cpp:30`) and is the precedent to copy.
- **D5 — Two latent hazards, currently invisible because the reference is pure ASCII:**
  - `ensure_ascii = false` means a non-ASCII map name or credits string is emitted as raw
    UTF-8, not `\uXXXX`. Valid JSON either way; **untested against the game's parser.**
  - `error_handler = strict` means `dump()` **throws** `type_error.316` on a `std::string`
    holding invalid UTF-8 (e.g. an ANSI-encoded Windows path or filename). Today that
    exception propagates out of `BuildSanmapJsonText` with no handler on the path —
    export dies rather than reporting. This is a Constitution §6 gap
    ("report failures clearly"), not just a formatting nit.
  - Minor, non-blocking: `kMaxExp = 15` means nlohmann switches to exponent notation for
    magnitudes ≥1e16 where a shortest-double printer using a 1e17 threshold would not.
    No value in the reference is anywhere near this. Noted so it is not rediscovered as
    a mystery later.

---

## 4. ⚠️ The key-ordering question — the human must choose, and one option is not real

**Byte-identical round-trip of `Pandemonium Isthmus.sanmap` is NOT achievable, and no
amount of exporter work makes it achievable.** Stated plainly rather than promised:

1. The reference's top-level order is **one authoring tool's insertion order**, not a rule.
   There is no schema, no sort, and no derivation SanGen could reproduce — the `-- X`
   divider keys exist precisely because the order is editorial.
2. The reference is **internally inconsistent**: 304 `rotation` objects are `w,x,y,z` and
   1180 are `x,y,z,w` (§1.1). There is no single convention to conform to. A conforming
   exporter would have to reproduce *which producer wrote which region*.
3. **P2 float literals cannot be regenerated from `float` storage** (§1.2). Recovering
   `44.5151672` requires either keeping the original literal text or a float32-aware
   shortest printer — and even a float32-aware printer cannot recover P3
   (`1103.367933379766` is not a float32 value at all; it is destroyed by the narrowing
   at `JsonPrimitives_IO.h:26`, before any printer sees it).

So the honest requirement is **stable, deterministic, readable, and convention-matching**
— not byte-identical. The choice below is about **how far to go on preservation**, and
must be answered before the Coder starts:

> ### CHOICE — pick one, by name
>
> **(A) CONVENTION-MATCH ONLY** *(recommended default)*
> Ratify F1–F10 as SanGen's own emission contract and test them. Accept D1/D2/D3/D4.
> SanGen-authored maps look exactly like shipped maps and are byte-stable across runs;
> an imported third-party map comes back reordered but semantically intact.
> *Scope: §5 + §6 + §7. Small ticket.*
>
> **(B) CONVENTION-MATCH + POSITION-PRESERVING PASSTHROUGH** — **CHOSEN by the human.**
> (A), plus: `UnknownImportBag` records each unknown key's original adjacency and
> `MergeUnknownImportKeys` reinserts each key next to that neighbor instead of appending, and
> switches the bag's own JSON member from `nlohmann::json` to `nlohmann::ordered_json` so
> unknown keys stop being alphabetized inside the bag itself. Fixes D2 — the `-- X` dividers
> keep their positions and their meaning. Does not fix D1 for *known* keys (SanGen's own
> fixed emission order for recognized fields is unchanged), and does not touch D3.
> ⚠️ **Amendment finding — read §6.5 before scoping this, do not use the one-line mechanism
> above as the spec.** A bag-only type change is not sufficient: the document that reaches
> `CaptureUnknownTopLevelKeys` has *already* lost its original top-level order by the time it
> gets there (it is parsed as plain `nlohmann::json`, alphabetically ordered — §6.5 traces
> this exactly), so "record the index this loop encountered the key at" records the
> alphabetical rank, not the original position, and would not actually fix D2. §6.5 specifies
> the real mechanism: a second, order-preserving parse taken solely to learn adjacency,
> kept fully separate from the working document every existing reader already uses.
> *Scope: (A) + §6.5's capture/reinsertion mechanism — additive-only (no reader's document
> type changes), but touches more files than originally sketched here. See §6.5's blast-radius
> note and the updated Files-touched list.*
>
> **(C) FULL FIDELITY** — **do not choose this without a further conversation.**
> Would require the importer to retain the original document verbatim and the exporter to
> re-emit untouched subtrees as their original text, bypassing `dump()` entirely. That is
> a different IO architecture (a text-splicing exporter), it defeats the migration system,
> and it is the IO Architecture Expert's ticket, not this one. Listed only so it is
> visibly considered and visibly declined.

Everything in §5–§7 below is **common to (A) and (B)**. §6 item 6 is **(B)-only** and is
marked as such — a Coder given (A) skips it and touches neither file.

---

## 5. The requirement — SanGen's ratified emission contract

`MapExporter::BuildSanmapJsonText` and the write path beneath it **must** guarantee, for
every export:

- **R1** 4-space indentation, space character, one level per nesting depth.
- **R2** `": "` between key and value; `,` immediately after a value with no space before
  the newline.
- **R3** **LF (`\n`) line endings, never CRLF, on every platform.** Non-negotiable — this
  is the one that broke a real file.
- **R4** No terminating newline after the closing `}`.
- **R5** No trailing whitespace on any line.
- **R6** One array element / one object member per line. Empty containers as `[]` / `{}`.
- **R7** Every JSON-number value emitted from a C++ floating-point field carries a decimal
  point or an exponent — never a bare integer spelling.
- **R8** Every value the format types as an integer is emitted as a JSON integer, with no
  decimal point (`"width": 2048`, `"waterLevel": 78`).
- **R9** Given the same `Params::MapRecipe` and the same `UnknownImportBag`, two exports in
  the same process and two exports in different processes produce **byte-identical** text.
  Determinism is the guarantee that replaces byte-identity-with-the-reference.
- **R10** A string field containing bytes that are not valid UTF-8 produces a **logged,
  reported export failure**, never an uncaught exception and never a truncated file.

**R1, R2, R4, R5, R6, R7 are free** — nlohmann 3.11.3 with `dump(4)` already satisfies
them (§3). This ticket's job for those is to **pin them with a test** so they cannot
silently regress, not to write new emission code.

**R3 needs a real guard** (§6.1). **R8 needs a real audit** (§6.2). **R10 needs a real
handler** (§6.3). **R9 needs a test** (§7.2).

---

## 6. Scope

### 6.1 — Protect R3 (line endings). `src/io/MapExporter_IO.cpp` (EDIT, comment only)

`WriteBinaryFileBytes`'s `std::ios::binary` (`FilesystemPrimitives_IO.cpp:33`) is what
currently makes R3 true, and nothing says so. Add a short comment at the
`WriteBinaryFileBytes` call site (`MapExporter_IO.cpp:23`) stating that the `.sanmap`
document is written through the **binary** primitive specifically so nlohmann's `\n` is
not translated to `\r\n` by Windows text mode, that shipped maps are LF-only (measured),
and that this is the exact defect that damaged the live reference map on 2026-08-21.
Grep-findable wording; reference this work-order by name.

**Do not** add a second write path, do not add a "text mode" option, and do not add a
newline-normalization pass over the string — the correct behaviour is already in place and
only needs to be documented and tested.

### 6.2 — R8 audit: format-integer fields stored as `float` (`MapExporter_*_IO.cpp`)

`height` is already correct — `static_cast<int>(std::lround(geometry.terrainMaxHeight))`
(`MapExporter_DocumentAssembly_IO.cpp:30`), with a comment explaining that the format
types it as a C# int. **Apply the same treatment to every other format-integer field
currently emitted from a `float`.**

Confirmed instance: **`waterLevel`** (`MapExporter_DocumentAssembly_IO.cpp:34`, from
`Params::Water::waterLevelMaximum`, `src/params/Water_PARAMS.h:10`) — reference shows
`"waterLevel": 78`, SanGen emits `78.0`.

The Coder must **audit, not assume**: walk every `document[...] =` / `json[...] =`
assignment across `src/io/MapExporter_*.cpp`, and for each, check the corresponding value
in the reference map. Emit `static_cast<int>(std::lround(x))` **only** where the reference
demonstrably shows a bare integer. Where the reference shows `.0`, leave it alone.

⚠️ **`waterDepth` is NOT one of these** — the reference has
`"waterDepth": 30.615638732910156`. Do not "tidy" it.

⚠️ **Report, do not silently expand scope.** If the audit finds more than ~3 fields,
stop and list them in the work-order's own follow-up section rather than changing a dozen
values in one pass — each one is a live, in-game-visible semantic change, not a
formatting change.

### 6.3 — R10: `dump()` must not throw out of the exporter (`MapExporter_Recipe_IO.cpp`)

`dump()` runs with `error_handler_t::strict` (`json.hpp:20576-20577`) and throws
`nlohmann::json::type_error` (316) on invalid UTF-8 in any string. Wrap the `dump` call at
`MapExporter_Recipe_IO.cpp:33` so the failure is **caught, reported, and returned as an
empty string**, letting `WriteSanmapDocument` (`MapExporter_IO.cpp:22-28`) take its
existing failure path — it already logs `"Failed to write …"` and returns `false`.

Prefer the smallest change that satisfies Constitution §6's "report failures clearly":
catch `const nlohmann::json::exception&`, return `std::string()`, and have the caller
treat empty text as a hard failure. **Do not** switch `error_handler` to `replace` or
`ignore` — silently mangling a designer's map name is worse than refusing to write.

**Keep `ensure_ascii = false`** (raw UTF-8 output). It matches the reference in every
observable way (the reference is pure ASCII, where both settings agree) and preserves
bytes. Note in a comment that the `\uXXXX` alternative is **untested against the game's
own parser** and is a deliberate open question, not an oversight.

### 6.4 — Document the contract (`src/io/MapExporter_Recipe_IO.cpp`, header comment)

Add R1–R10 as a compact numbered comment block above `BuildSanmapJsonText`, each item
noting whether nlohmann provides it for free or SanGen enforces it. This is the file a
future reader lands on; the contract belongs there, not only in this work-order.

### 6.5 — **(B)-ONLY, CHOSEN.** Position-preserving unknown-key passthrough

**Skip this section entirely if the human chose (A).** They chose (B) — this section is now
in scope, and it **supersedes** the one-line mechanism sketched in §4's option list, which
undersold what this requires once the actual import pipeline was read end to end.

**⚠️ Load-bearing finding: the document has already lost its original top-level key order by
the time the existing capture code sees it — independent of anything the bag itself does.**
`MapImporter::ParseSanmapJsonText` parses the whole document exactly once, as plain
`nlohmann::json` — `document = nlohmann::json::parse(documentText);`
(`MapImporter_ParseDocument_IO.cpp:111`) — and `nlohmann::json`'s object type is
`std::map`-backed and alphabetically ordered, not insertion-ordered. That same `document` is
what threads through `RunSanmapMigrations` → `CaptureUnknownTopLevelKeys`
(`Sanmap_MigrationRunner_IO.cpp:44-53`), whose `for (const auto& [key, value] :
document.items())` loop (line 49) therefore already walks keys **alphabetically**, not in
file order. Recording "the index this loop encountered the key at" — the mechanism §4
sketched — would record the alphabetical rank, not the original position. It would not fix
D2: it would replace one wrong order (nested-and-alphabetized, today's bug) with another
(top-level-and-alphabetized) that is indistinguishable from today's bug in the one case that
matters, because `-` sorts before every letter and all eight `-- X` dividers would still
clump together at the front regardless.

**The fix must learn adjacency from a representation that has not lost it.** The only such
representation available without a bigger IO-architecture change (Option (C), declined in §4)
is the raw JSON text itself, parsed order-preservingly before anything discards that order.

- **Capture side — a second, throwaway, order-preserving parse.** In
  `MapImporter::ParseSanmapJsonText` (`MapImporter_ParseDocument_IO.cpp`), immediately
  alongside the existing `nlohmann::json::parse(documentText)` call (line 111), add
  `nlohmann::ordered_json orderedDocument = nlohmann::ordered_json::parse(documentText);`
  parsed from the **same, already-validated** source text (reuse the existing try/catch — do
  not duplicate the error-reporting path for a parse of text the first call already proved
  valid). `nlohmann::ordered_json`'s object type preserves source order, so walking
  `orderedDocument.items()` at the top level recovers the true original order without
  touching a single existing reader — `ParseDocumentEnvelopeJson` / `ParseEntityDomainsJson`
  / `ParseStackDomainsJson` / `ParseSimulationDomainsJson` and everything each of them calls
  keep reading the original, unordered `document`, byte-for-byte unchanged. This is the whole
  point of a second parse instead of changing `document`'s own type: switching `document`
  itself to `nlohmann::ordered_json` would ripple `const nlohmann::json&` parameter changes
  across the entire `Read*Json` reader surface (dozens of functions across
  `src/io/MapImporter_*.cpp`) — that is Option-(C)-sized, not this ticket.
- **Capture side — record an anchor, not a raw index.** Thread `orderedDocument` (or the
  ordered list of its top-level key names) as a new parameter into `RunSanmapMigrations` →
  `CaptureUnknownTopLevelKeys`. Walk its top-level keys in order; for each key
  `IsKnownTopLevelSanmapKey` classifies unknown, record into the bag not just the value but
  **the name of the immediately preceding top-level key in that same ordered walk** (known or
  unknown, whichever it is), or a reserved sentinel (empty string) meaning "was the very first
  top-level key." A raw index goes stale the instant any earlier key is one SanGen's exporter
  drops or adds; a named anchor degrades gracefully (see the reinsertion fallback below) and
  stays legible in a debugger. Still seed from any incoming `UnknownImport` object first, per
  the existing STEP28 rule (`Sanmap_MigrationRunner_IO.cpp:39-42`) — those keys' anchor is
  whatever top-level key precedes the `UnknownImport` key itself in `orderedDocument`, since
  STEP28's flattening rule means they rejoin the top level as ordinary unknown keys on the
  next export.
- **Bag shape.** `src/io/UnknownImportBag_IO.h` — change `unknownTopLevelKeys` from
  `nlohmann::json` to **`nlohmann::ordered_json`** (stops the bag's own alphabetical re-sort,
  `UnknownImportBag_IO.h:25` — orthogonal to but still needed alongside the anchor mechanism,
  since the bag is iterated directly during reinsertion) and add a parallel, ordered record of
  `{key name → anchor key name}` — a `std::vector<std::pair<std::string, std::string>>` in
  original-file order is sufficient and keeps the struct free of a second live
  `nlohmann::json` object. Both members keep default-constructing to empty, matching the
  struct's existing default-construction contract; nothing that merely holds or resets a whole
  `UnknownImportBag` needs to change (see "other consumers" below).
- **Export side — reinsert by walking the anchor list in original-file order, splicing after
  each anchor once it exists in the built document.** `MergeUnknownImportKeys`
  (`MapExporter_UnknownImportMerge_IO.h`) still runs strictly last — after every
  `MapExporter_DocumentAssembly_IO.cpp` helper has already written SanGen's own fixed-order
  known keys into `document` (D1 stands; this ticket does not reorder known keys). It walks
  the bag's `{key → anchor}` list in the original-file order recorded at capture time, and for
  each entry: if `document` already contains a member equal to the anchor name (either a
  known key SanGen wrote, or an earlier unknown key already spliced in by an earlier step of
  this same walk — walking in original-file order guarantees any unknown key's anchor that is
  itself another unknown key was already placed), insert the new key **immediately after** the
  anchor's current position; if the anchor is the "was first" sentinel, or the named anchor
  key does not exist anywhere in the freshly built `document` (e.g. a known key the reference
  had that this recipe's export path never writes), insert at the very front of the top level
  instead of silently dropping the key. Every placement is then either genuinely correct
  (anchor survived) or a visible, safe, front-of-document fallback — never a crash, never
  silent loss, and never today's worst case (nested at the very end under `UnknownImport`,
  alphabetized).
  `nlohmann::ordered_json`'s object type does not expose an "insert at position N" through
  `operator[]` (a fresh key via `operator[]` always appends at the end); the Coder will need
  `document.get_ref<nlohmann::ordered_json::object_t&>()` to reach the underlying
  vector-of-pairs and its `insert(iterator, value)`. Confirm this against the vendored
  nlohmann 3.11.3 headers (`CMakeLists.txt:30-34`) before committing to the exact call, and
  leave a comment at the call site recording the confirmed API — easy to get subtly wrong.

**Blast radius, stated explicitly:** additive on `UnknownImportBag_IO.h` (one field's type
changes, one field is added; both keep default-construction working). One new parameter
threads through `MapImporter_ParseDocument_IO.cpp`'s `ParseSanmapJsonText` (internal local
only — its own public signature, `MapImporter_IO.h:92,99`, does not change) →
`Sanmap_MigrationRunner_IO.h`'s `RunSanmapMigrations` → `CaptureUnknownTopLevelKeys`. A
rewrite (not a tweak) of `MergeUnknownImportKeys`'s body. **No reader's document type
changes anywhere** — that is the property that keeps this additive instead of becoming
Option (C).

**Other consumers of `UnknownImportBag`, checked this session (every hit of a repo-wide grep
read) — none need code changes:**
- `Sanmap_MigrationRunner_IO_Test.cpp` (multiple existing cases) only ever calls
  `.contains(...)`, `operator[]`, `.empty()`, and equality against
  `bag.unknownTopLevelKeys` — all order-independent operations `nlohmann::ordered_json`
  supports with identical semantics to `nlohmann::json`. These existing tests need no edits;
  new (B)-specific tests are purely additive (§7 amendment below).
- `src/ui/Application_AssetBridge_UI.h` / `Application_UI.cpp` / `FilesTab_Actions_UI.cpp` /
  `FilesTab_UI.h` hold or reset a whole `UnknownImportBag` by pointer/value
  (`std::make_unique<Io::UnknownImportBag>()`, `*state.unknownImportData =
  Io::UnknownImportBag()`) and never touch `unknownTopLevelKeys`'s internals directly — opaque
  to this type change.
- `MapExporter_IO.h` / `MapImporter_IO.h` / `Sanmap_MigrationRunner_IO.h` only
  forward-declare `struct UnknownImportBag;` — unaffected by a member's type changing.

⚠️ This **still changes the meaning of the `UnknownImport` key** ratified by
`STEP28_UnknownImportNesting_IO` and described in `IO_MIGRATION_SPEC.md` §6 — arguably more
than the original one-line sketch implied: a document that round-trips successfully no longer
nests its unknown keys under `UnknownImport` at all (they're spliced back to the top level);
`UnknownImport` now appears only as the landing zone for an anchor that could not be
resolved, a narrower role than today's "every unknown key, always." That is a format-truth
change and requires a `SANMAP_FORMAT_SPEC` Correction from the Format Expert **before** the
Coder starts — unchanged from the original ruling, restated because the mechanism grew and
the format-truth consequence grew with it. Raise the Correction first and treat 6.5 as a
separate follow-up ticket gated on it landing.

### Out of scope — do not build, do not stub

- Any change to which keys are written, their names, or their values (beyond §6.2's
  integer typing).
- Option (C) / text-splicing export, verbatim-subtree retention, original-literal
  preservation. Declined in §4.
- A float32-aware shortest-round-trip printer to recover population P2. It cannot recover
  P3, so it buys partial fidelity at the cost of a custom dtoa in the IO layer. Not worth
  it; revisit only if the human explicitly asks.
- Import-side changes of any kind, except the second order-preserving parse and anchor
  capture that §6.5 requires under (B).
- Moving nlohmann into `src/third_party/` per ARCH_07_03_VendoredThirdPartyHeaders.md §7.3 — it is
  FetchContent'd (`CMakeLists.txt:30-34`), never vendored into the tree, so §7.3's placement rule
  is not currently engaged. **Observation only, for the ARCH Expert; not this ticket's business.**
- Any modification to any `.sanmap` file, anywhere, ever.

---

## 7. Acceptance test — NEW `src/io/MapExporter_Formatting_IO_Test.cpp`

The verification method is a **byte comparison against a checked-in fixture**, which the
Coder can actually run offline with no game install present.

### 7.1 The fixture — NEW `src/io/testdata/FormattingReference.sanmap`

A **new, small, hand-written file created by the Coder in the repo** — around 40 lines,
not a copy of any real map. It must exercise every property in §5 at least once:

- nesting to depth 3+, an object, an array of objects, an empty `[]` and an empty `{}`;
- an integral float (`128.0`), a fractional float (`0.5`), a negative float (`-20.0`), a
  true integer (`2048`), a boolean, a string containing an unescaped `/`;
- LF endings, no trailing newline, no trailing whitespace.

Author it to be exactly what `BuildSanmapJsonText` produces for the fixture recipe in
7.2 — that is the whole point.

⚠️ **Create it with the repo's normal file-writing tools and verify its bytes** (`od -c`
the head and tail) before committing. A fixture accidentally saved as CRLF makes this
entire test assert the wrong thing, permanently and invisibly. Add a `.gitattributes`
entry (`*.sanmap -text`) so git never normalizes it on checkout.

#### 7.1b — **(B)-ONLY** second fixture: `src/io/testdata/FormattingReferenceWithUnknownKeys.sanmap`

Needed only under (B). A second small hand-written fixture (~15-20 lines), **kept separate**
from §7.1's `FormattingReference.sanmap` so the byte-exact test in §7.2 item 1 stays a pure
formatting check with no `UnknownImportBag` involved. This one exists solely to exercise
position-preserving passthrough (§6.5) and must:
- interleave at least two unrecognized top-level keys among at least three known top-level
  keys, in a pattern that mimics the reference map's `-- X: Settings` dividers — e.g.
  `"-- Test Group": "Settings"` immediately followed by two real known keys such as
  `"width"` and `"height"`;
- include one unrecognized key as the very first top-level key in the file (exercises the
  "was first" sentinel anchor from §6.5);
- include one unrecognized key whose value is itself a nested object, not a scalar, to prove
  the whole value survives the splice (cheap to assert, expensive to silently regress).

Same LF / no-BOM / no-trailing-newline byte discipline as §7.1's fixture; already covered by
the repo-wide `.gitattributes` entry §7.1 adds — no second entry needed.

### 7.2 Tests

1. **⚠️ Load-bearing — byte-exact fixture match.** Build a `Params::MapRecipe` matching
   the fixture, call `MapExporter::BuildSanmapJsonText`, read the fixture in
   `std::ios::binary`, and assert **`producedText == fixtureBytes`**, exact `std::string`
   equality. On mismatch, report the first differing byte offset and 40 bytes of context
   on each side — a raw "not equal" on a 2 KB string is useless to debug.
2. **R9 determinism.** Call `BuildSanmapJsonText` twice on the same recipe; assert the two
   strings are byte-identical. Then assert the same for a recipe rebuilt from scratch with
   the same values (catches any address- or hash-order dependence).
3. **R3 line endings.** Assert `producedText.find('\r') == std::string::npos`. Assert
   `producedText.find("\n\n") == std::string::npos` (no blank lines).
4. **R4 no trailing newline.** Assert `producedText.back() == '}'`.
5. **R5 no trailing whitespace.** Split on `\n`; assert no line ends in a space.
6. **R1 indentation.** Assert every line's leading-space count is a multiple of 4, and
   that at least one line has 12 (proves depth is really being exercised).
7. **R7 float spelling.** Assert the produced text contains `"fadeDistance": 128.0` — the
   `.0` is present, not `128`. (`MapExporter_DocumentAssembly_IO.cpp:44` writes `128.0f`
   unconditionally, so this is stable regardless of recipe content.)
8. **R8 integer spelling.** Assert the produced text contains `"width": 2048` and
   **not** `"width": 2048.0`. After §6.2, add the same assertion for
   `"waterLevel": 78` from a recipe with `waterLevelMaximum = 78.0f`.
9. **R10 invalid UTF-8.** Set `recipe.mapName` to a string containing a lone `0xFF` byte;
   assert `BuildSanmapJsonText` **returns empty and does not throw** (wrap the call in a
   `try`/`catch(...)` that fails the test if anything escapes).
10. **Round-trip stability.** `BuildSanmapJsonText` → `ParseSanmapJsonText` →
    `BuildSanmapJsonText`; assert the first and third texts are byte-identical. This is
    the regression that catches a future float-narrowing or key-order change, and it is
    the one that would have caught the original incident **if the incident had been in
    SanGen's own code rather than a Python script.**

#### (B)-ONLY test cases, added to the same test binary once §6.5 lands

11. **Anchor capture.** Parse `FormattingReferenceWithUnknownKeys.sanmap` via
    `ParseSanmapJsonText` with a non-null `UnknownImportBag*`; assert the bag's `{key →
    anchor}` record has the expected pairs for all three unrecognized keys, including the
    "was first" sentinel for the leading one.
12. **Splice reinsertion, single round-trip.** Build the `Params::MapRecipe` for the fixture,
    call `BuildSanmapJsonText` with that same populated bag; assert each unrecognized key
    appears in the output **immediately adjacent to its recorded anchor's known-key output**,
    not nested under an `UnknownImport` key (unless its anchor could not be resolved — see
    case 13).
13. **Anchor-not-found fallback.** Construct a bag whose one anchor name does not appear
    anywhere in a freshly built export (e.g. `waterLevel` when the recipe/export path never
    writes it); assert the orphaned key lands at the very front of the top level — not
    dropped, and `BuildSanmapJsonText` does not throw.
14. **Passthrough determinism (extends R9/test 2).** Same as test 2, but with a populated
    `UnknownImportBag` in play — two `BuildSanmapJsonText` calls on the same recipe + bag must
    still be byte-identical.
15. **Full round-trip with passthrough (extends test 10).** `BuildSanmapJsonText` (with the
    unknown-keys fixture's bag) → `ParseSanmapJsonText` (into a fresh bag) → assert the fresh
    bag's anchor record and `unknownTopLevelKeys` content equal the original bag's, and that a
    second `BuildSanmapJsonText` from the fresh bag reproduces the same text byte-for-byte.
    This is the test that would have caught D2 in the first place.

`CMakeLists.txt` addition, alongside the other IO test registrations:
```cmake
add_sangen_test(MapExporter_Formatting_IO_Test src/io/MapExporter_Formatting_IO_Test.cpp)
target_link_libraries(MapExporter_Formatting_IO_Test PRIVATE nlohmann_json::nlohmann_json)
```
(The test reads the fixture from disk, so it needs the fixture path — follow whatever
convention `AssetPipeline_TestSupport_IO.h` already uses for test-relative paths rather
than inventing a new one.)

---

## Files touched

**Under (A) — the recommended default:**
- EDIT `src/io/MapExporter_IO.cpp` (§6.1 — comment at the `WriteBinaryFileBytes` call site)
- EDIT `src/io/MapExporter_DocumentAssembly_IO.cpp` (§6.2 — `waterLevel` integer typing,
  plus anything else the audit confirms)
- EDIT `src/io/MapExporter_Recipe_IO.cpp` (§6.3 exception guard, §6.4 contract comment)
- NEW `src/io/MapExporter_Formatting_IO_Test.cpp`
- NEW `src/io/testdata/FormattingReference.sanmap`
- EDIT `.gitattributes` (`*.sanmap -text`) — create if absent
- EDIT `CMakeLists.txt` (one test registration)

**Additionally under (B)** — only after the `SANMAP_FORMAT_SPEC` Correction lands:
- EDIT `src/io/UnknownImportBag_IO.h` — field type change + new anchor-record field (§6.5).
- EDIT `src/io/MapExporter_UnknownImportMerge_IO.h` — rewrite `MergeUnknownImportKeys` to
  splice by anchor instead of appending one nested `UnknownImport` object (§6.5).
- EDIT `src/io/MapImporter_ParseDocument_IO.cpp` — add the second, order-preserving
  `nlohmann::ordered_json` parse of the same document text inside `ParseSanmapJsonText`, and
  pass it (or its derived top-level key order) down to `RunSanmapMigrations` (§6.5). The
  public signature of `ParseSanmapJsonText` itself (`MapImporter_IO.h:92,99`) is unaffected —
  the ordered parse is an internal local, not a new out-param.
- EDIT `src/io/Sanmap_MigrationRunner_IO.h` / `.cpp` — `RunSanmapMigrations` and
  `CaptureUnknownTopLevelKeys` gain the ordered top-level-key input and do the
  anchor-recording described in §6.5, replacing today's plain `document.items()` walk at
  `Sanmap_MigrationRunner_IO.cpp:49`.
- NEW `src/io/testdata/FormattingReferenceWithUnknownKeys.sanmap` (§7 amendment).
- No changes needed to `Sanmap_MigrationRunner_IO_Test.cpp`'s existing cases, and no changes
  needed anywhere in `src/ui/` — see §6.5's "other consumers" check.

`src/io/*.cpp`/`*.h` are `GLOB_RECURSE`'d into `SANGEN_V2_SOURCES` (`CMakeLists.txt`), so
the new `.cpp` needs no explicit source-list entry — only the test target does.

**File-size ceilings (ARCH_01_05_FileSizeCeilings.md §1.5):** every edit above is additive by a handful of lines and
none pushes its file past the hard 150-line ceiling. The new test file is expected to
exceed 150 lines (10 cases); `*_Test.cpp` files across `src/io/` already run well past it
(`MapImporter_IO_Test.cpp` is 1600+), so this follows established precedent rather than
requiring a new exception — but the Coder should state that explicitly in the file header
rather than leaving it implicit.

---

## Verify

- `MapExporter_Formatting_IO_Test` passes, all cases — 10 under (A); all 15 (1-10 plus the
  §6.5 (B)-only 11-15) now that (B) is the chosen scope.
- Full solo rebuild + `ctest -C Debug`: 100% pass, zero pre-existing test files edited
  (`Sanmap_MigrationRunner_IO_Test.cpp`'s existing cases in particular — §6.5 confirmed they
  need no edits; a diff there is a signal something drifted from this ticket's scope).
- `od -c src/io/testdata/FormattingReference.sanmap | head -3` and `| tail -3` confirm LF
  endings and a final `}` with no newline — check the fixture's bytes, not just that the
  test using it went green. Repeat for `FormattingReferenceWithUnknownKeys.sanmap` under (B).
- Grep `src/io/MapExporter_IO.cpp` for the §6.1 comment — the documentation edit most
  likely to be silently skipped, and the one guarding the defect that opened this ticket.
- **(B)-only gate: confirm the `SANMAP_FORMAT_SPEC` Correction (§6.5) is ratified by the
  Format Expert before any of §6.5's code is written.** The Coder should not start §6.5 on
  this ticket's say-so alone.
- **Confirm nothing under `E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\`
  was modified.** No `.sanmap` file outside the repo is touched by any part of this work.

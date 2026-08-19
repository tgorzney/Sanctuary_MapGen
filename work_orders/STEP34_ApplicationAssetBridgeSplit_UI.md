# Work-Order — Step 34: extract the asset-bridge cluster out of `Application_UI.h`

*Constitution §7. Executor: SanGen Coder. UI Expert consult.*

## Root problem
`src/ui/Application_UI.h` is 157 lines, over the 150-line hard ceiling. It is declaration-heavy
(no function bodies — every method is defined in one of 15 `Application_*.cpp` aspect files
already, so the §1.5 "large class splits its `.cpp` bodies across files" escape hatch is already
fully used). The overage is in the class's own declared data-member surface.

**Real defect found alongside this (fix as part of this ticket, not a separate one):**
`Application_UI.h` uses two types by value without including anything that declares them —
`IconAtlasManifest` (declared in `IconGridWidget_UI.h`) and `Io::AssetAtlasCache` (declared in
`../io/AssetAtlasCache_IO.h`) — currently compiling only by transitive include-order luck. The
split below fixes this for free.

## Ruled by this ticket
**New `src/ui/Application_AssetBridge_UI.h`** — new struct `ApplicationAssetBridge`, pure data, a
"member file of `Application_UI.h`" matching the exact existing pattern already used for
`Application_HostedSettings_UI.h`/`Application_TabState_UI.h`. Moves verbatim (including their
existing load-bearing comments) from `Application_UI.h`: `atlasResidency`, `assetAtlasCache`,
`assetPackReader` (+ its "separate from cache" comment), `unknownImportData` (+ its pimpl-for-
json-hygiene comment, unchanged), `iconManifest`, `iconTemplateIdentifiers`,
`assetStatusMessage`, `sanpackPath[260]`, `bAssetLoadRequested`, `bAssetLoadAnnounced`. This
matches the file's own existing banner comment naming this exact cluster as one unit ("the one
unit that legally sees IO and UI at once").

Adds the two missing includes (`IconGridWidget_UI.h`, `../io/AssetAtlasCache_IO.h`) plus keeps
`../io/SanpackReader_IO.h`, `../sys/AtlasResidency_SYS.h`, and the `namespace Io { struct
UnknownImportBag; }` forward declare — all move here from `Application_UI.h`.

`Application_UI.h` changes: add `#include "Application_AssetBridge_UI.h"`; remove the now-
redundant direct includes (`SanpackReader_IO.h`, `AtlasResidency_SYS.h`, the `UnknownImportBag`
forward declare); replace the 10 removed field declarations with one line:
`ApplicationAssetBridge assetBridge;`; update the banner's "Member headers" list.

**Public API unchanged**: `SanpackPath()`, `IconManifest()`, `TemplateIdentifierOfIcon()`,
`AssetStatusMessage()`, `ActiveIconManifest()` keep their exact declared signatures — only their
`.cpp` bodies (across `Application_Assets_UI.cpp`, `Application_AppSettings_UI.cpp`,
`Application_AssetPanel_UI.cpp`, `Application_Window_UI.cpp`, `Application_Frame_UI.cpp`,
`Application_UI.cpp`) change to read `assetBridge.<field>` instead of the bare field.

**Confirmed no other tab file touches these fields directly** — everything external goes through
the accessors, so this split's blast radius stops at `Application`'s own aspect `.cpp` files.
**One check required**: `ApplicationShell_IconBridge_UI_Test.cpp`/`ApplicationShell_AppSettings_
UI_Test.cpp` — confirm they use accessors, not direct field pokes; fix if they don't.

## Target files
- New `src/ui/Application_AssetBridge_UI.h`.
- `src/ui/Application_UI.h` — remove the 10 fields + 3 includes, add 1 include + 1 field.
- `src/ui/Application_Assets_UI.cpp`, `Application_AppSettings_UI.cpp`,
  `Application_AssetPanel_UI.cpp`, `Application_Window_UI.cpp`, `Application_Frame_UI.cpp`,
  `Application_UI.cpp` — update field access to go through `assetBridge.`.
- `ApplicationShell_IconBridge_UI_Test.cpp`, `ApplicationShell_AppSettings_UI_Test.cpp` — verify/
  fix if they poke fields directly.

## Explicit out-of-scope
- Any other data member in `Application_UI.h` — none of the rest form as clean a single-purpose
  cluster; do not manufacture additional groupings.

## Layer & accuracy class
UI only, pure refactor. Accuracy class: Exact.

## Acceptance test
1. `Application_UI.h` lands at ~137 lines, `Application_AssetBridge_UI.h` at ~55-65 lines.
2. `Application_UI.h` compiles standalone without relying on transitive includes for
   `IconAtlasManifest`/`Io::AssetAtlasCache` (the real defect this ticket fixes).
3. Every existing UI test continues to pass unchanged, especially the two flagged icon-
   bridge/app-settings tests.
4. Full `SanGenV2` build stays clean.

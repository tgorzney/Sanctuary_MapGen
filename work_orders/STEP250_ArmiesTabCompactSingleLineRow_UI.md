# STEP250 — Armies tab: collapse each army's settings onto one compact line

**Layer:** UI. **Domain:** `src/ui/Combo_UI.h`, `src/ui/Combo_UI.cpp`, `src/ui/ArmiesTab_UI.cpp`.
**Sequence:** depends on nothing undone. Purely a per-row layout change — touches no PARAMS, no IO,
no preview wiring (SCOPE NOTE 1 in `ArmiesTab_UI.h` already establishes none of these fields notify
`PreviewDriver`, unaffected by this ticket).

**Origin:** human request, 2026-09-03, direct in-chat — not a design-session ticket. Ruled here so the
Coder has one settled spec instead of re-deriving intent from a chat transcript.

---

## 0. The ruling (settled by the human, do not re-litigate)

1. **`Army::alias`'s text box is REMOVED from this tab.** Confirmed dead: `grep -rn "\.alias\b" src`
   shows it is written only by this tab's own `DrawTextInput("Alias", ...)` and round-tripped by
   `MapExporter_Armies_IO.cpp`/`MapImporter_Armies_IO.cpp` — nothing reads it for matching, display,
   or any other logic (`ArmyRowLabel` uses `displayName`, never `alias`). **This ticket removes only
   the UI edit path** — `Params::Army::alias`, its IO round-trip, and the `.sanmap` `alias` field stay
   exactly as they are (Constitution §6: never silently discard already-authored data; an old map with
   a non-empty `alias` keeps it, just not editable from this tab anymore). Do **not** touch
   `Army_PARAMS.h`, `MapExporter_Armies_IO.cpp`, or `MapImporter_Armies_IO.cpp` in this ticket.
2. **Team Color's RT toggle is removed**, same reasoning and same mechanism as ruling 3 — a color
   swatch commit is cheap, no reason to ever defer it. Precedent: `AreasTab_UI.h:46` already does
   this (`options.bRealtimeToggleHidden = true; // STEP221 — area color is always realtime, no choice`).
3. **Starting Alloys' AND Starting Energy's RT toggles are both removed**, always real-time, never
   user-togglable. The human named Alloys explicitly; Energy is its structural sibling (same slider
   type, same row treatment today) and is widened identically rather than left inconsistent — flagged
   here rather than silently assumed.
4. **Faction and Team Color draw with no visible label.** Engine ID keeps its existing plain-text
   form (it is a read-only diagnostic, not a labeled input — nothing to strip). Starting Alloys and
   Starting Energy **keep** their labels (explicit human instruction: "Starting: Alloy (label)").
   Name keeps no visible label — placeholder/hint text substitutes (ruling 5).
5. **Name's visible label is replaced by hint (placeholder) text**, `"Army Name"`, shown greyed while
   the box is empty — the existing `DrawTextInput` `hintText` parameter, already shipped, does exactly
   this.
6. **Everything drawn on ONE line** via `SameLine()`, in this order: Engine ID → Name → Team Color →
   Faction → Starting Alloys (label + compact slider) → Starting Energy (label + compact slider) →
   Mirror button (only when `CanMirrorArmy` — unchanged visibility rule).

---

## 1. Widen `Combo_UI` — the one real gap

Every other control already has a label-hidden + fixed-width composition mode (`TextInput`'s
`bLabelHidden`/`fixedWidthPixels`, `ColorSwatch`'s `bLabelHidden`, `SliderScalarCompact`'s whole
existence). `DrawCombo` has neither — it always draws its label on its own line
(`ImGui::TextUnformatted(label)`) and always claims full remaining width
(`ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x)`). Faction cannot join a `SameLine()` row
without this.

**`src/ui/Combo_UI.h` (EDIT)** — two new `ComboOptions` fields, mirroring `ColorSwatchOptions`'s
existing shape exactly (same names, same "0/false preserves prior behavior" contract, so this is
default-safe for all 22 existing `DrawCombo`/`ComboOptions` call sites):

```cpp
struct ComboOptions {
    const char* const* labels          = nullptr;
    int                count           = 0;
    const char*        emptyLabel      = "<none>";  // shown when the list is empty or nothing is picked

    // NEW — STEP250. Mirrors ColorSwatchOptions::bLabelHidden: skip the TextUnformatted(label) line so
    // the closed row can sit on ONE line via SameLine (a compact-row slot). `label` still scopes
    // ImGui::PushID; only the visible text is skipped. Every existing call site is unaffected
    // (default false, byte-identical to today).
    bool  bLabelHidden      = false;
    // NEW — STEP250. Mirrors TextInput_UI.h's fixedWidthPixels: <= 0 keeps today's "fill remaining
    // content width" behavior; a positive value fixes the closed row's own width instead, the seam
    // that lets a caller sit this control beside others via SameLine() instead of always claiming the
    // rest of the line.
    float fixedWidthPixels  = 0.0f;
};
```

**`src/ui/Combo_UI.cpp` (EDIT)** — `DrawCombo` body, two one-line changes:

```cpp
ImGui::PushID(label);
if (!options.bLabelHidden) ImGui::TextUnformatted(label);
ImGui::SetNextItemWidth(options.fixedWidthPixels > 0.0f ? options.fixedWidthPixels
                                                        : ImGui::GetContentRegionAvail().x);
```

No other line in `DrawCombo` changes. `ComboSelectionLabel`/`ResolvedComboSelection`/
`StepComboInteraction` (the pure, headless-tested logic in the header) are untouched — this is a draw-
path-only change, and `Combo_UI.cpp`'s own header comment already states rendering here is "verified
by eye against a live frame, never by test," so no new `Combo_UI_Test.cpp` case is required for this
half (confirm no regression in the existing suite instead).

**File-size check:** `Combo_UI.h` is 67 lines, `Combo_UI.cpp` is 44 — both land comfortably under the
100-line soft ceiling after this.

---

## 2. `ArmiesTab_UI.cpp` — `DrawArmySettings` rewrite

Current shape (`ArmiesTab_UI.cpp:96-127`) draws eight stacked blocks (Engine ID, Name, Alias, Team
Color, Faction, Starting Alloys, Starting Energy, Mirror button), each on its own line(s). Replace with
one `SameLine()`-chained row, per ruling 6's order:

```cpp
void DrawArmySettings(std::vector<Params::Army>& armies, int armyIndex, ArmiesTabState& state) {
    Params::Army& army = armies[static_cast<std::size_t>(armyIndex)];

    // Engine ID — machine-owned (STEP76 ruling 2), no input, unchanged from before this ticket.
    ImGui::TextDisabled("%s", army.name.c_str());
    ImGui::SameLine();

    // Name — no visible label; hint text substitutes (ruling 5). displayName only, never `name`.
    TextInputRules displayNameRules;
    displayNameRules.maximumLength = 48;
    displayNameRules.bAllowEmpty   = true;
    DrawTextInput("Name", army.displayName, displayNameRules, WidgetStyle(), "Army Name",
                 /*bLabelHidden=*/true, /*fixedWidthPixels=*/140.0f);
    ImGui::SameLine();

    // STEP250: Army::alias's text box is REMOVED from this tab (ruling 1). Do NOT re-add a
    // DrawTextInput("Alias", ...) call here -- the field itself, its PARAMS member, and its IO
    // round-trip are untouched; only this tab's edit path is gone.

    // Team Color — no label, no RT (ruling 2), fixed small swatch.
    ColorSwatchOptions teamColorOptions = state.armyColorOptions;
    teamColorOptions.bLabelHidden          = true;
    teamColorOptions.bRealtimeToggleHidden = true;
    teamColorOptions.swatchWidth           = 28.0f;
    DrawColorSwatch("Team Color", army.armyColor, teamColorOptions, state.armyColorToggle);
    ImGui::SameLine();

    // Faction — no label (ruling 4), fixed width.
    int factionIndex = static_cast<int>(army.faction);
    ComboOptions factionOptions;
    factionOptions.labels           = armyFactionLabels;
    factionOptions.count            = kArmyFactionCount;
    factionOptions.bLabelHidden     = true;
    factionOptions.fixedWidthPixels = 90.0f;
    if (DrawCombo("Faction", factionIndex, factionOptions).bCommitted)
        army.faction = static_cast<Params::Faction>(factionIndex);
    ImGui::SameLine();

    // Starting Alloys / Starting Energy — keep their labels (explicit instruction), lose RT
    // (ruling 3), compact single-line form.
    ImGui::TextUnformatted("Starting Alloys");
    ImGui::SameLine();
    DrawSliderScalarCompact("Starting Alloys", army.alloys, state.alloysRange, state.alloysToggle,
                            /*trackWidthPixels=*/100.0f, /*fieldWidthPixels=*/60.0f, WidgetStyle(),
                            "%.0f", /*bShowRealtimeToggle=*/false);
    ImGui::SameLine();
    ImGui::TextUnformatted("Starting Energy");
    ImGui::SameLine();
    DrawSliderScalarCompact("Starting Energy", army.energy, state.energyRange, state.energyToggle,
                            /*trackWidthPixels=*/100.0f, /*fieldWidthPixels=*/60.0f, WidgetStyle(),
                            "%.0f", /*bShowRealtimeToggle=*/false);

    // Mirror button — unchanged visibility rule (CanMirrorArmy), just joins the row.
    ImGui::SameLine();
    DrawMirrorArmyButton(armies, armyIndex, state.pendingMirrorSourceArmyIndex,
                         state.mirrorConfirmDialogState);
}
```

Notes for the Coder:
- `DrawTextInput`'s `bLabelHidden`/`fixedWidthPixels` parameters and `DrawSliderScalarCompact`'s
  `bShowRealtimeToggle` are **already shipped** (`TextInput_UI.h`, `SliderScalar_UI.h`) — this ticket
  is the first caller to exercise them in this exact combination, not new plumbing.
- Pixel widths above (140 / 28 / 90 / 100 / 60) are a reasonable starting layout, not measured against
  a live frame — the Coder may adjust them for visual balance (no clipped text, no excess dead space)
  as long as the row stays on one line at the tab's typical width; this is a Visual/Exact-adjacent
  accuracy class (no pipeline correctness at stake), so eyeballing against a live frame is the
  verification method, matching `Combo_UI.cpp`'s own stated policy.
- `state.armyColorOptions` is copied into a local before the two overrides so the shared caller-owned
  `ArmiesTabState` field itself is not mutated (it may be reused elsewhere later; copy-then-override is
  the existing house pattern for a one-off per-call option tweak).
- `DrawArmiesTab`'s outer function (`ArmiesTab_UI.cpp:131-167`) is untouched — this ticket is scoped to
  `DrawArmySettings` only.

**File-size check:** `ArmiesTab_UI.cpp` is 170 lines today; this edit is roughly line-count-neutral
(removes the Alias block, adds a handful of `SameLine()`/option-override lines). Re-check against the
150-line soft ceiling after editing; split into a new `ArmiesTab_RowLayout_UI.h` aspect file (the
`ArmiesTab_Mirror_UI.h`/`ArmiesTab_Units_UI.h` precedent) if it lands over.

---

## 3. Out of scope — do not build, do not stub

- **Removing `Params::Army::alias` itself, or its IO round-trip.** Ruling 1 is UI-only.
- **Adding a `Combo_UI_Test.cpp` render-pixel test** for the new options — `Combo_UI.cpp`'s own header
  comment already exempts this file's draw path from test coverage; the pure logic functions it
  changed nothing about are already covered.
- **Reflowing any other tab** that uses `DrawCombo`/`ComboOptions` — the new fields default to
  today's behavior everywhere else; this ticket touches call sites in `ArmiesTab_UI.cpp` only.
- **Changing `CanMirrorArmy`'s visibility rule** or the mirror confirm dialog itself
  (`ArmiesTab_Mirror_UI.cpp`) — only where the button sits in the row changes.

## 4. Files touched

- EDIT `src/ui/Combo_UI.h` — two new `ComboOptions` fields (§1)
- EDIT `src/ui/Combo_UI.cpp` — two-line `DrawCombo` body change (§1)
- EDIT `src/ui/ArmiesTab_UI.cpp` — `DrawArmySettings` rewrite (§2)

## 5. Verify

- Full solo rebuild + `ctest -C Debug` at 100%, no pre-existing test file edited (this ticket's changes
  are draw-path-only and touch no pure/tested logic beyond `ComboOptions`' two new inert defaults).
- Live frame check (per Combo_UI.cpp's own testing policy): open the Armies tab, expand an army row,
  confirm all controls sit on one visual line at a typical window width, Name shows greyed "Army Name"
  placeholder when empty, no Alias box anywhere, Faction/Team Color show no label text, Starting
  Alloys/Energy keep their labels but show no RT button, Mirror button still only appears on an
  even-indexed row with a successor.
- `grep -n "\"Alias\"" src/ui/ArmiesTab_UI.cpp` returns nothing.
- `grep -rn "army.alias\|army\.alias" src/io` still shows the existing IO read/write lines, unchanged.

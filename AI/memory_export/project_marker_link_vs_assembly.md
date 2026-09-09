---
name: project-marker-link-vs-assembly
description: "Assembly (cross-domain rigid-move tag) vs. the new Link mechanic (Markers-only, cross-type Group grouping) are different features — don't conflate them."
metadata: 
  node_type: memory
  type: project
  originSessionId: c0ade29f-2765-476d-a450-1f7e25895860
  modified: 2026-08-31T23:53:41.440Z
---

Assembly (`work_orders/BRIEF_Assembly_R1.md` / `DESIGN_Assembly_R1.md`) is a real but **unratified**
concept: a scalar `assemblyIdentifier` tag on manual prop/decal/marker instances so they can be
moved/rotated together as a rigid group. It explicitly never reassigns Layer/Bundle membership —
"each member stays on its own original layer" — and has no type-homogeneity requirement.

Link (`work_orders/BRIEF_MarkerLink_R1.md`, drafted 2026-08-31, design pass in progress) is a
**different, Markers-only** mechanic: clicking "+ Link" on a cross-type marker selection mints a
Link id/name, creates a same-named/same-id Group in every Marker-Type section a selected instance
belongs to, and actually moves each instance into its type's copy of that Group. Links also own
propagated settings (color override + toggle confirmed; full list still being ruled) that fan out
to every one of that Link's per-type Groups so they stay in sync. Deleting a Link ungroups (deletes
the per-type Groups, keeps the instances, drops only the link relation).

**Why this matters**: the user's own first instinct was "isn't this an Assembly?" — it isn't. If a
future session sees "Link" and "Assembly" both mentioned near marker grouping, don't merge them or
assume one supersedes the other; they solve different problems (rigid group transform vs. persistent
cross-type Layer/Group membership + settings propagation).

**How to apply**: when picking up Markers Tab UI work, check `work_orders/DESIGN_MarkerLink_R1.md`
for the proposed Link data model before writing any code touching marker Groups/Layers across
multiple Marker Types. See also [[project_workorder_consolidation_2026_08]] for the broader
work-order backlog state.

**Status as of 2026-08-31**: `work_orders/BRIEF_MarkerLink_R1.md` and
`work_orders/DESIGN_MarkerLink_R1.md` are both written and fully advisory-ruled (UI Expert design
pass + ARCH + Format + IO Architecture Expert consults, all in agreement). §1 (Delete key — revised
to be universal across Markers/Props/Decals per direct human pushback, not Markers-only) and §2
(+Group/+Layer move-selection) are coder-dispatchable now, no blocker. §3 (the Link mechanic — new
`Params::MarkerLink` type, `linkIdentifier` back-refs on Bundle+Layer) and §4's new-field half (dual
per-type icon-size scalars, `scaleSelectedAlloy/Plasma/Spawn`) have a COMPLETE advisory design but
are not yet formally dispatchable — the only remaining gap is procedural: writing the advisory
ruling into real `ARCH_19_XX` section files in a dedicated ARCH ratification session (same process
that turned Assembly's own ❓s into `ARCH_19_05`/`06`/`08`). Key ruling worth remembering: Link
color-override/visibility propagate via **read-and-resolve** (Group's own field becomes an inert
mirror while linked) but Link **Name** propagates via a **different** mechanism — a one-shot
cascade-write on rename, after which the Group's name stays independently editable. Don't conflate
the two mechanisms if picking this back up later.

Also surfaced as a side-finding, unrelated to Link itself: the live `DrawMarkersTab`'s outer
Type-section loop is still hardcoded to the fixed 3-entry Alloy/Plasma/Spawn array — the dynamic
enumeration function that implements `ARCH_19_14`/`ARCH_19_15`'s ratified "derive sections from
whatever `markerTypeName` values exist" rule (`DrawMarkerTypeSections` in
`MarkersTab_TypeSections_UI.h`/`.cpp`) has zero live callers anywhere. Custom/`(Unassigned)`
marker-type sections likely don't render in the shipped UI despite ARCH recording that as done —
flag this as a real, separate defect if it comes up.

Also confirmed this session (2026-08-31), independent of Link/Assembly: Marker Layers/Groups are
ratified as one-`markerTypeName`-per-container (`ARCH_19_13`/`ARCH_19_14`) but only **soft**-enforced
(`ARCH_19_12_SoftTypeConsistency.md`) — nothing in code actually blocks a mixed-type Group. And
keyboard Delete is wholly unspecified anywhere in the SanGen UI (no keyboard shortcuts exist at all
today) — the one prior single-instance delete button (`MarkersTab_ManualInstance_UI.cpp`) is dead
code with no live call path.

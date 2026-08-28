[← ARCH index](ARCH.md) · [§21 ARCH_21_CanvasInteractionUnification](ARCH_21_CanvasInteractionUnification.md) · SanGen ARCH §21.4. **Only the ARCH Expert writes this file.**

### 21.4 `PropTransform`/`DecalTransform` gain `instanceIdentifier`/`symmetryGroupIdentifier` — verbatim mirrors of `MarkerTransform`'s own fields

**Human-decided, ratified as the prerequisite for §21.3's Prop/Decal drag consumers and for closing
a live selection-identity bug.** `src/params/PropInstance_PARAMS.h` (confirmed by direct read):
```cpp
struct PropTransform  { InstancedTransform transform; int layerIndex = 0;
                        int instanceIdentifier = -1; int symmetryGroupIdentifier = 0; };
struct DecalTransform { InstancedTransform transform; int layerIndex = 0;
                        int instanceIdentifier = -1; int symmetryGroupIdentifier = 0; };
```
Same defaults, same sentinels, same semantics as `MarkerTransform` (§19.16, §16.5): `instanceIdentifier`
is stable, minted, never reused, `-1` = unassigned, globally unique WITHIN its own domain's roster
(Props and Decals each get their own independent number space — never shared with Markers' or each
other's, exactly as `OverlayInstanceKey_UI::collection` already discriminates domain for the same
reason §19.25 fixed the Markers-only collision). `symmetryGroupIdentifier`: `0` = ungrouped (both
"never grouped" and "Break Symmetry Link" share this sentinel; real group ids start at 1) — unused
by any consumer until §21.3's `PropDragTraits`/`DecalDragTraits` ship.

**Minting.** Per-domain, mirroring `NextMarkerInstanceIdentifier`'s exact shape (`max(instanceIdentifier)
+ 1` scanned across every group's transforms): `NextPropInstanceIdentifier`/`NextDecalInstanceIdentifier`.
**Placed in PARAMS, co-located with the structs they walk** (`PropInstance_PARAMS.h`, beside
`PropTransform`/`DecalTransform`) — NOT mirroring `NextMarkerInstanceIdentifier`'s own UI-layer
placement (`MarkerInstanceId_UI.h`), which §20.2 already recorded as a standing, non-blocking
misplacement by §3.5's own test. These two ship correctly placed from day one, exactly the posture
§20.2 already ruled for the sibling grid-snap/symmetry resolvers.

**Legacy-backfill on import** mirrors §19.16 exactly: an absent `"InstanceIdentifier"` on a
transform assigns the next value of a running counter starting at 0, incremented once per
transform, threaded across the entire nested group-walk for THAT domain only (never reset per
group; never shared with the other domain's counter) — every legacy Prop/Decal transform in the
file gets a fresh, domain-unique id in encounter order.

**Wire keys — deferred to the Format Expert**, same posture as §16.4 handled the Marker
`MarkerGroups`/`MarkerRuleLayer` shape before its own exact spelling landed: `"InstanceIdentifier"`/
`"SymmetryGroupIdentifier"` are the natural candidates (matching `MarkerTransform`'s own spelling
exactly) but are not asserted here as binding. **Additive only, no `SanGenVersion` bump** — same
precedent class as every other new scalar merged directly onto an existing format-native transform
object this whole family of ratifications has used (§16.5, §19.16, §19.11).

**The correctness gap these fields close — confirmed live by direct read of
`MapCanvas_IconLayer_CullManual_UI.cpp`'s `ResolvePropsManual`/`ResolveDecalsManual` (lines 95-120,
208-230): every `ConsiderManualInstance` call for Props/Decals passes the per-group `transforms`
array INDEX as the selection key and never passes `bManual=true`** (the parameter defaults `false`
and is never overridden at either call site) — the exact `OverlayInstanceKey_UI` index-space
collision §19.25 already fixed for Markers (array position vs. a stable minted identity, sharing
one untagged number space under the same `collection` tag), still live and unfixed for Props/Decals
today. **Ruled, not built by this ratification**: once these two fields exist, both call sites must
switch to `propTransform.instanceIdentifier`/`decalTransform.instanceIdentifier` as the key and
pass `/*bManual=*/true` — the exact shape `ResolveMarkersManual` (lines 129-204) already uses. A
coder work-order building §21.4's PARAMS/IO alone does not have to also fix this in the same ticket
(§20.4's own "does not block §20.1-§20.3/§20.5/§20.6" sequencing precedent applies identically
here), but no Prop/Decal SELECTION work (§21.1-§21.3, §21.5, §21.6) may ship without it, since a
stable key is the whole precondition those sections depend on.

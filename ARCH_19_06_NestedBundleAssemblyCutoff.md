[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.6. **Only the ARCH Expert writes this file.**

### 19.6 Nested child Bundle with its own different `assemblyIdentifier` stops the recursive walk there — ratified, new rule
Not present in either prior document; forced by §19.5's recursion and ruled here for the first
time, binding on both Assembly's `CollectAssemblyRecursiveMembership` walk and any future
per-domain Bundle's equivalent walk.

**Ruled: if a nested child Bundle has its OWN `assemblyIdentifier` set to a *different* Assembly
than its ancestor, the walk stops at that child.** It belongs to the other Assembly, full stop —
the same no-multi-membership invariant §19.5 already applies at the Bundle tier, now applied
recursively at *every* tier of nesting, not only checked once at the top of the walk. An untagged
(`-1`) child Bundle is walked through transitively as part of its tagged ancestor — only an
explicit, different, non-`-1` tag on a descendant cuts the walk off.

**Why this is the correct rule, not an arbitrary one.** Without it, a Bundle nested three levels
deep under an Assembly-tagged ancestor Bundle, but itself deliberately re-tagged to a second,
different Assembly, would have its members silently double-counted (or ambiguously attributed) by
whichever Assembly's query ran the walk first — the exact "which Assembly's move wins" ambiguity
`BRIEF_Assembly_R1.md`'s own ground truth already rejected for leaf instances. Applying the same
no-multi-membership discipline at every tier, not only at the leaf, is the only rule consistent
with that already-decided ground truth.

**Implementation shape** (informs, does not re-specify, `CollectAssemblyRecursiveMembership`'s
Bundle-table-walking extension, §19.5): the walk carries the assembly identifier it is currently
resolving; at each Bundle node visited, if that Bundle's own `assemblyIdentifier != -1` AND
`!= callerAssemblyIdentifier`, the walk does not descend into that Bundle's children or fold in
its resolved members — it simply skips that subtree and continues with the next sibling.

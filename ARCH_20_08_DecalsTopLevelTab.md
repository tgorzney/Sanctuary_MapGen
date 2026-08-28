[← ARCH index](ARCH.md) · [§20 ARCH_20_PropsDecalsAuthoringParity](ARCH_20_PropsDecalsAuthoringParity.md) · SanGen ARCH §20.8. **Only the ARCH Expert writes this file.**

### 20.8 Decals is a standalone top-level tab — ratifies the already-shipped split (STEP159), closes a dangling forward-reference
Unrelated to `§20.1`–`§20.7`'s forward-looking rulings — this subsection exists because shipped
source comments already cite it. Confirmed by direct read: `src/ui/DecalsTab_UI.h`/`.cpp` is a
real, already-shipped top-level tab (`DrawDecalsTab`, composing `DecalsTab_Manual_UI.h` +
`DecalsTab_Rules_UI.h`), and `src/ui/PropsTab_UI.h`'s own header comment confirms Decals "were
originally a sub-block of this tab but are now their own standalone tab." A STEP159 comment pass
correctly retired the stale STEP22-era "no separate DecalsTab exists" premise (also relayed,
historically, in `work_orders/DESIGN_Assembly_R1.md`'s open selection-state question) — but that
pass cited a forward-reference, `ARCH_20_DecalsTopLevelTab.md`, that did not exist in the ARCH
pack until this file. **Ratified here, formally, matching what is already true in the live
tree:** Decals owns its own top-level tab, separate from Props; `PropsTab_UI.h` retains the
procedural prop stack and manual prop layers only; `DecalsTab_UI.h` owns the procedural decal
stack and manual decal layers. This does not change any behavior — it is a documentation
ratification of an already-shipped, already-correct fact.

**Standing, non-blocking citation defect (not fixed by this ARCH pass — no code/comment
edits are made by the ARCH Expert):** the three shipped comments
(`src/ui/PropsTab_UI.h:5`, `src/ui/DecalsTab_UI.h:3`, `src/ui/DecalsTab_Manual_UI.h:2-3`) cite
the filename `ARCH_20_DecalsTopLevelTab.md` (no subsection-number infix). The real file this
ruling lands in is `ARCH_20_08_DecalsTopLevelTab.md`, per this pack's established
`ARCH_NN_MM_Topic.md` subsection-naming convention (`ARCH.md`'s own "§14.9 →
`ARCH_14_09_RenderingPerformance.md`" rule). A future coder work-order touching any of these
three files should correct the citation to `ARCH_20_08_DecalsTopLevelTab.md` (§20.8) while it is
there for other reasons — not urgent enough to justify a comment-only ticket on its own.

# Work-Order M4-1 — `EntityIdBuffer_DATA` (picking buffer)

*Constitution §7. Milestone M4 (Preview/WYSIWYG). **BATCH 1 (parallel).** No dependency
on any other M4 work-order; own file. Executor: SanGen Coder.*

## Title
The per-pixel entity-ID buffer that makes picking O(1).

## Root problem
Clicking the preview must resolve which entity is under the cursor without testing 100k
items. The preview writes a per-pixel entity ID; a click reads one cell. That buffer is a
DATA field, produced by the composite (M4-3) and read by picking (M4-4).

## Target files
- `src/data/EntityIdBuffer_DATA.h` (+ `_Test.cpp`).

## Layer & accuracy
`DATA`. Plain SoA buffer, no behavior, no GPU handles (the GPU-side copy is `SYS`'s; this
is the CPU-side readback target).

## Solution
A contiguous `uint32_t` buffer sized to the preview resolution, row-major.
`static constexpr uint32_t emptySentinel = 0xFFFFFFFFu`. `Resize(width,height)`,
`Get(x,y)`, `Set(x,y,id)`, `Data()`, `Clear()` (fill sentinel), `Width/Height`. A click
is `buffer.Get(x,y)`; `emptySentinel` means empty space.

## Acceptance
Resize/clear/get-set; cleared buffer reads `emptySentinel` everywhere; row-major index
correct; ASan/UBSan clean. Files within §1.5 ceilings.

## Out of scope
The composite that fills it (M4-3); picking logic (M4-4).

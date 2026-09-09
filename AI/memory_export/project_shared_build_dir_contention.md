---
name: project-shared-build-dir-contention
description: "Parallel SanGen Coder dispatches on disjoint source files can still collide on the shared build/ output directory (PDB locks, C1041) — a distinct conflict class from source-file edits."
metadata: 
  node_type: memory
  type: project
  originSessionId: 839ff35d-4d20-422c-ace0-33b4c5629979
  modified: 2026-08-30T17:24:23.325Z
---

Two SanGen Coder agents dispatched in parallel (STEP229 and STEP230, 2026-08-30) had fully disjoint
source-file lists and were correctly cleared via peer-session file-conflict checks (see
[[feedback_peer_check_per_file]]), but still hit a real, transient conflict: both were compiling the
same `SanGenV2.vcxproj` into the same shared `build/` output directory at the same time, producing a
`C1041` PDB-lock error across dozens of unrelated files. The STEP230 coder diagnosed it correctly (via
`Get-Process`, spotting another session's MSBuild/cl.exe actively rebuilding), waited for the peer
build to finish, and retried cleanly with no code impact.

**Why this matters:** disjoint source files is not sufficient to guarantee conflict-free parallel
coder dispatch in this repo — the build output directory is itself shared, unversioned state that two
concurrent builds can contend over even when their source edits never overlap.

**How to apply:** when dispatching multiple SanGen Coder agents in parallel in the future, either (a)
warn them in the prompt that a PDB-lock/C1041 error mid-build is likely a benign peer-build collision,
not a real compile failure — retry after a short wait rather than treating it as a bug to fix, or (b)
if build contention becomes frequent, consider per-agent build directories (worktree isolation via the
Agent tool's `isolation: "worktree"` option, or a distinct CMake build dir per agent) as a more robust
fix than relying on retry-on-lock behavior.

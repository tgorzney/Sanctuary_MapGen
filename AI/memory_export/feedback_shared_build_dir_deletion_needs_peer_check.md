---
name: feedback-shared-build-dir-deletion-needs-peer-check
description: "Deleting the repo-root build/ directory during disk cleanup broke another session's/the human's live Visual Studio CMake build, even after checking for locally-running processes."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 878635f5-326e-4217-8fc8-a893492c03ee
  modified: 2026-09-09T00:07:19.980Z
---

During a disk-space cleanup pass (2026-09-08), I deleted the whole repo-root `build/` directory
(`rm -rf build`) after checking `tasklist` for the app executable and waiting for the human to close
it. But Visual Studio itself (`devenv.exe` + ServiceHub processes) was still running the entire time —
I noticed this and only accounted for it locking the small `.vs/` IntelliSense cache, not for the
fact that VS's own live CMake integration was using that `build/` tree as its **active build
configuration** for the main SanGen project. A peer session reported the human's live VS build broke
as a result (`CMakeCache.txt`/`CMakeFiles/`/generated `.vcxproj` files gone out from under it).

**Why this matters:** at repo root, `build/` is not disposable cache the way `.vs/` or `out/` are in
this project — it is shared, actively-rebuilt state that concurrent Claude sessions *and* the human's
own IDE depend on throughout the day (see [[project_shared_build_dir_contention]] for the parallel
build-compile-time version of this same hazard). Checking for a locally-running test executable is not
sufficient evidence that the build directory is safe to delete — devenv.exe being open at all means its
CMake integration may have that exact build tree loaded live, even with no app process running from it.

**How to apply:** before deleting or bulk-clearing the repo-root `build/` directory (or any shared,
non-worktree-local build output) in this repo, either (a) confirm via `ListAgents`/peer messaging that
no other session is mid-build or depends on that tree right now, not just check local `tasklist`, or
(b) ask the human directly whether Visual Studio (or another editor) currently has the CMake project
loaded, not just whether the app is running. Recovery is low-cost (`cmake -S . -B build` regenerates it,
no source lost — `src/` is git-tracked), but it still interrupts whoever was relying on it. Worktree-
local build dirs (e.g. `build_sanmodel_test/` inside an agent worktree) don't carry this risk since
they're private to that worktree.

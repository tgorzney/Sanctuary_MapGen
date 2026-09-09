---
name: project-git-staging-pitfall-full-index-commit
description: "In this multi-session repo, `git add <my files>` then a plain `git commit` can silently sweep in unrelated changes another session already staged — always inspect for pre-existing D/R/M entries in the index before committing, and prefer `git commit -- <pathspec>` to commit a subset."
metadata:
  type: project
  originSessionId: 8dbd531c-49df-4bf7-b903-0ce172d2c517
  modified: 2026-09-06T03:32:11.864Z
---

While following the new agent-commit protocol ([[project_agents_now_commit_protocol]]) for
work_orders/BUGFIX_UniversalCoordinateConversionAndDragRewrite_UI.md, `git status --porcelain` at
the start already showed a staged deletion (`D  work_orders/STEP203_...md`) and two staged renames
(`R  work_orders/SPEC-2_...md -> shipped/...`, same for SPEC-3) — left in the index by another
session, not yet committed. These were correctly identified as "not mine" and deliberately excluded
from `git add`. But `git add <54 files>` followed by a plain `git commit` still committed all 57
changes (54 mine + those 3), because `git add` only ADDS to the index — it doesn't remove or isolate
what's already there, and a pathspec-less `git commit` commits the entire index regardless of who
staged what.

**Why this matters:** in a repo where multiple concurrent sessions may leave WIP changes staged
(not committed) in the shared working tree, "I only `git add`ed my own files" is not sufficient
isolation — the commit itself must also be scoped, or it silently absorbs any other already-staged
work into your commit message/attribution. This was caught only by noticing the reported "57 files
changed" didn't match the intended 54-file list, not by the staging step itself.

**How to apply:** before running a plain `git commit` in this repo, run `git status --porcelain` and
check for ANY `A `/`M `/`D `/`R ` (staged) entries not in your own intended file list — if present,
either commit with an explicit pathspec (`git commit -m "..." -- file1 file2 ...`, which commits
only those paths' content and leaves everything else in the index untouched for its actual owner)
or ask peers before touching anything. The fix used when this was caught after the fact: since the
bad commit was local and unpushed (`git rev-parse @{u}` confirmed `ahead 1`, safe to rewrite), ran
`git reset --soft HEAD~1` (non-destructive — only moves HEAD, index/working tree untouched) then
re-committed with the correct explicit pathspec, leaving the 3 unrelated staged changes exactly as
they were for their owner. `git reset --soft` is only safe to use this way on a commit that is
still local/unpushed — always check `@{u}` / `ahead N` first.

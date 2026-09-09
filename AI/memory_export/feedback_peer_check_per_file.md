---
name: feedback-peer-check-per-file
description: "Work-order session-coordination instructions must require a peer/conflict check before EVERY individual file edit, not just once at ticket/session start."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 839ff35d-4d20-422c-ace0-33b4c5629979
  modified: 2026-08-30T16:45:04.261Z
---

When authoring a work order's "Session coordination" section (checking `ListAgents`/`SendMessage`
peer sessions for file conflicts before a coder edits), the check must be required before **each
individual file edit**, not just once up front before the ticket starts.

**Why:** the user corrected this directly — a multi-file ticket can span a long working session, and
a peer session can start editing any of the ticket's files at any point after an initial one-time
check. A stale "no conflict" from the start of the session is not proof the file is still clear by
the time the coder actually gets to editing it, especially for tickets touching many files (see
[[project_workorder_consolidation_2026_08]] for how this repo's parallel-dispatch conventions already
existed around `EXECUTION_CONFLICT_MAP.md`, but that document only covers static/isolated-worktree
conflict analysis, not live per-file coordination with concurrently-running sessions in a shared
working directory).

**How to apply:** any future work order (or generally, any instruction to a coder/agent that touches
multiple files while peer sessions may be active in the same working directory) should phrase the
coordination requirement as "check before touching file X, then check again before touching file Y,"
not "check once, then edit every file." A one-time pre-check done while drafting the ticket (by the
authoring session) is only a starting point/context note — always state explicitly that it does not
substitute for the coder re-checking per file at actual edit time.

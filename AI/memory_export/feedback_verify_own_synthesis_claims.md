---
name: feedback-verify-own-synthesis-claims
description: "When writing a summary/status doc, verify your own status claims against real evidence before asserting them — don't just trust that a fact \"must be true\" because it fits the narrative."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 79343935-b349-47c3-92aa-7dd2aa8ae463
  modified: 2026-08-22T04:41:48.163Z
---

While synthesizing a large audit (`work_orders/IMPLEMENTATION_STATUS.md`), the assistant wrote
that `STEP51_OverlayLayerDataModel_UI.md` was "already shipped, confirmed IMPLEMENTED" — but
STEP51 was never actually part of the audit batch that produced that document (that batch only
covered STEP1–45; STEP51 is in the STEP46+ range, tracked separately). The claim was asserted
without checking, purely because it read plausibly next to other confirmed-implemented tickets. A
downstream agent later caught it by grepping `src/` directly (zero matches).

**Why:** the user pushed back hard ("Recheck everything") specifically because this was exactly
the class of error the whole multi-session effort had been trying to root out — old text nobody
re-verified, relayed forward as fact. The assistant's own synthesis prose is not exempt from that
same discipline just because *it* wrote it.

**How to apply:** when writing a summary/status document that makes factual claims about
implementation state, commit status, or "X was verified," each claim must trace to an actual
piece of evidence gathered in that same pass (a grep result, a file read, a git command output) —
not to inference from adjacent facts or narrative flow. If a claim can't be traced to real evidence
gathered this session, either verify it before writing it down, or mark it as unverified/inherited
from an external source explicitly. This applies with extra force to any claim used to justify a
subsequent decision (e.g., "since X is shipped, the fix is Y") — those are exactly the claims whose
errors compound.

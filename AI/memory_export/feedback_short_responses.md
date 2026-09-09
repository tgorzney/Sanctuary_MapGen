---
name: feedback-short-responses
description: "User wants short, condensed chat responses: only what they need to know/decide, no internal narration, questions visually distinct"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 95f5d3f8-902f-4d01-94f2-c0cfff2c468f
  modified: 2026-08-19T19:36:55.431Z
---

Keep responses short and condensed by default — avoid long structured write-ups with many headers/tables unless the task genuinely needs that depth (e.g. a formal work-order document itself).

**Why:** Explicit user instruction after a session with several long, heavily-formatted design-discussion and status-report responses (marker UI design, IO-parity status check). Reinforced a second time later the same session with a sharper version: cut anything not meant for the user to read, and make questions visually distinct.

**How to apply:**
- Default to terse prose or short bullet lists in chat. Save detailed structure (tables, multi-section breakdowns) for artifacts/files actually requested (e.g. work-order docs in `work_orders/`), not conversational replies.
- Cut anything that reads as notes-to-self or process narration — internal agent IDs, restating what a tool call did, "let me..." framing, verbose synthesis of background-agent results the user didn't ask to see in full. State conclusions and decisions directly; don't narrate the path to them. If it's internal reasoning, don't mark it — cut it entirely; it never belongs in the response.
- When in doubt, trim.

**Marking scheme (explicit user instruction, applies across sessions/projects, not just this repo):** since this text-only interface can't render actual color, use these fixed prefixes so each line's type is unmistakable at a glance:
- `❓ Question:` — anything needing the user's decision or answer. Never bury a question inside a paragraph of status text.
- `⚠️ Issue:` — a problem, blocker, or discrepancy found, not necessarily needing an immediate answer but the user must see it.
- Everything else: plain, unprefixed, terse text — normal statements/results don't need a marker, only questions and issues do.

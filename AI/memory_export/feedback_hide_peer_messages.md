---
name: feedback-hide-peer-messages
description: "Don't narrate or surface inter-session peer-coordination messages to the user; handle them silently and give a plain status recap when asked."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: c0ade29f-2765-476d-a450-1f7e25895860
  modified: 2026-09-04T03:18:15.925Z
---

Handle cross-session coordination messages (ListAgents/SendMessage traffic with peer Claude Code
sessions on this repo) silently — don't relay their content as chat turns to the user. Peer
coordination ([[feedback_peer_coordination_required]], [[feedback_peer_check_per_file]]) is still
required, but its back-and-forth is internal bookkeeping, not something to narrate.

**Why:** the user said directly, after a string of peer-message acknowledgements cluttered the
conversation: "I do not need to see the messages between other sessions. Now I don't know where we
are in what we were doing." The peer traffic buried the actual task status. The user repeated this
correction a second time (2026-09-03) even after a reply that only gave a one-line summary of a
peer's message ("That was a peer session doing its own pre-edit conflict check... nothing needed
from you there.") — that one-liner was STILL unwanted: "do not display messages or responses to
other sessions to me." The bar is stricter than "don't quote them" — it's "don't mention peer
coordination happened at all" unless it materially changes the user's own task.

**How to apply:** still check `ListAgents` and message peers before touching shared files (this
remains required, [[feedback_peer_coordination_required]]/[[feedback_peer_check_per_file]]), but
treat ALL of it — including even a brief one-line acknowledgement that a peer exchange occurred —
as internal bookkeeping the user's turns should never mention. Do not add a sentence like "that was
a peer session doing X, no action needed" — just silently handle it and move on. The ONLY exception:
when a peer message changes something the user needs to know for their own task (a real file
conflict that blocks or reshapes work they asked for), fold that fact into the next substantive
status update as a plain task-status fact, not framed as "a peer said...". When the user seems to
have lost track of where things stand, proactively give a plain task-status recap with zero
reference to inter-session traffic.

**Repeated a third time (2026-09-03/04, same session as the second correction, later turn):** "Stop
showing me messages from other sessions." This happened again even after the second correction
above, triggered by inbound cross-session messages arriving mid-conversation (a peer confirming
receipt, a peer flagging a future edit and asking to be told when clear) — each got a reply
summarizing the exchange back to the user in chat. The pattern that keeps causing repeat corrections:
an inbound `<cross-session-message>` shows up as a user-facing turn, and the instinct is to
acknowledge/summarize it in the visible response even when only one line. **The actual rule: an
inbound peer message should produce a reply to the peer (via SendMessage) but ZERO visible text to
the user**, not even "map-generator-X confirmed receipt" or "told peer Y it's clear." Treat handling
a peer message as a pure background action with no chat footprint at all — the same as not narrating
a tool call. Only break silence if the peer's content changes the user's own task in a way they need
to act on or know about right now.

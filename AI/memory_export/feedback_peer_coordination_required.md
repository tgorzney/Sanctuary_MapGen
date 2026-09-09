---
name: feedback-peer-coordination-required
description: User requires proactive coordination with other active Claude Code sessions on the same repo before any coding or work-order drafting
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 1020d8fc-2688-405c-b760-425353e898e9
  modified: 2026-08-25T21:43:33.357Z
---

Always communicate with other active/open Claude Code sessions before coding or drafting work orders in this repo — check `ListAgents` for peer sessions, and message them proactively about which files/STEP-number ranges you're about to touch, even if they haven't reached out first.

**Why:** the user runs multiple concurrent Claude Code sessions in the same working directory (`D:\Projects\Sanctuary\Map Generator`) simultaneously, each often focused on a different subsystem. Silent, uncoordinated edits risk file collisions, STEP-number collisions, and wasted duplicate work. The user reinforced this explicitly mid-session as a standing requirement, not a one-off ask ("When doing any coding or drafting of work orders etc, communicate with other active or open sessions").

**How to apply:** before drafting any work order or dispatching any coder work, use `ListAgents` to check for peer sessions, and send them a heads-up naming the specific files/STEP range you're about to claim — don't wait for them to message first. When a peer flags a potential overlap, resolve it explicitly (confirm file lists don't intersect, or negotiate STEP-number ranges) before proceeding. This is a durable practice for this project, not scoped to one conversation.

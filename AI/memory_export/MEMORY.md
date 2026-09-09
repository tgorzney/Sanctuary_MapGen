> Snapshot exported 2026-09-09 from the live Claude Code memory store for this
> project (outside this repo, under the human's Claude Code config). May be
> stale — if you can reach the live store, prefer it. See ../README.md.

- [No manual/interactive testing](feedback_no_manual_testing.md) — never launch/click through the app yourself; verify via automated test binaries and code reading only.
- [Short responses](feedback_short_responses.md) — keep chat replies terse; save long structure for actual documents.
- [Real-world verification gap](project_realworld_verification_gap.md) — 94/94 tests passing didn't mean the app actually works; human's real run found it broken.
- [Texture importer scope](project_texture_importer_scope.md) — future sanpack/icon importer; single game-root picker, real paths noted, deferred conversation.
- [Work-order consolidation state](project_workorder_consolidation_2026_08.md) — 5 sessions merged, 91/93 foundation tickets confirmed built, real backlog is STEP46+, audit re-verified twice.
- [Verify own synthesis claims](feedback_verify_own_synthesis_claims.md) — a status doc's own prose needs the same evidence discipline as any other claim; caught asserting "shipped" without checking.
- [Prop InstanceId game-load confirmed](project_prop_instanceid_gameload_confirmed.md) — live test proved .sanmap tolerates new per-instance fields on props; de-risks Assembly design.
- [Peer coordination required](feedback_peer_coordination_required.md) — proactively message other active Claude Code sessions before any coding/drafting, don't wait for them to reach out.
- [Import leaves layers empty](project_import_layers_empty_bug.md) — FIXED (STEP115), was: real (non-SanGen) maps imported with zero marker/prop/decal layers.
- [Peer check per file](feedback_peer_check_per_file.md) — work-order coordination sections must require a peer check before EVERY file edit, not just once at ticket start.
- [Shared build dir contention](project_shared_build_dir_contention.md) — disjoint source files still collide on shared build/ output (PDB locks); needs its own handling separate from file checks.
- [No click-sim tests](feedback_no_click_sim_tests.md) — never run GL-backed click-simulation test binaries; they interrupt the user's own Visual Studio debug session.
- [Hide peer messages](feedback_hide_peer_messages.md) — don't narrate inter-session coordination chatter to the user; act on it silently, recap task status instead.
- [Agents now commit (2026-09)](project_agents_now_commit_protocol.md) — CLAUDE.md's law changed: agents commit ratified work under a strict 5-step protocol; verify live, don't assume old "human commits" rule.
- [Git staging pitfall](project_git_staging_pitfall_full_index_commit.md) — a plain `git commit` sweeps in ANY already-staged peer changes; check index first, or commit with an explicit pathspec.
- [Shared build/ deletion needs peer check](feedback_shared_build_dir_deletion_needs_peer_check.md) — deleting repo-root build/ broke a live VS build; local tasklist check wasn't enough, ask peers/human first.

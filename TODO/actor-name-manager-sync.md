# actor-name-manager-sync

## Problem

`Python/helpers/actor_name_manager.py` keeps a Python-side cache of spawned actor names that never reconciles with the actual level:

- After a server restart or in-editor actor deletion, the cache is stale (`:72-100`); uniqueness checks pass on names that exist, or block names that don't.
- Removal is manual-only via `remove_actor()` (`:106-108`) — nothing calls it when actors are deleted by other means, so the set grows unbounded.
- Session ID is the last 6 digits of epoch time (`:21`) — collidable.
- Concurrent spawns race on the check-then-spawn pattern (`:168-181`); "already exists" handling is recovery, not prevention.

## Fix

Preferred: **delete the class.** Have the C++ `SpawnActor` handler guarantee uniqueness — Unreal already auto-uniquifies actor labels (`SetActorLabel` with a requested name returns the final label). Return the final name in the spawn response and let Unreal be the single source of truth.

Fallback (if keeping Python-side naming): seed the cache from `get_actors_in_level` at first use, and re-verify against Unreal on any "already exists" spawn failure.

## Acceptance

- Restarting the Python server, deleting actors in-editor, then spawning with a previously used base name produces no silent failures or duplicate-label surprises.

# add-spawn-actors-batch

**Depends on:** `fix-cpp-message-framing` (batch payloads exceed the current 8KB single-recv limit).

## Problem

Every helper spawns one actor per TCP command. A castle is ~800–1000 sequential `spawn_actor` calls (`Python/helpers/castle_creation.py:73-165` etc.), a mansion ~500–700, a town more. Each call pays a fresh TCP connection plus a ~50ms average accept-poll delay on the C++ side (see `persistent-tcp-connection`). This is why the 300-second timeout tier exists for `create_town` / `create_castle_fortress` / etc. — most of that time is protocol overhead, not Unreal work.

## Fix

1. **C++:** Add a `SpawnActorsBatch` command to `FEpicUnrealMCPEditorCommands` taking an array of spawn specs (`ActorClass`/mesh, `Name`, `Location`, `Rotation`, `Scale`, material params). Execute the whole array in a single game-thread task; return per-item success plus a summary count. Route it in `EpicUnrealMCPBridge.cpp`.
2. **Python:** Add a `spawn_actors_batch` path in the server, then rewrite the construction helpers (`castle_creation.py`, `mansion_creation.py`, `building_creation.py`, `bridge_aqueduct_creation.py`, `house_construction.py`, `advanced_buildings.py`, maze/town code) to accumulate spawn specs into a list and issue one (or a few chunked) batch calls.
3. Consider chunking batches at ~500 items to keep individual responses bounded.

## Notes

- Combine with `fix-cpp-message-framing` first — batch request payloads will far exceed the current 8KB single-recv limit.
- After this lands, the 300s `LARGE_OPERATION_COMMANDS` timeout tier can likely be reduced or removed.

## Acceptance

- `create_castle_fortress` issues ≤ a handful of TCP commands and completes in seconds, not minutes.

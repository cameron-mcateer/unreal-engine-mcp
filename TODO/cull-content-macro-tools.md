# cull-content-macro-tools

## Problem

Roughly 14 of the 54 MCP tools are canned content macros: `create_town`, `create_castle_fortress`, `create_suspension_bridge`, `create_aqueduct`, `construct_mansion`, `create_maze`, `create_pyramid`, `create_tower`, `create_wall`, `create_arch`, `create_staircase`, `construct_house`, `spawn_physics_blueprint_actor`, etc. They are hardcoded Python loops over `spawn_actor` (e.g. `Python/helpers/castle_creation.py:73-165` is four near-identical wall loops; the same wall pattern is duplicated in `mansion_creation.py:153-201`).

For the actual use case — an agent designing games — these are the wrong layer: they eat tool-list budget and constrain output to canned shapes the agent could compose itself once `add-spawn-actors-batch` exists.

## Fix

Either:
- **Delete them** and let the agent compose structures from `spawn_actor` / `spawn_actors_batch` primitives, or
- **Consolidate** all of them behind a single `build_structure(kind: str, params: dict)` tool with a compact docstring listing the kinds, keeping the helper modules as the implementation.

Also dedupe the repeated wall-building loops across `castle_creation.py` / `mansion_creation.py` / `house_construction.py` into a shared helper if the modules are kept.

## Acceptance

- Tool count drops from 54 to ~40 or fewer; total docstring weight drops accordingly (currently ~41k chars / ~10.3k tokens across all tools).

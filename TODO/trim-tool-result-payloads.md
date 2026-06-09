# trim-tool-result-payloads

## Problem

Several MCP tools return unbounded payloads straight into the agent's context:

- `create_castle_fortress` (`Python/unreal_mcp_server_advanced.py:1912`) and `create_town` (`:1849`) return `"actors": all_actors` — the full per-actor dict list (name, type, location, scale) for 500–1000 actors. One castle call injects ~100KB of JSON the agent will never use.
- `create_maze`, `construct_mansion`, `construct_house`, and the other builder tools follow the same pattern (full `actors` list in the result).
- `read_data_table`, `get_actors_in_level`, and `analyze_blueprint_graph` have the same unbounded-payload shape with no pagination or summarization option.

## Fix

- Replace `"actors": all_actors` with a summary: `total_actors`, bounding box / footprint, and a handful of sample names (or just the name prefix used, since names are generated from a known pattern).
- For inspection tools (`get_actors_in_level`, `read_data_table`, `analyze_blueprint_graph`), add `limit`/`offset` params or a `summary_only` flag defaulting to a compact form.

## Acceptance

- A `create_castle_fortress` call returns < 1KB of JSON.
- Agent can still find spawned actors afterwards (via name prefix + `find_actors_by_name`).

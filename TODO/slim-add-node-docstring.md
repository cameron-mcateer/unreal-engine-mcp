# slim-add-node-docstring

## Problem

`add_node` in `Python/unreal_mcp_server_advanced.py` has an 11,554-character docstring (~2.9k tokens for one tool, 16 params) — it's an encyclopedia of every NodeType and its Properties. The whole 54-tool surface carries ~41k chars (~10.3k tokens) of docstrings, and this one tool is ~28% of it. MCP clients ship every docstring into the agent's context whether or not Blueprint editing is used.

## Fix

- Keep in the docstring: the NodeType enum (names only), the common params, and **one** representative example.
- Move per-node-type details (required Properties, pin names, examples) into an on-demand tool: `get_node_type_help(node_type: str) -> str`, backed by a dict in a helper module. The agent calls it only when it actually needs a node type's specifics.
- Apply the same treatment to `set_node_property` (4.3k chars) if it shares the per-type documentation.

## Acceptance

- `add_node` docstring ≤ ~2k chars with no loss of discoverability (everything removed is reachable via `get_node_type_help`).

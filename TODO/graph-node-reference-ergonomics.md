# graph-node-reference-ergonomics

## Problem

Blueprint graph editing forces the agent to babysit brittle identifiers:

- `add_node` returns a node **GUID** the agent must retain to later call `connect_nodes` (`Python/helpers/blueprint_graph/connector_manager.py:13-86`). After a long session / context compaction, the GUIDs are gone and the agent must re-dump the graph via `analyze_blueprint_graph` to recover.
- Pin names must exactly match the C++ `UFunction` signature ("execute", "InString" — `connector_manager.py:17,113,183`) with no discovery or fuzzy matching; a wrong pin name only fails at the C++ layer.

## Fix

1. **`add_node` response:** include the created node's title and its full pin list (names + directions + types) so the agent learns the wiring surface at creation time without a follow-up query.
2. **`connect_nodes`:** accept node *titles/names* (resolved C++-side within the target graph, erroring on ambiguity) in addition to GUIDs.
3. On pin-name mismatch, have the C++ error message list the actual available pins on both nodes instead of a bare failure.

## Acceptance

- An agent can wire two nodes it created earlier in the session using only human-readable names, and a typo'd pin name returns the valid pin list in the error.

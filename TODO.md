# TODO

Design-review follow-ups. This list is the source of truth for overall priority: items are ordered highest-priority first. Items with a `depends on` attribute must not be started (or re-ordered to run) before the items they depend on. Details for each item live in `TODO/<item-id>.md`.

- [ ] **trim-tool-result-payloads** — Return summaries instead of full actor lists from build tools; biggest agent-context win
- [ ] **fix-cpp-message-framing** — Newline-delimit/length-prefix the TCP protocol; C++ currently drops any command over 8KB
- [ ] **add-spawn-actors-batch** — Add a SpawnActorsBatch command so builders issue one round trip instead of hundreds
  - depends on: `fix-cpp-message-framing`
- [ ] **persistent-tcp-connection** — Stop reconnecting per command; removes the ~50ms accept-poll tax on every call
  - depends on: `fix-cpp-message-framing`
  - pairs with: `socket-cleanup-on-disconnect` (do together)
- [ ] **socket-cleanup-on-disconnect** — Reset ClientSocket on disconnect and handle dead clients before going persistent
- [ ] **cull-content-macro-tools** — Remove or consolidate the ~14 canned builder tools (castle, town, maze, …) into one
- [ ] **slim-add-node-docstring** — Cut add_node's 11.5k-char docstring; move node-type details to an on-demand help tool
- [ ] **consolidate-variable-properties-params** — Collapse set_blueprint_variable_properties' 23 params into a properties dict
- [ ] **graph-node-reference-ergonomics** — Let connect_nodes accept node names and return pin names from add_node
- [ ] **actor-name-manager-sync** — Fix or delete the Python-side actor-name cache that desyncs from the level
- [ ] **registry-command-routing** — Replace if/else command routing chains with registries (Bridge, handlers, NodeManager)
- [ ] **unify-response-envelope** — Standardize on one response shape; handlers and bridge currently disagree

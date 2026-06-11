# registry-command-routing

Maintainability item — pick up when next working in the C++ code anyway.

## Problem

Command routing is hand-written if/else chains at every layer:

- `EpicUnrealMCPBridge.cpp:265-330` — ~25 branches routing ~44 commands to handler classes.
- `EpicUnrealMCPBlueprintCommands.cpp:41-116` — 18+ branches; `EpicUnrealMCPBlueprintGraphCommands.cpp:22-75` — 13 branches; `EpicUnrealMCPEditorCommands.cpp:32-66` — ~7.
- `NodeManager.cpp:122-249` — a 33-branch chain dispatching node types to creator classes.

Adding a command or node type means editing a giant dispatch function (CLAUDE.md even documents "add routing before the final error return" as a required step). Worse, each command string is bookkept in **two** places: the handler's chain and the bridge's per-handler forwarding list — adding `get_world_settings` required touching both, and forgetting the bridge entry yields "unknown command" even though the handler exists.

## Fix

Decision (2026-06-11): constructor-populated registry maps, **not** macro/static-init auto-registration. Auto-registration (FAutoConsoleCommand-style) only pays off if external modules need to register commands; for internal cleanup it adds static-init-order/lifetime machinery and hurts grep-ability. The registry data structure is the same either way, so migrating later is cheap.

- Replace each handler's chain with a `TMap<FString, TFunction<TSharedPtr<FJsonObject>(const TSharedPtr<FJsonObject>&)>>` populated once in the constructor (`Commands.Add(TEXT("foo"), [this](const auto& P){ return HandleFoo(P); });`).
- Bridge dispatch: merge the handlers' maps (or probe each handler "do you handle X") so the bridge's per-handler string lists at `EpicUnrealMCPBridge.cpp:265-330` are deleted and the bridge never needs editing when a command is added.
- For `NodeManager`, a registry mapping NodeType string → creator function; new node-creator files are added as one table line.
- Optional, cheap once the registry exists: per-command metadata (e.g. timeout class — currently a hardcoded name list in Python) and a `list_commands` introspection command that iterates the registry.
- Update the CLAUDE.md "Adding a New MCP Tool" / "Adding a New Blueprint Node Type" steps to match.

Out of scope: the Python side keeps one `@mcp.tool()` function per command — each tool needs its own signature/docstring for MCP, so that duplication is inherent.

## Acceptance

- Adding a new command/node type touches one table entry plus the new handler — no dispatch-function edits, no bridge edits.
- Each command string appears exactly once in C++.

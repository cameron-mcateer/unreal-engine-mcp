# registry-command-routing

Maintainability item — pick up when next working in the C++ code anyway.

## Problem

Command routing is hand-written if/else chains at every layer:

- `EpicUnrealMCPBridge.cpp:265-330` — ~25 branches routing ~44 commands to handler classes.
- `EpicUnrealMCPBlueprintCommands.cpp:41-116` — 18+ branches; `EpicUnrealMCPBlueprintGraphCommands.cpp:22-75` — 13 branches; `EpicUnrealMCPEditorCommands.cpp:32-66` — ~7.
- `NodeManager.cpp:122-249` — a 33-branch chain dispatching node types to creator classes.

Adding a command or node type means editing a giant dispatch function (CLAUDE.md even documents "add routing before the final error return" as a required step).

## Fix

- Replace each chain with a `TMap<FString, TFunction<TSharedPtr<FJsonObject>(const TSharedPtr<FJsonObject>&)>>` (or member-fn-pointer map) populated once in the constructor.
- For `NodeManager`, a static registry mapping NodeType string → creator function; new node-creator files self-register or are added to one table line.
- Update the CLAUDE.md "Adding a New Blueprint Node Type" steps to match.

## Acceptance

- Adding a new command/node type touches one table entry plus the new handler — no dispatch-function edits.

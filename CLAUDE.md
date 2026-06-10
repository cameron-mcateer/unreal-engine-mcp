# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Running the MCP Server

The Python server lives in `Python/`. Install dependencies and run it:

```bash
cd Python
pip install mcp[cli] fastmcp uvicorn fastapi "pydantic>=2.6.1" requests
python unreal_mcp_server_advanced.py
```

Or configure it as an MCP server in your client config:
```json
{
  "mcpServers": {
    "unreal-advanced": {
      "command": "python",
      "args": ["/absolute/path/to/Python/unreal_mcp_server_advanced.py"]
    }
  }
}
```

**Requirements**: Python >=3.10, <3.14. Unreal must be open with the plugin enabled before tools can be invoked.

### WSL / Remote Setup

The Python MCP server can run in WSL while the Unreal plugin runs on the Windows host. Set `UNREAL_HOST` to the Windows host IP:

```bash
# Find the Windows host IP from WSL
export UNREAL_HOST=$(ip route show default | awk '{print $3}')
export UNREAL_PORT=55557  # optional, this is the default
python unreal_mcp_server_advanced.py
```

Or in the MCP client config:
```json
{
  "mcpServers": {
    "unreal-advanced": {
      "command": "python",
      "args": ["/absolute/path/to/Python/unreal_mcp_server_advanced.py"],
      "env": {
        "UNREAL_HOST": "<windows-host-ip>"
      }
    }
  }
}
```

On the **Windows host**, two things are needed:

1. **Plugin bind address**: In Unreal, go to **Project Settings > Plugins > UnrealMCP** and set `Bind Address` to `0.0.0.0` (accepts connections from all interfaces, including WSL). The default is `127.0.0.1` (localhost only).

2. **Firewall rule**: Allow inbound TCP on the MCP port. From an **elevated PowerShell**:
   ```powershell
   New-NetFirewallRule -DisplayName "UnrealMCP" -Direction Inbound -Protocol TCP -LocalPort 55557 -Action Allow
   ```

## Building the C++ Plugin

Copy the `UnrealMCP/` directory into your Unreal project's `Plugins/` folder, then open the project in UE5.5+ and compile from the editor (Build > Build Solution), or use Unreal's `GenerateProjectFiles` script then build with Visual Studio / Rider. There is no standalone CMake or Makefile — the plugin is compiled as part of an Unreal project.

## Architecture

```
AI Client (Claude / Cursor / Windsurf)
    ↓  MCP Protocol
Python/unreal_mcp_server_advanced.py   ← all MCP tool definitions (~2800 lines)
Python/helpers/                         ← helper modules invoked by the server
    ↓  TCP JSON  (127.0.0.1:55557)
UnrealMCP C++ plugin
    EpicUnrealMCPBridge (UEditorSubsystem) ← routes commands to 3 handlers
        FEpicUnrealMCPEditorCommands        ← actors, transforms, level ops
        FEpicUnrealMCPBlueprintCommands     ← Blueprint asset creation/compilation
        FEpicUnrealMCPBlueprintGraphCommands← Blueprint graph editing
            NodeManager → per-type creator classes (ControlFlowNodes, MathNodes, …)
```

**Wire format** — newline-delimited JSON in both directions: each message is one line of condensed JSON terminated by `\n`. Python sends:
```json
{"type": "CommandName", "params": { … }}
```
(C++ also accepts `"command"` as an alias for `"type"`.) C++ responds:
```json
{"status": "success", "result": { … }}   // or {"status": "error", "error": "…"}
```

Timeouts: 30 s default; 300 s for large ops (`create_town`, `create_castle_fortress`, `create_suspension_bridge`, `create_aqueduct`, `construct_mansion`, `create_maze`).

## Adding a New MCP Tool

1. **Python tool definition** (`Python/unreal_mcp_server_advanced.py`):
   ```python
   @mcp.tool()
   async def tool_name(param1: str, param2: int = 100) -> Dict[str, Any]:
       """Description. Args: … Returns: …"""
       try:
           result = await send_unreal_command("UnrealCommandName", {
               "Param1": param1, "Param2": param2
           })
           return {"success": True, "result": result}
       except Exception as e:
           return {"success": False, "error": str(e)}
   ```

2. **C++ command handler** — add a handler method to the appropriate class (`FEpicUnrealMCPEditorCommands`, `FEpicUnrealMCPBlueprintCommands`, or `FEpicUnrealMCPBlueprintGraphCommands`) and route `"UnrealCommandName"` to it in `EpicUnrealMCPBridge.cpp`.

3. **Helper module** (if logic is complex) — create `Python/helpers/my_module.py` and import it from the server.

## Adding a New Blueprint Node Type

1. Create `Public/Commands/BlueprintGraph/Nodes/FooNodes.h` with class `FFooNodeCreator`
2. Create `Private/Commands/BlueprintGraph/Nodes/FooNodes.cpp` implementing the creator
3. `#include "…/FooNodes.h"` in `NodeManager.cpp`
4. Add routing `else if (NodeType.Equals(TEXT("Foo"), …))` in `NodeManager.cpp` before the final error return
5. Add a convenience wrapper in `Python/helpers/blueprint_graph/node_manager.py`
6. Add the new `NodeType` value to the `add_node` tool docstring in `unreal_mcp_server_advanced.py`

**Math node pattern** — use `UK2Node_PromotableOperator` with `SetFromFunction(UFunction*)` called **before** `PostPlacedNewNode()` and `AllocateDefaultPins()`. UE5 uses doubles internally; prefer `_DoubleDouble` function name variants over `_FloatFloat`.

## Key Files

| File | Purpose |
|---|---|
| `Python/unreal_mcp_server_advanced.py` | All MCP tool definitions, TCP connection management |
| `Python/helpers/blueprint_graph/node_manager.py` | Python-side node creation entry point |
| `Python/helpers/blueprint_graph/connector_manager.py` | Wiring nodes together |
| `Python/helpers/blueprint_graph/variable_manager.py` | Blueprint variable creation |
| `UnrealMCP/Source/UnrealMCP/Private/Commands/BlueprintGraph/NodeManager.cpp` | C++ node-type routing |
| `UnrealMCP/Source/UnrealMCP/Public/Commands/BlueprintGraph/Nodes/` | Node creator class headers |
| `UnrealMCP/Source/UnrealMCP/UnrealMCP.Build.cs` | Plugin module dependencies |

## Common Command Patterns

```python
# Spawn an actor
result = await send_unreal_command("SpawnActor", {
    "ActorClass": "/Game/Path/To/Blueprint",
    "Location": {"X": 0, "Y": 0, "Z": 100},
    "Rotation": {"Pitch": 0, "Yaw": 0, "Roll": 0},
    "Scale": {"X": 1, "Y": 1, "Z": 1}
})

# Add a Blueprint graph node
result = await send_unreal_command("AddNode", {
    "BlueprintPath": "/Game/Blueprints/BP_Example",
    "NodeType": "Branch",
    "GraphName": "EventGraph",
    "LocationX": 100, "LocationY": 200,
    "Properties": {}
})
```

## Unimplemented / Future Work

- **Blueprint nodes**: Custom events, Timeline editing, String/Array operation nodes, Animation Blueprint support
- **Asset management**: Import/export assets, create materials programmatically, texture manipulation, static mesh creation/modification
- **Level/World**: Level streaming, world composition, lighting manipulation, post-process volume control
- **Gameplay systems**: Collision preset management, input action/axis binding, game mode/state manipulation, Widget/UMG creation
- **Editor operations**: Content browser operations, viewport camera control, Play-In-Editor automation, package/build automation

## TODO Tracking

Work items live in `TODO.md` (index) plus one detail file per item in `TODO/<item-id>.md`.

- **Item ids** contain only `a-z`, `0-9`, and `-`, and double as the detail filename.
- **`TODO.md` is the source of truth for priority**: items are ordered highest-priority first. Each entry is a single checkbox line — `- [ ] **<item-id>** — one-line summary` — optionally followed by indented attribute sub-bullets:
  - `depends on: <item-id>` — this item must not be started, or re-ordered to run, before the listed item.
  - `pairs with: <item-id>` — best landed together, but neither blocks the other.
- **Detail files** (`TODO/<item-id>.md`) hold everything else: a `## Problem` section with `file:line` references, a `## Fix` section, and `## Acceptance` criteria. Do **not** put priority ranks in detail files — priority is expressed only by ordering in `TODO.md`.

When adding an item: create `TODO/<item-id>.md` first, then insert its line into `TODO.md` at the right priority position (never above anything it depends on). When completing an item: remove its line from `TODO.md` and delete its detail file — completed items live in git history, not in the list. Also remove any `depends on:` / `pairs with:` references to the completed item from remaining entries.

## Logs & Debugging

- Server log: `Python/unreal_mcp_advanced.log`
- Port 55557 must be free; Unreal must have the plugin loaded before the Python server can connect
- See `DEBUGGING.md` for common setup issues (missing `mcp` module, config file location, etc.)
- UE5.6 engine source is readable from WSL at `/mnt/d/Epic Games/UE_5.6/`

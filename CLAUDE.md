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

**Wire format** — Python sends:
```json
{"command": "CommandName", "params": { … }}
```
C++ responds:
```json
{"success": true, "data": { … }}   // or {"success": false, "error": "…"}
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

## Logs & Debugging

- Server log: `Python/unreal_mcp_advanced.log`
- Port 55557 must be free; Unreal must have the plugin loaded before the Python server can connect
- See `DEBUGGING.md` for common setup issues (missing `mcp` module, config file location, etc.)
- UE5.6 engine source is readable from WSL at `/mnt/d/Epic Games/UE_5.6/`

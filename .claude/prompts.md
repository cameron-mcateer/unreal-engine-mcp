# Unreal Engine MCP Server - Project Context

## Project Overview

This is an **MCP (Model Context Protocol) server** that enables AI clients (Claude, Cursor, Windsurf) to control Unreal Engine 5.5+ through natural language commands. The system allows autonomous AI agents to build 3D worlds, edit Blueprints, create complex architectural structures, and manage game assets.

## Architecture

```
AI Client (Claude/Cursor/Windsurf)
    ↓ MCP Protocol
Python Server (unreal_mcp_server_advanced.py)
    ↓ TCP Socket (port 55557)
C++ Plugin (UnrealMCP)
    ↓ Native Unreal API
Unreal Engine 5.5+ (Editor & Runtime)
```

### Key Components

1. **Python Server** (`Python/unreal_mcp_server_advanced.py`)
   - FastMCP-based server implementing MCP protocol
   - TCP socket communication with Unreal plugin
   - Advanced world-building algorithms
   - Blueprint graph manipulation logic
   - Connection management with retry/reconnection

2. **C++ Unreal Plugin** (`UnrealMCP/`)
   - Native Unreal Engine plugin
   - TCP server listening on port 55557
   - Direct access to Unreal Editor APIs
   - Actor, component, and Blueprint management
   - Material and physics systems

3. **Helper Modules** (`Python/helpers/`)
   - `blueprint_graph/` - Blueprint node creation, connections, variables, functions
   - `castle_creation.py` - Medieval fortress generation
   - `house_construction.py` - Building generation
   - `mansion_creation.py` - Complex mansion structures
   - `infrastructure_creation.py` - Town/city systems
   - `actor_utilities.py` - Actor spawning and management
   - `bridge_aqueduct_creation.py` - Large structures

## Core Capabilities

### 1. Blueprint Visual Scripting
- **23+ node types** across 6 categories (Control Flow, Data, Casting, Utility, Specialized, Animation)
- **Graph management**: Create/delete/connect nodes, manage execution flow
- **Variable system**: Full property control (public/private, replication, tooltips, ranges)
- **Function management**: Custom functions with inputs/outputs
- **Analysis tools**: Read Blueprint content, inspect graphs, analyze execution flow

### 2. World Building
- **Town/city generation**: Street grids, buildings, decorations, infrastructure
- **Architecture**: Houses, mansions, towers, arches, staircases
- **Epic structures**: Castles, fortresses, suspension bridges, aqueducts
- **Level design**: Mazes, pyramids, walls

### 3. Actor & Material Management
- Spawn/delete actors
- Transform manipulation (location, rotation, scale)
- Material application and color modification
- Physics properties and simulations

## Development Guidelines

### Adding New MCP Tools

When adding new MCP tools to give Claude more autonomy:

1. **Define the tool in `unreal_mcp_server_advanced.py`**:
   ```python
   @mcp.tool()
   async def tool_name(
       param1: str,
       param2: int = 100
   ) -> Dict[str, Any]:
       """
       Clear description of what the tool does.

       Args:
           param1: Description of param1
           param2: Description of param2 (default: 100)

       Returns:
           Dictionary with 'success' and results
       """
       try:
           result = await send_unreal_command("UnrealCommandName", {
               "Param1": param1,
               "Param2": param2
           })
           return {"success": True, "result": result}
       except Exception as e:
           return {"success": False, "error": str(e)}
   ```

2. **Implement C++ handler in UnrealMCP plugin** (if new command needed):
   - Add command handler to process JSON requests
   - Use Unreal Engine APIs to perform actions
   - Return JSON response

3. **Add helper functions** (if complex):
   - Create module in `Python/helpers/`
   - Keep logic modular and reusable
   - Document parameters and return values

### Communication Protocol

**Python → Unreal**:
- JSON command format: `{"command": "CommandName", "params": {...}}`
- TCP socket on 127.0.0.1:55557
- Automatic retry with exponential backoff

**Unreal → Python**:
- JSON response format: `{"success": true/false, "data": {...}, "error": "..."}`
- Large operations (towns, castles) may take 30-300 seconds

### Common Patterns

#### Spawning Actors
```python
result = await send_unreal_command("SpawnActor", {
    "ActorClass": "/Game/Path/To/Blueprint",
    "Location": {"X": 0, "Y": 0, "Z": 100},
    "Rotation": {"Pitch": 0, "Yaw": 0, "Roll": 0},
    "Scale": {"X": 1, "Y": 1, "Z": 1}
})
```

#### Blueprint Node Creation
```python
result = await send_unreal_command("AddNode", {
    "BlueprintPath": "/Game/Blueprints/BP_Example",
    "NodeType": "Branch",
    "GraphName": "EventGraph",
    "LocationX": 100,
    "LocationY": 200,
    "Properties": {...}
})
```

## Key Files to Know

- `Python/unreal_mcp_server_advanced.py` - Main MCP server, all tool definitions
- `Python/helpers/blueprint_graph/node_manager.py` - Blueprint node creation
- `Python/helpers/blueprint_graph/connector_manager.py` - Node connections
- `Python/helpers/blueprint_graph/variable_manager.py` - Blueprint variables
- `Python/helpers/blueprint_graph/function_manager.py` - Function management
- `Python/helpers/castle_creation.py` - Complex structure generation example
- `README.md` - User-facing documentation
- `Guides/blueprint-graph-guide.md` - Blueprint programming guide
- `Guides/tools-reference.md` - Complete tool reference

## Current Limitations & Improvement Areas

Based on the README, potential areas for enhancement:

1. **Blueprint System**:
   - More node types (Math, String, Array operations)
   - Custom event creation
   - Component events (OnBeginOverlap, OnHit, etc.)
   - Timeline editing
   - Animation blueprint support

2. **Asset Management**:
   - Import/export assets
   - Create materials programmatically
   - Texture manipulation
   - Static mesh creation/modification

3. **Level/World Management**:
   - Level streaming
   - World composition
   - Lighting manipulation
   - Post-process volume control

4. **Gameplay Systems**:
   - Collision preset management
   - Input action/axis binding
   - Game mode/state manipulation
   - Widget/UMG creation

5. **Editor Operations**:
   - Content browser operations
   - Viewport camera control
   - Play-in-editor automation
   - Package/build automation

## Testing & Debugging

- **Server logs**: `Python/unreal_mcp_advanced.log`
- **Test connection**: Ensure Unreal project is open with plugin enabled
- **Port conflicts**: Check port 55557 is available
- **MCP client config**: Verify paths in `mcp.json` are absolute
- **See `DEBUGGING.md`** for common issues

## Community Resources

- Discord: https://discord.gg/3KNkke3rnH
- YouTube: https://youtube.com/@flopperam
- Twitter: https://twitter.com/Flopperam
- Documentation: https://flopperam.com/docs

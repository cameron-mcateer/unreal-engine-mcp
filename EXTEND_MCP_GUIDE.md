# MCP Server Extension Guide

**For the Claude instance working on the game project:**

You're working with an Unreal Engine project that has MCP (Model Context Protocol) integration. When you encounter tasks you cannot complete due to MCP server limitations, generate a prompt for the developer to extend the server.

## When to Generate Extension Prompts

Generate an extension prompt when:
- You need to read/modify Unreal Engine data that isn't accessible via existing tools
- You hit errors about missing MCP tools or capabilities
- You need to perform operations on Blueprints, actors, or assets that aren't supported
- You need Editor functionality (viewport control, content browser, etc.) that doesn't exist
- You need gameplay system access (input, collision, game mode, UI) that's missing

## Prompt Template to Generate

When you hit a limitation, generate this exact format:

```
**MCP EXTENSION NEEDED**

**What I was trying to do:**
[Describe the game development task in 1-2 sentences]

**What's missing from MCP:**
[Specific capability that doesn't exist - be precise about what Unreal API or feature is needed]

**Current MCP tools tried:**
[List which MCP tools you attempted to use and why they didn't work]

**Proposed solution:**
[Suggest the new MCP tool name and what it should do]

**Example usage:**
[Show how the new tool would be called to solve this specific problem]

**Unreal Engine APIs needed:**
[If known, mention which UE C++ classes or Blueprint functions this would need to access]
```

## Current MCP Server Capabilities

The MCP server currently supports:

### Blueprint Operations
- Creating/compiling Blueprints
- Adding nodes (23+ types: Branch, Variable Get/Set, Function calls, etc.)
- Connecting nodes
- Creating/managing variables with full properties
- Creating/managing functions with inputs/outputs
- Reading Blueprint content and analyzing graphs
- Adding components to Blueprints

### World Building
- Spawning actors
- Creating complex structures (towns, castles, houses, mazes, bridges)
- Transform operations (move, rotate, scale)
- Deleting actors
- Querying actors in level

### Materials & Physics
- Getting available materials
- Applying materials to actors/Blueprints
- Setting mesh material colors
- Physics properties and simulations

### Analysis
- Reading Blueprint content
- Getting actor lists
- Finding actors by name
- Getting actor material info

## What's Likely Missing

Common gaps you might encounter:
- Asset import/export operations
- Material creation (only application exists)
- Texture manipulation
- Level/World composition tools
- Lighting control
- Widget/UMG creation
- Input binding setup
- Collision preset management
- Gameplay framework (GameMode, GameState, PlayerController setup)
- Editor viewport control
- Content browser operations
- Animation Blueprint support
- Sequencer/Cinematics control
- Sound/Audio system access
- Particle/Niagara system control
- Landscape editing tools
- Foliage system access

## Example Extension Request

```
**MCP EXTENSION NEEDED**

**What I was trying to do:**
Create a widget Blueprint with button components and bind click events to print messages.

**What's missing from MCP:**
UMG/Widget Blueprint creation and widget component manipulation. Need to create widget Blueprints, add widget components (buttons, text blocks, images), and set up their properties and events.

**Current MCP tools tried:**
- `create_blueprint` - Only creates Actor Blueprints, not Widget Blueprints
- `add_component_to_blueprint` - Only works with Actor components, not UMG widgets
- `add_node` - Could add nodes to EventGraph, but can't create the widget structure

**Proposed solution:**
New tools: `create_widget_blueprint`, `add_widget_to_canvas`, `bind_widget_event`

**Example usage:**
1. `create_widget_blueprint(name="WBP_MainMenu", parent_class="UserWidget")`
2. `add_widget_to_canvas(widget_blueprint="WBP_MainMenu", widget_type="Button", name="StartButton", position=(100, 100), size=(200, 50))`
3. `bind_widget_event(widget_blueprint="WBP_MainMenu", widget_name="StartButton", event="OnClicked", function_name="StartGame")`

**Unreal Engine APIs needed:**
- UWidgetBlueprint class for widget Blueprint creation
- UCanvasPanel, UButton, UTextBlock widget classes
- Widget component hierarchy manipulation
- Event binding via FOnButtonClickedEvent or similar delegates
```

---

**Instructions:** When you generate an extension request using the template above, the user will copy it and paste it into the MCP development project where another Claude instance will implement the new functionality.

# consolidate-variable-properties-params

## Problem

`set_blueprint_variable_properties` in `Python/unreal_mcp_server_advanced.py` takes **23 parameters** and carries a 4.8k-char docstring. Wide flat signatures bloat the JSON schema the MCP client sends to the model and make calls error-prone (the model must position/name many optional args).

## Fix

Collapse to a small core signature plus one dict:

```python
async def set_blueprint_variable_properties(
    blueprint_path: str,
    variable_name: str,
    properties: Dict[str, Any],
) -> Dict[str, Any]:
```

Document the recognized `properties` keys compactly in the docstring (or in the same on-demand help mechanism as `slim-add-node-docstring`). Keep the wire format to C++ unchanged — just build the params dict from `properties` server-side.

## Acceptance

- Tool schema is 3 params; existing C++ handler untouched; all previously settable properties still settable.

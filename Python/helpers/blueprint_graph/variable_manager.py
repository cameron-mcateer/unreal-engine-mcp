"""
Filename: variable_manager.py
Description: Python wrapper for Blueprint variable management
"""

import json
import logging
from typing import Dict, Any, Optional

logger = logging.getLogger("BlueprintGraph.VariableManager")


def create_variable(
    unreal_connection,
    blueprint_name: str,
    variable_name: str,
    variable_type: str,
    default_value: Any = None,
    is_public: bool = False,
    is_blueprint_writable: bool = True,
    tooltip: str = "",
    category: str = "Default"
) -> Dict[str, Any]:
    """
    Create a variable in a Blueprint.

    Args:
        unreal_connection: Connection to Unreal Engine
        blueprint_name: Name of the Blueprint to modify
        variable_name: Name of the variable to create
        variable_type: Type of the variable. Supported types:
            - "bool" - Boolean (True/False)
            - "int" - Integer number
            - "float" - Floating point number
            - "string" - Text string
            - "vector" - 3D vector [x, y, z]
            - "rotator" - 3D rotation [pitch, yaw, roll]
        default_value: Default value for the variable (optional)
            - bool: True or False
            - int: any integer (e.g., 0, 100, -5)
            - float: any decimal (e.g., 0.0, 10.5, -3.14)
            - string: any text (e.g., "", "Hello")
            - vector: list of 3 floats (e.g., [0.0, 0.0, 0.0])
            - rotator: list of 3 floats (e.g., [0.0, 90.0, 0.0])
        is_public: Whether the variable should be public/editable (default: False)
        is_blueprint_writable: Whether the variable can be set in Blueprint (default: True)
            Setting this to True allows VariableSet nodes to modify this variable.
            Set to False for read-only/constant variables.
        tooltip: Tooltip text for the variable (optional)
        category: Category for organizing variables (default: "Default")

    Returns:
        Dictionary containing:
            - success (bool): Whether operation succeeded
            - variable (dict): Variable details if successful
            - error (str): Error message if failed

    Examples:
        >>> # Create a float variable for health
        >>> create_variable(unreal, "BP_Player", "Health", "float", 100.0)

        >>> # Create a public boolean variable
        >>> create_variable(unreal, "BP_Enemy", "IsAlive", "bool", True, is_public=True)

        >>> # Create a vector variable with tooltip and category
        >>> create_variable(
        ...     unreal, "BP_Actor", "SpawnLocation", "vector",
        ...     [0.0, 0.0, 100.0], tooltip="Where to spawn", category="Gameplay"
        ... )
    """
    try:
        params = {
            "blueprint_name": blueprint_name,
            "variable_name": variable_name,
            "variable_type": variable_type
        }

        if default_value is not None:
            params["default_value"] = default_value
        if is_public:
            params["is_public"] = is_public
        if is_blueprint_writable is not None:
            params["is_blueprint_writable"] = is_blueprint_writable
        if tooltip:
            params["tooltip"] = tooltip
        if category != "Default":
            params["category"] = category

        response = unreal_connection.send_command("create_variable", params)
        
        if response.get("success"):
            logger.info(
                f"Successfully created variable '{variable_name}' ({variable_type}) in {blueprint_name}"
            )
        else:
            logger.error(
                f"Failed to create variable: {response.get('error', 'Unknown error')}"
            )
        
        return response
        
    except Exception as e:
        logger.error(f"Exception in create_variable: {e}")
        return {
            "success": False,
            "error": str(e)
        }


def set_blueprint_variable_properties(
    unreal_connection,
    blueprint_name: str,
    variable_name: str,
    var_name: Optional[str] = None,
    var_type: Optional[str] = None,
    is_blueprint_readable: Optional[bool] = None,
    is_blueprint_writable: Optional[bool] = None,
    is_public: Optional[bool] = None,
    is_editable_in_instance: Optional[bool] = None,
    tooltip: Optional[str] = None,
    category: Optional[str] = None,
    default_value: Any = None,
    expose_on_spawn: Optional[bool] = None,
    expose_to_cinematics: Optional[bool] = None,
    slider_range_min: Optional[str] = None,
    slider_range_max: Optional[str] = None,
    value_range_min: Optional[str] = None,
    value_range_max: Optional[str] = None,
    units: Optional[str] = None,
    bitmask: Optional[bool] = None,
    bitmask_enum: Optional[str] = None,
    replication_enabled: Optional[bool] = None,
    replication_condition: Optional[int] = None,
    is_private: Optional[bool] = None
) -> Dict[str, Any]:
    """
    Modify properties of an existing Blueprint variable without deleting it.
    Preserves all VariableGet and VariableSet nodes connected to this variable.

    Args:
        unreal_connection: Connection to Unreal Engine
        blueprint_name: Name of the Blueprint to modify
        variable_name: Name of the variable to modify
        is_blueprint_readable: Allow reading in Blueprint (VariableGet) (optional)
        is_blueprint_writable: Allow writing in Blueprint (Set) (optional)
        is_public: Visible in Blueprint editor (optional)
        is_editable_in_instance: Modifiable on instances (optional)
        tooltip: Variable description (optional)
        category: Variable category (optional)
        default_value: New default value (optional)

    Returns:
        Dictionary containing:
            - success (bool): Whether operation succeeded
            - variable_name (str): Name of the modified variable
            - properties_updated (dict): Properties that were updated
            - error (str): Error message if failed

    Example:
        >>> result = set_blueprint_variable_properties(
        ...     unreal,
        ...     "TestWorkflowBP",
        ...     "TimerCounter",
        ...     is_blueprint_readable=True,
        ...     is_blueprint_writable=True,
        ...     category="Gameplay"
        ... )
        >>> print(result["properties_updated"])
        {'is_blueprint_readable': True, 'is_blueprint_writable': True, 'category': 'Gameplay'}
    """
    try:
        params = {
            "blueprint_name": blueprint_name,
            "variable_name": variable_name
        }

        # Only include optional parameters if they are specified
        if var_name is not None:
            params["var_name"] = var_name
        if var_type is not None:
            params["var_type"] = var_type
        if is_blueprint_readable is not None:
            params["is_blueprint_readable"] = is_blueprint_readable
        if is_blueprint_writable is not None:
            params["is_blueprint_writable"] = is_blueprint_writable
        if is_public is not None:
            params["is_public"] = is_public
        if is_editable_in_instance is not None:
            params["is_editable_in_instance"] = is_editable_in_instance
        if tooltip is not None:
            params["tooltip"] = tooltip
        if category is not None:
            params["category"] = category
        if default_value is not None:
            params["default_value"] = default_value
        if expose_on_spawn is not None:
            params["expose_on_spawn"] = expose_on_spawn
        if expose_to_cinematics is not None:
            params["expose_to_cinematics"] = expose_to_cinematics
        if slider_range_min is not None:
            params["slider_range_min"] = slider_range_min
        if slider_range_max is not None:
            params["slider_range_max"] = slider_range_max
        if value_range_min is not None:
            params["value_range_min"] = value_range_min
        if value_range_max is not None:
            params["value_range_max"] = value_range_max
        if units is not None:
            params["units"] = units
        if bitmask is not None:
            params["bitmask"] = bitmask
        if bitmask_enum is not None:
            params["bitmask_enum"] = bitmask_enum
        if replication_enabled is not None:
            params["replication_enabled"] = replication_enabled
        if replication_condition is not None:
            params["replication_condition"] = replication_condition
        if is_private is not None:
            params["is_private"] = is_private

        response = unreal_connection.send_command("set_blueprint_variable_properties", params)

        if response.get("success"):
            logger.info(
                f"Successfully updated properties of variable '{variable_name}' in {blueprint_name}"
            )
        else:
            logger.error(
                f"Failed to update variable properties: {response.get('error', 'Unknown error')}"
            )

        return response

    except Exception as e:
        logger.error(f"Exception in set_blueprint_variable_properties: {e}")
        return {
            "success": False,
            "error": str(e)
        }
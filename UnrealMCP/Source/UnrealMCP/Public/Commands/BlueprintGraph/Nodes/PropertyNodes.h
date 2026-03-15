// Header for creating property get/set nodes targeting component properties

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"

class UK2Node;
class UBlueprint;

/**
 * Creator for property access nodes (Get/Set) on component or object properties.
 *
 * Unlike VariableGet/VariableSet which target Blueprint-level variables,
 * these nodes target UPROPERTYs on components or other UObjects.
 * In the Blueprint editor, these are the nodes you get when you drag off
 * a component reference and select "Set MaxWalkSpeed" etc.
 *
 * Two resolution modes:
 *   - component_name: Resolves via SCS (user-added) or FObjectProperty lookup
 *     on the class hierarchy (inherited C++ components like CharacterMovement).
 *   - target_class: Direct class name resolution (fallback for non-component objects).
 */
class FPropertyNodeCreator
{
public:
	/**
	 * Creates a Property Get node (K2Node_VariableGet targeting a component/object property)
	 * @param Blueprint - The Blueprint owning the graph
	 * @param Graph - The graph to add the node to
	 * @param Params - JSON parameters:
	 *   - component_name (str): Component variable name (SCS or inherited), OR
	 *   - target_class (str): Direct class name (e.g. "CharacterMovementComponent")
	 *   - property_name (str): UPROPERTY name on the target class
	 *   - pos_x, pos_y (float): Node position
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreatePropertyGetNode(UBlueprint* Blueprint, UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);

	/**
	 * Creates a Property Set node (K2Node_VariableSet targeting a component/object property)
	 * @param Blueprint - The Blueprint owning the graph
	 * @param Graph - The graph to add the node to
	 * @param Params - JSON parameters:
	 *   - component_name (str): Component variable name (SCS or inherited), OR
	 *   - target_class (str): Direct class name (e.g. "CharacterMovementComponent")
	 *   - property_name (str): UPROPERTY name on the target class
	 *   - pos_x, pos_y (float): Node position
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreatePropertySetNode(UBlueprint* Blueprint, UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);

private:
	/**
	 * Resolves a component name to its UClass.
	 * 1. Checks SCS (user-added components)
	 * 2. Finds FObjectProperty on GeneratedClass/ParentClass by variable name
	 *    (handles inherited C++ components like CharacterMovement on ACharacter)
	 * @param Blueprint - The Blueprint to search
	 * @param ComponentName - Name of the component variable
	 * @return The component's UClass, or nullptr if not found
	 */
	static UClass* ResolveComponentClass(UBlueprint* Blueprint, const FString& ComponentName);
};

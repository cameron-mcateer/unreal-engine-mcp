// Header for creating utility nodes (Print, CallFunction, Select, SpawnActor)

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"

class UK2Node;

/**
 * Creator for Unreal Blueprint utility nodes
 */
class FUtilityNodeCreator
{
public:
	/**
	 * Creates a Print node (K2Node_CallFunction for PrintString)
	 * @param Graph - The graph to add the node to
	 * @param Params - JSON parameters containing pos_x, pos_y, message
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreatePrintNode(UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);

	/**
	 * Creates a Call Function node (K2Node_CallFunction)
	 * @param Graph - The graph to add the node to
	 * @param Params - JSON parameters containing pos_x, pos_y, function_name, target_object
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreateCallFunctionNode(UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);

	/**
	 * Creates a Select node (K2Node_Select)
	 * @param Graph - The graph to add the node to
	 * @param Params - JSON parameters containing pos_x, pos_y, pin_type
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreateSelectNode(UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);

	/**
	 * Creates a Spawn Actor From Class node (K2Node_SpawnActorFromClass)
	 * @param Graph - The graph to add the node to
	 * @param Params - JSON parameters containing pos_x, pos_y, actor_class
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreateSpawnActorNode(UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);

	/**
	 * Creates a CallFunction node for a known engine function by friendly name.
	 * Bypasses fuzzy lookup by resolving the UFunction directly from its owner class.
	 *
	 * @param Graph  - The graph to add the node to
	 * @param Params - JSON parameters:
	 *                   - pos_x, pos_y       : graph position
	 *                   - target_function    : friendly name from the supported set:
	 *                       Actor     : GetActorLocation, GetDistanceTo, DestroyActor
	 *                       Pawn      : GetController, AddMovementInput
	 *                       Statics   : GetPlayerCharacter, ApplyDamage
	 *                       Utility   : IsValid
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreateEngineCallNode(UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);
};

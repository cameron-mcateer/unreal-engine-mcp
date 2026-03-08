// Header for creating Enhanced Input Blueprint nodes

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"

class UK2Node;

/**
 * Creator for Enhanced Input Blueprint nodes (UK2Node_EnhancedInputAction).
 * Creates input action event nodes that generate exec pins for
 * Started, Triggered, Ongoing, Canceled, Completed events.
 */
class FInputNodeCreator
{
public:
	/**
	 * Creates an Enhanced Input Action event node (UK2Node_EnhancedInputAction)
	 *
	 * @param Graph - The event graph to add the node to
	 * @param Params - JSON parameters containing:
	 *   - input_action (string, required): Asset path to the UInputAction
	 *     (e.g. "/Game/Input/IA_Attack" or "/Game/ThirdPerson/Input/IA_Attack")
	 *   - pos_x, pos_y: Position in graph
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreateInputActionEventNode(UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);
};

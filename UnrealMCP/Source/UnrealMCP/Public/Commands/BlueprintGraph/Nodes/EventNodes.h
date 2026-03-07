// Header for creating event-related nodes (ComponentBoundEvent)

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"

class UK2Node;
class UBlueprint;

/**
 * Creator for Unreal Blueprint event nodes that require special initialization
 * (component bound events, etc.)
 */
class FEventNodeCreator
{
public:
	/**
	 * Creates a Component Bound Event node (UK2Node_ComponentBoundEvent)
	 *
	 * Binds to a multicast delegate on a component that already exists in the Blueprint's
	 * SimpleConstructionScript (added via add_component_to_blueprint).
	 *
	 * @param Blueprint - The Blueprint that owns the component
	 * @param Graph - The event graph to add the node to
	 * @param Params - JSON parameters containing:
	 *   - component_name (string, required): Name of the component variable in the Blueprint
	 *   - event_type (string, required): Delegate name (e.g. "OnComponentBeginOverlap")
	 *   - pos_x, pos_y: Position in graph
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreateComponentBoundEventNode(UBlueprint* Blueprint, UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);
};

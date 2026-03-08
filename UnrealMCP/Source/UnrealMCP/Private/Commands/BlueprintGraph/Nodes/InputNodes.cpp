#include "Commands/BlueprintGraph/Nodes/InputNodes.h"
#include "Commands/BlueprintGraph/Nodes/NodeCreatorUtils.h"
#include "K2Node_EnhancedInputAction.h"
#include "InputAction.h"
#include "EditorAssetLibrary.h"
#include "Json.h"

UK2Node* FInputNodeCreator::CreateInputActionEventNode(UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Graph || !Params.IsValid())
	{
		return nullptr;
	}

	// Get the input action asset path
	FString InputActionPath;
	if (!Params->TryGetStringField(TEXT("input_action"), InputActionPath))
	{
		UE_LOG(LogTemp, Error, TEXT("InputActionEvent: Missing 'input_action' parameter"));
		return nullptr;
	}

	// Normalize the path
	if (!InputActionPath.StartsWith(TEXT("/")))
	{
		InputActionPath = TEXT("/Game/") + InputActionPath;
	}
	if (!InputActionPath.Contains(TEXT(".")))
	{
		InputActionPath += TEXT(".") + FPaths::GetBaseFilename(InputActionPath);
	}

	// Load the input action asset
	UInputAction* InputAction = LoadObject<UInputAction>(nullptr, *InputActionPath);
	if (!InputAction)
	{
		UE_LOG(LogTemp, Error, TEXT("InputActionEvent: InputAction not found: %s"), *InputActionPath);
		return nullptr;
	}

	// Create the node
	UK2Node_EnhancedInputAction* ActionNode = NewObject<UK2Node_EnhancedInputAction>(Graph);
	if (!ActionNode)
	{
		UE_LOG(LogTemp, Error, TEXT("InputActionEvent: Failed to create UK2Node_EnhancedInputAction"));
		return nullptr;
	}

	// Set the input action BEFORE allocating pins
	ActionNode->InputAction = InputAction;

	// Set position
	double PosX, PosY;
	FNodeCreatorUtils::ExtractNodePosition(Params, PosX, PosY);
	ActionNode->NodePosX = static_cast<int32>(PosX);
	ActionNode->NodePosY = static_cast<int32>(PosY);

	// Add to graph
	Graph->AddNode(ActionNode, false, false);
	ActionNode->CreateNewGuid();

	// Initialize pins — this generates exec pins for each ETriggerEvent
	ActionNode->AllocateDefaultPins();
	ActionNode->PostReconstructNode();

	return ActionNode;
}

#include "Commands/BlueprintGraph/Nodes/EventNodes.h"
#include "Commands/BlueprintGraph/Nodes/NodeCreatorUtils.h"
#include "K2Node_ComponentBoundEvent.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Json.h"

UK2Node* FEventNodeCreator::CreateComponentBoundEventNode(UBlueprint* Blueprint, UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Blueprint || !Graph || !Params.IsValid())
	{
		return nullptr;
	}

	// Extract required parameters
	FString ComponentName;
	if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
	{
		UE_LOG(LogTemp, Error, TEXT("ComponentEvent: Missing 'component_name' parameter"));
		return nullptr;
	}

	FString EventType;
	if (!Params->TryGetStringField(TEXT("event_type"), EventType))
	{
		UE_LOG(LogTemp, Error, TEXT("ComponentEvent: Missing 'event_type' parameter"));
		return nullptr;
	}

	// Find the SCS node for the component
	USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
	if (!SCS)
	{
		UE_LOG(LogTemp, Error, TEXT("ComponentEvent: Blueprint has no SimpleConstructionScript"));
		return nullptr;
	}

	USCS_Node* SCSNode = SCS->FindSCSNode(FName(*ComponentName));
	if (!SCSNode)
	{
		UE_LOG(LogTemp, Error, TEXT("ComponentEvent: Component '%s' not found in Blueprint SCS"), *ComponentName);
		return nullptr;
	}

	if (!SCSNode->ComponentTemplate)
	{
		UE_LOG(LogTemp, Error, TEXT("ComponentEvent: Component '%s' has no template"), *ComponentName);
		return nullptr;
	}

	// Get the component class to find the delegate property
	UClass* ComponentClass = SCSNode->ComponentTemplate->GetClass();
	if (!ComponentClass)
	{
		UE_LOG(LogTemp, Error, TEXT("ComponentEvent: Could not determine class for component '%s'"), *ComponentName);
		return nullptr;
	}

	// Find the multicast delegate property on the component class
	FMulticastDelegateProperty* DelegateProp = FindFProperty<FMulticastDelegateProperty>(
		ComponentClass, FName(*EventType));

	if (!DelegateProp)
	{
		UE_LOG(LogTemp, Error, TEXT("ComponentEvent: Delegate '%s' not found on component class '%s'"),
			*EventType, *ComponentClass->GetName());
		return nullptr;
	}

	// Find the FObjectProperty for the component variable on the generated class.
	// After compilation the SCS node's InternalVariableName becomes a property on GeneratedClass.
	FName VariableName = SCSNode->GetVariableName();
	FObjectProperty* ComponentProperty = FindFProperty<FObjectProperty>(
		Blueprint->GeneratedClass, VariableName);

	if (!ComponentProperty)
	{
		// The Blueprint may not have been compiled since the component was added.
		// Try compiling and looking again.
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		ComponentProperty = FindFProperty<FObjectProperty>(
			Blueprint->GeneratedClass, VariableName);

		if (!ComponentProperty)
		{
			UE_LOG(LogTemp, Error,
				TEXT("ComponentEvent: Could not find property '%s' on GeneratedClass (even after recompile)"),
				*VariableName.ToString());
			return nullptr;
		}
	}

	// Create the node
	UK2Node_ComponentBoundEvent* EventNode = NewObject<UK2Node_ComponentBoundEvent>(Graph);
	if (!EventNode)
	{
		return nullptr;
	}

	// Set position
	double PosX, PosY;
	FNodeCreatorUtils::ExtractNodePosition(Params, PosX, PosY);
	EventNode->NodePosX = static_cast<int32>(PosX);
	EventNode->NodePosY = static_cast<int32>(PosY);

	// Add to graph before initialization (required by InitializeComponentBoundEventParams
	// which calls GetBlueprint() on the node)
	Graph->AddNode(EventNode, false, false);
	EventNode->CreateNewGuid();

	// Initialize from the property pair — this sets DelegatePropertyName,
	// DelegateOwnerClass, ComponentPropertyName, EventReference, CustomFunctionName,
	// bOverrideFunction, and bInternalEvent.
	EventNode->InitializeComponentBoundEventParams(ComponentProperty, DelegateProp);

	// ReconstructNode updates EventReference from delegate signature and allocates pins
	EventNode->ReconstructNode();

	return EventNode;
}

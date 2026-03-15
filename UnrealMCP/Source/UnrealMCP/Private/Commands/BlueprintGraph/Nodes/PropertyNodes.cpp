#include "Commands/BlueprintGraph/Nodes/PropertyNodes.h"
#include "Commands/BlueprintGraph/Nodes/NodeCreatorUtils.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "UObject/FieldPath.h"
#include "Json.h"

/**
 * Resolve a class name to a UClass*.
 * Supports: Blueprint asset paths (/Game/...), full script paths (/Script/...),
 * short native class names (e.g. "CharacterMovementComponent"), and _C suffix.
 */
static UClass* ResolveTargetClass(const FString& Name)
{
	if (Name.IsEmpty())
	{
		return nullptr;
	}

	if (UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *Name))
	{
		return BP->GeneratedClass;
	}

	if (!Name.EndsWith(TEXT("_C")))
	{
		if (UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *(Name + TEXT("_C"))))
		{
			return BP->GeneratedClass;
		}
	}

	if (UClass* NativeClass = FindFirstObject<UClass>(*Name,
		EFindFirstObjectOptions::NativeFirst | EFindFirstObjectOptions::EnsureIfAmbiguous))
	{
		return NativeClass;
	}

	FString ClassName = Name;
	int32 DotIdx;
	if (ClassName.FindLastChar(TEXT('.'), DotIdx))
	{
		ClassName = ClassName.Mid(DotIdx + 1);
		if (UClass* NativeClass = FindFirstObject<UClass>(*ClassName,
			EFindFirstObjectOptions::NativeFirst | EFindFirstObjectOptions::EnsureIfAmbiguous))
		{
			return NativeClass;
		}
	}

	return Cast<UClass>(StaticFindObject(UClass::StaticClass(), nullptr, *Name));
}

UClass* FPropertyNodeCreator::ResolveComponentClass(UBlueprint* Blueprint, const FString& ComponentName)
{
	if (!Blueprint)
	{
		return nullptr;
	}

	// 1. Check SCS for user-added components
	if (USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript)
	{
		if (USCS_Node* SCSNode = SCS->FindSCSNode(FName(*ComponentName)))
		{
			if (SCSNode->ComponentTemplate)
			{
				return SCSNode->ComponentTemplate->GetClass();
			}
		}
	}

	// 2. Find the FObjectProperty on the class hierarchy by variable name.
	//    Inherited C++ components (e.g. CharacterMovement on ACharacter) are
	//    UPROPERTYs on the parent class. The property's class tells us the
	//    component type. This is more reliable than CDO component name matching
	//    because UE5 internal object names differ from UPROPERTY names
	//    (e.g. "CharMoveComp" vs "CharacterMovement").
	auto TryResolveFromProperty = [&](UClass* SearchClass) -> UClass*
	{
		if (!SearchClass)
		{
			return nullptr;
		}

		FProperty* VarProp = SearchClass->FindPropertyByName(FName(*ComponentName));
		if (!VarProp)
		{
			return nullptr;
		}

		if (FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(VarProp))
		{
			return ObjProp->PropertyClass;
		}

		return nullptr;
	};

	// Check generated class first (includes both inherited and BP-defined variables)
	if (UClass* Result = TryResolveFromProperty(Blueprint->GeneratedClass))
	{
		return Result;
	}

	// Fall back to parent class (in case GeneratedClass is stale)
	if (UClass* Result = TryResolveFromProperty(Blueprint->ParentClass))
	{
		return Result;
	}

	return nullptr;
}

UK2Node* FPropertyNodeCreator::CreatePropertyGetNode(UBlueprint* Blueprint, UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Blueprint || !Graph || !Params.IsValid())
	{
		return nullptr;
	}

	FString PropertyName;
	if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
	{
		UE_LOG(LogTemp, Error, TEXT("PropertyGet: Missing 'property_name' parameter"));
		return nullptr;
	}

	// Resolve the target class — either from component_name or target_class
	UClass* TargetClass = nullptr;
	FString ComponentName;
	FString TargetClassName;

	if (Params->TryGetStringField(TEXT("component_name"), ComponentName) && !ComponentName.IsEmpty())
	{
		TargetClass = ResolveComponentClass(Blueprint, ComponentName);
		if (!TargetClass)
		{
			UE_LOG(LogTemp, Error, TEXT("PropertyGet: Component '%s' not found in Blueprint class hierarchy"), *ComponentName);
			return nullptr;
		}
	}
	else if (Params->TryGetStringField(TEXT("target_class"), TargetClassName) && !TargetClassName.IsEmpty())
	{
		TargetClass = ResolveTargetClass(TargetClassName);
		if (!TargetClass)
		{
			UE_LOG(LogTemp, Error, TEXT("PropertyGet: Could not resolve target class '%s'"), *TargetClassName);
			return nullptr;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PropertyGet: Requires either 'component_name' or 'target_class' parameter"));
		return nullptr;
	}

	// Find the property on the target class
	FProperty* Property = TargetClass->FindPropertyByName(FName(*PropertyName));
	if (!Property)
	{
		UE_LOG(LogTemp, Error, TEXT("PropertyGet: Property '%s' not found on class '%s'"),
			*PropertyName, *TargetClass->GetName());
		return nullptr;
	}

	// Create the VariableGet node
	UK2Node_VariableGet* VarGetNode = NewObject<UK2Node_VariableGet>(Graph);
	if (!VarGetNode)
	{
		return nullptr;
	}

	// Configure to target the property on the class (not self-context)
	// This creates a "Target" pin for the object reference
	VarGetNode->SetFromProperty(Property, false, TargetClass);

	double PosX, PosY;
	FNodeCreatorUtils::ExtractNodePosition(Params, PosX, PosY);
	VarGetNode->NodePosX = static_cast<int32>(PosX);
	VarGetNode->NodePosY = static_cast<int32>(PosY);

	Graph->AddNode(VarGetNode, true, false);
	FNodeCreatorUtils::InitializeK2Node(VarGetNode, Graph);

	return VarGetNode;
}

UK2Node* FPropertyNodeCreator::CreatePropertySetNode(UBlueprint* Blueprint, UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Blueprint || !Graph || !Params.IsValid())
	{
		return nullptr;
	}

	FString PropertyName;
	if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
	{
		UE_LOG(LogTemp, Error, TEXT("PropertySet: Missing 'property_name' parameter"));
		return nullptr;
	}

	// Resolve the target class — either from component_name or target_class
	UClass* TargetClass = nullptr;
	FString ComponentName;
	FString TargetClassName;

	if (Params->TryGetStringField(TEXT("component_name"), ComponentName) && !ComponentName.IsEmpty())
	{
		TargetClass = ResolveComponentClass(Blueprint, ComponentName);
		if (!TargetClass)
		{
			UE_LOG(LogTemp, Error, TEXT("PropertySet: Component '%s' not found in Blueprint class hierarchy"), *ComponentName);
			return nullptr;
		}
	}
	else if (Params->TryGetStringField(TEXT("target_class"), TargetClassName) && !TargetClassName.IsEmpty())
	{
		TargetClass = ResolveTargetClass(TargetClassName);
		if (!TargetClass)
		{
			UE_LOG(LogTemp, Error, TEXT("PropertySet: Could not resolve target class '%s'"), *TargetClassName);
			return nullptr;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PropertySet: Requires either 'component_name' or 'target_class' parameter"));
		return nullptr;
	}

	// Find the property on the target class
	FProperty* Property = TargetClass->FindPropertyByName(FName(*PropertyName));
	if (!Property)
	{
		UE_LOG(LogTemp, Error, TEXT("PropertySet: Property '%s' not found on class '%s'"),
			*PropertyName, *TargetClass->GetName());
		return nullptr;
	}

	// Create the VariableSet node
	UK2Node_VariableSet* VarSetNode = NewObject<UK2Node_VariableSet>(Graph);
	if (!VarSetNode)
	{
		return nullptr;
	}

	// Configure to target the property on the class (not self-context)
	// This creates a "Target" pin for the object reference + exec pins + value pin
	VarSetNode->SetFromProperty(Property, false, TargetClass);

	double PosX, PosY;
	FNodeCreatorUtils::ExtractNodePosition(Params, PosX, PosY);
	VarSetNode->NodePosX = static_cast<int32>(PosX);
	VarSetNode->NodePosY = static_cast<int32>(PosY);

	Graph->AddNode(VarSetNode, true, false);
	FNodeCreatorUtils::InitializeK2Node(VarSetNode, Graph);

	return VarSetNode;
}

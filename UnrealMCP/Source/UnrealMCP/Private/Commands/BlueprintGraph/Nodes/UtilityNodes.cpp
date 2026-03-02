#include "Commands/BlueprintGraph/Nodes/UtilityNodes.h"
#include "Commands/BlueprintGraph/Nodes/NodeCreatorUtils.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Select.h"
#include "K2Node_SpawnActorFromClass.h"
#include "EdGraphSchema_K2.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Json.h"

UK2Node* FUtilityNodeCreator::CreatePrintNode(UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Graph || !Params.IsValid())
	{
		return nullptr;
	}

	UK2Node_CallFunction* PrintNode = NewObject<UK2Node_CallFunction>(Graph);
	if (!PrintNode)
	{
		return nullptr;
	}

	UFunction* PrintFunc = UKismetSystemLibrary::StaticClass()->FindFunctionByName(
		GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, PrintString)
	);

	if (!PrintFunc)
	{
		return nullptr;
	}

	// Set function reference BEFORE initialization
	PrintNode->SetFromFunction(PrintFunc);

	double PosX, PosY;
	FNodeCreatorUtils::ExtractNodePosition(Params, PosX, PosY);
	PrintNode->NodePosX = static_cast<int32>(PosX);
	PrintNode->NodePosY = static_cast<int32>(PosY);

	Graph->AddNode(PrintNode, true, false);
	FNodeCreatorUtils::InitializeK2Node(PrintNode, Graph);

	// Set message if provided AFTER initialization
	FString Message;
	if (Params->TryGetStringField(TEXT("message"), Message))
	{
		UEdGraphPin* InStringPin = PrintNode->FindPin(TEXT("InString"));
		if (InStringPin)
		{
			InStringPin->DefaultValue = Message;
		}
	}

	return PrintNode;
}

UK2Node* FUtilityNodeCreator::CreateCallFunctionNode(UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Graph || !Params.IsValid())
	{
		return nullptr;
	}

	// Get target function name
	FString TargetFunction;
	if (!Params->TryGetStringField(TEXT("target_function"), TargetFunction))
	{
		return nullptr;
	}

	UK2Node_CallFunction* CallNode = NewObject<UK2Node_CallFunction>(Graph);
	if (!CallNode)
	{
		return nullptr;
	}

	// Find the function to call
	UFunction* TargetFunc = nullptr;
	FString ClassName;
	if (Params->TryGetStringField(TEXT("target_class"), ClassName))
	{
		UClass* TargetClass = Cast<UClass>(StaticFindObject(UClass::StaticClass(), nullptr, *ClassName));
		if (TargetClass)
		{
			TargetFunc = TargetClass->FindFunctionByName(FName(*TargetFunction));
		}
	}
	else
	{
		// Try common Unreal classes
		TargetFunc = UKismetSystemLibrary::StaticClass()->FindFunctionByName(FName(*TargetFunction));
	}

	if (!TargetFunc)
	{
		return nullptr;
	}

	// Set function reference BEFORE initialization
	CallNode->SetFromFunction(TargetFunc);

	double PosX, PosY;
	FNodeCreatorUtils::ExtractNodePosition(Params, PosX, PosY);
	CallNode->NodePosX = static_cast<int32>(PosX);
	CallNode->NodePosY = static_cast<int32>(PosY);

	Graph->AddNode(CallNode, true, false);
	FNodeCreatorUtils::InitializeK2Node(CallNode, Graph);

	return CallNode;
}

UK2Node* FUtilityNodeCreator::CreateSelectNode(UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Graph || !Params.IsValid())
	{
		return nullptr;
	}

	UK2Node_Select* SelectNode = NewObject<UK2Node_Select>(Graph);
	if (!SelectNode)
	{
		return nullptr;
	}

	double PosX, PosY;
	FNodeCreatorUtils::ExtractNodePosition(Params, PosX, PosY);
	SelectNode->NodePosX = static_cast<int32>(PosX);
	SelectNode->NodePosY = static_cast<int32>(PosY);

	Graph->AddNode(SelectNode, true, false);
	FNodeCreatorUtils::InitializeK2Node(SelectNode, Graph);

	return SelectNode;
}

UK2Node* FUtilityNodeCreator::CreateSpawnActorNode(UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Graph || !Params.IsValid())
	{
		return nullptr;
	}

	UK2Node_SpawnActorFromClass* SpawnActorNode = NewObject<UK2Node_SpawnActorFromClass>(Graph);
	if (!SpawnActorNode)
	{
		return nullptr;
	}

	double PosX, PosY;
	FNodeCreatorUtils::ExtractNodePosition(Params, PosX, PosY);
	SpawnActorNode->NodePosX = static_cast<int32>(PosX);
	SpawnActorNode->NodePosY = static_cast<int32>(PosY);

	Graph->AddNode(SpawnActorNode, true, false);
	FNodeCreatorUtils::InitializeK2Node(SpawnActorNode, Graph);

	return SpawnActorNode;
}

UK2Node* FUtilityNodeCreator::CreateEngineCallNode(UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Graph || !Params.IsValid())
	{
		return nullptr;
	}

	FString TargetFunction;
	if (!Params->TryGetStringField(TEXT("target_function"), TargetFunction))
	{
		return nullptr;
	}

	// Explicit lookup table: friendly name -> (owner class, exact UFunction name).
	// Function names verified against UE 5.6 source headers.
	UClass* OwnerClass = nullptr;
	FName FuncName;

	// AActor functions
	if      (TargetFunction.Equals(TEXT("GetActorLocation"),   ESearchCase::IgnoreCase)) { OwnerClass = AActor::StaticClass();                FuncName = FName("K2_GetActorLocation"); }
	else if (TargetFunction.Equals(TEXT("GetDistanceTo"),      ESearchCase::IgnoreCase)) { OwnerClass = AActor::StaticClass();                FuncName = FName("GetDistanceTo");        }
	else if (TargetFunction.Equals(TEXT("DestroyActor"),       ESearchCase::IgnoreCase)) { OwnerClass = AActor::StaticClass();                FuncName = FName("K2_DestroyActor");      }
	// APawn functions
	else if (TargetFunction.Equals(TEXT("GetController"),      ESearchCase::IgnoreCase)) { OwnerClass = APawn::StaticClass();                 FuncName = FName("GetController");        }
	else if (TargetFunction.Equals(TEXT("AddMovementInput"),   ESearchCase::IgnoreCase)) { OwnerClass = APawn::StaticClass();                 FuncName = FName("AddMovementInput");     }
	// UGameplayStatics functions (static, no self pin)
	else if (TargetFunction.Equals(TEXT("GetPlayerCharacter"), ESearchCase::IgnoreCase)) { OwnerClass = UGameplayStatics::StaticClass();      FuncName = FName("GetPlayerCharacter");   }
	else if (TargetFunction.Equals(TEXT("ApplyDamage"),        ESearchCase::IgnoreCase)) { OwnerClass = UGameplayStatics::StaticClass();      FuncName = FName("ApplyDamage");          }
	// UKismetSystemLibrary functions (static)
	else if (TargetFunction.Equals(TEXT("IsValid"),            ESearchCase::IgnoreCase)) { OwnerClass = UKismetSystemLibrary::StaticClass();  FuncName = FName("IsValid");              }

	if (!OwnerClass)
	{
		return nullptr;
	}

	UFunction* TargetFunc = OwnerClass->FindFunctionByName(FuncName);
	if (!TargetFunc)
	{
		return nullptr;
	}

	UK2Node_CallFunction* CallNode = NewObject<UK2Node_CallFunction>(Graph);
	if (!CallNode)
	{
		return nullptr;
	}

	// SetFromFunction must be called before graph insertion and pin allocation
	CallNode->SetFromFunction(TargetFunc);

	double PosX, PosY;
	FNodeCreatorUtils::ExtractNodePosition(Params, PosX, PosY);
	CallNode->NodePosX = static_cast<int32>(PosX);
	CallNode->NodePosY = static_cast<int32>(PosY);

	Graph->AddNode(CallNode, true, false);
	FNodeCreatorUtils::InitializeK2Node(CallNode, Graph);

	return CallNode;
}


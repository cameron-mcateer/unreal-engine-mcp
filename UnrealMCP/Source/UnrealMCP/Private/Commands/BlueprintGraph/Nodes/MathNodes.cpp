#include "Commands/BlueprintGraph/Nodes/MathNodes.h"
#include "Commands/BlueprintGraph/Nodes/NodeCreatorUtils.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_PromotableOperator.h"
#include "K2Node_CallFunction.h"
#include "Kismet/KismetMathLibrary.h"

UK2Node* FMathNodeCreator::CreateMathOperatorNode(UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Graph || !Params.IsValid())
	{
		return nullptr;
	}

	FString OperatorStr;
	if (!Params->TryGetStringField(TEXT("operator"), OperatorStr))
	{
		return nullptr;
	}

	// Resolve the UFunction that defines this operation
	UFunction* OpFunc = ResolveOperatorFunction(OperatorStr);
	if (!OpFunc)
	{
		return nullptr;
	}

	double PosX, PosY;
	FNodeCreatorUtils::ExtractNodePosition(Params, PosX, PosY);

	// NormalizeVector uses a plain CallFunction node (single typed input, no promotion needed)
	if (OperatorStr.Equals(TEXT("NormalizeVector"), ESearchCase::IgnoreCase))
	{
		UK2Node_CallFunction* CallNode = NewObject<UK2Node_CallFunction>(Graph);
		if (!CallNode)
		{
			return nullptr;
		}

		CallNode->SetFromFunction(OpFunc);
		CallNode->NodePosX = static_cast<int32>(PosX);
		CallNode->NodePosY = static_cast<int32>(PosY);
		Graph->AddNode(CallNode, false, false);
		CallNode->CreateNewGuid();
		CallNode->PostPlacedNewNode();
		CallNode->AllocateDefaultPins();
		return CallNode;
	}

	// All other operators use UK2Node_PromotableOperator for automatic type promotion.
	// SetFromFunction must be called before PostPlacedNewNode/AllocateDefaultPins —
	// it sets the internal OperationName ("Subtract", "Add", etc.) from the function,
	// which the node uses to find the best matching function as pin types change.
	UK2Node_PromotableOperator* OpNode = NewObject<UK2Node_PromotableOperator>(Graph);
	if (!OpNode)
	{
		return nullptr;
	}

	OpNode->SetFromFunction(OpFunc);
	OpNode->NodePosX = static_cast<int32>(PosX);
	OpNode->NodePosY = static_cast<int32>(PosY);

	Graph->AddNode(OpNode, false, false);
	OpNode->CreateNewGuid();
	OpNode->PostPlacedNewNode();
	OpNode->AllocateDefaultPins();

	return OpNode;
}

UFunction* FMathNodeCreator::ResolveOperatorFunction(const FString& OperatorStr)
{
	// Maps user-facing operator names to UKismetMathLibrary function names.
	// Double-precision variants are used as defaults since UE5 uses doubles internally.
	// VectorSubtract uses the vector-vector overload directly.
	static const TMap<FString, FName> OperatorFunctionMap =
	{
		// Arithmetic
		{ TEXT("Add"),            FName(TEXT("Add_DoubleDouble"))         },
		{ TEXT("Subtract"),       FName(TEXT("Subtract_DoubleDouble"))    },
		{ TEXT("VectorSubtract"), FName(TEXT("Subtract_VectorVector"))    },
		{ TEXT("Multiply"),       FName(TEXT("Multiply_DoubleDouble"))    },
		{ TEXT("Divide"),         FName(TEXT("Divide_DoubleDouble"))      },
		// Comparison
		{ TEXT("Less"),           FName(TEXT("Less_DoubleDouble"))        },
		{ TEXT("LessEqual"),      FName(TEXT("LessEqual_DoubleDouble"))   },
		{ TEXT("Greater"),        FName(TEXT("Greater_DoubleDouble"))     },
		{ TEXT("GreaterEqual"),   FName(TEXT("GreaterEqual_DoubleDouble"))},
		{ TEXT("Equal"),          FName(TEXT("EqualEqual_DoubleDouble"))  },
		{ TEXT("NotEqual"),       FName(TEXT("NotEqual_DoubleDouble"))    },
		// Boolean
		{ TEXT("BooleanAND"),     FName(TEXT("BooleanAND"))               },
		{ TEXT("BooleanOR"),      FName(TEXT("BooleanOR"))                },
		// Vector (routed to CallFunction path)
		{ TEXT("NormalizeVector"),FName(TEXT("Normal"))                   },
	};

	for (const auto& Pair : OperatorFunctionMap)
	{
		if (Pair.Key.Equals(OperatorStr, ESearchCase::IgnoreCase))
		{
			return UKismetMathLibrary::StaticClass()->FindFunctionByName(Pair.Value);
		}
	}

	return nullptr;
}

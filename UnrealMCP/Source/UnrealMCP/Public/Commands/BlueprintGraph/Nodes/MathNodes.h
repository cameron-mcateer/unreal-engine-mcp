// Header for creating math operator nodes (UK2Node_PromotableOperator)

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"

class UK2Node;
class UFunction;

/**
 * Creator for Unreal Blueprint math operator nodes.
 * Uses UK2Node_PromotableOperator for arithmetic/comparison ops with automatic
 * type promotion, and UK2Node_CallFunction for special ops like NormalizeVector.
 */
class FMathNodeCreator
{
public:
	/**
	 * Creates a Math Operator node.
	 * @param Graph  - The graph to add the node to
	 * @param Params - JSON parameters:
	 *                   - pos_x, pos_y   : graph position
	 *                   - operator (str) : operator name (see below)
	 *
	 * Supported operator values:
	 *   Arithmetic : Add, Subtract, Multiply, Divide
	 *   Comparison : Less, LessEqual, Greater, GreaterEqual, Equal, NotEqual
	 *   Boolean    : BooleanAND, BooleanOR
	 *   Vector     : NormalizeVector, VectorSubtract
	 *
	 * Arithmetic and comparison nodes default to double precision and
	 * auto-promote when connected to other typed pins.
	 *
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreateMathOperatorNode(UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);

private:
	/**
	 * Resolves the UKismetMathLibrary UFunction for the given user-facing operator name.
	 * Returns nullptr if the operator is unrecognised or the function is not found.
	 */
	static UFunction* ResolveOperatorFunction(const FString& OperatorStr);
};

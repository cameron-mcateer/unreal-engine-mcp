#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for Enhanced Input MCP commands.
 * Supports creating InputAction assets, adding key mappings to
 * InputMappingContext assets.
 */
class FEpicUnrealMCPInputCommands
{
public:
    FEpicUnrealMCPInputCommands();

    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    TSharedPtr<FJsonObject> HandleCreateInputAction(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddInputMapping(const TSharedPtr<FJsonObject>& Params);
};

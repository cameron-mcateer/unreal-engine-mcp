#pragma once

#include "CoreMinimal.h"
#include "Json.h"

class UWidgetBlueprint;
class UWidget;
class UPanelWidget;

/**
 * Handler class for Widget Blueprint MCP commands.
 * Supports creating Widget Blueprints, adding UMG widgets to
 * canvas/panel hierarchies, and querying widget tree structure.
 */
class FEpicUnrealMCPWidgetCommands
{
public:
    FEpicUnrealMCPWidgetCommands();

    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    TSharedPtr<FJsonObject> HandleCreateWidgetBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddWidgetChild(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetWidgetChildren(const TSharedPtr<FJsonObject>& Params);

    // Helpers
    static UWidgetBlueprint* FindWidgetBlueprint(const FString& BlueprintName);
    static UClass* ResolveWidgetClass(const FString& WidgetType);
    static void ApplyWidgetProperties(UWidget* Widget, const TSharedPtr<FJsonObject>& Properties);
    static FLinearColor GetColorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName);
    static TSharedPtr<FJsonObject> WidgetToJson(UWidget* Widget);
};

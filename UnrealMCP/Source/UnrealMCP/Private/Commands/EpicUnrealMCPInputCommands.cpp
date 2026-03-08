#include "Commands/EpicUnrealMCPInputCommands.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"

#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedActionKeyMapping.h"

#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

FEpicUnrealMCPInputCommands::FEpicUnrealMCPInputCommands()
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPInputCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_input_action"))
    {
        return HandleCreateInputAction(Params);
    }
    else if (CommandType == TEXT("add_input_mapping"))
    {
        return HandleAddInputMapping(Params);
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown input command: %s"), *CommandType));
}

// ---------------------------------------------------------------------------
// create_input_action
// ---------------------------------------------------------------------------
TSharedPtr<FJsonObject> FEpicUnrealMCPInputCommands::HandleCreateInputAction(const TSharedPtr<FJsonObject>& Params)
{
    FString ActionName;
    if (!Params->TryGetStringField(TEXT("name"), ActionName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Optional path, default to /Game/Input/
    FString PackagePath = TEXT("/Game/Input/");
    FString PathParam;
    if (Params->TryGetStringField(TEXT("path"), PathParam) && !PathParam.IsEmpty())
    {
        PackagePath = PathParam;
        if (!PackagePath.EndsWith(TEXT("/")))
        {
            PackagePath += TEXT("/");
        }
    }

    // Check if asset already exists
    FString FullPath = PackagePath + ActionName;
    if (UEditorAssetLibrary::DoesAssetExist(FullPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Input Action already exists: %s"), *FullPath));
    }

    // Resolve value type
    EInputActionValueType ValueType = EInputActionValueType::Boolean;
    FString ValueTypeStr;
    if (Params->TryGetStringField(TEXT("value_type"), ValueTypeStr))
    {
        if (ValueTypeStr.Equals(TEXT("bool"), ESearchCase::IgnoreCase) || ValueTypeStr.Equals(TEXT("boolean"), ESearchCase::IgnoreCase))
        {
            ValueType = EInputActionValueType::Boolean;
        }
        else if (ValueTypeStr.Equals(TEXT("float"), ESearchCase::IgnoreCase) || ValueTypeStr.Equals(TEXT("axis1d"), ESearchCase::IgnoreCase))
        {
            ValueType = EInputActionValueType::Axis1D;
        }
        else if (ValueTypeStr.Equals(TEXT("vector2d"), ESearchCase::IgnoreCase) || ValueTypeStr.Equals(TEXT("axis2d"), ESearchCase::IgnoreCase))
        {
            ValueType = EInputActionValueType::Axis2D;
        }
        else if (ValueTypeStr.Equals(TEXT("vector3d"), ESearchCase::IgnoreCase) || ValueTypeStr.Equals(TEXT("vector"), ESearchCase::IgnoreCase) || ValueTypeStr.Equals(TEXT("axis3d"), ESearchCase::IgnoreCase))
        {
            ValueType = EInputActionValueType::Axis3D;
        }
        else
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
                TEXT("Unknown value_type: '%s'. Valid: bool, float, vector2d, vector3d"), *ValueTypeStr));
        }
    }

    // Create package and asset
    UPackage* Package = CreatePackage(*FullPath);
    if (!Package)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create package"));
    }

    UInputAction* NewAction = NewObject<UInputAction>(Package, *ActionName, RF_Public | RF_Standalone);
    if (!NewAction)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create UInputAction"));
    }

    NewAction->ValueType = ValueType;

    // Optional: consume input setting
    bool bConsumeInput;
    if (Params->TryGetBoolField(TEXT("consume_input"), bConsumeInput))
    {
        NewAction->bConsumeInput = bConsumeInput;
    }

    // Optional: trigger when paused
    bool bTriggerWhenPaused;
    if (Params->TryGetBoolField(TEXT("trigger_when_paused"), bTriggerWhenPaused))
    {
        NewAction->bTriggerWhenPaused = bTriggerWhenPaused;
    }

    // Register with asset registry and mark dirty
    FAssetRegistryModule::AssetCreated(NewAction);
    Package->MarkPackageDirty();

    // Build response
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("name"), ActionName);
    ResultObj->SetStringField(TEXT("path"), FullPath);
    ResultObj->SetStringField(TEXT("value_type"), ValueTypeStr.IsEmpty() ? TEXT("bool") : ValueTypeStr);
    return ResultObj;
}

// ---------------------------------------------------------------------------
// add_input_mapping
// ---------------------------------------------------------------------------
TSharedPtr<FJsonObject> FEpicUnrealMCPInputCommands::HandleAddInputMapping(const TSharedPtr<FJsonObject>& Params)
{
    FString MappingContextPath;
    if (!Params->TryGetStringField(TEXT("mapping_context"), MappingContextPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'mapping_context' parameter"));
    }

    FString InputActionPath;
    if (!Params->TryGetStringField(TEXT("input_action"), InputActionPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'input_action' parameter"));
    }

    FString KeyName;
    if (!Params->TryGetStringField(TEXT("key"), KeyName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'key' parameter"));
    }

    // Normalize asset paths (add .AssetName suffix if missing)
    auto NormalizePath = [](FString& Path)
    {
        if (!Path.StartsWith(TEXT("/")))
        {
            Path = TEXT("/Game/") + Path;
        }
        if (!Path.Contains(TEXT(".")))
        {
            Path += TEXT(".") + FPaths::GetBaseFilename(Path);
        }
    };

    NormalizePath(MappingContextPath);
    NormalizePath(InputActionPath);

    // Load the mapping context
    UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(nullptr, *MappingContextPath);
    if (!MappingContext)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
            TEXT("InputMappingContext not found: %s"), *MappingContextPath));
    }

    // Load the input action
    UInputAction* InputAction = LoadObject<UInputAction>(nullptr, *InputActionPath);
    if (!InputAction)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
            TEXT("InputAction not found: %s"), *InputActionPath));
    }

    // Resolve the FKey
    FKey MappedKey{FName(*KeyName)};
    if (!MappedKey.IsValid())
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
            TEXT("Invalid key name: '%s'. Examples: LeftMouseButton, SpaceBar, Gamepad_FaceButton_Bottom, W, A, S, D"), *KeyName));
    }

    // Add the mapping
    MappingContext->MapKey(InputAction, MappedKey);

    // Mark the package dirty so it can be saved
    MappingContext->GetOutermost()->MarkPackageDirty();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("mapping_context"), MappingContextPath);
    ResultObj->SetStringField(TEXT("input_action"), InputActionPath);
    ResultObj->SetStringField(TEXT("key"), KeyName);
    return ResultObj;
}

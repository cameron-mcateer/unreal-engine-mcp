#include "Commands/EpicUnrealMCPEditorCommands.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "ImageUtils.h"
#include "HighResScreenshot.h"
#include "Engine/GameViewportClient.h"
#include "Misc/FileHelper.h"
#include "GameFramework/Actor.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EditorAssetLibrary.h"
#include "Engine/DataTable.h"
#include "DataTableUtils.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/GameModeBase.h"
#include "Commands/EpicUnrealMCPBlueprintCommands.h"

FEpicUnrealMCPEditorCommands::FEpicUnrealMCPEditorCommands()
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // Actor manipulation commands
    if (CommandType == TEXT("get_actors_in_level"))
    {
        return HandleGetActorsInLevel(Params);
    }
    else if (CommandType == TEXT("find_actors_by_name"))
    {
        return HandleFindActorsByName(Params);
    }
    else if (CommandType == TEXT("spawn_actor"))
    {
        return HandleSpawnActor(Params);
    }
    else if (CommandType == TEXT("delete_actor"))
    {
        return HandleDeleteActor(Params);
    }
    else if (CommandType == TEXT("set_actor_transform"))
    {
        return HandleSetActorTransform(Params);
    }
    // Blueprint actor spawning
    else if (CommandType == TEXT("spawn_blueprint_actor"))
    {
        return HandleSpawnBlueprintActor(Params);
    }
    // DataTable commands
    else if (CommandType == TEXT("read_data_table"))
    {
        return HandleReadDataTable(Params);
    }
    // World settings commands
    else if (CommandType == TEXT("get_world_settings"))
    {
        return HandleGetWorldSettings(Params);
    }
    else if (CommandType == TEXT("set_level_gamemode"))
    {
        return HandleSetLevelGameMode(Params);
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown editor command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleGetActorsInLevel(const TSharedPtr<FJsonObject>& Params)
{
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> ActorArray;
    for (AActor* Actor : AllActors)
    {
        if (Actor)
        {
            ActorArray.Add(FEpicUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), ActorArray);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleFindActorsByName(const TSharedPtr<FJsonObject>& Params)
{
    FString Pattern;
    if (!Params->TryGetStringField(TEXT("pattern"), Pattern))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pattern' parameter"));
    }
    
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> MatchingActors;
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName().Contains(Pattern))
        {
            MatchingActors.Add(FEpicUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), MatchingActors);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleSpawnActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorType;
    if (!Params->TryGetStringField(TEXT("type"), ActorType))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
    }

    // Get actor name (required parameter)
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Get optional transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FEpicUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }
    if (Params->HasField(TEXT("scale")))
    {
        Scale = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Resolve the actor class from the type string
    UClass* ActorClass = nullptr;
    if (ActorType == TEXT("StaticMeshActor"))
    {
        ActorClass = AStaticMeshActor::StaticClass();
    }
    else if (ActorType == TEXT("PointLight"))
    {
        ActorClass = APointLight::StaticClass();
    }
    else if (ActorType == TEXT("SpotLight"))
    {
        ActorClass = ASpotLight::StaticClass();
    }
    else if (ActorType == TEXT("DirectionalLight"))
    {
        ActorClass = ADirectionalLight::StaticClass();
    }
    else if (ActorType == TEXT("CameraActor"))
    {
        ActorClass = ACameraActor::StaticClass();
    }
    else
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown actor type: %s"), *ActorType));
    }

    // Use editor subsystem so actors are registered with the level and persist on save
    UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
    if (!EditorActorSubsystem)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get UEditorActorSubsystem"));
    }

    AActor* NewActor = EditorActorSubsystem->SpawnActorFromClass(ActorClass, Location, Rotation);
    if (!NewActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create actor"));
    }

    NewActor->SetActorLabel(ActorName);

    // Set scale
    NewActor->SetActorScale3D(Scale);

    // For StaticMeshActors, optionally assign a mesh
    if (ActorType == TEXT("StaticMeshActor"))
    {
        FString MeshPath;
        if (Params->TryGetStringField(TEXT("static_mesh"), MeshPath))
        {
            AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(NewActor);
            if (MeshActor)
            {
                UStaticMesh* Mesh = Cast<UStaticMesh>(UEditorAssetLibrary::LoadAsset(MeshPath));
                if (Mesh)
                {
                    MeshActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Could not find static mesh at path: %s"), *MeshPath);
                }
            }
        }
    }

    // Return the created actor's details
    return FEpicUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleDeleteActor(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            // Store actor info before deletion for the response
            TSharedPtr<FJsonObject> ActorInfo = FEpicUnrealMCPCommonUtils::ActorToJsonObject(Actor);
            
            // Delete via editor subsystem so external actor files are cleaned up
            UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
            EditorActorSubsystem->DestroyActor(Actor);
            
            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetObjectField(TEXT("deleted_actor"), ActorInfo);
            return ResultObj;
        }
    }
    
    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get transform parameters
    FTransform NewTransform = TargetActor->GetTransform();

    if (Params->HasField(TEXT("location")))
    {
        NewTransform.SetLocation(FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location")));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        NewTransform.SetRotation(FQuat(FEpicUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"))));
    }
    if (Params->HasField(TEXT("scale")))
    {
        NewTransform.SetScale3D(FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
    }

    // Set the new transform
    TargetActor->SetActorTransform(NewTransform);

    // Return updated actor info
    return FEpicUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
    // This function will now correctly call the implementation in BlueprintCommands
    FEpicUnrealMCPBlueprintCommands BlueprintCommands;
    return BlueprintCommands.HandleCommand(TEXT("spawn_blueprint_actor"), Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleReadDataTable(const TSharedPtr<FJsonObject>& Params)
{
    FString DataTablePath;
    if (!Params->TryGetStringField(TEXT("data_table_path"), DataTablePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'data_table_path' parameter"));
    }

    UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(DataTablePath);
    if (!LoadedAsset)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Asset not found: %s"), *DataTablePath));
    }

    UDataTable* DataTable = Cast<UDataTable>(LoadedAsset);
    if (!DataTable)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Asset is not a DataTable: %s"), *DataTablePath));
    }

    // Get row struct info
    const UScriptStruct* RowStruct = DataTable->GetRowStruct();
    FString RowStructName = RowStruct ? RowStruct->GetName() : TEXT("Unknown");

    // Check if a specific row was requested
    FString RowName;
    bool bSingleRow = Params->TryGetStringField(TEXT("row_name"), RowName);

    if (bSingleRow)
    {
        // Find the specific row
        uint8* const* RowDataPtr = DataTable->GetRowMap().Find(FName(*RowName));
        if (!RowDataPtr)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Row not found: %s"), *RowName));
        }

        // Export just this row using property iteration
        TSharedPtr<FJsonObject> RowObj = MakeShared<FJsonObject>();
        if (RowStruct)
        {
            for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
            {
                FProperty* Property = *PropIt;
                FString ValueStr;
                Property->ExportTextItem_Direct(ValueStr, Property->ContainerPtrToValuePtr<void>(*RowDataPtr), nullptr, nullptr, PPF_None);
                RowObj->SetStringField(Property->GetName(), ValueStr);
            }
        }

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("data_table_path"), DataTablePath);
        ResultObj->SetStringField(TEXT("row_struct"), RowStructName);
        ResultObj->SetStringField(TEXT("row_name"), RowName);
        ResultObj->SetObjectField(TEXT("row_data"), RowObj);
        return ResultObj;
    }

    // Export the entire table as JSON and parse it back
    FString JsonString = DataTable->GetTableAsJSON(EDataTableExportFlags::UseJsonObjectsForStructs);

    TArray<TSharedPtr<FJsonValue>> ParsedRows;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, ParsedRows))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to serialize DataTable to JSON"));
    }

    // Build a map of row_name -> row_data using the Name field from each row
    TSharedPtr<FJsonObject> RowsObj = MakeShared<FJsonObject>();
    for (const TSharedPtr<FJsonValue>& RowValue : ParsedRows)
    {
        const TSharedPtr<FJsonObject>* RowObj;
        if (RowValue->TryGetObject(RowObj))
        {
            FString Name;
            if ((*RowObj)->TryGetStringField(TEXT("Name"), Name))
            {
                RowsObj->SetObjectField(Name, *RowObj);
            }
        }
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("data_table_path"), DataTablePath);
    ResultObj->SetStringField(TEXT("row_struct"), RowStructName);
    ResultObj->SetNumberField(TEXT("row_count"), DataTable->GetRowMap().Num());
    ResultObj->SetObjectField(TEXT("rows"), RowsObj);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleGetWorldSettings(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world is currently loaded"));
    }

    AWorldSettings* WorldSettings = World->GetWorldSettings();
    if (!WorldSettings)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Current world has no WorldSettings actor"));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("level"), World->GetOutermost()->GetName());
    ResultObj->SetStringField(TEXT("world_settings_class"), WorldSettings->GetClass()->GetPathName());
    if (WorldSettings->DefaultGameMode)
    {
        ResultObj->SetStringField(TEXT("game_mode_override"), WorldSettings->DefaultGameMode->GetPathName());
    }
    else
    {
        ResultObj->SetField(TEXT("game_mode_override"), MakeShared<FJsonValueNull>());
    }
    ResultObj->SetNumberField(TEXT("kill_z"), WorldSettings->KillZ);
    ResultObj->SetBoolField(TEXT("global_gravity_set"), WorldSettings->bGlobalGravitySet);
    ResultObj->SetNumberField(TEXT("global_gravity_z"), WorldSettings->GlobalGravityZ);
    ResultObj->SetNumberField(TEXT("world_to_meters"), WorldSettings->WorldToMeters);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleSetLevelGameMode(const TSharedPtr<FJsonObject>& Params)
{
    FString GameModePath;
    if (!Params->TryGetStringField(TEXT("game_mode_path"), GameModePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'game_mode_path' parameter"));
    }

    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world is currently loaded"));
    }

    AWorldSettings* WorldSettings = World->GetWorldSettings();
    if (!WorldSettings)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Current world has no WorldSettings actor"));
    }

    // Empty path clears the override (falls back to the project default game mode)
    UClass* GameModeClass = nullptr;
    if (!GameModePath.TrimStartAndEnd().IsEmpty())
    {
        FString FindError;
        UBlueprint* GameModeBlueprint = FEpicUnrealMCPCommonUtils::FindBlueprintByName(GameModePath, FindError);
        if (GameModeBlueprint && GameModeBlueprint->GeneratedClass)
        {
            GameModeClass = GameModeBlueprint->GeneratedClass;
        }
        else
        {
            // Not a Blueprint asset — try a direct class load (native class or explicit _C path)
            GameModeClass = LoadClass<AGameModeBase>(nullptr, *GameModePath);
        }

        if (!GameModeClass)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
                TEXT("Could not resolve a game mode class from '%s'%s%s"),
                *GameModePath,
                FindError.IsEmpty() ? TEXT("") : TEXT(": "),
                *FindError));
        }

        if (!GameModeClass->IsChildOf(AGameModeBase::StaticClass()))
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
                TEXT("Class '%s' is not a subclass of AGameModeBase"), *GameModeClass->GetPathName()));
        }
    }

    WorldSettings->Modify();
    WorldSettings->DefaultGameMode = GameModeClass;
    WorldSettings->MarkPackageDirty();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("level"), World->GetOutermost()->GetName());
    if (GameModeClass)
    {
        ResultObj->SetStringField(TEXT("game_mode_override"), GameModeClass->GetPathName());
    }
    else
    {
        ResultObj->SetField(TEXT("game_mode_override"), MakeShared<FJsonValueNull>());
    }
    return ResultObj;
}

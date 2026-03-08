#include "Commands/EpicUnrealMCPWidgetCommands.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"

// Widget Blueprint creation
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"

// UMG widget types
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/Border.h"
#include "Components/Spacer.h"
#include "Components/SizeBox.h"
#include "Components/PanelWidget.h"

// Asset management
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"

FEpicUnrealMCPWidgetCommands::FEpicUnrealMCPWidgetCommands()
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_widget_blueprint"))
    {
        return HandleCreateWidgetBlueprint(Params);
    }
    else if (CommandType == TEXT("add_widget_child"))
    {
        return HandleAddWidgetChild(Params);
    }
    else if (CommandType == TEXT("get_widget_children"))
    {
        return HandleGetWidgetChildren(Params);
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown widget command: %s"), *CommandType));
}

// ---------------------------------------------------------------------------
// create_widget_blueprint
// ---------------------------------------------------------------------------
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleCreateWidgetBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    FString PackagePath = TEXT("/Game/Blueprints/");
    if (UEditorAssetLibrary::DoesAssetExist(PackagePath + BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget Blueprint already exists: %s"), *BlueprintName));
    }

    UClass* ParentClass = UUserWidget::StaticClass();

    // Allow custom parent class that must derive from UUserWidget
    FString ParentClassName;
    if (Params->TryGetStringField(TEXT("parent_class"), ParentClassName) && !ParentClassName.IsEmpty())
    {
        if (!ParentClassName.Equals(TEXT("UserWidget"), ESearchCase::IgnoreCase))
        {
            // Try to resolve the parent class
            FString ClassName = ParentClassName;
            if (!ClassName.StartsWith(TEXT("U")))
            {
                ClassName = TEXT("U") + ClassName;
            }

            UClass* FoundClass = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/UMG.%s"), *ClassName));
            if (!FoundClass)
            {
                FoundClass = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/Engine.%s"), *ClassName));
            }
            if (FoundClass && FoundClass->IsChildOf(UUserWidget::StaticClass()))
            {
                ParentClass = FoundClass;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Could not find UUserWidget-derived class '%s', defaulting to UUserWidget"), *ParentClassName);
            }
        }
    }

    UPackage* Package = CreatePackage(*(PackagePath + BlueprintName));

    UWidgetBlueprint* NewBP = CastChecked<UWidgetBlueprint>(
        FKismetEditorUtilities::CreateBlueprint(
            ParentClass,
            Package,
            *BlueprintName,
            BPTYPE_Normal,
            UWidgetBlueprint::StaticClass(),
            UWidgetBlueprintGeneratedClass::StaticClass(),
            NAME_None
        )
    );

    if (!NewBP)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Widget Blueprint"));
    }

    // Create a CanvasPanel as the root widget (standard UE default)
    if (NewBP->WidgetTree->RootWidget == nullptr)
    {
        UWidget* Root = NewBP->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
        NewBP->WidgetTree->RootWidget = Root;
        NewBP->OnVariableAdded(Root->GetFName());
    }

    FAssetRegistryModule::AssetCreated(NewBP);
    Package->MarkPackageDirty();

    // Compile so it's usable immediately
    FKismetEditorUtilities::CompileBlueprint(NewBP);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("name"), BlueprintName);
    ResultObj->SetStringField(TEXT("path"), PackagePath + BlueprintName);
    ResultObj->SetStringField(TEXT("root_widget"), TEXT("RootCanvas"));
    ResultObj->SetStringField(TEXT("root_widget_type"), TEXT("CanvasPanel"));
    return ResultObj;
}

// ---------------------------------------------------------------------------
// add_widget_child
// ---------------------------------------------------------------------------
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleAddWidgetChild(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString WidgetType;
    if (!Params->TryGetStringField(TEXT("widget_type"), WidgetType))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_type' parameter"));
    }

    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
    }

    // Find the Widget Blueprint
    UWidgetBlueprint* WidgetBP = FindWidgetBlueprint(BlueprintName);
    if (!WidgetBP)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget Blueprint not found: %s"), *BlueprintName));
    }

    // Resolve the widget class
    UClass* WidgetClass = ResolveWidgetClass(WidgetType);
    if (!WidgetClass)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown widget type: %s. Supported: CanvasPanel, ProgressBar, TextBlock, Image, Button, HorizontalBox, VerticalBox, Border, Spacer, SizeBox"), *WidgetType));
    }

    // Resolve parent widget
    UPanelWidget* ParentWidget = nullptr;
    FString ParentWidgetName;
    if (Params->TryGetStringField(TEXT("parent_widget_name"), ParentWidgetName) && !ParentWidgetName.IsEmpty())
    {
        UWidget* Found = WidgetBP->WidgetTree->FindWidget(FName(*ParentWidgetName));
        if (!Found)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Parent widget not found: %s"), *ParentWidgetName));
        }
        ParentWidget = Cast<UPanelWidget>(Found);
        if (!ParentWidget)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Parent widget '%s' is not a panel/container widget"), *ParentWidgetName));
        }
    }
    else
    {
        // Default to root widget
        ParentWidget = Cast<UPanelWidget>(WidgetBP->WidgetTree->RootWidget);
        if (!ParentWidget)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Root widget is not a panel/container. Specify parent_widget_name."));
        }
    }

    // Check if a widget with this name already exists
    if (WidgetBP->WidgetTree->FindWidget(FName(*WidgetName)))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget with name '%s' already exists"), *WidgetName));
    }

    // Construct the widget
    UWidget* NewWidget = WidgetBP->WidgetTree->ConstructWidget<UWidget>(WidgetClass, FName(*WidgetName));
    if (!NewWidget)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to construct widget of type %s"), *WidgetType));
    }

    // Add to parent
    UPanelSlot* Slot = ParentWidget->AddChild(NewWidget);
    if (!Slot)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to add widget to parent"));
    }

    // Register as a variable so it's accessible in the event graph
    WidgetBP->OnVariableAdded(NewWidget->GetFName());

    // Configure canvas slot properties if parent is a CanvasPanel
    UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot);
    if (CanvasSlot)
    {
        // Position
        const TArray<TSharedPtr<FJsonValue>>* PositionArray;
        if (Params->TryGetArrayField(TEXT("position"), PositionArray) && PositionArray->Num() >= 2)
        {
            float X = (*PositionArray)[0]->AsNumber();
            float Y = (*PositionArray)[1]->AsNumber();
            CanvasSlot->SetPosition(FVector2D(X, Y));
        }

        // Size
        const TArray<TSharedPtr<FJsonValue>>* SizeArray;
        if (Params->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray->Num() >= 2)
        {
            float W = (*SizeArray)[0]->AsNumber();
            float H = (*SizeArray)[1]->AsNumber();
            CanvasSlot->SetSize(FVector2D(W, H));
        }

        // Anchors [MinX, MinY, MaxX, MaxY]
        const TArray<TSharedPtr<FJsonValue>>* AnchorsArray;
        if (Params->TryGetArrayField(TEXT("anchors"), AnchorsArray) && AnchorsArray->Num() >= 4)
        {
            FAnchors Anchors;
            Anchors.Minimum.X = (*AnchorsArray)[0]->AsNumber();
            Anchors.Minimum.Y = (*AnchorsArray)[1]->AsNumber();
            Anchors.Maximum.X = (*AnchorsArray)[2]->AsNumber();
            Anchors.Maximum.Y = (*AnchorsArray)[3]->AsNumber();
            CanvasSlot->SetAnchors(Anchors);
        }

        // Alignment [X, Y]
        const TArray<TSharedPtr<FJsonValue>>* AlignmentArray;
        if (Params->TryGetArrayField(TEXT("alignment"), AlignmentArray) && AlignmentArray->Num() >= 2)
        {
            float X = (*AlignmentArray)[0]->AsNumber();
            float Y = (*AlignmentArray)[1]->AsNumber();
            CanvasSlot->SetAlignment(FVector2D(X, Y));
        }

        // Z-order
        int32 ZOrder;
        if (Params->TryGetNumberField(TEXT("z_order"), ZOrder))
        {
            CanvasSlot->SetZOrder(ZOrder);
        }
    }

    // Apply widget-specific properties
    const TSharedPtr<FJsonObject>* PropertiesObj;
    if (Params->TryGetObjectField(TEXT("properties"), PropertiesObj))
    {
        ApplyWidgetProperties(NewWidget, *PropertiesObj);
    }

    // Mark dirty and compile
    FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
    ResultObj->SetStringField(TEXT("widget_type"), WidgetType);
    ResultObj->SetStringField(TEXT("parent"), ParentWidget->GetName());
    ResultObj->SetBoolField(TEXT("has_canvas_slot"), CanvasSlot != nullptr);
    return ResultObj;
}

// ---------------------------------------------------------------------------
// get_widget_children
// ---------------------------------------------------------------------------
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleGetWidgetChildren(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UWidgetBlueprint* WidgetBP = FindWidgetBlueprint(BlueprintName);
    if (!WidgetBP)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget Blueprint not found: %s"), *BlueprintName));
    }

    TArray<TSharedPtr<FJsonValue>> WidgetArray;

    WidgetBP->WidgetTree->ForEachWidget([&WidgetArray](UWidget* Widget)
    {
        if (Widget)
        {
            TSharedPtr<FJsonObject> WidgetObj = MakeShared<FJsonObject>();
            WidgetObj->SetStringField(TEXT("name"), Widget->GetName());

            // Type name without the U prefix
            FString TypeName = Widget->GetClass()->GetName();
            WidgetObj->SetStringField(TEXT("type"), TypeName);

            // Parent info
            UPanelWidget* ParentPanel = Widget->GetParent();
            if (ParentPanel)
            {
                WidgetObj->SetStringField(TEXT("parent"), ParentPanel->GetName());
            }
            else
            {
                WidgetObj->SetField(TEXT("parent"), MakeShared<FJsonValueNull>());
            }

            // Is this a container?
            WidgetObj->SetBoolField(TEXT("is_panel"), Widget->IsA<UPanelWidget>());
            if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
            {
                WidgetObj->SetNumberField(TEXT("child_count"), Panel->GetChildrenCount());
            }

            // Slot info (canvas slot properties)
            UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot);
            if (CanvasSlot)
            {
                TSharedPtr<FJsonObject> SlotObj = MakeShared<FJsonObject>();

                FVector2D Position = CanvasSlot->GetPosition();
                TArray<TSharedPtr<FJsonValue>> PosArr;
                PosArr.Add(MakeShared<FJsonValueNumber>(Position.X));
                PosArr.Add(MakeShared<FJsonValueNumber>(Position.Y));
                SlotObj->SetArrayField(TEXT("position"), PosArr);

                FVector2D Size = CanvasSlot->GetSize();
                TArray<TSharedPtr<FJsonValue>> SizeArr;
                SizeArr.Add(MakeShared<FJsonValueNumber>(Size.X));
                SizeArr.Add(MakeShared<FJsonValueNumber>(Size.Y));
                SlotObj->SetArrayField(TEXT("size"), SizeArr);

                FAnchors Anchors = CanvasSlot->GetAnchors();
                TArray<TSharedPtr<FJsonValue>> AnchorsArr;
                AnchorsArr.Add(MakeShared<FJsonValueNumber>(Anchors.Minimum.X));
                AnchorsArr.Add(MakeShared<FJsonValueNumber>(Anchors.Minimum.Y));
                AnchorsArr.Add(MakeShared<FJsonValueNumber>(Anchors.Maximum.X));
                AnchorsArr.Add(MakeShared<FJsonValueNumber>(Anchors.Maximum.Y));
                SlotObj->SetArrayField(TEXT("anchors"), AnchorsArr);

                FVector2D Alignment = CanvasSlot->GetAlignment();
                TArray<TSharedPtr<FJsonValue>> AlignArr;
                AlignArr.Add(MakeShared<FJsonValueNumber>(Alignment.X));
                AlignArr.Add(MakeShared<FJsonValueNumber>(Alignment.Y));
                SlotObj->SetArrayField(TEXT("alignment"), AlignArr);

                SlotObj->SetNumberField(TEXT("z_order"), CanvasSlot->GetZOrder());

                WidgetObj->SetObjectField(TEXT("slot"), SlotObj);
            }

            WidgetArray.Add(MakeShared<FJsonValueObject>(WidgetObj));
        }
    });

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint"), BlueprintName);
    ResultObj->SetArrayField(TEXT("widgets"), WidgetArray);
    ResultObj->SetNumberField(TEXT("widget_count"), WidgetArray.Num());
    return ResultObj;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
UWidgetBlueprint* FEpicUnrealMCPWidgetCommands::FindWidgetBlueprint(const FString& BlueprintName)
{
    // Reuse the common blueprint finder, then cast
    UBlueprint* BP = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!BP)
    {
        return nullptr;
    }
    return Cast<UWidgetBlueprint>(BP);
}

UClass* FEpicUnrealMCPWidgetCommands::ResolveWidgetClass(const FString& WidgetType)
{
    static TMap<FString, UClass*> WidgetClassMap;
    if (WidgetClassMap.Num() == 0)
    {
        WidgetClassMap.Add(TEXT("CanvasPanel"), UCanvasPanel::StaticClass());
        WidgetClassMap.Add(TEXT("ProgressBar"), UProgressBar::StaticClass());
        WidgetClassMap.Add(TEXT("TextBlock"), UTextBlock::StaticClass());
        WidgetClassMap.Add(TEXT("Image"), UImage::StaticClass());
        WidgetClassMap.Add(TEXT("Button"), UButton::StaticClass());
        WidgetClassMap.Add(TEXT("HorizontalBox"), UHorizontalBox::StaticClass());
        WidgetClassMap.Add(TEXT("VerticalBox"), UVerticalBox::StaticClass());
        WidgetClassMap.Add(TEXT("Border"), UBorder::StaticClass());
        WidgetClassMap.Add(TEXT("Spacer"), USpacer::StaticClass());
        WidgetClassMap.Add(TEXT("SizeBox"), USizeBox::StaticClass());
    }

    UClass** Found = WidgetClassMap.Find(WidgetType);
    return Found ? *Found : nullptr;
}

FLinearColor FEpicUnrealMCPWidgetCommands::GetColorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    const TArray<TSharedPtr<FJsonValue>>* ColorArray;
    if (JsonObject->TryGetArrayField(FieldName, ColorArray) && ColorArray->Num() >= 3)
    {
        float R = (*ColorArray)[0]->AsNumber();
        float G = (*ColorArray)[1]->AsNumber();
        float B = (*ColorArray)[2]->AsNumber();
        float A = ColorArray->Num() >= 4 ? (*ColorArray)[3]->AsNumber() : 1.0f;
        return FLinearColor(R, G, B, A);
    }
    return FLinearColor::White;
}

void FEpicUnrealMCPWidgetCommands::ApplyWidgetProperties(UWidget* Widget, const TSharedPtr<FJsonObject>& Properties)
{
    if (!Widget || !Properties.IsValid())
    {
        return;
    }

    // Common properties
    FString VisibilityStr;
    if (Properties->TryGetStringField(TEXT("Visibility"), VisibilityStr))
    {
        if (VisibilityStr == TEXT("Visible")) Widget->SetVisibility(ESlateVisibility::Visible);
        else if (VisibilityStr == TEXT("Collapsed")) Widget->SetVisibility(ESlateVisibility::Collapsed);
        else if (VisibilityStr == TEXT("Hidden")) Widget->SetVisibility(ESlateVisibility::Hidden);
        else if (VisibilityStr == TEXT("HitTestInvisible")) Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
        else if (VisibilityStr == TEXT("SelfHitTestInvisible")) Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    bool bIsEnabled;
    if (Properties->TryGetBoolField(TEXT("IsEnabled"), bIsEnabled))
    {
        Widget->SetIsEnabled(bIsEnabled);
    }

    double RenderOpacity;
    if (Properties->TryGetNumberField(TEXT("RenderOpacity"), RenderOpacity))
    {
        Widget->SetRenderOpacity(static_cast<float>(RenderOpacity));
    }

    FString ToolTipText;
    if (Properties->TryGetStringField(TEXT("ToolTipText"), ToolTipText))
    {
        Widget->SetToolTipText(FText::FromString(ToolTipText));
    }

    // ProgressBar
    if (UProgressBar* ProgressBar = Cast<UProgressBar>(Widget))
    {
        double Percent;
        if (Properties->TryGetNumberField(TEXT("Percent"), Percent))
        {
            ProgressBar->SetPercent(static_cast<float>(Percent));
        }

        if (Properties->HasField(TEXT("FillColorAndOpacity")))
        {
            ProgressBar->SetFillColorAndOpacity(GetColorFromJson(Properties, TEXT("FillColorAndOpacity")));
        }

        bool bIsMarquee;
        if (Properties->TryGetBoolField(TEXT("IsMarquee"), bIsMarquee))
        {
            ProgressBar->SetIsMarquee(bIsMarquee);
        }
    }

    // TextBlock
    if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
    {
        FString Text;
        if (Properties->TryGetStringField(TEXT("Text"), Text))
        {
            TextBlock->SetText(FText::FromString(Text));
        }

        if (Properties->HasField(TEXT("ColorAndOpacity")))
        {
            FLinearColor Color = GetColorFromJson(Properties, TEXT("ColorAndOpacity"));
            TextBlock->SetColorAndOpacity(FSlateColor(Color));
        }

        double FontSize;
        if (Properties->TryGetNumberField(TEXT("FontSize"), FontSize))
        {
            FSlateFontInfo FontInfo = TextBlock->GetFont();
            FontInfo.Size = static_cast<int32>(FontSize);
            TextBlock->SetFont(FontInfo);
        }

        FString Justification;
        if (Properties->TryGetStringField(TEXT("Justification"), Justification))
        {
            if (Justification == TEXT("Left")) TextBlock->SetJustification(ETextJustify::Left);
            else if (Justification == TEXT("Center")) TextBlock->SetJustification(ETextJustify::Center);
            else if (Justification == TEXT("Right")) TextBlock->SetJustification(ETextJustify::Right);
        }
    }

    // Image
    if (UImage* Image = Cast<UImage>(Widget))
    {
        if (Properties->HasField(TEXT("ColorAndOpacity")))
        {
            Image->SetColorAndOpacity(GetColorFromJson(Properties, TEXT("ColorAndOpacity")));
        }

        FString BrushTexturePath;
        if (Properties->TryGetStringField(TEXT("Brush"), BrushTexturePath))
        {
            UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *BrushTexturePath);
            if (Texture)
            {
                Image->SetBrushFromTexture(Texture);
            }
        }
    }

    // Button
    if (UButton* Button = Cast<UButton>(Widget))
    {
        if (Properties->HasField(TEXT("BackgroundColor")))
        {
            Button->SetBackgroundColor(GetColorFromJson(Properties, TEXT("BackgroundColor")));
        }
    }

    // Border
    if (UBorder* Border = Cast<UBorder>(Widget))
    {
        if (Properties->HasField(TEXT("ContentColorAndOpacity")))
        {
            Border->SetContentColorAndOpacity(GetColorFromJson(Properties, TEXT("ContentColorAndOpacity")));
        }

        if (Properties->HasField(TEXT("BrushColor")))
        {
            Border->SetBrushColor(GetColorFromJson(Properties, TEXT("BrushColor")));
        }
    }

    // SizeBox
    if (USizeBox* SizeBox = Cast<USizeBox>(Widget))
    {
        double WidthOverride;
        if (Properties->TryGetNumberField(TEXT("WidthOverride"), WidthOverride))
        {
            SizeBox->SetWidthOverride(static_cast<float>(WidthOverride));
        }

        double HeightOverride;
        if (Properties->TryGetNumberField(TEXT("HeightOverride"), HeightOverride))
        {
            SizeBox->SetHeightOverride(static_cast<float>(HeightOverride));
        }
    }
}

TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::WidgetToJson(UWidget* Widget)
{
    if (!Widget)
    {
        return nullptr;
    }

    TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
    Obj->SetStringField(TEXT("name"), Widget->GetName());
    Obj->SetStringField(TEXT("type"), Widget->GetClass()->GetName());
    return Obj;
}

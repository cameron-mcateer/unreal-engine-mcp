#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UnrealMCPSettings.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnUnrealMCPSettingsChanged);

/**
 * Settings for the UnrealMCP plugin.
 * Accessible via Project Settings > Plugins > UnrealMCP.
 */
UCLASS(config=Engine, defaultconfig, meta=(DisplayName="UnrealMCP"))
class UNREALMCP_API UUnrealMCPSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UUnrealMCPSettings()
		: BindAddress(TEXT("127.0.0.1"))
		, Port(55557)
	{
	}

	/** IP address the MCP server binds to. Use 0.0.0.0 to accept connections from any interface (e.g. WSL). */
	UPROPERTY(config, EditAnywhere, Category="Server", meta=(DisplayName="Bind Address"))
	FString BindAddress;

	/** TCP port the MCP server listens on. */
	UPROPERTY(config, EditAnywhere, Category="Server", meta=(DisplayName="Port", ClampMin=1, ClampMax=65535))
	int32 Port;

	FOnUnrealMCPSettingsChanged OnSettingsChanged;

	virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);
		OnSettingsChanged.Broadcast();
	}
#endif
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DTDigitalTwinGameMode.generated.h"

UCLASS()
class BUILDING_API ADTDigitalTwinGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADTDigitalTwinGameMode();
	virtual void BeginPlay() override;
	virtual void RestartPlayer(AController* NewPlayer) override;
};

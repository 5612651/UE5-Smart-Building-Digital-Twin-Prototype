#include "DTDigitalTwinGameMode.h"

#include "DTDigitalTwinPlayerController.h"
#include "DigitalTwinManager.h"
#include "GameFramework/HUD.h"
#include "GameFramework/SpectatorPawn.h"

ADTDigitalTwinGameMode::ADTDigitalTwinGameMode()
{
	PlayerControllerClass = ADTDigitalTwinPlayerController::StaticClass();
	DefaultPawnClass = ASpectatorPawn::StaticClass();
	HUDClass = AHUD::StaticClass();
	bStartPlayersAsSpectators = true;
}

void ADTDigitalTwinGameMode::BeginPlay()
{
	Super::BeginPlay();
	ADigitalTwinManager::FindOrCreate(GetWorld());
}

void ADTDigitalTwinGameMode::RestartPlayer(AController* NewPlayer)
{
	// This presentation is camera-driven and intentionally has no possessed gameplay pawn.
	// Skipping the base implementation avoids a redundant PlayerStart/pawn spawn attempt.
	(void)NewPlayer;
}

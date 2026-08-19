#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DTDigitalTwinPlayerController.generated.h"

class ADigitalTwinManager;
class UDigitalTwinControlWidget;
class UDTDigitalTwinLoadingWidget;

UCLASS()
class BUILDING_API ADTDigitalTwinPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	UFUNCTION() void ShowMainInterface();

	UPROPERTY(Transient) TObjectPtr<ADigitalTwinManager> Manager;
	UPROPERTY(Transient) TObjectPtr<UDigitalTwinControlWidget> ControlWidget;
	UPROPERTY(Transient) TObjectPtr<UDTDigitalTwinLoadingWidget> LoadingWidget;
	FTimerHandle LoadingTimerHandle;
	bool bInterfaceReady = false;

	bool bWasLeftMouseDown = false;
	bool bSceneClickActive = false;
	bool bPointerMovedDuringClick = false;
	FVector2D MousePressPosition = FVector2D::ZeroVector;
	FVector2D PreviousMousePosition = FVector2D::ZeroVector;
};

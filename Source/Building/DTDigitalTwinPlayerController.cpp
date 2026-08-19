#include "DTDigitalTwinPlayerController.h"

#include "DigitalTwinControlWidget.h"
#include "DigitalTwinManager.h"
#include "DTOrbitCamera.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogDigitalTwinInput, Log, All);

void ADTDigitalTwinPlayerController::BeginPlay()
{
	Super::BeginPlay();
	Manager = ADigitalTwinManager::FindOrCreate(GetWorld());

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	LoadingWidget = CreateWidget<UDTDigitalTwinLoadingWidget>(this, UDTDigitalTwinLoadingWidget::StaticClass());
	if (LoadingWidget)
	{
		LoadingWidget->AddToViewport(200);
	}
	GetWorldTimerManager().SetTimer(LoadingTimerHandle, this,
		&ADTDigitalTwinPlayerController::ShowMainInterface, 1.5f, false);
}

void ADTDigitalTwinPlayerController::ShowMainInterface()
{
	if (LoadingWidget)
	{
		LoadingWidget->RemoveFromParent();
		LoadingWidget = nullptr;
	}
	ControlWidget = CreateWidget<UDigitalTwinControlWidget>(this, UDigitalTwinControlWidget::StaticClass());
	if (ControlWidget) ControlWidget->AddToViewport(100);
	bInterfaceReady = ControlWidget != nullptr;
	if (bInterfaceReady)
	{
		UE_LOG(LogDigitalTwinInput, Log, TEXT("Digital Twin interface ready"));
	}
	else
	{
		UE_LOG(LogDigitalTwinInput, Error, TEXT("Failed to create Digital Twin interface"));
	}
}

void ADTDigitalTwinPlayerController::PlayerTick(const float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (!Manager || !bInterfaceReady) return;

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	const bool bHasMouse = GetMousePosition(MouseX, MouseY);
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	GetViewportSize(ViewportWidth, ViewportHeight);
	const bool bPointerInScene = ViewportWidth > 0 && ViewportHeight > 0
		&& MouseX > ViewportWidth * 0.015f && MouseX < ViewportWidth * 0.735f
		&& MouseY > ViewportHeight * 0.195f && MouseY < ViewportHeight * 0.78f;
	const bool bLeftMouseDown = IsInputKeyDown(EKeys::LeftMouseButton);
	const FVector2D Current(MouseX, MouseY);

	if (bLeftMouseDown && !bWasLeftMouseDown)
	{
		bSceneClickActive = bHasMouse && bPointerInScene;
		if (bSceneClickActive)
		{
			MousePressPosition = Current;
			PreviousMousePosition = Current;
			bPointerMovedDuringClick = false;
			if (Manager->GetViewMode() == EDTViewMode::Orbit)
			{
				Manager->BeginManualOrbitControl();
			}
		}
	}

	if (bSceneClickActive && bLeftMouseDown && bWasLeftMouseDown && bHasMouse && bPointerInScene
		&& Manager->GetViewMode() == EDTViewMode::Orbit && Manager->GetOrbitCamera())
	{
		const FVector2D Delta = Current - PreviousMousePosition;
		if (FVector2D::Distance(Current, MousePressPosition) > 5.0f)
		{
			bPointerMovedDuringClick = true;
			if (!Delta.IsNearlyZero())
			{
				Manager->GetOrbitCamera()->AddOrbitInput(Delta.X, Delta.Y);
			}
		}
		PreviousMousePosition = Current;
	}

	if (bSceneClickActive && !bLeftMouseDown && bWasLeftMouseDown && bHasMouse && bPointerInScene
		&& !bPointerMovedDuringClick && Manager->GetViewMode() != EDTViewMode::Room)
	{
		FVector RayOrigin;
		FVector RayDirection;
		if (DeprojectScreenPositionToWorld(MouseX, MouseY, RayOrigin, RayDirection))
		{
			Manager->SelectRoomFromRay(RayOrigin, RayDirection);
		}
	}
	if (!bLeftMouseDown) bSceneClickActive = false;
	bWasLeftMouseDown = bLeftMouseDown;

	const float Wheel = GetInputAnalogKeyState(EKeys::MouseWheelAxis);
	if (!FMath::IsNearlyZero(Wheel) && bHasMouse && bPointerInScene
		&& Manager->GetViewMode() == EDTViewMode::Orbit && Manager->GetOrbitCamera())
	{
		Manager->GetOrbitCamera()->AddZoomInput(Wheel);
	}
}

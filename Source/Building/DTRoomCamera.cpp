#include "DTRoomCamera.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

ADTRoomCamera::ADTRoomCamera()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SceneRoot);
	// Use the level's exposure consistently. Per-camera exposure overrides made the
	// overview and room views much darker than the orbit view in this scene.
	Camera->PostProcessSettings.bOverride_AutoExposureBias = false;
}

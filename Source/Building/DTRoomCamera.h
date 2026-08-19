#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DTRoomCamera.generated.h"

class UCameraComponent;
class USceneComponent;

UCLASS(Blueprintable)
class BUILDING_API ADTRoomCamera : public AActor
{
	GENERATED_BODY()

public:
	ADTRoomCamera();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Digital Twin")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Digital Twin")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Digital Twin")
	FName RoomId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Digital Twin")
	bool bDefaultView = false;
};

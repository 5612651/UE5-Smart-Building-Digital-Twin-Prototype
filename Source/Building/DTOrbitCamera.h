#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "DTOrbitCamera.generated.h"

class UCameraComponent;
class USceneComponent;
class USpringArmComponent;

UCLASS(Blueprintable)
class BUILDING_API ADTOrbitCamera : public APawn
{
	GENERATED_BODY()

public:
	ADTOrbitCamera();
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category="Digital Twin|Camera")
	void AddOrbitInput(float DeltaYaw, float DeltaPitch);

	UFUNCTION(BlueprintCallable, Category="Digital Twin|Camera")
	void AddZoomInput(float WheelDelta);

	UFUNCTION(BlueprintCallable, Category="Digital Twin|Camera")
	void ConfigureFromViewTransform(const FTransform& CameraTransform, float InDistance = 2500.0f);
	void ConfigureFromPivot(const FVector& PivotLocation, const FRotator& ViewRotation, float InDistance);

	void SetFramingOffset(float RightOffset, float VerticalOffset);

	void SetAutomaticRotation(bool bEnabled) { bAutomaticRotation = bEnabled; }
	bool IsAutomaticRotationEnabled() const { return bAutomaticRotation; }

	UFUNCTION(BlueprintPure, Category="Digital Twin|Camera")
	UCameraComponent* GetDigitalTwinCamera() const { return Camera; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Digital Twin|Camera")
	TObjectPtr<USceneComponent> PivotRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Digital Twin|Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Digital Twin|Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Digital Twin|Camera")
	float OrbitSensitivity = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Digital Twin|Camera")
	float ZoomStep = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Digital Twin|Camera")
	float MinimumDistance = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Digital Twin|Camera")
	float MaximumDistance = 30000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Digital Twin|Camera")
	float AutomaticRotationSpeed = 6.0f;

private:
	bool bAutomaticRotation = true;
};

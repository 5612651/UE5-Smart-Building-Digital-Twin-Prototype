#include "DTOrbitCamera.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

ADTOrbitCamera::ADTOrbitCamera()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;

	PivotRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PivotRoot"));
	SetRootComponent(PivotRoot);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(PivotRoot);
	SpringArm->TargetArmLength = 2500.0f;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 10.0f;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 12.0f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
}

void ADTOrbitCamera::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bAutomaticRotation)
	{
		FRotator Rotation = SpringArm->GetRelativeRotation();
		Rotation.Yaw += AutomaticRotationSpeed * DeltaSeconds;
		SpringArm->SetRelativeRotation(Rotation);
	}
}

void ADTOrbitCamera::AddOrbitInput(const float DeltaYaw, const float DeltaPitch)
{
	FRotator Rotation = SpringArm->GetRelativeRotation();
	Rotation.Yaw += DeltaYaw * OrbitSensitivity;
	Rotation.Pitch = FMath::Clamp(Rotation.Pitch + DeltaPitch * OrbitSensitivity, -80.0f, -5.0f);
	Rotation.Roll = 0.0f;
	SpringArm->SetRelativeRotation(Rotation);
}

void ADTOrbitCamera::AddZoomInput(const float WheelDelta)
{
	SpringArm->TargetArmLength = FMath::Clamp(
		SpringArm->TargetArmLength - WheelDelta * ZoomStep,
		MinimumDistance,
		MaximumDistance);
}

void ADTOrbitCamera::ConfigureFromViewTransform(const FTransform& CameraTransform, const float InDistance)
{
	const float Distance = FMath::Clamp(InDistance, MinimumDistance, MaximumDistance);
	const FVector PivotLocation = CameraTransform.GetLocation() + CameraTransform.GetRotation().GetForwardVector() * Distance;
	SetActorLocationAndRotation(PivotLocation, FRotator::ZeroRotator);
	SpringArm->TargetArmLength = Distance;
	SpringArm->SetRelativeRotation(CameraTransform.Rotator());
}

void ADTOrbitCamera::ConfigureFromPivot(const FVector& PivotLocation, const FRotator& ViewRotation, const float InDistance)
{
	SetActorLocationAndRotation(PivotLocation, FRotator::ZeroRotator);
	SpringArm->TargetArmLength = FMath::Clamp(InDistance, MinimumDistance, MaximumDistance);
	SpringArm->SetRelativeRotation(ViewRotation);
}

void ADTOrbitCamera::SetFramingOffset(const float RightOffset, const float VerticalOffset)
{
	// Move the camera in its local right/down directions without changing the
	// orbit pivot. The building consequently appears left/up in screen space.
	SpringArm->SocketOffset = FVector(0.0f, RightOffset, VerticalOffset);
}

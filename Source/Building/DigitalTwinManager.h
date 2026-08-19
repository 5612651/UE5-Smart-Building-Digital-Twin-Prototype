#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DigitalTwinTypes.h"
#include "DigitalTwinManager.generated.h"

class ADTOrbitCamera;
class ADTRoomCamera;
class APointLight;
class UDirectionalLightComponent;
class UDigitalTwinDataSubsystem;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDigitalTwinStateChanged);

UCLASS()
class BUILDING_API ADigitalTwinManager : public AActor
{
	GENERATED_BODY()

public:
	ADigitalTwinManager();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	bool SetViewMode(EDTViewMode NewMode);
	bool SelectRoom(FName RoomId);
	void SetSimulationDateTime(const FDateTime& NewDateTime);
	void SetTimePlaybackEnabled(bool bEnabled);
	void ChangeDay(int32 DayDelta);
	void ChangeMonth(int32 MonthDelta);
	void SetSelectedMetric(EDTMetric NewMetric);
	void ApplyPresentationPreset(EDTPresentationPreset NewPreset);
	void BeginManualOrbitControl();
	bool SelectRoomFromRay(const FVector& RayOrigin, const FVector& RayDirection);

	bool HasRoomCamera(FName RoomId) const;
	TArray<FName> GetRoomIds() const;
	EDTViewMode GetViewMode() const { return ViewMode; }
	FName GetSelectedRoomId() const { return SelectedRoomId; }
	FDateTime GetSimulationDateTime() const { return SimulationDateTime; }
	EDTMetric GetSelectedMetric() const { return SelectedMetric; }
	EDTPresentationPreset GetPresentationPreset() const { return PresentationPreset; }
	bool IsTimePlaybackActive() const;
	bool IsTimePlaybackEnabled() const { return bTimePlaybackEnabled; }
	ADTOrbitCamera* GetOrbitCamera() const { return OrbitCamera; }
	int32 GetRoomBoxCount() const { return RoomBoxes.Num(); }
	static FLinearColor GetTemperatureColor(float TemperatureC);
	static FLinearColor GetMetricColor(EDTMetric Metric, const FRoomSensorData& Sensor, const FRoomMetadata& Metadata);
	static FString GetMetricDisplayName(EDTMetric Metric);
	static FString GetPresentationPresetDisplayName(EDTPresentationPreset Preset);
	static void GetMetricLegend(EDTMetric Metric, TArray<FLinearColor>& OutColors, TArray<FString>& OutLabels);

	bool GetSelectedRoomData(FRoomMetadata& OutMetadata, FRoomSensorData& OutSensorData) const;
	bool GetOverviewData(FBuildingOverviewData& OutData) const;

	UPROPERTY(BlueprintAssignable)
	FOnDigitalTwinStateChanged OnStateChanged;

	static ADigitalTwinManager* FindOrCreate(UWorld* World);

private:
	void DiscoverRoomCameras();
	void DiscoverRoomActors();
	void DiscoverDayNightRig();
	void CreateRuntimeCameras();
	void UpdateDayNightLighting();
	void UpdateNightLights();
	void UpdateRoomBoxColors();
	void NotifyStateChanged(bool bUpdateEnvironment);
	static FName ExtractRoomIdTag(const AActor* Actor);
	AActor* GetTargetForMode(EDTViewMode Mode) const;
	UDigitalTwinDataSubsystem* GetDataSubsystem() const;
	void BroadcastState();

	UPROPERTY(Transient)
	TObjectPtr<ADTOrbitCamera> OrbitCamera;

	UPROPERTY(Transient)
	TObjectPtr<ADTRoomCamera> OverviewCamera;

	UPROPERTY(Transient)
	TObjectPtr<ADTRoomCamera> FocusCamera;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<ADTRoomCamera>> RoomCameras;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<AActor>> RoomBoxes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<APointLight>> RoomLights;

	UPROPERTY(Transient)
	TObjectPtr<UDirectionalLightComponent> SunLight;

	UPROPERTY(Transient)
	TObjectPtr<UDirectionalLightComponent> MoonLight;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UMaterialInstanceDynamic>> RoomBoxMaterials;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> TemperatureOverlayMaterial;

	EDTViewMode ViewMode = EDTViewMode::Orbit;
	FName SelectedRoomId = NAME_None;
	FDateTime SimulationDateTime = FDateTime(2027, 1, 1, 0, 0, 0);
	EDTMetric SelectedMetric = EDTMetric::Temperature;
	EDTPresentationPreset PresentationPreset = EDTPresentationPreset::Normal;
	float CameraBlendTime = 0.6f;
	float PlaybackAccumulatorSeconds = 0.0f;
	float RoomDiscoveryAccumulatorSeconds = 0.0f;
	float SecondsPerSimulatedHour = 2.0f;
	bool bTimePlaybackEnabled = true;
	float SolarLatitudeDegrees = 22.34f;
	float SolarNorthYawDegrees = 0.0f;
};

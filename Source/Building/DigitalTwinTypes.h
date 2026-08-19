#pragma once

#include "CoreMinimal.h"
#include "DigitalTwinTypes.generated.h"

UENUM(BlueprintType)
enum class EDTViewMode : uint8
{
	Orbit,
	Room,
	Overview
};

UENUM(BlueprintType)
enum class EDTMetric : uint8
{
	Temperature,
	Humidity,
	CO2,
	Occupancy,
	Energy,
	HVAC
};

UENUM(BlueprintType)
enum class EDTPresentationPreset : uint8
{
	Custom,
	Normal,
	Peak,
	Incident
};

USTRUCT(BlueprintType)
struct BUILDING_API FRoomMetadata
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FName RoomId;
	UPROPERTY(BlueprintReadOnly) FName UEActorTag;
	UPROPERTY(BlueprintReadOnly) int32 Floor = 0;
	UPROPERTY(BlueprintReadOnly) FString FunctionCN;
	UPROPERTY(BlueprintReadOnly) FString Function;
	UPROPERTY(BlueprintReadOnly) float AreaSqM = 0.0f;
	UPROPERTY(BlueprintReadOnly) int32 Capacity = 0;
	UPROPERTY(BlueprintReadOnly) FString HVACZone;
	UPROPERTY(BlueprintReadOnly) FString AHUId;
	UPROPERTY(BlueprintReadOnly) FString TemperatureSensorId;
	UPROPERTY(BlueprintReadOnly) FString HumiditySensorId;
	UPROPERTY(BlueprintReadOnly) FString CO2SensorId;
	UPROPERTY(BlueprintReadOnly) FString OccupancySensorId;
	UPROPERTY(BlueprintReadOnly) FString EnergyMeterId;
};

USTRUCT(BlueprintType)
struct BUILDING_API FRoomSensorData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FDateTime Timestamp;
	UPROPERTY(BlueprintReadOnly) FString Date;
	UPROPERTY(BlueprintReadOnly) FString DayOfWeek;
	UPROPERTY(BlueprintReadOnly) int32 Hour = 0;
	UPROPERTY(BlueprintReadOnly) FName RoomId;
	UPROPERTY(BlueprintReadOnly) float TemperatureC = 0.0f;
	UPROPERTY(BlueprintReadOnly) float HumidityPct = 0.0f;
	UPROPERTY(BlueprintReadOnly) int32 CO2ppm = 0;
	UPROPERTY(BlueprintReadOnly) int32 Occupancy = 0;
	UPROPERTY(BlueprintReadOnly) float EnergyKWh = 0.0f;
	UPROPERTY(BlueprintReadOnly) float HVACLoadPct = 0.0f;
	UPROPERTY(BlueprintReadOnly) FString HVACMode;
	UPROPERTY(BlueprintReadOnly) float FreshAirPct = 0.0f;
	UPROPERTY(BlueprintReadOnly) float LightingKW = 0.0f;
};

USTRUCT(BlueprintType)
struct BUILDING_API FBuildingOverviewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FDateTime Timestamp;
	UPROPERTY(BlueprintReadOnly) FString Date;
	UPROPERTY(BlueprintReadOnly) FString DayOfWeek;
	UPROPERTY(BlueprintReadOnly) int32 Hour = 0;
	UPROPERTY(BlueprintReadOnly) float TotalEnergyKWh = 0.0f;
	UPROPERTY(BlueprintReadOnly) int32 TotalOccupancy = 0;
	UPROPERTY(BlueprintReadOnly) float OccupancyRatePct = 0.0f;
	UPROPERTY(BlueprintReadOnly) float AverageTemperatureC = 0.0f;
	UPROPERTY(BlueprintReadOnly) float AverageHumidityPct = 0.0f;
	UPROPERTY(BlueprintReadOnly) int32 AverageCO2ppm = 0;
	UPROPERTY(BlueprintReadOnly) float AverageHVACLoadPct = 0.0f;
	UPROPERTY(BlueprintReadOnly) int32 HVACActiveRooms = 0;
	UPROPERTY(BlueprintReadOnly) float AverageFreshAirPct = 0.0f;
	UPROPERTY(BlueprintReadOnly) float TotalLightingKW = 0.0f;
	UPROPERTY(BlueprintReadOnly) int32 AlertRooms = 0;
};

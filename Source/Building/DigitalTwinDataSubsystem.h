#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DigitalTwinTypes.h"
#include "DigitalTwinDataSubsystem.generated.h"

UCLASS()
class BUILDING_API UDigitalTwinDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category="Digital Twin|Data")
	bool GetRoomMetadata(FName RoomId, FRoomMetadata& OutMetadata) const;

	UFUNCTION(BlueprintCallable, Category="Digital Twin|Data")
	bool GetRoomSensorData(FName RoomId, const FDateTime& DateTime, FRoomSensorData& OutData);

	UFUNCTION(BlueprintCallable, Category="Digital Twin|Data")
	bool GetBuildingOverview(const FDateTime& DateTime, FBuildingOverviewData& OutData);

	UFUNCTION(BlueprintPure, Category="Digital Twin|Data")
	TArray<FName> GetKnownRoomIds() const;

	UFUNCTION(BlueprintPure, Category="Digital Twin|Data")
	FString GetLastError() const { return LastError; }

	static FDateTime GetMinimumDateTime();
	static FDateTime GetMaximumDateTime();

private:
	bool LoadMetadata();
	bool LoadRoomFile(FName RoomId);
	bool LoadOverview();
	bool ReadCsv(const FString& RelativePath, TArray<TArray<FString>>& OutRows);
	static FString MakeTimeKey(const FDateTime& DateTime);
	void SetError(const FString& Message);

	UPROPERTY(Transient)
	TMap<FName, FRoomMetadata> MetadataByRoom;

	TMap<FName, TMap<FString, FRoomSensorData>> RoomDataCache;
	TMap<FString, FBuildingOverviewData> OverviewCache;
	TSet<FName> FailedRoomLoads;
	bool bOverviewLoadAttempted = false;
	FString LastError;
};

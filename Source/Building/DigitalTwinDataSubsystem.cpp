#include "DigitalTwinDataSubsystem.h"

#include "Serialization/Csv/CsvParser.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogDigitalTwinData, Log, All);

namespace
{
	FString Cell(const TArray<FString>& Row, const int32 Index)
	{
		return Row.IsValidIndex(Index) ? Row[Index].TrimStartAndEnd() : FString();
	}

	bool ParseIsoDateTime(const FString& Value, FDateTime& OutDateTime)
	{
		return FDateTime::ParseIso8601(*Value, OutDateTime) || FDateTime::Parse(Value, OutDateTime);
	}
}

void UDigitalTwinDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadMetadata();
}

FDateTime UDigitalTwinDataSubsystem::GetMinimumDateTime()
{
	return FDateTime(2026, 1, 1, 0, 0, 0);
}

FDateTime UDigitalTwinDataSubsystem::GetMaximumDateTime()
{
	return FDateTime(2027, 12, 31, 23, 0, 0);
}

bool UDigitalTwinDataSubsystem::ReadCsv(const FString& RelativePath, TArray<TArray<FString>>& OutRows)
{
	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), RelativePath);
	FString CsvText;
	if (!FFileHelper::LoadFileToString(CsvText, *FullPath))
	{
		SetError(FString::Printf(TEXT("无法读取数据文件: %s"), *FullPath));
		return false;
	}

	FCsvParser Parser(CsvText);
	for (const TArray<const TCHAR*>& SourceRow : Parser.GetRows())
	{
		TArray<FString>& TargetRow = OutRows.AddDefaulted_GetRef();
		TargetRow.Reserve(SourceRow.Num());
		for (const TCHAR* SourceCell : SourceRow)
		{
			TargetRow.Emplace(SourceCell ? SourceCell : TEXT(""));
		}
	}
	return OutRows.Num() > 1;
}

bool UDigitalTwinDataSubsystem::LoadMetadata()
{
	TArray<TArray<FString>> Rows;
	if (!ReadCsv(TEXT("Data/DT_RoomMetadata.csv"), Rows))
	{
		return false;
	}

	for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
	{
		const TArray<FString>& Row = Rows[RowIndex];
		if (Row.Num() < 15 || Cell(Row, 1).IsEmpty())
		{
			continue;
		}

		FRoomMetadata Data;
		Data.RoomId = FName(*Cell(Row, 1));
		Data.UEActorTag = FName(*Cell(Row, 2));
		Data.Floor = FCString::Atoi(*Cell(Row, 3));
		Data.FunctionCN = Cell(Row, 4);
		Data.Function = Cell(Row, 5);
		Data.AreaSqM = FCString::Atof(*Cell(Row, 6));
		Data.Capacity = FCString::Atoi(*Cell(Row, 7));
		Data.HVACZone = Cell(Row, 8);
		Data.AHUId = Cell(Row, 9);
		Data.TemperatureSensorId = Cell(Row, 10);
		Data.HumiditySensorId = Cell(Row, 11);
		Data.CO2SensorId = Cell(Row, 12);
		Data.OccupancySensorId = Cell(Row, 13);
		Data.EnergyMeterId = Cell(Row, 14);
		MetadataByRoom.Add(Data.RoomId, MoveTemp(Data));
	}

	UE_LOG(LogDigitalTwinData, Log, TEXT("Loaded %d room metadata rows"), MetadataByRoom.Num());
	return MetadataByRoom.Num() > 0;
}

bool UDigitalTwinDataSubsystem::LoadRoomFile(const FName RoomId)
{
	if (RoomDataCache.Contains(RoomId))
	{
		return true;
	}
	if (FailedRoomLoads.Contains(RoomId))
	{
		return false;
	}

	TArray<TArray<FString>> Rows;
	const FString RelativePath = FString::Printf(TEXT("Data/BMSHourly/DT_BMS_%s.csv"), *RoomId.ToString());
	if (!ReadCsv(RelativePath, Rows))
	{
		FailedRoomLoads.Add(RoomId);
		return false;
	}

	TMap<FString, FRoomSensorData>& Cache = RoomDataCache.Add(RoomId);
	Cache.Reserve(FMath::Max(0, Rows.Num() - 1));
	for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
	{
		const TArray<FString>& Row = Rows[RowIndex];
		if (Row.Num() < 15)
		{
			continue;
		}
		FRoomSensorData Data;
		if (!ParseIsoDateTime(Cell(Row, 1), Data.Timestamp))
		{
			continue;
		}
		Data.Date = Cell(Row, 2);
		Data.DayOfWeek = Cell(Row, 3);
		Data.Hour = FCString::Atoi(*Cell(Row, 4));
		Data.RoomId = FName(*Cell(Row, 5));
		Data.TemperatureC = FCString::Atof(*Cell(Row, 6));
		Data.HumidityPct = FCString::Atof(*Cell(Row, 7));
		Data.CO2ppm = FCString::Atoi(*Cell(Row, 8));
		Data.Occupancy = FCString::Atoi(*Cell(Row, 9));
		Data.EnergyKWh = FCString::Atof(*Cell(Row, 10));
		Data.HVACLoadPct = FCString::Atof(*Cell(Row, 11));
		Data.HVACMode = Cell(Row, 12);
		Data.FreshAirPct = FCString::Atof(*Cell(Row, 13));
		Data.LightingKW = FCString::Atof(*Cell(Row, 14));
		Cache.Add(MakeTimeKey(Data.Timestamp), MoveTemp(Data));
	}

	UE_LOG(LogDigitalTwinData, Log, TEXT("Loaded %d records for %s"), Cache.Num(), *RoomId.ToString());
	return Cache.Num() > 0;
}

bool UDigitalTwinDataSubsystem::LoadOverview()
{
	if (bOverviewLoadAttempted)
	{
		return OverviewCache.Num() > 0;
	}
	bOverviewLoadAttempted = true;

	TArray<TArray<FString>> Rows;
	if (!ReadCsv(TEXT("Data/BMSHourly/DT_BMS_Overview.csv"), Rows))
	{
		return false;
	}

	OverviewCache.Reserve(FMath::Max(0, Rows.Num() - 1));
	for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
	{
		const TArray<FString>& Row = Rows[RowIndex];
		if (Row.Num() < 16)
		{
			continue;
		}
		FBuildingOverviewData Data;
		if (!ParseIsoDateTime(Cell(Row, 1), Data.Timestamp))
		{
			continue;
		}
		Data.Date = Cell(Row, 2);
		Data.DayOfWeek = Cell(Row, 3);
		Data.Hour = FCString::Atoi(*Cell(Row, 4));
		Data.TotalEnergyKWh = FCString::Atof(*Cell(Row, 5));
		Data.TotalOccupancy = FCString::Atoi(*Cell(Row, 6));
		Data.OccupancyRatePct = FCString::Atof(*Cell(Row, 7));
		Data.AverageTemperatureC = FCString::Atof(*Cell(Row, 8));
		Data.AverageHumidityPct = FCString::Atof(*Cell(Row, 9));
		Data.AverageCO2ppm = FCString::Atoi(*Cell(Row, 10));
		Data.AverageHVACLoadPct = FCString::Atof(*Cell(Row, 11));
		Data.HVACActiveRooms = FCString::Atoi(*Cell(Row, 12));
		Data.AverageFreshAirPct = FCString::Atof(*Cell(Row, 13));
		Data.TotalLightingKW = FCString::Atof(*Cell(Row, 14));
		Data.AlertRooms = FCString::Atoi(*Cell(Row, 15));
		OverviewCache.Add(MakeTimeKey(Data.Timestamp), MoveTemp(Data));
	}
	return OverviewCache.Num() > 0;
}

bool UDigitalTwinDataSubsystem::GetRoomMetadata(const FName RoomId, FRoomMetadata& OutMetadata) const
{
	if (const FRoomMetadata* Found = MetadataByRoom.Find(RoomId))
	{
		OutMetadata = *Found;
		return true;
	}
	return false;
}

bool UDigitalTwinDataSubsystem::GetRoomSensorData(const FName RoomId, const FDateTime& DateTime, FRoomSensorData& OutData)
{
	if (!LoadRoomFile(RoomId))
	{
		return false;
	}
	if (const FRoomSensorData* Found = RoomDataCache.FindChecked(RoomId).Find(MakeTimeKey(DateTime)))
	{
		OutData = *Found;
		LastError.Reset();
		return true;
	}
	SetError(FString::Printf(TEXT("找不到 %s 在 %s 的数据"), *RoomId.ToString(), *DateTime.ToString()));
	return false;
}

bool UDigitalTwinDataSubsystem::GetBuildingOverview(const FDateTime& DateTime, FBuildingOverviewData& OutData)
{
	if (!LoadOverview())
	{
		return false;
	}
	if (const FBuildingOverviewData* Found = OverviewCache.Find(MakeTimeKey(DateTime)))
	{
		OutData = *Found;
		LastError.Reset();
		return true;
	}
	SetError(FString::Printf(TEXT("找不到 %s 的总览数据"), *DateTime.ToString()));
	return false;
}

TArray<FName> UDigitalTwinDataSubsystem::GetKnownRoomIds() const
{
	TArray<FName> Result;
	MetadataByRoom.GetKeys(Result);
	Result.Sort(FNameLexicalLess());
	return Result;
}

FString UDigitalTwinDataSubsystem::MakeTimeKey(const FDateTime& DateTime)
{
	return DateTime.ToString(TEXT("%Y%m%d_%H"));
}

void UDigitalTwinDataSubsystem::SetError(const FString& Message)
{
	LastError = Message;
	UE_LOG(LogDigitalTwinData, Warning, TEXT("%s"), *Message);
}

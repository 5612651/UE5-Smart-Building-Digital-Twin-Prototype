#include "DigitalTwinManager.h"

#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TimelineComponent.h"
#include "DigitalTwinDataSubsystem.h"
#include "DTOrbitCamera.h"
#include "DTRoomCamera.h"
#include "EngineUtils.h"
#include "Engine/PointLight.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogDigitalTwin, Log, All);

namespace
{
	void NormalizeDirectionalLightPriorities(UWorld* World)
	{
		TArray<UDirectionalLightComponent*> Lights;
		for (TObjectIterator<UDirectionalLightComponent> It; It; ++It)
		{
			if (It->GetWorld() == World && It->IsRegistered() && It->IsVisible())
			{
				Lights.Add(*It);
			}
		}

		Lights.Sort([](const UDirectionalLightComponent& A, const UDirectionalLightComponent& B)
		{
			return A.Intensity > B.Intensity;
		});
		for (int32 Index = 0; Index < Lights.Num(); ++Index)
		{
			Lights[Index]->SetForwardShadingPriority(Index == 0 ? 1 : 0);
		}
		if (Lights.Num() > 1)
		{
			UE_LOG(LogDigitalTwin, Log, TEXT("Normalized forward-shading priority for %d directional lights"), Lights.Num());
		}
	}

	bool RayIntersectsBox(const FVector& Origin, const FVector& Direction, const FBox& Box, float& OutDistance)
	{
		float Near = 0.0f;
		float Far = TNumericLimits<float>::Max();
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			const float AxisDirection = Direction[Axis];
			if (FMath::Abs(AxisDirection) < KINDA_SMALL_NUMBER)
			{
				if (Origin[Axis] < Box.Min[Axis] || Origin[Axis] > Box.Max[Axis]) return false;
				continue;
			}
			float T1 = (Box.Min[Axis] - Origin[Axis]) / AxisDirection;
			float T2 = (Box.Max[Axis] - Origin[Axis]) / AxisDirection;
			if (T1 > T2) Swap(T1, T2);
			Near = FMath::Max(Near, T1);
			Far = FMath::Min(Far, T2);
			if (Near > Far) return false;
		}
		OutDistance = Near;
		return Far >= 0.0f;
	}
}

ADigitalTwinManager::ADigitalTwinManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ADigitalTwinManager::BeginPlay()
{
	Super::BeginPlay();
	NormalizeDirectionalLightPriorities(GetWorld());
	DiscoverRoomCameras();
	DiscoverRoomActors();
	DiscoverDayNightRig();
	CreateRuntimeCameras();
	ApplyPresentationPreset(EDTPresentationPreset::Normal);
}

void ADigitalTwinManager::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (RoomBoxes.Num() < 26)
	{
		RoomDiscoveryAccumulatorSeconds += DeltaSeconds;
		if (RoomDiscoveryAccumulatorSeconds >= 0.5f)
		{
			RoomDiscoveryAccumulatorSeconds = 0.0f;
			DiscoverRoomActors();
			UpdateNightLights();
			UpdateRoomBoxColors();
		}
	}
	if (!IsTimePlaybackActive())
	{
		PlaybackAccumulatorSeconds = 0.0f;
		return;
	}

	PlaybackAccumulatorSeconds += DeltaSeconds;
	while (PlaybackAccumulatorSeconds >= SecondsPerSimulatedHour && IsTimePlaybackActive())
	{
		PlaybackAccumulatorSeconds -= SecondsPerSimulatedHour;
		SetSimulationDateTime(SimulationDateTime + FTimespan::FromHours(1.0));
	}
}

bool ADigitalTwinManager::IsTimePlaybackActive() const
{
	return bTimePlaybackEnabled
		&& SimulationDateTime >= UDigitalTwinDataSubsystem::GetMinimumDateTime()
		&& SimulationDateTime < UDigitalTwinDataSubsystem::GetMaximumDateTime();
}

FName ADigitalTwinManager::ExtractRoomIdTag(const AActor* Actor)
{
	if (!Actor) return NAME_None;
	for (const FName Tag : Actor->Tags)
	{
		const FString Value = Tag.ToString();
		if (Value.StartsWith(TEXT("Room")) && Value.Len() == 7)
		{
			return Tag;
		}
	}
	return NAME_None;
}

void ADigitalTwinManager::DiscoverRoomActors()
{
	RoomBoxes.Reset();
	RoomLights.Reset();
	RoomBoxMaterials.Reset();
	TemperatureOverlayMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DigitalTwin/M_DTTemperatureOverlay.M_DTTemperatureOverlay"));

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		bool bIsRoomBox = Actor->ActorHasTag(TEXT("DT_RoomBox"));
		bool bIsRoomLight = Actor->ActorHasTag(TEXT("DT_RoomLight"));
		FName FallbackRoomId = NAME_None;
#if WITH_EDITOR
		const FString RuntimeActorLabel = Actor->GetActorLabel();
		if (!bIsRoomBox && RuntimeActorLabel.StartsWith(TEXT("RoomBox_")))
		{
			bIsRoomBox = true;
			const FString Suffix = RuntimeActorLabel.Right(3);
			if (Suffix.IsNumeric()) FallbackRoomId = FName(*FString::Printf(TEXT("Room%s"), *Suffix));
		}
		if (!bIsRoomLight && RuntimeActorLabel.StartsWith(TEXT("PointLight")) && Cast<APointLight>(Actor))
		{
			bIsRoomLight = true;
		}
#endif
		if (bIsRoomBox)
		{
			const FName TaggedRoomId = ExtractRoomIdTag(Actor);
			const FName RoomId = TaggedRoomId.IsNone() ? FallbackRoomId : TaggedRoomId;
			if (!RoomId.IsNone() && !RoomBoxes.Contains(RoomId))
			{
				RoomBoxes.Add(RoomId, Actor);
				Actor->SetActorHiddenInGame(false);
				if (TemperatureOverlayMaterial)
				{
					if (UStaticMeshComponent* Mesh = Actor->FindComponentByClass<UStaticMeshComponent>())
					{
						Mesh->SetVisibility(true, true);
						UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(TemperatureOverlayMaterial, this);
						Material->SetScalarParameterValue(TEXT("Opacity"), 0.18f);
						Mesh->SetMaterial(0, Material);
						Mesh->SetCastShadow(false);
						Mesh->SetTranslucentSortPriority(20);
						RoomBoxMaterials.Add(RoomId, Material);
					}
				}
			}
		}
		if (bIsRoomLight)
		{
			if (APointLight* Light = Cast<APointLight>(Actor))
			{
				RoomLights.Add(Light);
			}
		}
	}
	UE_LOG(LogDigitalTwin, Log, TEXT("Discovered room visualization actors: boxes=%d lights=%d material=%s"),
		RoomBoxes.Num(), RoomLights.Num(), TemperatureOverlayMaterial ? TEXT("yes") : TEXT("no"));
}

void ADigitalTwinManager::DiscoverDayNightRig()
{
	SunLight = nullptr;
	MoonLight = nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		const FString ClassName = Actor->GetClass()->GetName();
		const FString ActorName = Actor->GetName();
		if (!ClassName.Contains(TEXT("BP_Light"), ESearchCase::IgnoreCase)
			&& !ActorName.Contains(TEXT("BP_Light"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		Actor->SetActorTickEnabled(false);
		TArray<UTimelineComponent*> Timelines;
		Actor->GetComponents<UTimelineComponent>(Timelines);
		for (UTimelineComponent* Timeline : Timelines)
		{
			if (Timeline)
			{
				Timeline->Stop();
				Timeline->SetComponentTickEnabled(false);
			}
		}

		TArray<UDirectionalLightComponent*> DirectionalLights;
		Actor->GetComponents<UDirectionalLightComponent>(DirectionalLights);
		for (UDirectionalLightComponent* Light : DirectionalLights)
		{
			if (!Light) continue;
			const FString ComponentName = Light->GetName();
			if (ComponentName.Contains(TEXT("Sun"), ESearchCase::IgnoreCase))
			{
				SunLight = Light;
			}
			else if (ComponentName.Contains(TEXT("Moon"), ESearchCase::IgnoreCase))
			{
				MoonLight = Light;
			}
		}
		break;
	}

	UpdateDayNightLighting();
	if (SunLight && MoonLight)
	{
		UE_LOG(LogDigitalTwin, Log, TEXT("Day/night rig: sun=yes moon=yes"));
	}
	else
	{
		UE_LOG(LogDigitalTwin, Warning, TEXT("Day/night rig incomplete: sun=%s moon=%s"),
			SunLight ? TEXT("yes") : TEXT("no"), MoonLight ? TEXT("yes") : TEXT("no"));
	}
}

void ADigitalTwinManager::UpdateDayNightLighting()
{
	if (!SunLight && !MoonLight) return;

	const int32 DayOfYear = SimulationDateTime.GetDayOfYear();
	const float SolarHour = static_cast<float>(SimulationDateTime.GetHour())
		+ static_cast<float>(SimulationDateTime.GetMinute()) / 60.0f;
	const float Latitude = FMath::DegreesToRadians(SolarLatitudeDegrees);
	const float Declination = FMath::DegreesToRadians(
		23.44f * FMath::Sin(FMath::DegreesToRadians((360.0f / 365.0f) * (284.0f + DayOfYear))));
	const float HourAngle = FMath::DegreesToRadians(15.0f * (SolarHour - 12.0f));

	const float East = -FMath::Cos(Declination) * FMath::Sin(HourAngle);
	const float North = FMath::Cos(Latitude) * FMath::Sin(Declination)
		- FMath::Sin(Latitude) * FMath::Cos(Declination) * FMath::Cos(HourAngle);
	const float Up = FMath::Sin(Latitude) * FMath::Sin(Declination)
		+ FMath::Cos(Latitude) * FMath::Cos(Declination) * FMath::Cos(HourAngle);
	FVector SunVector(East, North, Up);
	SunVector = SunVector.RotateAngleAxis(SolarNorthYawDegrees, FVector::UpVector).GetSafeNormal();

	if (SunLight)
	{
		SunLight->SetWorldRotation((-SunVector).Rotation());
		SunLight->SetVisibility(Up > -0.04f, true);
		SunLight->MarkRenderStateDirty();
	}
	if (MoonLight)
	{
		MoonLight->SetWorldRotation(SunVector.Rotation());
		MoonLight->SetVisibility(Up <= 0.02f, true);
		MoonLight->MarkRenderStateDirty();
	}
}

ADigitalTwinManager* ADigitalTwinManager::FindOrCreate(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ADigitalTwinManager> It(World); It; ++It)
	{
		return *It;
	}
	return World->SpawnActor<ADigitalTwinManager>();
}

void ADigitalTwinManager::DiscoverRoomCameras()
{
	RoomCameras.Reset();
	for (TActorIterator<ADTRoomCamera> It(GetWorld()); It; ++It)
	{
		ADTRoomCamera* Camera = *It;
		if (Camera->Camera)
		{
			Camera->Camera->PostProcessSettings.bOverride_AutoExposureBias = false;
		}
		if (Camera->bDefaultView)
		{
			if (!OverviewCamera)
			{
				OverviewCamera = Camera;
			}
			continue;
		}
		if (Camera->RoomId.IsNone())
		{
			continue;
		}
		if (!RoomCameras.Contains(Camera->RoomId))
		{
			RoomCameras.Add(Camera->RoomId, Camera);
		}
		else
		{
			UE_LOG(LogDigitalTwin, Warning, TEXT("Duplicate room camera: %s"), *Camera->RoomId.ToString());
		}
	}
	UE_LOG(LogDigitalTwin, Log, TEXT("Discovered %d persistent room cameras"), RoomCameras.Num());
}

void ADigitalTwinManager::CreateRuntimeCameras()
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;

	OrbitCamera = GetWorld()->SpawnActor<ADTOrbitCamera>(ADTOrbitCamera::StaticClass(), FTransform::Identity, Params);
	if (OrbitCamera)
	{
		FBox BuildingBounds(ForceInit);
		for (const TPair<FName, TObjectPtr<AActor>>& Pair : RoomBoxes)
		{
			if (Pair.Value) BuildingBounds += Pair.Value->GetComponentsBoundingBox(true);
		}
		const FVector BuildingCenter = BuildingBounds.IsValid ? BuildingBounds.GetCenter() : FVector::ZeroVector;
		const float InitialOrbitDistance = 15000.0f;
		OrbitCamera->ConfigureFromPivot(BuildingCenter, FRotator(-22.0f, 0.0f, 0.0f), InitialOrbitDistance);
		// The playable scene occupies roughly the left 75% of the viewport and
		// excludes the header/timeline. These offsets center the building in that
		// safe region instead of in the full screen.
		OrbitCamera->SetFramingOffset(1600.0f, -800.0f);
		OrbitCamera->Camera->SetFieldOfView(67.0f);
		UE_LOG(LogDigitalTwin, Log, TEXT("Orbit framing: center=%s extent=%s distance=%.0f"),
			*BuildingCenter.ToCompactString(), BuildingBounds.IsValid ? *BuildingBounds.GetExtent().ToCompactString() : TEXT("unavailable"), InitialOrbitDistance);
	}

	if (!OverviewCamera)
	{
		OverviewCamera = GetWorld()->SpawnActor<ADTRoomCamera>(
			ADTRoomCamera::StaticClass(),
			FTransform(FRotator(-13.2613f, 138.0811f, -0.5357f), FVector(5205.0f, -4350.0f, 2025.0f)),
			Params);
		if (OverviewCamera)
		{
			OverviewCamera->bDefaultView = true;
			OverviewCamera->Camera->SetFieldOfView(64.656f);
		}
	}
	if (OverviewCamera && OverviewCamera->Camera)
	{
		// Reserve the right side for the data panel and the bottom for the
		// timeline. Room cameras intentionally keep their authored transforms.
		OverviewCamera->Camera->SetRelativeLocation(FVector(0.0f, 850.0f, -330.0f));
		OverviewCamera->Camera->SetFieldOfView(68.0f);
	}

	FocusCamera = GetWorld()->SpawnActor<ADTRoomCamera>(ADTRoomCamera::StaticClass(), FTransform::Identity, Params);
	if (FocusCamera)
	{
		FocusCamera->Camera->SetFieldOfView(55.0f);
	}

	UE_LOG(LogDigitalTwin, Log, TEXT("Runtime cameras: orbit=%s overview=%s rooms=%d"),
		OrbitCamera ? TEXT("yes") : TEXT("no"), OverviewCamera ? TEXT("yes") : TEXT("no"), RoomCameras.Num());
}

bool ADigitalTwinManager::SetViewMode(const EDTViewMode NewMode)
{
	AActor* Target = GetTargetForMode(NewMode);
	if (!Target)
	{
		UE_LOG(LogDigitalTwin, Warning, TEXT("View mode %d has no camera"), static_cast<int32>(NewMode));
		return false;
	}

	ViewMode = NewMode;
	if (NewMode == EDTViewMode::Orbit && OrbitCamera)
	{
		OrbitCamera->SetAutomaticRotation(true);
	}
	if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
	{
		Controller->SetViewTargetWithBlend(Target, CameraBlendTime, VTBlend_Cubic);
	}
	UE_LOG(LogDigitalTwin, Log, TEXT("View changed: mode=%d room=%s"), static_cast<int32>(ViewMode), *SelectedRoomId.ToString());
	NotifyStateChanged(true);
	return true;
}

void ADigitalTwinManager::BeginManualOrbitControl()
{
	if (OrbitCamera)
	{
		OrbitCamera->SetAutomaticRotation(false);
	}
}

bool ADigitalTwinManager::SelectRoomFromRay(const FVector& RayOrigin, const FVector& RayDirection)
{
	FName BestRoom = NAME_None;
	float BestDistance = TNumericLimits<float>::Max();
	for (const TPair<FName, TObjectPtr<AActor>>& Pair : RoomBoxes)
	{
		if (!Pair.Value) continue;
		float Distance = 0.0f;
		if (RayIntersectsBox(RayOrigin, RayDirection, Pair.Value->GetComponentsBoundingBox(true), Distance)
			&& Distance < BestDistance)
		{
			BestDistance = Distance;
			BestRoom = Pair.Key;
		}
	}
	if (BestRoom.IsNone()) return false;
	UE_LOG(LogDigitalTwin, Log, TEXT("Room box selected: %s"), *BestRoom.ToString());
	return SelectRoom(BestRoom);
}

bool ADigitalTwinManager::SelectRoom(const FName RoomId)
{
	if (!RoomCameras.Contains(RoomId))
	{
		UE_LOG(LogDigitalTwin, Warning, TEXT("Room camera unavailable: %s"), *RoomId.ToString());
		return false;
	}
	SelectedRoomId = RoomId;
	ADTRoomCamera* const RoomCamera = RoomCameras.FindRef(RoomId);
	if (!RoomCamera)
	{
		UE_LOG(LogDigitalTwin, Warning, TEXT("Room camera became invalid: %s"), *RoomId.ToString());
		return false;
	}
	return SetViewMode(EDTViewMode::Room);
}

void ADigitalTwinManager::SetSimulationDateTime(const FDateTime& NewDateTime)
{
	const FDateTime Min = UDigitalTwinDataSubsystem::GetMinimumDateTime();
	const FDateTime Max = UDigitalTwinDataSubsystem::GetMaximumDateTime();
	const FDateTime WholeHour(NewDateTime.GetYear(), NewDateTime.GetMonth(), NewDateTime.GetDay(), NewDateTime.GetHour());
	SimulationDateTime = WholeHour < Min ? Min : (WholeHour > Max ? Max : WholeHour);
	PresentationPreset = EDTPresentationPreset::Custom;
	UE_LOG(LogDigitalTwin, Log, TEXT("Simulation time: %s"), *SimulationDateTime.ToString(TEXT("%Y-%m-%d %H:00")));
	NotifyStateChanged(true);
}

void ADigitalTwinManager::SetTimePlaybackEnabled(const bool bEnabled)
{
	if (bTimePlaybackEnabled == bEnabled) return;
	bTimePlaybackEnabled = bEnabled;
	PlaybackAccumulatorSeconds = 0.0f;
	UE_LOG(LogDigitalTwin, Log, TEXT("Time playback: %s"), bTimePlaybackEnabled ? TEXT("realtime") : TEXT("frozen"));
	NotifyStateChanged(false);
}

void ADigitalTwinManager::ChangeDay(const int32 DayDelta)
{
	SetSimulationDateTime(SimulationDateTime + FTimespan::FromDays(DayDelta));
}

void ADigitalTwinManager::ChangeMonth(const int32 MonthDelta)
{
	int32 Year = SimulationDateTime.GetYear();
	int32 Month = SimulationDateTime.GetMonth() + MonthDelta;
	while (Month < 1) { Month += 12; --Year; }
	while (Month > 12) { Month -= 12; ++Year; }
	const int32 Day = FMath::Min(SimulationDateTime.GetDay(), FDateTime::DaysInMonth(Year, Month));
	SetSimulationDateTime(FDateTime(Year, Month, Day, SimulationDateTime.GetHour()));
}

void ADigitalTwinManager::SetSelectedMetric(const EDTMetric NewMetric)
{
	if (SelectedMetric == NewMetric)
	{
		return;
	}
	SelectedMetric = NewMetric;
	PresentationPreset = EDTPresentationPreset::Custom;
	NotifyStateChanged(false);
}

void ADigitalTwinManager::ApplyPresentationPreset(const EDTPresentationPreset NewPreset)
{
	if (NewPreset == EDTPresentationPreset::Custom) return;

	PresentationPreset = NewPreset;
	bTimePlaybackEnabled = false;
	PlaybackAccumulatorSeconds = 0.0f;
	SelectedRoomId = NAME_None;
	ViewMode = EDTViewMode::Orbit;
	switch (NewPreset)
	{
	case EDTPresentationPreset::Normal:
		SimulationDateTime = FDateTime(2027, 9, 11, 9);
		SelectedMetric = EDTMetric::Temperature;
		break;
	case EDTPresentationPreset::Peak:
		SimulationDateTime = FDateTime(2026, 5, 26, 14);
		SelectedMetric = EDTMetric::Occupancy;
		break;
	case EDTPresentationPreset::Incident:
		SimulationDateTime = FDateTime(2027, 10, 20, 14);
		SelectedMetric = EDTMetric::CO2;
		break;
	default:
		break;
	}

	if (OrbitCamera) OrbitCamera->SetAutomaticRotation(true);
	if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (AActor* Target = GetTargetForMode(EDTViewMode::Orbit))
		{
			Controller->SetViewTargetWithBlend(Target, CameraBlendTime, VTBlend_Cubic);
		}
	}
	UE_LOG(LogDigitalTwin, Log, TEXT("Presentation preset: %s at %s"),
		*GetPresentationPresetDisplayName(NewPreset), *SimulationDateTime.ToString(TEXT("%Y-%m-%d %H:00")));
	NotifyStateChanged(true);
}

FString ADigitalTwinManager::GetPresentationPresetDisplayName(const EDTPresentationPreset Preset)
{
	switch (Preset)
	{
	case EDTPresentationPreset::Normal: return TEXT("正常运行");
	case EDTPresentationPreset::Peak: return TEXT("上课高峰");
	case EDTPresentationPreset::Incident: return TEXT("异常事件");
	default: return TEXT("自定义");
	}
}

FLinearColor ADigitalTwinManager::GetTemperatureColor(const float TemperatureC)
{
	if (TemperatureC < 20.0f) return FLinearColor(0.05f, 0.25f, 1.0f, 1.0f);
	if (TemperatureC < 22.0f) return FLinearColor(0.0f, 0.75f, 1.0f, 1.0f);
	if (TemperatureC < 24.0f) return FLinearColor(0.1f, 0.85f, 0.3f, 1.0f);
	if (TemperatureC < 26.0f) return FLinearColor(1.0f, 0.55f, 0.05f, 1.0f);
	return FLinearColor(1.0f, 0.08f, 0.04f, 1.0f);
}

FLinearColor ADigitalTwinManager::GetMetricColor(const EDTMetric Metric, const FRoomSensorData& Sensor, const FRoomMetadata& Metadata)
{
	auto FiveBand = [](const float Value, const float A, const float B, const float C, const float D)
	{
		if (Value < A) return FLinearColor(0.05f, 0.28f, 1.0f, 1.0f);
		if (Value < B) return FLinearColor(0.0f, 0.72f, 1.0f, 1.0f);
		if (Value < C) return FLinearColor(0.08f, 0.82f, 0.34f, 1.0f);
		if (Value < D) return FLinearColor(1.0f, 0.58f, 0.04f, 1.0f);
		return FLinearColor(1.0f, 0.08f, 0.04f, 1.0f);
	};

	switch (Metric)
	{
	case EDTMetric::Temperature: return GetTemperatureColor(Sensor.TemperatureC);
	case EDTMetric::Humidity: return FiveBand(Sensor.HumidityPct, 35.0f, 45.0f, 60.0f, 70.0f);
	case EDTMetric::CO2: return FiveBand(static_cast<float>(Sensor.CO2ppm), 500.0f, 700.0f, 900.0f, 1100.0f);
	case EDTMetric::Occupancy:
		return FiveBand(Metadata.Capacity > 0 ? 100.0f * Sensor.Occupancy / Metadata.Capacity : 0.0f, 10.0f, 35.0f, 65.0f, 90.0f);
	case EDTMetric::Energy: return FiveBand(Sensor.EnergyKWh, 0.6f, 1.0f, 1.5f, 2.0f);
	case EDTMetric::HVAC: return FiveBand(Sensor.HVACLoadPct, 10.0f, 35.0f, 60.0f, 85.0f);
	default: return FLinearColor::Gray;
	}
}

FString ADigitalTwinManager::GetMetricDisplayName(const EDTMetric Metric)
{
	switch (Metric)
	{
	case EDTMetric::Temperature: return TEXT("温度");
	case EDTMetric::Humidity: return TEXT("湿度");
	case EDTMetric::CO2: return TEXT("CO₂");
	case EDTMetric::Occupancy: return TEXT("占用率");
	case EDTMetric::Energy: return TEXT("能耗");
	case EDTMetric::HVAC: return TEXT("HVAC 负载");
	default: return TEXT("指标");
	}
}

void ADigitalTwinManager::GetMetricLegend(const EDTMetric Metric, TArray<FLinearColor>& OutColors, TArray<FString>& OutLabels)
{
	OutColors.Reset(5);
	OutLabels.Reset(5);
	FRoomMetadata Metadata;
	Metadata.Capacity = 100;
	FRoomSensorData Sensor;
	TArray<float> Samples;
	switch (Metric)
	{
	case EDTMetric::Temperature: Samples = {18.0f, 21.0f, 23.0f, 25.0f, 27.0f}; OutLabels = {TEXT("< 20°C  偏冷"), TEXT("20–22°C  凉爽"), TEXT("22–24°C  舒适"), TEXT("24–26°C  偏暖"), TEXT("≥ 26°C  过热")}; break;
	case EDTMetric::Humidity: Samples = {30.0f, 40.0f, 52.0f, 65.0f, 75.0f}; OutLabels = {TEXT("< 35%  干燥"), TEXT("35–45%  较干"), TEXT("45–60%  舒适"), TEXT("60–70%  偏湿"), TEXT("≥ 70%  潮湿")}; break;
	case EDTMetric::CO2: Samples = {450.0f, 600.0f, 800.0f, 1000.0f, 1200.0f}; OutLabels = {TEXT("< 500 ppm  优"), TEXT("500–700  良"), TEXT("700–900  一般"), TEXT("900–1100  偏高"), TEXT("≥ 1100  告警")}; break;
	case EDTMetric::Occupancy: Samples = {5.0f, 20.0f, 50.0f, 75.0f, 95.0f}; OutLabels = {TEXT("< 10%  空闲"), TEXT("10–35%  低占用"), TEXT("35–65%  适中"), TEXT("65–90%  拥挤"), TEXT("≥ 90%  满载")}; break;
	case EDTMetric::Energy: Samples = {0.4f, 0.8f, 1.2f, 1.7f, 2.2f}; OutLabels = {TEXT("< 0.6 kWh  低"), TEXT("0.6–1.0  较低"), TEXT("1.0–1.5  正常"), TEXT("1.5–2.0  较高"), TEXT("≥ 2.0  高")}; break;
	case EDTMetric::HVAC: Samples = {5.0f, 20.0f, 45.0f, 70.0f, 95.0f}; OutLabels = {TEXT("< 10%  待机"), TEXT("10–35%  低负载"), TEXT("35–60%  正常"), TEXT("60–85%  高负载"), TEXT("≥ 85%  满负载")}; break;
	default: break;
	}

	for (const float Sample : Samples)
	{
		Sensor.TemperatureC = Sample;
		Sensor.HumidityPct = Sample;
		Sensor.CO2ppm = FMath::RoundToInt(Sample);
		Sensor.Occupancy = FMath::RoundToInt(Sample);
		Sensor.EnergyKWh = Sample;
		Sensor.HVACLoadPct = Sample;
		OutColors.Add(GetMetricColor(Metric, Sensor, Metadata));
	}
}

bool ADigitalTwinManager::HasRoomCamera(const FName RoomId) const
{
	return RoomCameras.Contains(RoomId);
}

TArray<FName> ADigitalTwinManager::GetRoomIds() const
{
	TArray<FName> Result;
	RoomCameras.GetKeys(Result);
	Result.Sort(FNameLexicalLess());
	return Result;
}

bool ADigitalTwinManager::GetSelectedRoomData(FRoomMetadata& OutMetadata, FRoomSensorData& OutSensorData) const
{
	if (UDigitalTwinDataSubsystem* Data = GetDataSubsystem())
	{
		return Data->GetRoomMetadata(SelectedRoomId, OutMetadata)
			&& Data->GetRoomSensorData(SelectedRoomId, SimulationDateTime, OutSensorData);
	}
	return false;
}

bool ADigitalTwinManager::GetOverviewData(FBuildingOverviewData& OutData) const
{
	if (UDigitalTwinDataSubsystem* Data = GetDataSubsystem())
	{
		return Data->GetBuildingOverview(SimulationDateTime, OutData);
	}
	return false;
}

AActor* ADigitalTwinManager::GetTargetForMode(const EDTViewMode Mode) const
{
	switch (Mode)
	{
	case EDTViewMode::Orbit:
		return OrbitCamera;
	case EDTViewMode::Room:
		return RoomCameras.FindRef(SelectedRoomId);
	case EDTViewMode::Overview:
		return OverviewCamera;
	default:
		return nullptr;
	}
}

UDigitalTwinDataSubsystem* ADigitalTwinManager::GetDataSubsystem() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UDigitalTwinDataSubsystem>() : nullptr;
}

void ADigitalTwinManager::BroadcastState()
{
	NotifyStateChanged(true);
}

void ADigitalTwinManager::NotifyStateChanged(const bool bUpdateEnvironment)
{
	if (bUpdateEnvironment)
	{
		UpdateNightLights();
		UpdateDayNightLighting();
	}
	UpdateRoomBoxColors();
	OnStateChanged.Broadcast();
}

void ADigitalTwinManager::UpdateNightLights()
{
	const int32 Hour = SimulationDateTime.GetHour();
	const bool bNight = Hour >= 17 || Hour <= 7;
	for (APointLight* Light : RoomLights)
	{
		if (!Light) continue;
		Light->SetActorHiddenInGame(!bNight);
		if (UPointLightComponent* Component = Light->FindComponentByClass<UPointLightComponent>())
		{
			Component->SetCastShadows(false);
			Component->SetVisibility(bNight, true);
		}
	}
	UE_LOG(LogDigitalTwin, Log, TEXT("Room lights: %s at %02d:00 (%d actors)"), bNight ? TEXT("on") : TEXT("off"), Hour, RoomLights.Num());
}

void ADigitalTwinManager::UpdateRoomBoxColors()
{
	UDigitalTwinDataSubsystem* Data = GetDataSubsystem();
	if (!Data) return;
	for (const TPair<FName, TObjectPtr<AActor>>& Pair : RoomBoxes)
	{
		if (Pair.Value)
		{
			if (UStaticMeshComponent* Mesh = Pair.Value->FindComponentByClass<UStaticMeshComponent>())
			{
				// Room boxes are an overview/orbit visualization layer. Keeping even the
				// selected box visible places its translucent geometry in front of the
				// corresponding interior camera.
				const bool bVisible = ViewMode != EDTViewMode::Room;
				Mesh->SetVisibility(bVisible, true);
			}
		}
	}
	for (const TPair<FName, TObjectPtr<UMaterialInstanceDynamic>>& Pair : RoomBoxMaterials)
	{
		if (!Pair.Value) continue;
		FRoomSensorData Sensor;
		FRoomMetadata Metadata;
		if (Data->GetRoomSensorData(Pair.Key, SimulationDateTime, Sensor) && Data->GetRoomMetadata(Pair.Key, Metadata))
		{
			Pair.Value->SetVectorParameterValue(TEXT("RoomColor"), GetMetricColor(SelectedMetric, Sensor, Metadata));
			Pair.Value->SetScalarParameterValue(TEXT("Opacity"), Pair.Key == SelectedRoomId ? 0.28f : 0.16f);
		}
	}
}

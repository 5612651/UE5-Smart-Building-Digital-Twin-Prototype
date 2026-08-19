#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Blueprint/UserWidget.h"
#include "DigitalTwinTypes.h"
#include "DigitalTwinControlWidget.generated.h"

class ADigitalTwinManager;
class UCanvasPanel;
class UBorder;
class UHorizontalBox;
class UScrollBox;
class USizeBox;
class UVerticalBox;
class USlider;
class UTextBlock;
class UUniformGridPanel;
class UProgressBar;
class SWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNativeMetricClicked, EDTMetric, Metric);

UCLASS()
class BUILDING_API UDTDigitalTwinLoadingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient) TObjectPtr<UProgressBar> ProgressBar;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ProgressText;
	float ElapsedSeconds = 0.0f;
};

UCLASS()
class BUILDING_API UDTMetricButton : public UButton
{
	GENERATED_BODY()

public:
	void Setup(EDTMetric InMetric);

	UPROPERTY(BlueprintAssignable)
	FOnNativeMetricClicked OnMetricClicked;

private:
	UFUNCTION()
	void ForwardClick();

	EDTMetric Metric = EDTMetric::Temperature;
};

UCLASS()
class BUILDING_API UDigitalTwinControlWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildInterface();
	void BuildHeader(UCanvasPanel* Root);
	void BuildPresetBar(UCanvasPanel* Root);
	void BuildDetails(UCanvasPanel* Root);
	void BuildTimeline(UCanvasPanel* Root);
	void AddMetricButton(UVerticalBox* Column, EDTMetric Metric, const FString& Label);
	void AddLegendRow(UVerticalBox* Column, const FLinearColor& Color, const FString& Label);
	UButton* CreateButton(FName Name, const FString& Label, int32 FontSize = 15);
	UTextBlock* CreateText(FName Name, const FString& Text, int32 FontSize, FLinearColor Color);
	void RefreshDetails();
	void RefreshOrbitAlerts();
	void SetDetailsPresentation(bool bOrbitAlerts);
	void RefreshButtonStyles();
	void RefreshLegend();
	void ResetMetricValues(const FString& Placeholder = TEXT("--"));

	UFUNCTION() void OnOrbitClicked();
	UFUNCTION() void OnOverviewClicked();
	UFUNCTION() void OnPreviousMonthClicked();
	UFUNCTION() void OnPreviousDayClicked();
	UFUNCTION() void OnNextDayClicked();
	UFUNCTION() void OnNextMonthClicked();
	UFUNCTION() void OnHourChanged(float Value);
	UFUNCTION() void OnRealtimePreviewClicked();
	UFUNCTION() void OnFreezeClicked();
	UFUNCTION() void OnMetricClicked(EDTMetric Metric);
	UFUNCTION() void OnNormalPresetClicked();
	UFUNCTION() void OnPeakPresetClicked();
	UFUNCTION() void OnIncidentPresetClicked();
	UFUNCTION() void OnExitClicked();
	UFUNCTION() void OnRoomSelected(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() UWidget* GenerateRoomSelectorEntry(FString Item);
	UFUNCTION() void RefreshState();

	UPROPERTY(Transient) TObjectPtr<ADigitalTwinManager> Manager;
	UPROPERTY(Transient) TObjectPtr<UButton> OrbitButton;
	UPROPERTY(Transient) TObjectPtr<UButton> OverviewButton;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> RoomSelector;
	UPROPERTY(Transient) TObjectPtr<UButton> PreviousMonthButton;
	UPROPERTY(Transient) TObjectPtr<UButton> PreviousDayButton;
	UPROPERTY(Transient) TObjectPtr<UButton> NextDayButton;
	UPROPERTY(Transient) TObjectPtr<UButton> NextMonthButton;
	UPROPERTY(Transient) TObjectPtr<USlider> HourSlider;
	UPROPERTY(Transient) TObjectPtr<UButton> RealtimePreviewButton;
	UPROPERTY(Transient) TObjectPtr<UButton> FreezeButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DateText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HourText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailsTitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> RoomSummaryText;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> OrbitAlertScroll;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> OrbitAlertList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MetricDetailText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UButton> NormalPresetButton;
	UPROPERTY(Transient) TObjectPtr<UButton> PeakPresetButton;
	UPROPERTY(Transient) TObjectPtr<UButton> IncidentPresetButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PresetSummaryText;
	UPROPERTY(Transient) TObjectPtr<UButton> ExitButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LegendTitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> EquipmentTitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> EquipmentText;
	UPROPERTY(Transient) TMap<EDTMetric, TObjectPtr<UDTMetricButton>> MetricButtons;
	UPROPERTY(Transient) TMap<EDTMetric, TObjectPtr<UTextBlock>> MetricValueTexts;
	UPROPERTY(Transient) TArray<TObjectPtr<USizeBox>> MetricContainers;
	UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> LegendSwatches;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> LegendLabels;
	UPROPERTY(Transient) TArray<TObjectPtr<UHorizontalBox>> LegendRows;

	bool bSynchronizingSlider = false;
	bool bSynchronizingRoomSelector = false;
};

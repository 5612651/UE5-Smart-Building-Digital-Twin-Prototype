#include "DigitalTwinControlWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "DigitalTwinDataSubsystem.h"
#include "DigitalTwinManager.h"
#include "Engine/GameInstance.h"
#include "Kismet/KismetSystemLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogDigitalTwinUI, Log, All);

namespace
{
	const FLinearColor Panel(0.015f, 0.035f, 0.055f, 0.94f);
	const FLinearColor PanelSoft(0.02f, 0.055f, 0.078f, 0.95f);
	const FLinearColor Accent(0.0f, 0.68f, 0.82f, 1.0f);
	const FLinearColor ButtonNormal(0.045f, 0.105f, 0.145f, 1.0f);
	const FLinearColor TextPrimary(0.88f, 0.95f, 0.97f, 1.0f);
	const FLinearColor TextSecondary(0.5f, 0.68f, 0.74f, 1.0f);
	const FLinearColor DataOnline(0.24f, 0.92f, 0.58f, 1.0f);
	const FString RoomSelectorPlaceholder(TEXT("房间选择"));

	UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Widget, FVector2D Min, FVector2D Max, FMargin Offsets = FMargin())
	{
		UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
		Slot->SetAnchors(FAnchors(Min.X, Min.Y, Max.X, Max.Y));
		Slot->SetOffsets(Offsets);
		Slot->SetZOrder(10);
		return Slot;
	}
}

void UDTMetricButton::Setup(const EDTMetric InMetric)
{
	Metric = InMetric;
	OnClicked.AddUniqueDynamic(this, &UDTMetricButton::ForwardClick);
}

void UDTMetricButton::ForwardClick()
{
	OnMetricClicked.Broadcast(Metric);
}

TSharedRef<SWidget> UDTDigitalTwinLoadingWidget::RebuildWidget()
{
	if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("LoadingWidgetTree"));
	if (!WidgetTree->RootWidget)
	{
		UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LoadingBackground"));
		Background->SetBrushColor(FLinearColor(0.006f, 0.018f, 0.03f, 1.0f));
		Background->SetHorizontalAlignment(HAlign_Center);
		Background->SetVerticalAlignment(VAlign_Center);
		WidgetTree->RootWidget = Background;

		USizeBox* ContentSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LoadingContentSize"));
		ContentSize->SetWidthOverride(560.0f);
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LoadingColumn"));
		ContentSize->SetContent(Column);
		Background->SetContent(ContentSize);

		UTextBlock* Eyebrow = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Eyebrow->SetText(FText::FromString(TEXT("HKUST  /  SMART BUILDING")));
		Eyebrow->SetColorAndOpacity(FSlateColor(Accent));
		FSlateFontInfo EyebrowFont = Eyebrow->GetFont(); EyebrowFont.Size = 13; Eyebrow->SetFont(EyebrowFont);
		Column->AddChildToVerticalBox(Eyebrow)->SetHorizontalAlignment(HAlign_Center);

		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Title->SetText(FText::FromString(TEXT("智能建筑数字孪生平台")));
		Title->SetColorAndOpacity(FSlateColor(TextPrimary));
		FSlateFontInfo TitleFont = Title->GetFont(); TitleFont.Size = 32; Title->SetFont(TitleFont);
		Column->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 7.0f));

		ProgressText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LoadingProgressText"));
		ProgressText->SetText(FText::FromString(TEXT("正在初始化建筑模型、环境与 BMS 数据  ·  0%")));
		ProgressText->SetColorAndOpacity(FSlateColor(TextSecondary));
		FSlateFontInfo ProgressFont = ProgressText->GetFont(); ProgressFont.Size = 13; ProgressText->SetFont(ProgressFont);
		Column->AddChildToVerticalBox(ProgressText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));

		ProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("LoadingProgressBar"));
		ProgressBar->SetPercent(0.0f);
		ProgressBar->SetFillColorAndOpacity(Accent);
		Column->AddChildToVerticalBox(ProgressBar);

		UTextBlock* Footnote = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Footnote->SetText(FText::FromString(TEXT("26 SPACES  ·  17,544 HOURS  ·  REAL-TIME VISUALIZATION")));
		Footnote->SetColorAndOpacity(FSlateColor(FLinearColor(0.24f, 0.39f, 0.45f, 1.0f)));
		FSlateFontInfo FootnoteFont = Footnote->GetFont(); FootnoteFont.Size = 11; Footnote->SetFont(FootnoteFont);
		Column->AddChildToVerticalBox(Footnote)->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
	}
	return Super::RebuildWidget();
}

void UDTDigitalTwinLoadingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ElapsedSeconds = 0.0f;
}

void UDTDigitalTwinLoadingWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	ElapsedSeconds += InDeltaTime;
	const float Percent = FMath::Clamp(ElapsedSeconds / 1.5f, 0.0f, 1.0f);
	if (ProgressBar) ProgressBar->SetPercent(Percent);
	if (ProgressText)
	{
		ProgressText->SetText(FText::FromString(FString::Printf(
			TEXT("正在初始化建筑模型、环境与 BMS 数据  ·  %d%%"), FMath::RoundToInt(Percent * 100.0f))));
	}
}

TSharedRef<SWidget> UDigitalTwinControlWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (!WidgetTree->RootWidget)
	{
		BuildInterface();
	}

	return Super::RebuildWidget();
}

void UDigitalTwinControlWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Manager = ADigitalTwinManager::FindOrCreate(GetWorld());
	if (Manager)
	{
		Manager->OnStateChanged.AddUniqueDynamic(this, &UDigitalTwinControlWidget::RefreshState);
		RefreshState();
	}
	UE_LOG(LogDigitalTwinUI, Log, TEXT("Native control widget constructed"));
}

void UDigitalTwinControlWidget::NativeDestruct()
{
	if (Manager)
	{
		Manager->OnStateChanged.RemoveDynamic(this, &UDigitalTwinControlWidget::RefreshState);
	}
	Super::NativeDestruct();
}

void UDigitalTwinControlWidget::BuildInterface()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DT_Root"));
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Root;
	BuildHeader(Root);
	BuildPresetBar(Root);
	BuildDetails(Root);
	BuildTimeline(Root);
}

void UDigitalTwinControlWidget::BuildPresetBar(UCanvasPanel* Root)
{
	UBorder* Bar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PresetBar"));
	Bar->SetBrushColor(PanelSoft);
	Bar->SetPadding(FMargin(16.0f, 7.0f));
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PresetRow"));
	Bar->SetContent(Row);
	UTextBlock* Label = CreateText(TEXT("PresetLabel"), TEXT("演示预设"), 12, TextSecondary);
	Row->AddChildToHorizontalBox(Label)->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));

	NormalPresetButton = CreateButton(TEXT("NormalPresetButton"), TEXT("正常运行"), 12);
	PeakPresetButton = CreateButton(TEXT("PeakPresetButton"), TEXT("上课高峰"), 12);
	IncidentPresetButton = CreateButton(TEXT("IncidentPresetButton"), TEXT("异常事件"), 12);
	NormalPresetButton->OnClicked.AddDynamic(this, &UDigitalTwinControlWidget::OnNormalPresetClicked);
	PeakPresetButton->OnClicked.AddDynamic(this, &UDigitalTwinControlWidget::OnPeakPresetClicked);
	IncidentPresetButton->OnClicked.AddDynamic(this, &UDigitalTwinControlWidget::OnIncidentPresetClicked);
	for (UButton* Button : {NormalPresetButton.Get(), PeakPresetButton.Get(), IncidentPresetButton.Get()})
	{
		UHorizontalBoxSlot* PresetButtonSlot = Row->AddChildToHorizontalBox(Button);
		PresetButtonSlot->SetPadding(FMargin(3.0f, 0.0f));
		PresetButtonSlot->SetVerticalAlignment(VAlign_Center);
	}
	PresetSummaryText = CreateText(TEXT("PresetSummary"), TEXT("低负荷运行  ·  环境舒适"), 11, TextSecondary);
	UHorizontalBoxSlot* SummarySlot = Row->AddChildToHorizontalBox(PresetSummaryText);
	SummarySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	SummarySlot->SetPadding(FMargin(14.0f, 0.0f, 0.0f, 0.0f));
	SummarySlot->SetHorizontalAlignment(HAlign_Right);
	SummarySlot->SetVerticalAlignment(VAlign_Center);
	Place(Root, Bar, FVector2D(0.015f, 0.125f), FVector2D(0.735f, 0.185f));
}

void UDigitalTwinControlWidget::BuildHeader(UCanvasPanel* Root)
{
	UBorder* Header = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Header"));
	Header->SetBrushColor(Panel);
	Header->SetPadding(FMargin(24.0f, 11.0f));
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderRow"));
	Header->SetContent(Row);

	UTextBlock* Title = CreateText(TEXT("Title"), TEXT("HKUST  ·  SMART BUILDING DIGITAL TWIN"), 19, TextPrimary);
	Title->SetClipping(EWidgetClipping::ClipToBounds);
	UHorizontalBoxSlot* TitleSlot = Row->AddChildToHorizontalBox(Title);
	TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TitleSlot->SetVerticalAlignment(VAlign_Center);

	OrbitButton = CreateButton(TEXT("OrbitButton"), TEXT("环绕视图"), 14);
	OverviewButton = CreateButton(TEXT("OverviewButton"), TEXT("建筑总览"), 14);
	OrbitButton->OnClicked.AddDynamic(this, &UDigitalTwinControlWidget::OnOrbitClicked);
	OverviewButton->OnClicked.AddDynamic(this, &UDigitalTwinControlWidget::OnOverviewClicked);
	for (UButton* Button : {OrbitButton.Get(), OverviewButton.Get()})
	{
		UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(Button);
		ButtonSlot->SetPadding(FMargin(8.0f, 0.0f));
		ButtonSlot->SetVerticalAlignment(VAlign_Center);
	}

	RoomSelector = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("RoomSelector"));
	RoomSelector->OnGenerateWidgetEvent.BindDynamic(this, &UDigitalTwinControlWidget::GenerateRoomSelectorEntry);
	RoomSelector->SetContentPadding(FMargin(12.0f, 5.0f, 8.0f, 5.0f));
	RoomSelector->SetMaxListHeight(520.0f);
	RoomSelector->AddOption(RoomSelectorPlaceholder);
	for (int32 RoomIndex = 1; RoomIndex <= 26; ++RoomIndex)
	{
		RoomSelector->AddOption(FString::Printf(TEXT("Room%03d"), RoomIndex));
	}
	RoomSelector->SetSelectedOption(RoomSelectorPlaceholder);
	RoomSelector->OnSelectionChanged.AddDynamic(this, &UDigitalTwinControlWidget::OnRoomSelected);
	USizeBox* RoomSelectorSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RoomSelectorSize"));
	RoomSelectorSize->SetWidthOverride(164.0f);
	RoomSelectorSize->SetMinDesiredHeight(34.0f);
	RoomSelectorSize->SetContent(RoomSelector);
	UHorizontalBoxSlot* RoomSelectorSlot = Row->AddChildToHorizontalBox(RoomSelectorSize);
	RoomSelectorSlot->SetVerticalAlignment(VAlign_Center);

	StatusText = CreateText(TEXT("StatusText"), TEXT("●  数据初始化中"), 12, DataOnline);
	UHorizontalBoxSlot* StatusSlot = Row->AddChildToHorizontalBox(StatusText);
	StatusSlot->SetPadding(FMargin(18.0f, 0.0f, 0.0f, 0.0f));
	StatusSlot->SetVerticalAlignment(VAlign_Center);

	ExitButton = CreateButton(TEXT("ExitButton"), TEXT("退出程序"), 12);
	ExitButton->SetBackgroundColor(FLinearColor(0.42f, 0.07f, 0.06f, 1.0f));
	ExitButton->OnClicked.AddDynamic(this, &UDigitalTwinControlWidget::OnExitClicked);
	UHorizontalBoxSlot* ExitSlot = Row->AddChildToHorizontalBox(ExitButton);
	ExitSlot->SetPadding(FMargin(14.0f, 0.0f, 0.0f, 0.0f));
	ExitSlot->SetVerticalAlignment(VAlign_Center);
	Place(Root, Header, FVector2D(0.015f, 0.02f), FVector2D(0.985f, 0.115f));
}

void UDigitalTwinControlWidget::BuildDetails(UCanvasPanel* Root)
{
	UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DetailsPanel"));
	Border->SetBrushColor(PanelSoft);
	Border->SetPadding(FMargin(18.0f, 16.0f));
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DetailsColumn"));
	Border->SetContent(Column);
	DetailsTitleText = CreateText(TEXT("DetailsTitle"), TEXT("实时运行数据"), 20, TextPrimary);
	Column->AddChildToVerticalBox(DetailsTitleText);
	RoomSummaryText = CreateText(TEXT("RoomSummary"), TEXT("点击建筑中的彩色房间查看空间状态"), 12, TextSecondary);
	RoomSummaryText->SetAutoWrapText(true);
	Column->AddChildToVerticalBox(RoomSummaryText)->SetPadding(FMargin(0.0f, 5.0f, 0.0f, 7.0f));
	OrbitAlertScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("OrbitAlertScroll"));
	OrbitAlertScroll->SetVisibility(ESlateVisibility::Collapsed);
	OrbitAlertScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
	OrbitAlertScroll->SetConsumeMouseWheel(EConsumeMouseWheel::Always);
	OrbitAlertList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OrbitAlertList"));
	OrbitAlertScroll->AddChild(OrbitAlertList);
	UVerticalBoxSlot* AlertScrollSlot = Column->AddChildToVerticalBox(OrbitAlertScroll);
	AlertScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	AlertScrollSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));

	MetricButtons.Reset();
	MetricValueTexts.Reset();
	MetricContainers.Reset();
	AddMetricButton(Column, EDTMetric::Temperature, TEXT("温度"));
	AddMetricButton(Column, EDTMetric::Humidity, TEXT("湿度"));
	AddMetricButton(Column, EDTMetric::CO2, TEXT("CO₂"));
	AddMetricButton(Column, EDTMetric::Occupancy, TEXT("人数"));
	AddMetricButton(Column, EDTMetric::Energy, TEXT("能耗"));
	AddMetricButton(Column, EDTMetric::HVAC, TEXT("HVAC"));

	MetricDetailText = CreateText(TEXT("MetricDetail"), TEXT("当前图层：温度"), 12, Accent);
	MetricDetailText->SetAutoWrapText(true);
	Column->AddChildToVerticalBox(MetricDetailText)->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 10.0f));
	LegendTitleText = CreateText(TEXT("LegendTitle"), TEXT("温度空间图层"), 13, TextPrimary);
	Column->AddChildToVerticalBox(LegendTitleText)->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 3.0f));
	LegendSwatches.Reset();
	LegendLabels.Reset();
	for (int32 Index = 0; Index < 5; ++Index)
	{
		AddLegendRow(Column, FLinearColor::Gray, TEXT("--"));
	}
	EquipmentTitleText = CreateText(TEXT("EquipmentTitle"), TEXT("设备与运行状态"), 13, TextPrimary);
	Column->AddChildToVerticalBox(EquipmentTitleText)->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 5.0f));
	EquipmentText = CreateText(TEXT("EquipmentText"), TEXT("等待设备数据"), 12, TextSecondary);
	EquipmentText->SetAutoWrapText(true);
	Column->AddChildToVerticalBox(EquipmentText);
	Place(Root, Border, FVector2D(0.755f, 0.135f), FVector2D(0.985f, 0.975f));
}

void UDigitalTwinControlWidget::AddMetricButton(UVerticalBox* Column, const EDTMetric Metric, const FString& Label)
{
	UDTMetricButton* Button = WidgetTree->ConstructWidget<UDTMetricButton>(UDTMetricButton::StaticClass());
	Button->Setup(Metric);
	Button->SetBackgroundColor(ButtonNormal);
	Button->OnMetricClicked.AddDynamic(this, &UDigitalTwinControlWidget::OnMetricClicked);
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* LabelText = CreateText(NAME_None, Label, 13, TextPrimary);
	UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText);
	LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	LabelSlot->SetPadding(FMargin(12.0f, 0.0f, 8.0f, 0.0f));
	UTextBlock* ValueText = CreateText(NAME_None, TEXT("--"), 13, TextPrimary);
	Row->AddChildToHorizontalBox(ValueText)->SetPadding(FMargin(8.0f, 0.0f, 12.0f, 0.0f));
	Button->SetContent(Row);
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Size->SetMinDesiredHeight(36.0f);
	Size->SetContent(Button);
	Column->AddChildToVerticalBox(Size)->SetPadding(FMargin(0.0f, 1.5f));
	MetricContainers.Add(Size);
	MetricButtons.Add(Metric, Button);
	MetricValueTexts.Add(Metric, ValueText);
}

void UDigitalTwinControlWidget::AddLegendRow(UVerticalBox* Column, const FLinearColor& Color, const FString& Label)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	LegendRows.Add(Row);
	UBorder* Swatch = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Swatch->SetBrushColor(Color);
	LegendSwatches.Add(Swatch);
	USizeBox* SwatchSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	SwatchSize->SetWidthOverride(28.0f);
	SwatchSize->SetHeightOverride(14.0f);
	SwatchSize->SetContent(Swatch);
	Row->AddChildToHorizontalBox(SwatchSize)->SetPadding(FMargin(0.0f, 2.0f, 8.0f, 2.0f));
	UTextBlock* LabelText = CreateText(NAME_None, Label, 11, TextSecondary);
	Row->AddChildToHorizontalBox(LabelText);
	LegendLabels.Add(LabelText);
	Column->AddChildToVerticalBox(Row);
}

void UDigitalTwinControlWidget::BuildTimeline(UCanvasPanel* Root)
{
	UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TimelinePanel"));
	Border->SetBrushColor(Panel);
	Border->SetPadding(FMargin(20.0f, 12.0f));
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TimelineColumn"));
	Border->SetContent(Column);

	UHorizontalBox* DateRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DateRow"));
	PreviousMonthButton = CreateButton(TEXT("PreviousMonth"), TEXT("‹ 月"), 13);
	PreviousDayButton = CreateButton(TEXT("PreviousDay"), TEXT("‹ 日"), 13);
	NextDayButton = CreateButton(TEXT("NextDay"), TEXT("日 ›"), 13);
	NextMonthButton = CreateButton(TEXT("NextMonth"), TEXT("月 ›"), 13);
	PreviousMonthButton->OnClicked.AddDynamic(this, &UDigitalTwinControlWidget::OnPreviousMonthClicked);
	PreviousDayButton->OnClicked.AddDynamic(this, &UDigitalTwinControlWidget::OnPreviousDayClicked);
	NextDayButton->OnClicked.AddDynamic(this, &UDigitalTwinControlWidget::OnNextDayClicked);
	NextMonthButton->OnClicked.AddDynamic(this, &UDigitalTwinControlWidget::OnNextMonthClicked);
	DateRow->AddChildToHorizontalBox(PreviousMonthButton);
	DateRow->AddChildToHorizontalBox(PreviousDayButton);
	DateText = CreateText(TEXT("DateValue"), TEXT("2027-01-01"), 20, Accent);
	UHorizontalBoxSlot* DateSlot = DateRow->AddChildToHorizontalBox(DateText);
	DateSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	DateSlot->SetHorizontalAlignment(HAlign_Center);
	DateSlot->SetVerticalAlignment(VAlign_Center);
	DateRow->AddChildToHorizontalBox(NextDayButton);
	DateRow->AddChildToHorizontalBox(NextMonthButton);
	Column->AddChildToVerticalBox(DateRow);

	UHorizontalBox* TimeRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TimeRow"));
	HourText = CreateText(TEXT("HourValue"), TEXT("00:00"), 22, Accent);
	UHorizontalBoxSlot* HourSlot = TimeRow->AddChildToHorizontalBox(HourText);
	HourSlot->SetPadding(FMargin(0.0f, 8.0f, 18.0f, 0.0f));
	HourSlot->SetVerticalAlignment(VAlign_Center);
	HourSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("HourSlider"));
	HourSlider->SetMinValue(0.0f);
	HourSlider->SetMaxValue(23.0f);
	HourSlider->SetStepSize(1.0f);
	HourSlider->SetSliderBarColor(FLinearColor(0.09f, 0.24f, 0.31f, 1.0f));
	HourSlider->SetSliderHandleColor(Accent);
	HourSlider->OnValueChanged.AddDynamic(this, &UDigitalTwinControlWidget::OnHourChanged);
	UHorizontalBoxSlot* SliderSlot = TimeRow->AddChildToHorizontalBox(HourSlider);
	SliderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	SliderSlot->SetVerticalAlignment(VAlign_Center);
	RealtimePreviewButton = CreateButton(TEXT("RealtimePreview"), TEXT("实时预览"), 12);
	FreezeButton = CreateButton(TEXT("FreezeTime"), TEXT("定格"), 12);
	RealtimePreviewButton->OnClicked.AddDynamic(this, &UDigitalTwinControlWidget::OnRealtimePreviewClicked);
	FreezeButton->OnClicked.AddDynamic(this, &UDigitalTwinControlWidget::OnFreezeClicked);
	for (UButton* PlaybackButton : {RealtimePreviewButton.Get(), FreezeButton.Get()})
	{
		UHorizontalBoxSlot* PlaybackSlot = TimeRow->AddChildToHorizontalBox(PlaybackButton);
		PlaybackSlot->SetPadding(FMargin(10.0f, 5.0f, 0.0f, 0.0f));
		PlaybackSlot->SetVerticalAlignment(VAlign_Center);
	}
	Column->AddChildToVerticalBox(TimeRow);
	UHorizontalBox* TickRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HourTicks"));
	for (const FString& Tick : {FString(TEXT("00:00")), FString(TEXT("06:00")), FString(TEXT("12:00")), FString(TEXT("18:00")), FString(TEXT("23:00"))})
	{
		UTextBlock* TickText = CreateText(NAME_None, Tick, 10, TextSecondary);
		UHorizontalBoxSlot* TickSlot = TickRow->AddChildToHorizontalBox(TickText);
		TickSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TickSlot->SetHorizontalAlignment(Tick == TEXT("23:00") ? HAlign_Right : (Tick == TEXT("00:00") ? HAlign_Left : HAlign_Center));
	}
	Column->AddChildToVerticalBox(TickRow)->SetPadding(FMargin(76.0f, -2.0f, 0.0f, 0.0f));
	Column->AddChildToVerticalBox(CreateText(TEXT("InputHelp"), TEXT("左键拖动旋转  ·  滚轮缩放  ·  单击彩色空间进入房间视角"), 11, TextSecondary))->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
	Place(Root, Border, FVector2D(0.015f, 0.80f), FVector2D(0.735f, 0.975f));
}

UButton* UDigitalTwinControlWidget::CreateButton(const FName Name, const FString& Label, const int32 FontSize)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	Button->SetBackgroundColor(ButtonNormal);
	Button->SetContent(CreateText(*FString::Printf(TEXT("%s_Text"), *Name.ToString()), Label, FontSize, TextPrimary));
	return Button;
}

UTextBlock* UDigitalTwinControlWidget::CreateText(const FName Name, const FString& Text, const int32 FontSize, const FLinearColor Color)
{
	UTextBlock* Result = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	Result->SetText(FText::FromString(Text));
	Result->SetColorAndOpacity(FSlateColor(Color));
	FSlateFontInfo Font = Result->GetFont();
	Font.Size = FontSize;
	Result->SetFont(Font);
	return Result;
}

void UDigitalTwinControlWidget::OnOrbitClicked()
{
	UE_LOG(LogDigitalTwinUI, Log, TEXT("UI click: Orbit"));
	if (Manager) Manager->SetViewMode(EDTViewMode::Orbit);
}

void UDigitalTwinControlWidget::OnOverviewClicked()
{
	UE_LOG(LogDigitalTwinUI, Log, TEXT("UI click: Overview"));
	if (Manager) Manager->SetViewMode(EDTViewMode::Overview);
}

void UDigitalTwinControlWidget::OnPreviousMonthClicked() { if (Manager) Manager->ChangeMonth(-1); }
void UDigitalTwinControlWidget::OnPreviousDayClicked() { if (Manager) Manager->ChangeDay(-1); }
void UDigitalTwinControlWidget::OnNextDayClicked() { if (Manager) Manager->ChangeDay(1); }
void UDigitalTwinControlWidget::OnNextMonthClicked() { if (Manager) Manager->ChangeMonth(1); }

void UDigitalTwinControlWidget::OnHourChanged(const float Value)
{
	if (!Manager || bSynchronizingSlider) return;
	const int32 Hour = FMath::Clamp(FMath::RoundToInt(Value), 0, 23);
	const FDateTime Current = Manager->GetSimulationDateTime();
	if (HourText) HourText->SetText(FText::FromString(FString::Printf(TEXT("%02d:00"), Hour)));
	UE_LOG(LogDigitalTwinUI, Log, TEXT("UI slider: %02d:00"), Hour);
	Manager->SetSimulationDateTime(FDateTime(Current.GetYear(), Current.GetMonth(), Current.GetDay(), Hour));
}

void UDigitalTwinControlWidget::OnRealtimePreviewClicked()
{
	UE_LOG(LogDigitalTwinUI, Log, TEXT("UI click: realtime preview"));
	if (Manager) Manager->SetTimePlaybackEnabled(true);
}

void UDigitalTwinControlWidget::OnFreezeClicked()
{
	UE_LOG(LogDigitalTwinUI, Log, TEXT("UI click: freeze time"));
	if (Manager) Manager->SetTimePlaybackEnabled(false);
}

void UDigitalTwinControlWidget::OnMetricClicked(const EDTMetric Metric)
{
	UE_LOG(LogDigitalTwinUI, Log, TEXT("UI metric click: %d"), static_cast<int32>(Metric));
	if (Manager) Manager->SetSelectedMetric(Metric);
}

void UDigitalTwinControlWidget::OnNormalPresetClicked() { if (Manager) Manager->ApplyPresentationPreset(EDTPresentationPreset::Normal); }
void UDigitalTwinControlWidget::OnPeakPresetClicked() { if (Manager) Manager->ApplyPresentationPreset(EDTPresentationPreset::Peak); }
void UDigitalTwinControlWidget::OnIncidentPresetClicked() { if (Manager) Manager->ApplyPresentationPreset(EDTPresentationPreset::Incident); }

void UDigitalTwinControlWidget::OnExitClicked()
{
	UE_LOG(LogDigitalTwinUI, Log, TEXT("UI click: exit application"));
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UDigitalTwinControlWidget::OnRoomSelected(const FString SelectedItem, const ESelectInfo::Type SelectionType)
{
	if (!Manager || bSynchronizingRoomSelector || SelectedItem.IsEmpty()) return;
	if (SelectedItem == RoomSelectorPlaceholder)
	{
		bSynchronizingRoomSelector = true;
		RoomSelector->SetSelectedOption(Manager->GetViewMode() == EDTViewMode::Room
			? Manager->GetSelectedRoomId().ToString()
			: RoomSelectorPlaceholder);
		bSynchronizingRoomSelector = false;
		return;
	}
	const FName RoomId(*SelectedItem);
	UE_LOG(LogDigitalTwinUI, Log, TEXT("UI room selection: %s"), *SelectedItem);
	if (!Manager->SelectRoom(RoomId) && RoomSelector)
	{
		bSynchronizingRoomSelector = true;
		RoomSelector->SetSelectedOption(Manager->GetViewMode() == EDTViewMode::Room
			? Manager->GetSelectedRoomId().ToString()
			: RoomSelectorPlaceholder);
		bSynchronizingRoomSelector = false;
	}
}

UWidget* UDigitalTwinControlWidget::GenerateRoomSelectorEntry(const FString Item)
{
	UBorder* EntryContainer = WidgetTree
		? WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass())
		: NewObject<UBorder>(this);
	EntryContainer->SetBrushColor(FLinearColor::Transparent);
	EntryContainer->SetPadding(FMargin(5.0f, 4.0f));
	UTextBlock* Entry = WidgetTree
		? WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass())
		: NewObject<UTextBlock>(this);
	Entry->SetText(FText::FromString(Item));
	Entry->SetColorAndOpacity(FSlateColor(TextPrimary));
	FSlateFontInfo EntryFont = Entry->GetFont();
	EntryFont.Size = 14;
	Entry->SetFont(EntryFont);
	EntryContainer->SetContent(Entry);
	return EntryContainer;
}

void UDigitalTwinControlWidget::RefreshState()
{
	if (!Manager) return;
	const FDateTime Current = Manager->GetSimulationDateTime();
	bSynchronizingSlider = true;
	if (HourSlider) HourSlider->SetValue(static_cast<float>(Current.GetHour()));
	bSynchronizingSlider = false;
	if (DateText) DateText->SetText(FText::FromString(Current.ToString(TEXT("%Y-%m-%d"))));
	if (HourText) HourText->SetText(FText::FromString(Current.ToString(TEXT("%H:00"))));
	const FString RoomSelectorText = Manager->GetViewMode() == EDTViewMode::Room
		? Manager->GetSelectedRoomId().ToString()
		: RoomSelectorPlaceholder;
	if (RoomSelector && RoomSelector->GetSelectedOption() != RoomSelectorText)
	{
		bSynchronizingRoomSelector = true;
		RoomSelector->SetSelectedOption(RoomSelectorText);
		bSynchronizingRoomSelector = false;
	}
	if (StatusText)
	{
		const FString Mode = Manager->GetViewMode() == EDTViewMode::Orbit ? TEXT("环绕") : (Manager->GetViewMode() == EDTViewMode::Room ? TEXT("房间") : TEXT("总览"));
		const FString Lighting = (Current.GetHour() >= 17 || Current.GetHour() <= 7) ? TEXT("夜间照明") : TEXT("日间模式");
		const FString Playback = Manager->IsTimePlaybackActive()
			? TEXT(" · 实时预览 2秒/小时")
			: (Manager->IsTimePlaybackEnabled() ? TEXT(" · 已到数据末端") : TEXT(" · 时间已定格"));
		const FString RoomStatus = Manager->GetViewMode() == EDTViewMode::Room
			? FString::Printf(TEXT(" · %s"), *Manager->GetSelectedRoomId().ToString())
			: FString();
		const FString Preset = ADigitalTwinManager::GetPresentationPresetDisplayName(Manager->GetPresentationPreset());
		StatusText->SetText(FText::FromString(FString::Printf(TEXT("●  数据在线   %s%s · %s · %s%s"), *Mode, *RoomStatus, *Preset, *Lighting, *Playback)));
	}
	if (PresetSummaryText)
	{
		switch (Manager->GetPresentationPreset())
		{
		case EDTPresentationPreset::Normal: PresetSummaryText->SetText(FText::FromString(TEXT("周末 09:00  ·  低负荷运行  ·  环境舒适"))); break;
		case EDTPresentationPreset::Peak: PresetSummaryText->SetText(FText::FromString(TEXT("工作日 14:00  ·  占用率 99.1%  ·  人流图层"))); break;
		case EDTPresentationPreset::Incident: PresetSummaryText->SetText(FText::FromString(TEXT("工作日 14:00  ·  23 个房间告警  ·  CO₂ 图层"))); break;
		default: PresetSummaryText->SetText(FText::FromString(TEXT("自定义时间与指标"))); break;
		}
	}
	const FDateTime Min = UDigitalTwinDataSubsystem::GetMinimumDateTime();
	const FDateTime Max = UDigitalTwinDataSubsystem::GetMaximumDateTime();
	if (PreviousMonthButton) PreviousMonthButton->SetIsEnabled(Current > Min);
	if (PreviousDayButton) PreviousDayButton->SetIsEnabled(Current > Min);
	if (NextDayButton) NextDayButton->SetIsEnabled(Current < Max);
	if (NextMonthButton) NextMonthButton->SetIsEnabled(Current < Max);
	RefreshButtonStyles();
	RefreshLegend();
	RefreshDetails();
}

void UDigitalTwinControlWidget::SetDetailsPresentation(const bool bOrbitAlerts)
{
	if (DetailsTitleText)
	{
		DetailsTitleText->SetText(FText::FromString(bOrbitAlerts ? TEXT("建筑状态") : TEXT("实时运行数据")));
	}
	const ESlateVisibility DataVisibility = bOrbitAlerts ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
	if (OrbitAlertScroll) OrbitAlertScroll->SetVisibility(bOrbitAlerts ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (MetricDetailText) MetricDetailText->SetVisibility(DataVisibility);
	for (USizeBox* Container : MetricContainers) if (Container) Container->SetVisibility(DataVisibility);
	if (LegendTitleText) LegendTitleText->SetVisibility(DataVisibility);
	for (UHorizontalBox* Row : LegendRows) if (Row) Row->SetVisibility(DataVisibility);
	if (EquipmentTitleText) EquipmentTitleText->SetVisibility(DataVisibility);
	if (EquipmentText) EquipmentText->SetVisibility(DataVisibility);
}

void UDigitalTwinControlWidget::RefreshOrbitAlerts()
{
	if (!Manager || !RoomSummaryText || !OrbitAlertList) return;
	OrbitAlertList->ClearChildren();
	UGameInstance* GameInstance = GetGameInstance();
	UDigitalTwinDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UDigitalTwinDataSubsystem>() : nullptr;
	if (!Data)
	{
		RoomSummaryText->SetText(FText::FromString(TEXT("建筑报警数据不可用")));
		OrbitAlertList->AddChildToVerticalBox(CreateText(NAME_None, TEXT("无法访问数据子系统"), 13, FLinearColor(1.0f, 0.25f, 0.2f, 1.0f)));
		return;
	}

	struct FAlertIssue
	{
		FString Text;
		FLinearColor Color;
		int32 Severity = 1;
	};
	struct FRoomAlertCard
	{
		FName RoomId;
		FString FunctionName;
		TArray<FAlertIssue> Issues;
		int32 MaximumSeverity = 1;
	};
	TArray<FRoomAlertCard> AlertCards;
	const FDateTime Current = Manager->GetSimulationDateTime();
	const int32 Hour = Current.GetHour();
	const bool bWeekday = Current.GetDayOfWeek() != EDayOfWeek::Saturday && Current.GetDayOfWeek() != EDayOfWeek::Sunday;
	for (const FName RoomId : Data->GetKnownRoomIds())
	{
		FRoomMetadata Metadata;
		FRoomSensorData Sensor;
		if (!Data->GetRoomMetadata(RoomId, Metadata) || !Data->GetRoomSensorData(RoomId, Current, Sensor)) continue;

		const bool bClassPeak = Metadata.Function == TEXT("Classroom") && bWeekday && TArray<int32>{9, 10, 12, 13, 15, 16, 18, 19}.Contains(Hour);
		const bool bCorridorPeak = Metadata.Function == TEXT("PublicCorridor") && bWeekday && TArray<int32>{8, 11, 14, 17, 20}.Contains(Hour);
		const bool bOfficePeak = Metadata.Function == TEXT("Office") && bWeekday && Hour >= 9 && Hour <= 17;
		const bool bStoragePeak = Metadata.Function == TEXT("Storage") && bWeekday && Hour >= 8 && Hour <= 19;
		const bool bPeak = bClassPeak || bCorridorPeak || bOfficePeak || bStoragePeak;
		const float RecommendedTemperature = bPeak ? 25.0f : 24.0f;
		const int32 RecommendedCO2 = bPeak ? 1200 : 900;
		int32 RecommendedOccupancy = FMath::Max(1, FMath::RoundToInt(Metadata.Capacity * (bPeak ? 0.95f : 0.25f)));
		if (Metadata.Function == TEXT("PublicCorridor") && !bPeak) RecommendedOccupancy = FMath::Min(20, Metadata.Capacity);
		const float RecommendedHumidity = bPeak ? 75.0f : 72.0f;
		const float RecommendedHVAC = bPeak ? 95.0f : 85.0f;
		const float PeakEnergyLimit = Metadata.Function == TEXT("Classroom") ? 3.5f
			: (Metadata.Function == TEXT("Office") ? 2.4f : (Metadata.Function == TEXT("PublicCorridor") ? 2.8f : 1.3f));
		const float RecommendedEnergy = bPeak ? PeakEnergyLimit : PeakEnergyLimit * 0.65f;

		FRoomAlertCard Card;
		Card.RoomId = RoomId;
		Card.FunctionName = Metadata.FunctionCN;
		auto AddIssue = [&Card](const FString& Text, const FLinearColor& Color, const int32 Severity)
		{
			Card.Issues.Add({Text, Color, Severity});
			Card.MaximumSeverity = FMath::Max(Card.MaximumSeverity, Severity);
		};
		if (Sensor.TemperatureC >= 27.0f)
			AddIssue(FString::Printf(TEXT("温度过高  %.1f°C  ·  建议标准 ≤ %.1f°C"), Sensor.TemperatureC, RecommendedTemperature), FLinearColor(1.0f, 0.22f, 0.12f, 1.0f), 3);
		if (Sensor.TemperatureC < 19.0f)
			AddIssue(FString::Printf(TEXT("温度过低  %.1f°C  ·  建议标准 ≥ 20.0°C"), Sensor.TemperatureC), FLinearColor(0.15f, 0.65f, 1.0f, 1.0f), 2);
		if (Sensor.Occupancy > RecommendedOccupancy)
			AddIssue(FString::Printf(TEXT("人流量过大  %d 人  ·  当前时段标准 ≤ %d 人"), Sensor.Occupancy, RecommendedOccupancy), FLinearColor(1.0f, 0.55f, 0.08f, 1.0f), 3);
		if (Sensor.CO2ppm >= RecommendedCO2)
			AddIssue(FString::Printf(TEXT("CO₂ 过高  %d ppm  ·  当前时段标准 < %d ppm"), Sensor.CO2ppm, RecommendedCO2), FLinearColor(1.0f, 0.82f, 0.16f, 1.0f), 3);
		if (Sensor.HumidityPct >= RecommendedHumidity || Sensor.HumidityPct < 32.0f)
			AddIssue(FString::Printf(TEXT("湿度异常  %.1f%%  ·  建议范围 40–%.0f%%"), Sensor.HumidityPct, RecommendedHumidity), FLinearColor(0.10f, 0.78f, 0.92f, 1.0f), 2);
		if (Sensor.HVACLoadPct >= RecommendedHVAC || Sensor.HVACMode.Equals(TEXT("Fault"), ESearchCase::IgnoreCase))
			AddIssue(FString::Printf(TEXT("HVAC 异常  %.1f%% / %s  ·  当前时段标准 < %.0f%%"), Sensor.HVACLoadPct, *Sensor.HVACMode, RecommendedHVAC), FLinearColor(0.78f, 0.42f, 1.0f, 1.0f), 2);
		if (Sensor.EnergyKWh > RecommendedEnergy)
			AddIssue(FString::Printf(TEXT("能耗过高  %.3f kWh  ·  当前时段标准 ≤ %.2f kWh"), Sensor.EnergyKWh, RecommendedEnergy), FLinearColor(1.0f, 0.36f, 0.62f, 1.0f), 2);
		if (!Card.Issues.IsEmpty()) AlertCards.Add(MoveTemp(Card));
	}

	AlertCards.Sort([](const FRoomAlertCard& A, const FRoomAlertCard& B)
	{
		return A.MaximumSeverity == B.MaximumSeverity ? FNameLexicalLess()(A.RoomId, B.RoomId) : A.MaximumSeverity > B.MaximumSeverity;
	});
	if (AlertCards.IsEmpty())
	{
		RoomSummaryText->SetText(FText::FromString(FString::Printf(TEXT("当前无报警  ·  %s"), *Current.ToString(TEXT("%Y-%m-%d  %H:00")))));
		UTextBlock* NormalText = CreateText(NAME_None, TEXT("● 26 个房间运行正常"), 15, FLinearColor(0.24f, 0.92f, 0.58f, 1.0f));
		OrbitAlertList->AddChildToVerticalBox(NormalText)->SetPadding(FMargin(8.0f, 16.0f));
	}
	else
	{
		const int32 TotalAlertRooms = AlertCards.Num();
		int32 DisplayLimit = 12;
		switch (Manager->GetPresentationPreset())
		{
		case EDTPresentationPreset::Normal: DisplayLimit = 3; break;
		case EDTPresentationPreset::Peak: DisplayLimit = 8; break;
		case EDTPresentationPreset::Incident: DisplayLimit = 12; break;
		default: break;
		}
		const int32 DisplayCount = FMath::Min(TotalAlertRooms, DisplayLimit);
		RoomSummaryText->SetText(FText::FromString(FString::Printf(
			TEXT("当前 %d 个房间报警  ·  展示最高优先级 %d 项\n%s  ·  标准已按时段动态调整"),
			TotalAlertRooms, DisplayCount, *Current.ToString(TEXT("%Y-%m-%d  %H:00")))));
		for (int32 CardIndex = 0; CardIndex < DisplayCount; ++CardIndex)
		{
			const FRoomAlertCard& Card = AlertCards[CardIndex];
			const FLinearColor CardColor = Card.MaximumSeverity >= 3
				? FLinearColor(0.24f, 0.045f, 0.035f, 0.96f)
				: FLinearColor(0.17f, 0.08f, 0.20f, 0.96f);
			UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			CardBorder->SetBrushColor(CardColor);
			CardBorder->SetPadding(FMargin(11.0f, 8.0f));
			UVerticalBox* CardColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			CardBorder->SetContent(CardColumn);
			const FLinearColor RoomColor = Card.MaximumSeverity >= 3
				? FLinearColor(1.0f, 0.32f, 0.20f, 1.0f)
				: FLinearColor(0.88f, 0.56f, 1.0f, 1.0f);
			CardColumn->AddChildToVerticalBox(CreateText(NAME_None,
				FString::Printf(TEXT("%s  ·  %s"), *Card.RoomId.ToString(), *Card.FunctionName), 15, RoomColor));
			for (const FAlertIssue& Issue : Card.Issues)
			{
				UTextBlock* IssueText = CreateText(NAME_None, FString::Printf(TEXT("—  %s"), *Issue.Text), 11, Issue.Color);
				IssueText->SetAutoWrapText(true);
				CardColumn->AddChildToVerticalBox(IssueText)->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 0.0f));
			}
			OrbitAlertList->AddChildToVerticalBox(CardBorder)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 7.0f));
		}
	}
}

void UDigitalTwinControlWidget::RefreshButtonStyles()
{
	if (!Manager) return;
	OrbitButton->SetBackgroundColor(Manager->GetViewMode() == EDTViewMode::Orbit ? Accent : ButtonNormal);
	OverviewButton->SetBackgroundColor(Manager->GetViewMode() == EDTViewMode::Overview ? Accent : ButtonNormal);
	if (RealtimePreviewButton) RealtimePreviewButton->SetBackgroundColor(Manager->IsTimePlaybackEnabled() ? Accent : ButtonNormal);
	if (FreezeButton) FreezeButton->SetBackgroundColor(Manager->IsTimePlaybackEnabled() ? ButtonNormal : Accent);
	if (NormalPresetButton) NormalPresetButton->SetBackgroundColor(Manager->GetPresentationPreset() == EDTPresentationPreset::Normal ? Accent : ButtonNormal);
	if (PeakPresetButton) PeakPresetButton->SetBackgroundColor(Manager->GetPresentationPreset() == EDTPresentationPreset::Peak ? Accent : ButtonNormal);
	if (IncidentPresetButton) IncidentPresetButton->SetBackgroundColor(Manager->GetPresentationPreset() == EDTPresentationPreset::Incident ? Accent : ButtonNormal);
	for (const TPair<EDTMetric, TObjectPtr<UDTMetricButton>>& Pair : MetricButtons)
	{
		Pair.Value->SetBackgroundColor(Pair.Key == Manager->GetSelectedMetric() ? Accent : ButtonNormal);
	}
}

void UDigitalTwinControlWidget::RefreshLegend()
{
	if (!Manager) return;
	TArray<FLinearColor> Colors;
	TArray<FString> Labels;
	ADigitalTwinManager::GetMetricLegend(Manager->GetSelectedMetric(), Colors, Labels);
	if (LegendTitleText)
	{
		LegendTitleText->SetText(FText::FromString(ADigitalTwinManager::GetMetricDisplayName(Manager->GetSelectedMetric()) + TEXT("空间图层")));
	}
	for (int32 Index = 0; Index < LegendSwatches.Num() && Index < LegendLabels.Num(); ++Index)
	{
		if (Colors.IsValidIndex(Index)) LegendSwatches[Index]->SetBrushColor(Colors[Index]);
		if (Labels.IsValidIndex(Index)) LegendLabels[Index]->SetText(FText::FromString(Labels[Index]));
	}
}

void UDigitalTwinControlWidget::ResetMetricValues(const FString& Placeholder)
{
	for (const TPair<EDTMetric, TObjectPtr<UTextBlock>>& Pair : MetricValueTexts)
	{
		if (Pair.Value) Pair.Value->SetText(FText::FromString(Placeholder));
	}
}

void UDigitalTwinControlWidget::RefreshDetails()
{
	if (!Manager || !RoomSummaryText || !MetricDetailText) return;
	const bool bOrbitAlerts = Manager->GetViewMode() == EDTViewMode::Orbit;
	SetDetailsPresentation(bOrbitAlerts);
	if (bOrbitAlerts)
	{
		RefreshOrbitAlerts();
		return;
	}
	auto SetMetricValue = [this](const EDTMetric Metric, const FString& Value)
	{
		if (const TObjectPtr<UTextBlock>* Text = MetricValueTexts.Find(Metric))
		{
			(*Text)->SetText(FText::FromString(Value));
		}
	};
	FString SelectedDescription;
	if (Manager->GetViewMode() == EDTViewMode::Overview)
	{
		FBuildingOverviewData Data;
		if (!Manager->GetOverviewData(Data))
		{
			RoomSummaryText->SetText(FText::FromString(TEXT("总览数据不可用")));
			MetricDetailText->SetText(FText::FromString(TEXT("请检查 DT_BMS_Overview.csv")));
			if (EquipmentText) EquipmentText->SetText(FText::FromString(TEXT("设备汇总不可用")));
			ResetMetricValues();
			return;
		}
		RoomSummaryText->SetText(FText::FromString(FString::Printf(TEXT("建筑总览  ·  占用率 %.1f%%  ·  告警 %d  ·  新风 %.1f%%"), Data.OccupancyRatePct, Data.AlertRooms, Data.AverageFreshAirPct)));
		SetMetricValue(EDTMetric::Temperature, FString::Printf(TEXT("%.2f °C"), Data.AverageTemperatureC));
		SetMetricValue(EDTMetric::Humidity, FString::Printf(TEXT("%.1f%%"), Data.AverageHumidityPct));
		SetMetricValue(EDTMetric::CO2, FString::Printf(TEXT("%d ppm"), Data.AverageCO2ppm));
		SetMetricValue(EDTMetric::Occupancy, FString::Printf(TEXT("%d 人"), Data.TotalOccupancy));
		SetMetricValue(EDTMetric::Energy, FString::Printf(TEXT("%.3f kWh"), Data.TotalEnergyKWh));
		SetMetricValue(EDTMetric::HVAC, FString::Printf(TEXT("%.1f%% / %d 房间"), Data.AverageHVACLoadPct, Data.HVACActiveRooms));
		switch (Manager->GetSelectedMetric())
		{
		case EDTMetric::Temperature: SelectedDescription = FString::Printf(TEXT("已高亮：平均温度 %.2f °C"), Data.AverageTemperatureC); break;
		case EDTMetric::Humidity: SelectedDescription = FString::Printf(TEXT("已高亮：平均湿度 %.1f%%"), Data.AverageHumidityPct); break;
		case EDTMetric::CO2: SelectedDescription = FString::Printf(TEXT("已高亮：平均 CO₂ %d ppm"), Data.AverageCO2ppm); break;
		case EDTMetric::Occupancy: SelectedDescription = FString::Printf(TEXT("已高亮：总人数 %d"), Data.TotalOccupancy); break;
		case EDTMetric::Energy: SelectedDescription = FString::Printf(TEXT("已高亮：总能耗 %.3f kWh"), Data.TotalEnergyKWh); break;
		default: SelectedDescription = FString::Printf(TEXT("已高亮：HVAC 平均负载 %.1f%%"), Data.AverageHVACLoadPct); break;
		}
		MetricDetailText->SetText(FText::FromString(SelectedDescription));
		if (EquipmentText)
		{
			EquipmentText->SetText(FText::FromString(FString::Printf(TEXT("HVAC 活跃房间   %d / 26\n平均新风比例   %.1f%%\n照明总负荷     %.3f kW\n数据时间       %s"),
				Data.HVACActiveRooms, Data.AverageFreshAirPct, Data.TotalLightingKW, *Data.Timestamp.ToString(TEXT("%Y-%m-%d  %H:00")))));
		}
		return;
	}

	FRoomMetadata Metadata;
	FRoomSensorData Sensor;
	if (!Manager->GetSelectedRoomData(Metadata, Sensor))
	{
		RoomSummaryText->SetText(FText::FromString(TEXT("房间数据不可用")));
		MetricDetailText->SetText(FText::FromString(TEXT("请检查房间元数据、摄像机与小时数据记录")));
		if (EquipmentText) EquipmentText->SetText(FText::FromString(TEXT("设备信息不可用")));
		ResetMetricValues();
		return;
	}
	RoomSummaryText->SetText(FText::FromString(FString::Printf(TEXT("%s  ·  %s  ·  %d 层\n%.1f m²  ·  容量 %d 人  ·  %s"),
		*Metadata.RoomId.ToString(), *Metadata.FunctionCN, Metadata.Floor, Metadata.AreaSqM, Metadata.Capacity, *Metadata.HVACZone)));
	SetMetricValue(EDTMetric::Temperature, FString::Printf(TEXT("%.2f °C"), Sensor.TemperatureC));
	SetMetricValue(EDTMetric::Humidity, FString::Printf(TEXT("%.1f%%"), Sensor.HumidityPct));
	SetMetricValue(EDTMetric::CO2, FString::Printf(TEXT("%d ppm"), Sensor.CO2ppm));
	SetMetricValue(EDTMetric::Occupancy, FString::Printf(TEXT("%d / %d 人"), Sensor.Occupancy, Metadata.Capacity));
	SetMetricValue(EDTMetric::Energy, FString::Printf(TEXT("%.3f kWh"), Sensor.EnergyKWh));
	SetMetricValue(EDTMetric::HVAC, FString::Printf(TEXT("%s · %.1f%%"), *Sensor.HVACMode, Sensor.HVACLoadPct));
	switch (Manager->GetSelectedMetric())
	{
	case EDTMetric::Temperature: SelectedDescription = FString::Printf(TEXT("当前图层：温度 %.2f °C"), Sensor.TemperatureC); break;
	case EDTMetric::Humidity: SelectedDescription = FString::Printf(TEXT("当前图层：湿度 %.1f%%"), Sensor.HumidityPct); break;
	case EDTMetric::CO2: SelectedDescription = FString::Printf(TEXT("当前图层：CO₂ %d ppm"), Sensor.CO2ppm); break;
	case EDTMetric::Occupancy: SelectedDescription = FString::Printf(TEXT("当前图层：人数 %d / %d"), Sensor.Occupancy, Metadata.Capacity); break;
	case EDTMetric::Energy: SelectedDescription = FString::Printf(TEXT("当前图层：能耗 %.3f kWh"), Sensor.EnergyKWh); break;
	default: SelectedDescription = FString::Printf(TEXT("当前图层：HVAC %s · 负载 %.1f%% · 新风 %.1f%%"), *Sensor.HVACMode, Sensor.HVACLoadPct, Sensor.FreshAirPct); break;
	}
	MetricDetailText->SetText(FText::FromString(SelectedDescription));
	if (EquipmentText)
	{
		EquipmentText->SetText(FText::FromString(FString::Printf(TEXT("空调机组       %s\nHVAC 分区      %s\n新风比例       %.1f%%\n照明负荷       %.3f kW\n数据时间       %s"),
			*Metadata.AHUId, *Metadata.HVACZone, Sensor.FreshAirPct, Sensor.LightingKW, *Sensor.Timestamp.ToString(TEXT("%Y-%m-%d  %H:00")))));
	}
}

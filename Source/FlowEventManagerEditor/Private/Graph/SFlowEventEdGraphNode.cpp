#include "Graph/SFlowEventEdGraphNode.h"

#include "EdGraph/EdGraph.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Graph/FlowEventEdGraphNode.h"
#include "SGraphPin.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SFlowEventEdGraphNode"

void SFlowEventEdGraphNode::Construct(const FArguments& InArgs, UFlowEventEdGraphNode* InNode)
{
	GraphNode = InNode;
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

void SFlowEventEdGraphNode::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();
	LeftNodeBox.Reset();
	RightNodeBox.Reset();

	ContentScale.Bind(this, &SGraphNode::GetContentScale);

	GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Graph.Node.Body"))
			.BorderBackgroundColor(this, &SGraphNode::GetNodeTitleColor)
			.Padding(0.0f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SAssignNew(LeftNodeBox, SVerticalBox)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0f, 6.0f)
				[
					SNew(SBox)
					.MinDesiredWidth(190.0f)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(this, &SFlowEventEdGraphNode::GetNodeTitleText)
							.TextStyle(FAppStyle::Get(), "Graph.Node.NodeTitle")
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 5.0f)
						[
							SNew(SSeparator)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 4.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(this, &SFlowEventEdGraphNode::GetDurationLabelText)
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SSpinBox<float>)
								.MinValue(0.0f)
								.MinSliderValue(0.0f)
								.MaxSliderValue(30.0f)
								.MinDesiredWidth(76.0f)
								.Value_Lambda([this]()
								{
									const UFlowEventEdGraphNode* FlowGraphNode = GetFlowGraphNode();
									return FlowGraphNode ? FlowGraphNode->FlowNode.EventDuration : 0.0f;
								})
								.OnValueChanged(this, &SFlowEventEdGraphNode::SetEventDuration)
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 4.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("NextModeLabel", "Next Mode"))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SComboButton)
								.ContentPadding(FMargin(8.0f, 2.0f))
								.OnGetMenuContent(this, &SFlowEventEdGraphNode::BuildNextModeMenu)
								.ButtonContent()
								[
									SNew(STextBlock)
									.Text(this, &SFlowEventEdGraphNode::GetNextModeText)
								]
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBox)
							.Visibility(this, &SFlowEventEdGraphNode::GetParallelDelayVisibility)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("ParallelDelayLabel", "Delay"))
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SSpinBox<float>)
									.MinValue(0.0f)
									.MinSliderValue(0.0f)
									.MaxSliderValue(30.0f)
									.MinDesiredWidth(76.0f)
									.Value_Lambda([this]()
									{
										const UFlowEventEdGraphNode* FlowGraphNode = GetFlowGraphNode();
										return FlowGraphNode ? FlowGraphNode->FlowNode.ParallelStartDelay : 0.0f;
									})
									.OnValueChanged(this, &SFlowEventEdGraphNode::SetParallelStartDelay)
								]
							]
						]
					]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SAssignNew(RightNodeBox, SVerticalBox)
				]
			]
		];

	CreatePinWidgets();
}

void SFlowEventEdGraphNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));

	if (PinToAdd->GetDirection() == EGPD_Input)
	{
		LeftNodeBox->AddSlot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			[
				PinToAdd
			];
		InputPins.Add(PinToAdd);
	}
	else
	{
		RightNodeBox->AddSlot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			[
				PinToAdd
			];
		OutputPins.Add(PinToAdd);
	}
}

UFlowEventEdGraphNode* SFlowEventEdGraphNode::GetFlowGraphNode() const
{
	return Cast<UFlowEventEdGraphNode>(GraphNode);
}

FText SFlowEventEdGraphNode::GetNodeTitleText() const
{
	return GraphNode ? GraphNode->GetNodeTitle(ENodeTitleType::FullTitle) : FText::GetEmpty();
}

FText SFlowEventEdGraphNode::GetDurationText() const
{
	const UFlowEventEdGraphNode* FlowGraphNode = GetFlowGraphNode();
	return FlowGraphNode ? FText::AsNumber(FlowGraphNode->FlowNode.EventDuration) : FText::GetEmpty();
}

FText SFlowEventEdGraphNode::GetDurationLabelText() const
{
	const UFlowEventEdGraphNode* FlowGraphNode = GetFlowGraphNode();
	return FlowGraphNode && FlowGraphNode->FlowNode.NodeType == EFlowEventNodeType::Delay
		? LOCTEXT("DelayDurationLabel", "Delay")
		: LOCTEXT("DurationLabel", "Duration");
}

FText SFlowEventEdGraphNode::GetNextModeText() const
{
	const UFlowEventEdGraphNode* FlowGraphNode = GetFlowGraphNode();
	if (!FlowGraphNode)
	{
		return FText::GetEmpty();
	}

	return FlowGraphNode->FlowNode.NextMode == EFlowEventNextMode::Parallel
		? LOCTEXT("ParallelMode", "Parallel")
		: LOCTEXT("SerialMode", "Serial");
}

FText SFlowEventEdGraphNode::GetParallelDelayText() const
{
	const UFlowEventEdGraphNode* FlowGraphNode = GetFlowGraphNode();
	return FlowGraphNode ? FText::AsNumber(FlowGraphNode->FlowNode.ParallelStartDelay) : FText::GetEmpty();
}

EVisibility SFlowEventEdGraphNode::GetParallelDelayVisibility() const
{
	const UFlowEventEdGraphNode* FlowGraphNode = GetFlowGraphNode();
	return FlowGraphNode && FlowGraphNode->FlowNode.NextMode == EFlowEventNextMode::Parallel
		? EVisibility::Visible
		: EVisibility::Collapsed;
}

void SFlowEventEdGraphNode::SetEventDuration(float NewValue)
{
	UFlowEventEdGraphNode* FlowGraphNode = GetFlowGraphNode();
	if (!FlowGraphNode)
	{
		return;
	}

	const float ClampedValue = FMath::Max(0.0f, NewValue);
	if (FMath::IsNearlyEqual(FlowGraphNode->FlowNode.EventDuration, ClampedValue))
	{
		return;
	}

	FlowGraphNode->Modify();
	FlowGraphNode->FlowNode.EventDuration = ClampedValue;
	NotifyNodeChanged();
}

void SFlowEventEdGraphNode::SetNextMode(EFlowEventNextMode NewMode)
{
	UFlowEventEdGraphNode* FlowGraphNode = GetFlowGraphNode();
	if (!FlowGraphNode || FlowGraphNode->FlowNode.NextMode == NewMode)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("SetNextMode", "Set Flow Event Next Mode"));
	FlowGraphNode->Modify();
	FlowGraphNode->FlowNode.NextMode = NewMode;
	NotifyNodeChanged();
}

void SFlowEventEdGraphNode::SetParallelStartDelay(float NewValue)
{
	UFlowEventEdGraphNode* FlowGraphNode = GetFlowGraphNode();
	if (!FlowGraphNode)
	{
		return;
	}

	const float ClampedValue = FMath::Max(0.0f, NewValue);
	if (FMath::IsNearlyEqual(FlowGraphNode->FlowNode.ParallelStartDelay, ClampedValue))
	{
		return;
	}

	FlowGraphNode->Modify();
	FlowGraphNode->FlowNode.ParallelStartDelay = ClampedValue;
	NotifyNodeChanged();
}

void SFlowEventEdGraphNode::NotifyNodeChanged()
{
	if (UFlowEventEdGraphNode* FlowGraphNode = GetFlowGraphNode())
	{
		FlowGraphNode->EnsureValidNodeIdentity(FlowGraphNode->SourceNodeIndex);
		if (UEdGraph* Graph = FlowGraphNode->GetGraph())
		{
			Graph->NotifyGraphChanged();
		}
	}
}

TSharedRef<SWidget> SFlowEventEdGraphNode::BuildNextModeMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.AddMenuEntry(
		LOCTEXT("SerialModeEntry", "Serial"),
		LOCTEXT("SerialModeTooltip", "Start the next node after this node finishes."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SFlowEventEdGraphNode::SetNextMode, EFlowEventNextMode::Serial)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ParallelModeEntry", "Parallel"),
		LOCTEXT("ParallelModeTooltip", "Start the next node after ParallelStartDelay while this node keeps running."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SFlowEventEdGraphNode::SetNextMode, EFlowEventNextMode::Parallel)));

	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE

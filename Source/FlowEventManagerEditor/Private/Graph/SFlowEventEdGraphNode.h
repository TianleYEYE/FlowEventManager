#pragma once

#include "CoreMinimal.h"
#include "FlowEventSequenceAsset.h"
#include "SGraphNode.h"

class SGraphPin;
class SVerticalBox;
class UFlowEventEdGraphNode;

class SFlowEventEdGraphNode : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SFlowEventEdGraphNode) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UFlowEventEdGraphNode* InNode);

	virtual void UpdateGraphNode() override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;

private:
	UFlowEventEdGraphNode* GetFlowGraphNode() const;
	FText GetNodeTitleText() const;
	FText GetDurationLabelText() const;
	FText GetDurationText() const;
	FText GetNextModeText() const;
	FText GetParallelDelayText() const;
	EVisibility GetParallelDelayVisibility() const;

	void SetEventDuration(float NewValue);
	void SetNextMode(EFlowEventNextMode NewMode);
	void SetParallelStartDelay(float NewValue);
	void NotifyNodeChanged();

	TSharedRef<SWidget> BuildNextModeMenu();

	TSharedPtr<SVerticalBox> LeftNodeBox;
	TSharedPtr<SVerticalBox> RightNodeBox;
};

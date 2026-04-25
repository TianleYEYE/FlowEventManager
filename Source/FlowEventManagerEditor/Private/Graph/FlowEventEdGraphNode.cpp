#include "Graph/FlowEventEdGraphNode.h"

#include "EdGraph/EdGraph.h"

#define LOCTEXT_NAMESPACE "FlowEventEdGraphNode"

const FName UFlowEventEdGraphNode::InputPinName(TEXT("In"));
const FName UFlowEventEdGraphNode::OutputPinName(TEXT("Out"));

void UFlowEventEdGraphNode::AllocateDefaultPins()
{
	FEdGraphPinType PinType;
	PinType.PinCategory = TEXT("FlowEvent");

	CreatePin(EGPD_Input, PinType, InputPinName);
	CreatePin(EGPD_Output, PinType, OutputPinName);
}

FText UFlowEventEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (!FlowNode.NodeId.IsNone())
	{
		return FText::FromName(FlowNode.NodeId);
	}

	if (!FlowNode.EventName.IsNone())
	{
		return FText::FromName(FlowNode.EventName);
	}

	return LOCTEXT("UnnamedFlowEventNode", "Flow Event");
}

FText UFlowEventEdGraphNode::GetTooltipText() const
{
	return FText::Format(
		LOCTEXT("FlowEventNodeTooltip", "Event: {0}\nDuration: {1}s"),
		FlowNode.EventName.IsNone() ? LOCTEXT("NoEvent", "None") : FText::FromName(FlowNode.EventName),
		FText::AsNumber(FlowNode.EventDuration));
}

FLinearColor UFlowEventEdGraphNode::GetNodeTitleColor() const
{
	return FlowNode.NextMode == EFlowEventNextMode::Parallel
		? FLinearColor(0.18f, 0.46f, 0.78f)
		: FLinearColor(0.16f, 0.56f, 0.36f);
}

bool UFlowEventEdGraphNode::CanUserDeleteNode() const
{
	return true;
}

void UFlowEventEdGraphNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	EnsureValidNodeIdentity(SourceNodeIndex);

	if (UEdGraph* OwningGraph = GetGraph())
	{
		OwningGraph->NotifyGraphChanged();
	}
}

UEdGraphPin* UFlowEventEdGraphNode::GetInputPin() const
{
	return FindPin(InputPinName, EGPD_Input);
}

UEdGraphPin* UFlowEventEdGraphNode::GetOutputPin() const
{
	return FindPin(OutputPinName, EGPD_Output);
}

void UFlowEventEdGraphNode::EnsureValidNodeIdentity(int32 FallbackIndex)
{
	if (FlowNode.NodeId.IsNone())
	{
		const int32 DisplayIndex = FallbackIndex == INDEX_NONE ? 1 : FallbackIndex + 1;
		FlowNode.NodeId = *FString::Printf(TEXT("Node_%d"), DisplayIndex);
	}

#if WITH_EDITORONLY_DATA
	if (!FlowNode.EditorNodeGuid.IsValid())
	{
		FlowNode.EditorNodeGuid = FGuid::NewGuid();
	}
#endif
}

#undef LOCTEXT_NAMESPACE

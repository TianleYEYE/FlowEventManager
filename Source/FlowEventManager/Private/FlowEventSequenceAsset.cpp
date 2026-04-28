#include "FlowEventSequenceAsset.h"

#define LOCTEXT_NAMESPACE "FlowEventSequenceAsset"

namespace FlowEventValidation
{
	void AddIssue(TArray<FFlowEventValidationIssue>& OutIssues, EFlowEventValidationSeverity Severity, int32 NodeIndex, const FFlowEventNode* Node, const FText& Message)
	{
		FFlowEventValidationIssue Issue;
		Issue.Severity = Severity;
		Issue.NodeIndex = NodeIndex;
		Issue.NodeId = Node ? Node->NodeId : NAME_None;
		Issue.Message = Message;
		OutIssues.Add(Issue);
	}
}

#if WITH_EDITOR
void FFlowEventNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FFlowEventNode, NextNodeIndex))
	{
		ParallelNextNodeIndices.Reset();
		if (NextMode == EFlowEventNextMode::Parallel && NextNodeIndex != INDEX_NONE)
		{
			ParallelNextNodeIndices.Add(NextNodeIndex);
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FFlowEventNode, NextMode) && NextMode == EFlowEventNextMode::Serial)
	{
		ParallelNextNodeIndices.Reset();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(FFlowEventNode, ParallelNextNodeIndices))
	{
		if (ParallelNextNodeIndices.Num() > 0)
		{
			NextMode = EFlowEventNextMode::Parallel;
			NextNodeIndex = ParallelNextNodeIndices[0];
		}
	}
}
#endif

void UFlowEventSequenceAsset::ValidateFlow(TArray<FFlowEventValidationIssue>& OutIssues) const
{
	ValidateNodes(Nodes, OutIssues);
}

void UFlowEventSequenceAsset::ValidateNodes(const TArray<FFlowEventNode>& InNodes, TArray<FFlowEventValidationIssue>& OutIssues)
{
	OutIssues.Reset();

	if (InNodes.Num() == 0)
	{
		FlowEventValidation::AddIssue(OutIssues, EFlowEventValidationSeverity::Error, INDEX_NONE, nullptr, LOCTEXT("NoNodes", "The flow has no nodes."));
		return;
	}

	TMap<FName, int32> NodeIdToIndex;
	TSet<int32> ReferencedNodeIndices;
	auto GetNextIndices = [&InNodes](const FFlowEventNode& Node, int32 CurrentIndex)
	{
		TArray<int32> NextIndices;
		if (Node.NextMode == EFlowEventNextMode::Parallel && Node.ParallelNextNodeIndices.Num() > 0)
		{
			for (int32 NextIndex : Node.ParallelNextNodeIndices)
			{
				NextIndices.AddUnique(NextIndex);
			}

			return NextIndices;
		}

		if (Node.NextNodeIndex == INDEX_NONE)
		{
			NextIndices.Add(CurrentIndex + 1);
		}
		else
		{
			if (Node.NextNodeIndex == MAX_int32)
			{
				return NextIndices;
			}

			NextIndices.Add(Node.NextNodeIndex);
		}

		return NextIndices;
	};

	for (int32 Index = 0; Index < InNodes.Num(); ++Index)
	{
		const FFlowEventNode& Node = InNodes[Index];

		if (Node.NodeId.IsNone())
		{
			FlowEventValidation::AddIssue(OutIssues, EFlowEventValidationSeverity::Warning, Index, &Node, LOCTEXT("MissingNodeId", "NodeId is empty. Runtime APIs are easier to use when every node has a stable id."));
		}
		else if (const int32* ExistingIndex = NodeIdToIndex.Find(Node.NodeId))
		{
			FlowEventValidation::AddIssue(
				OutIssues,
				EFlowEventValidationSeverity::Error,
				Index,
				&Node,
				FText::Format(LOCTEXT("DuplicateNodeId", "NodeId duplicates node {0}. Node ids must be unique."), FText::AsNumber(*ExistingIndex)));
		}
		else
		{
			NodeIdToIndex.Add(Node.NodeId, Index);
		}

		if (Node.NodeType == EFlowEventNodeType::Event && Node.EventName.IsNone())
		{
			FlowEventValidation::AddIssue(OutIssues, EFlowEventValidationSeverity::Warning, Index, &Node, LOCTEXT("MissingEventName", "EventName is empty. This node will not call a target event."));
		}

		if (Node.EventDuration < 0.0f)
		{
			FlowEventValidation::AddIssue(OutIssues, EFlowEventValidationSeverity::Error, Index, &Node, LOCTEXT("NegativeDuration", "EventDuration cannot be negative."));
		}

		if (Node.NextMode == EFlowEventNextMode::Parallel && Node.ParallelStartDelay < 0.0f)
		{
			FlowEventValidation::AddIssue(OutIssues, EFlowEventValidationSeverity::Error, Index, &Node, LOCTEXT("NegativeParallelDelay", "ParallelStartDelay cannot be negative."));
		}

		if (Node.NodeType == EFlowEventNodeType::Event)
		{
			switch (Node.TargetMode)
			{
			case EFlowEventTargetMode::ExplicitActor:
				if (!Node.TargetActor)
				{
					FlowEventValidation::AddIssue(OutIssues, EFlowEventValidationSeverity::Warning, Index, &Node, LOCTEXT("MissingExplicitActor", "TargetMode is Explicit Actor, but TargetActor is not set."));
				}
				break;

			case EFlowEventTargetMode::FirstActorWithTag:
			case EFlowEventTargetMode::AllActorsWithTag:
				if (Node.TargetTag.IsNone())
				{
					FlowEventValidation::AddIssue(OutIssues, EFlowEventValidationSeverity::Error, Index, &Node, LOCTEXT("MissingTargetTag", "TargetMode uses actor tags, but TargetTag is empty."));
				}
				break;

			case EFlowEventTargetMode::FirstActorOfClass:
			case EFlowEventTargetMode::AllActorsOfClass:
				if (!Node.TargetClass)
				{
					FlowEventValidation::AddIssue(OutIssues, EFlowEventValidationSeverity::Error, Index, &Node, LOCTEXT("MissingTargetClass", "TargetMode uses actor classes, but TargetClass is not set."));
				}
				break;

			default:
				break;
			}
		}

		const TArray<int32> NextIndices = GetNextIndices(Node, Index);
		for (int32 NextIndex : NextIndices)
		{
			if (NextIndex != InNodes.Num())
			{
				if (!InNodes.IsValidIndex(NextIndex))
				{
					FlowEventValidation::AddIssue(
						OutIssues,
						EFlowEventValidationSeverity::Error,
						Index,
						&Node,
						FText::Format(
							LOCTEXT("InvalidNextIndex", "Next node index {0} is outside the node array. For Parallel nodes, check ParallelNextNodeIndices as well as NextNodeIndex."),
							FText::AsNumber(NextIndex)));
				}
				else
				{
					ReferencedNodeIndices.Add(NextIndex);
				}
			}

			if (NextIndex == Index)
			{
				FlowEventValidation::AddIssue(OutIssues, EFlowEventValidationSeverity::Error, Index, &Node, LOCTEXT("SelfReference", "Node points to itself as the next node."));
			}
		}
	}

	for (int32 Index = 1; Index < InNodes.Num(); ++Index)
	{
		if (!ReferencedNodeIndices.Contains(Index))
		{
			FlowEventValidation::AddIssue(OutIssues, EFlowEventValidationSeverity::Warning, Index, &InNodes[Index], LOCTEXT("UnreferencedNode", "Node is not referenced by any previous node. It may be unreachable unless started manually."));
		}
	}

	for (int32 StartIndex = 0; StartIndex < InNodes.Num(); ++StartIndex)
	{
		TSet<int32> Path;
		TFunction<void(int32)> VisitPath = [&](int32 CurrentIndex)
		{
			if (!InNodes.IsValidIndex(CurrentIndex))
			{
				return;
			}

			if (Path.Contains(CurrentIndex))
			{
				FlowEventValidation::AddIssue(OutIssues, EFlowEventValidationSeverity::Error, CurrentIndex, &InNodes[CurrentIndex], LOCTEXT("CycleDetected", "A cycle was detected in this flow path. Cycles are not supported by the default runtime execution model."));
				return;
			}

			Path.Add(CurrentIndex);
			const FFlowEventNode& Node = InNodes[CurrentIndex];
			for (int32 NextIndex : GetNextIndices(Node, CurrentIndex))
			{
				if (NextIndex != InNodes.Num())
				{
					VisitPath(NextIndex);
				}
			}
			Path.Remove(CurrentIndex);
		};

		VisitPath(StartIndex);
	}
}

bool UFlowEventSequenceAsset::HasValidationErrors() const
{
	TArray<FFlowEventValidationIssue> Issues;
	ValidateFlow(Issues);

	for (const FFlowEventValidationIssue& Issue : Issues)
	{
		if (Issue.Severity == EFlowEventValidationSeverity::Error)
		{
			return true;
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE

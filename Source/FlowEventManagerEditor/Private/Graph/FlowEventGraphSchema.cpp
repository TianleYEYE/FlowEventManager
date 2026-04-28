#include "Graph/FlowEventGraphSchema.h"

#include "EdGraph/EdGraph.h"
#include "Graph/FlowEventEdGraphNode.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FlowEventGraphSchema"

UEdGraphNode* FFlowEventGraphSchemaAction_NewNode::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	if (!ParentGraph)
	{
		return nullptr;
	}

	const FScopedTransaction Transaction(LOCTEXT("CreateFlowEventNode", "Create Flow Event Node"));
	ParentGraph->Modify();

	UFlowEventEdGraphNode* NewNode = NewObject<UFlowEventEdGraphNode>(ParentGraph);
	NewNode->SetFlags(RF_Transactional);
	NewNode->CreateNewGuid();
	NewNode->NodePosX = Location.X;
	NewNode->NodePosY = Location.Y;
	NewNode->FlowNode.NodeType = NodeType;
	NewNode->FlowNode.NodeId = NodeType == EFlowEventNodeType::Delay
		? *FString::Printf(TEXT("Delay_%d"), ParentGraph->Nodes.Num() + 1)
		: *FString::Printf(TEXT("Node_%d"), ParentGraph->Nodes.Num() + 1);
#if WITH_EDITORONLY_DATA
	NewNode->FlowNode.EditorNodeGuid = FGuid::NewGuid();
	NewNode->FlowNode.EditorPosition = Location;
#endif
	NewNode->EnsureValidNodeIdentity(ParentGraph->Nodes.Num());
	NewNode->AllocateDefaultPins();
	ParentGraph->AddNode(NewNode, true, bSelectNewNode);

	if (FromPin)
	{
		if (UEdGraphPin* TargetPin = FromPin->Direction == EGPD_Output ? NewNode->GetInputPin() : NewNode->GetOutputPin())
		{
			FromPin->GetSchema()->TryCreateConnection(FromPin, TargetPin);
		}
	}

	ParentGraph->NotifyGraphChanged();
	return NewNode;
}

void UFlowEventGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	const TSharedPtr<FFlowEventGraphSchemaAction_NewNode> NewNodeAction = MakeShared<FFlowEventGraphSchemaAction_NewNode>(
		LOCTEXT("FlowEventCategory", "Flow Event"),
		LOCTEXT("AddFlowEventNode", "Add Flow Event Node"),
		LOCTEXT("AddFlowEventNodeTooltip", "Adds a flow event step to the sequence."),
		0,
		EFlowEventNodeType::Event);

	ContextMenuBuilder.AddAction(NewNodeAction);

	const TSharedPtr<FFlowEventGraphSchemaAction_NewNode> NewDelayNodeAction = MakeShared<FFlowEventGraphSchemaAction_NewNode>(
		LOCTEXT("FlowEventCategory", "Flow Event"),
		LOCTEXT("AddFlowDelayNode", "Add Delay Node"),
		LOCTEXT("AddFlowDelayNodeTooltip", "Adds a delay step that waits for its duration without calling a target event."),
		1,
		EFlowEventNodeType::Delay);

	ContextMenuBuilder.AddAction(NewDelayNodeAction);
}

const FPinConnectionResponse UFlowEventGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	if (!A || !B)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("InvalidPins", "Invalid pins."));
	}

	if (A == B || A->GetOwningNode() == B->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SameNode", "A node cannot connect to itself."));
	}

	if (A->Direction == B->Direction)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SameDirection", "Connect an output pin to an input pin."));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, LOCTEXT("MakeConnection", "Connect flow."));
}

bool UFlowEventGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	if (!A || !B || A->Direction == B->Direction)
	{
		return false;
	}

	UEdGraphPin* OutputPin = A->Direction == EGPD_Output ? A : B;
	UEdGraphPin* InputPin = A->Direction == EGPD_Input ? A : B;

	if (OutputPin->GetOwningNode() == InputPin->GetOwningNode())
	{
		return false;
	}

	if (OutputPin->LinkedTo.Contains(InputPin))
	{
		return true;
	}

	const FScopedTransaction Transaction(LOCTEXT("ConnectFlowEventNodes", "Connect Flow Event Nodes"));
	OutputPin->Modify();
	InputPin->Modify();

	InputPin->BreakAllPinLinks();
	OutputPin->MakeLinkTo(InputPin);

	if (UEdGraph* Graph = OutputPin->GetOwningNode()->GetGraph())
	{
		Graph->NotifyGraphChanged();
	}

	return true;
}

bool UFlowEventGraphSchema::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const
{
	return true;
}

#undef LOCTEXT_NAMESPACE

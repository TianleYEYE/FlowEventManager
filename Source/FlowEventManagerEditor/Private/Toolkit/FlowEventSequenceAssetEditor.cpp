#include "Toolkit/FlowEventSequenceAssetEditor.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphUtilities.h"
#include "Graph/FlowEventEdGraphNode.h"
#include "Graph/FlowEventGraphSchema.h"
#include "GraphEditor.h"
#include "GraphEditorActions.h"
#include "Framework/Commands/GenericCommands.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FlowEventSequenceAssetEditor"

const FName FFlowEventSequenceAssetEditor::GraphTabId(TEXT("FlowEventSequenceAssetEditor_Graph"));
const FName FFlowEventSequenceAssetEditor::DetailsTabId(TEXT("FlowEventSequenceAssetEditor_Details"));

void FFlowEventSequenceAssetEditor::InitFlowEventSequenceAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UFlowEventSequenceAsset* InFlowAsset)
{
	FlowAsset = InFlowAsset;
	CreateEditorGraph();
	RefreshValidationResults();

	GraphEditorCommands = MakeShared<FUICommandList>();
	GraphEditorCommands->MapAction(
		FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &FFlowEventSequenceAssetEditor::DeleteSelectedNodes),
		FCanExecuteAction::CreateSP(this, &FFlowEventSequenceAssetEditor::CanDeleteSelectedNodes));

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("FlowEventSequenceAssetEditor_Layout_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewStack()
					->AddTab(GraphTabId, ETabState::OpenedTab)
					->SetHideTabWell(true)
					->SetSizeCoefficient(0.72f)
				)
				->Split
				(
					FTabManager::NewStack()
					->AddTab(DetailsTabId, ETabState::OpenedTab)
					->SetHideTabWell(true)
					->SetSizeCoefficient(0.28f)
				)
			)
		);

	InitAssetEditor(Mode, InitToolkitHost, TEXT("FlowEventSequenceAssetEditorApp"), Layout, true, true, InFlowAsset);
	RegenerateMenusAndToolbars();
}

FName FFlowEventSequenceAssetEditor::GetToolkitFName() const
{
	return FName("FlowEventSequenceAssetEditor");
}

FText FFlowEventSequenceAssetEditor::GetBaseToolkitName() const
{
	return LOCTEXT("FlowEventSequenceAssetEditorName", "Flow Event Sequence");
}

FString FFlowEventSequenceAssetEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("Flow Event");
}

FLinearColor FFlowEventSequenceAssetEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.18f, 0.46f, 0.78f);
}

void FFlowEventSequenceAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(GraphTabId, FOnSpawnTab::CreateSP(this, &FFlowEventSequenceAssetEditor::SpawnGraphTab))
		.SetDisplayName(LOCTEXT("GraphTab", "Graph"));

	InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FFlowEventSequenceAssetEditor::SpawnDetailsTab))
		.SetDisplayName(LOCTEXT("DetailsTab", "Details"));
}

void FFlowEventSequenceAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	InTabManager->UnregisterTabSpawner(GraphTabId);
	InTabManager->UnregisterTabSpawner(DetailsTabId);

	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
}

FString FFlowEventSequenceAssetEditor::GetReferencerName() const
{
	return TEXT("FFlowEventSequenceAssetEditor");
}

void FFlowEventSequenceAssetEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(FlowAsset);
	Collector.AddReferencedObject(EditorGraph);
}

TSharedRef<SDockTab> FFlowEventSequenceAssetEditor::SpawnGraphTab(const FSpawnTabArgs& Args)
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("FlowEventGraphCornerText", "FLOW EVENT");

	SGraphEditor::FGraphEditorEvents Events;
	Events.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FFlowEventSequenceAssetEditor::OnSelectedNodesChanged);
	Events.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FFlowEventSequenceAssetEditor::OnNodeDoubleClicked);

	SAssignNew(GraphEditor, SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.Appearance(AppearanceInfo)
		.GraphToEdit(EditorGraph)
		.GraphEvents(Events)
		.IsEditable(true);

	return SNew(SDockTab)
		.Label(LOCTEXT("GraphTabLabel", "Graph"))
		[
			GraphEditor.ToSharedRef()
		];
}

TSharedRef<SDockTab> FFlowEventSequenceAssetEditor::SpawnDetailsTab(const FSpawnTabArgs& Args)
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->OnFinishedChangingProperties().AddSP(this, &FFlowEventSequenceAssetEditor::OnFinishedChangingProperties);
	DetailsView->SetObject(FlowAsset);

	return SNew(SDockTab)
		.Label(LOCTEXT("DetailsTabLabel", "Details"))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("ValidateFlowButton", "Validate Flow"))
					.ToolTipText(LOCTEXT("ValidateFlowButtonTooltip", "Checks this flow for missing targets, invalid links, duplicate ids, and unreachable nodes."))
					.OnClicked(this, &FFlowEventSequenceAssetEditor::OnValidateFlowClicked)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(6.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("AutoArrangeButton", "Auto Arrange"))
					.ToolTipText(LOCTEXT("AutoArrangeButtonTooltip", "Arranges nodes from left to right and aligns parallel branches in the same column."))
					.OnClicked(this, &FFlowEventSequenceAssetEditor::OnAutoArrangeClicked)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(6.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("FixNodeIdsButton", "Fix Node IDs"))
					.ToolTipText(LOCTEXT("FixNodeIdsButtonTooltip", "Assigns unique ids to nodes with empty or duplicate NodeId values."))
					.OnClicked(this, &FFlowEventSequenceAssetEditor::OnFixNodeIdsClicked)
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(this, &FFlowEventSequenceAssetEditor::GetValidationSummaryText)
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f, 0.0f, 4.0f, 4.0f)
			[
				SNew(SBox)
				.MaxDesiredHeight(120.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(STextBlock)
						.AutoWrapText(true)
						.Text(this, &FFlowEventSequenceAssetEditor::GetValidationDetailsText)
					]
				]
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				DetailsView.ToSharedRef()
			]
		];
}

void FFlowEventSequenceAssetEditor::CreateEditorGraph()
{
	EditorGraph = NewObject<UEdGraph>(GetTransientPackage(), NAME_None, RF_Transactional | RF_Transient);
	EditorGraph->Schema = UFlowEventGraphSchema::StaticClass();
	OnGraphChangedHandle = EditorGraph->AddOnGraphChangedHandler(FOnGraphChanged::FDelegate::CreateSP(this, &FFlowEventSequenceAssetEditor::OnGraphChanged));

	RebuildGraphFromAsset();
}

void FFlowEventSequenceAssetEditor::RebuildGraphFromAsset()
{
	if (!FlowAsset || !EditorGraph)
	{
		return;
	}

	TGuardValue<bool> SynchronizingGuard(bIsSynchronizing, true);

	EditorGraph->Nodes.Reset();
	TArray<UFlowEventEdGraphNode*> GraphNodes;
	GraphNodes.Reserve(FlowAsset->Nodes.Num());

	for (int32 Index = 0; Index < FlowAsset->Nodes.Num(); ++Index)
	{
		const FVector2D FallbackPosition(Index * 280.0f, 0.0f);
		UFlowEventEdGraphNode* NewNode = CreateGraphNodeFromFlowNode(FlowAsset->Nodes[Index], Index, FallbackPosition);
		GraphNodes.Add(NewNode);
	}

	for (int32 Index = 0; Index < FlowAsset->Nodes.Num(); ++Index)
	{
		const FFlowEventNode& FlowNode = FlowAsset->Nodes[Index];
		TArray<int32> NextIndices;
		if (FlowNode.NextMode == EFlowEventNextMode::Parallel && FlowNode.ParallelNextNodeIndices.Num() > 0)
		{
			NextIndices = FlowNode.ParallelNextNodeIndices;
		}
		else if (FlowNode.NextNodeIndex == INDEX_NONE)
		{
			NextIndices.Add(Index + 1);
		}
		else
		{
			NextIndices.Add(FlowNode.NextNodeIndex);
		}

		for (int32 NextIndex : NextIndices)
		{
			if (GraphNodes.IsValidIndex(Index) && GraphNodes.IsValidIndex(NextIndex))
			{
				UEdGraphPin* OutputPin = GraphNodes[Index]->GetOutputPin();
				UEdGraphPin* InputPin = GraphNodes[NextIndex]->GetInputPin();
				if (OutputPin && InputPin && !OutputPin->LinkedTo.Contains(InputPin))
				{
					OutputPin->MakeLinkTo(InputPin);
				}
			}
		}
	}

	EditorGraph->NotifyGraphChanged();
}

void FFlowEventSequenceAssetEditor::ExportGraphToAsset()
{
	if (!FlowAsset || !EditorGraph || bIsSynchronizing)
	{
		return;
	}

	TGuardValue<bool> SynchronizingGuard(bIsSynchronizing, true);
	const TArray<UFlowEventEdGraphNode*> OrderedNodes = GetOrderedGraphNodes();

	TMap<UFlowEventEdGraphNode*, int32> NodeToIndex;
	for (int32 Index = 0; Index < OrderedNodes.Num(); ++Index)
	{
		NodeToIndex.Add(OrderedNodes[Index], Index);
	}

	FlowAsset->Modify();
	FlowAsset->Nodes.Reset(OrderedNodes.Num());

	for (int32 Index = 0; Index < OrderedNodes.Num(); ++Index)
	{
		UFlowEventEdGraphNode* GraphNode = OrderedNodes[Index];
		GraphNode->EnsureValidNodeIdentity(Index);

		FFlowEventNode FlowNode = GraphNode->FlowNode;
#if WITH_EDITORONLY_DATA
		FlowNode.EditorPosition = FVector2D(GraphNode->NodePosX, GraphNode->NodePosY);
#endif
		FlowNode.NextNodeIndex = StopNextNodeIndex;
		FlowNode.ParallelNextNodeIndices.Reset();

		if (UEdGraphPin* OutputPin = GraphNode->GetOutputPin())
		{
			TArray<int32> LinkedIndices;
			for (UEdGraphPin* LinkedPin : OutputPin->LinkedTo)
			{
				if (UFlowEventEdGraphNode* LinkedNode = LinkedPin ? Cast<UFlowEventEdGraphNode>(LinkedPin->GetOwningNode()) : nullptr)
				{
					if (const int32* LinkedIndex = NodeToIndex.Find(LinkedNode))
					{
						LinkedIndices.AddUnique(*LinkedIndex);
					}
				}
			}

			if (LinkedIndices.Num() > 0)
			{
				FlowNode.NextNodeIndex = LinkedIndices[0];
				if (LinkedIndices.Num() > 1)
				{
					FlowNode.NextMode = EFlowEventNextMode::Parallel;
				}

				if (FlowNode.NextMode == EFlowEventNextMode::Parallel)
				{
					FlowNode.ParallelNextNodeIndices = LinkedIndices;
				}
			}
		}

		if (Index == OrderedNodes.Num() - 1 && FlowNode.NextNodeIndex == StopNextNodeIndex)
		{
			FlowNode.NextNodeIndex = INDEX_NONE;
		}

		FlowAsset->Nodes.Add(FlowNode);
	}

	FlowAsset->MarkPackageDirty();
	RefreshValidationResults();
}

void FFlowEventSequenceAssetEditor::OnGraphChanged(const FEdGraphEditAction& Action)
{
	ExportGraphToAsset();
	if (DetailsView.IsValid())
	{
		DetailsView->ForceRefresh();
	}
}

void FFlowEventSequenceAssetEditor::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FFlowEventNode, NextNodeIndex) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FFlowEventNode, ParallelNextNodeIndices))
	{
		SynchronizeGraphLinksFromNodeProperties();
	}

	if (GraphEditor.IsValid())
	{
		GraphEditor->NotifyGraphChanged();
	}

	ExportGraphToAsset();
	RefreshValidationResults();
}

void FFlowEventSequenceAssetEditor::OnSelectedNodesChanged(const TSet<UObject*>& NewSelection)
{
	if (!DetailsView.IsValid())
	{
		return;
	}

	TArray<UObject*> SelectedObjects;
	for (UObject* SelectedObject : NewSelection)
	{
		if (SelectedObject)
		{
			SelectedObjects.Add(SelectedObject);
		}
	}

	if (SelectedObjects.Num() > 0)
	{
		DetailsView->SetObjects(SelectedObjects);
	}
	else
	{
		DetailsView->SetObject(FlowAsset);
	}
}

void FFlowEventSequenceAssetEditor::OnNodeDoubleClicked(UEdGraphNode* Node)
{
	UFlowEventEdGraphNode* FlowGraphNode = Cast<UFlowEventEdGraphNode>(Node);
	if (!FlowGraphNode || !DetailsView.IsValid())
	{
		return;
	}

	FlowGraphNode->Modify();
	if (FlowGraphNode->FlowNode.NodeType == EFlowEventNodeType::Event)
	{
		FlowGraphNode->FlowNode.bUseTimelineCurve = true;
	}
	DetailsView->SetObject(FlowGraphNode);
	ExportGraphToAsset();
}

FReply FFlowEventSequenceAssetEditor::OnValidateFlowClicked()
{
	ExportGraphToAsset();
	RefreshValidationResults();
	return FReply::Handled();
}

FReply FFlowEventSequenceAssetEditor::OnAutoArrangeClicked()
{
	const FScopedTransaction Transaction(LOCTEXT("AutoArrangeFlowEventNodes", "Auto Arrange Flow Event Nodes"));
	AutoArrangeNodes();
	ExportGraphToAsset();
	if (GraphEditor.IsValid())
	{
		GraphEditor->NotifyGraphChanged();
	}
	return FReply::Handled();
}

FReply FFlowEventSequenceAssetEditor::OnFixNodeIdsClicked()
{
	const FScopedTransaction Transaction(LOCTEXT("FixFlowEventNodeIds", "Fix Flow Event Node IDs"));
	FixNodeIds();
	ExportGraphToAsset();
	if (DetailsView.IsValid())
	{
		DetailsView->ForceRefresh();
	}
	if (GraphEditor.IsValid())
	{
		GraphEditor->NotifyGraphChanged();
	}
	return FReply::Handled();
}

void FFlowEventSequenceAssetEditor::RefreshValidationResults()
{
	if (!FlowAsset)
	{
		ValidationSummaryText = LOCTEXT("ValidationNoAsset", "No flow asset loaded.");
		ValidationDetailsText = FText::GetEmpty();
		return;
	}

	TArray<FFlowEventValidationIssue> Issues;
	FlowAsset->ValidateFlow(Issues);

	int32 ErrorCount = 0;
	int32 WarningCount = 0;
	int32 InfoCount = 0;
	FString Details;

	for (const FFlowEventValidationIssue& Issue : Issues)
	{
		FString SeverityText;
		switch (Issue.Severity)
		{
		case EFlowEventValidationSeverity::Error:
			SeverityText = TEXT("Error");
			++ErrorCount;
			break;

		case EFlowEventValidationSeverity::Warning:
			SeverityText = TEXT("Warning");
			++WarningCount;
			break;

		default:
			SeverityText = TEXT("Info");
			++InfoCount;
			break;
		}

		Details += FString::Printf(
			TEXT("[%s] Node %d (%s): %s\n"),
			*SeverityText,
			Issue.NodeIndex,
			*Issue.NodeId.ToString(),
			*Issue.Message.ToString());
	}

	if (Issues.Num() == 0)
	{
		ValidationSummaryText = LOCTEXT("ValidationClean", "No validation issues.");
		ValidationDetailsText = LOCTEXT("ValidationCleanDetails", "This flow is ready to run.");
		return;
	}

	ValidationSummaryText = FText::Format(
		LOCTEXT("ValidationSummary", "{0} errors, {1} warnings, {2} info"),
		FText::AsNumber(ErrorCount),
		FText::AsNumber(WarningCount),
		FText::AsNumber(InfoCount));
	ValidationDetailsText = FText::FromString(Details);
}

FText FFlowEventSequenceAssetEditor::GetValidationSummaryText() const
{
	return ValidationSummaryText;
}

FText FFlowEventSequenceAssetEditor::GetValidationDetailsText() const
{
	return ValidationDetailsText;
}

void FFlowEventSequenceAssetEditor::DeleteSelectedNodes()
{
	if (!GraphEditor.IsValid() || !EditorGraph)
	{
		return;
	}

	const FGraphPanelSelectionSet SelectedNodes = GraphEditor->GetSelectedNodes();
	if (SelectedNodes.Num() == 0)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("DeleteFlowEventNodes", "Delete Flow Event Nodes"));
	EditorGraph->Modify();
	GraphEditor->ClearSelectionSet();

	for (UObject* SelectedObject : SelectedNodes)
	{
		if (UEdGraphNode* SelectedNode = Cast<UEdGraphNode>(SelectedObject))
		{
			if (SelectedNode->CanUserDeleteNode())
			{
				SelectedNode->Modify();
				SelectedNode->DestroyNode();
			}
		}
	}

	ExportGraphToAsset();
	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(FlowAsset);
	}
}

bool FFlowEventSequenceAssetEditor::CanDeleteSelectedNodes() const
{
	return GraphEditor.IsValid() && GraphEditor->GetSelectedNodes().Num() > 0;
}

TArray<UFlowEventEdGraphNode*> FFlowEventSequenceAssetEditor::GetOrderedGraphNodes() const
{
	TArray<UFlowEventEdGraphNode*> OrderedNodes;
	if (!EditorGraph)
	{
		return OrderedNodes;
	}

	for (UEdGraphNode* Node : EditorGraph->Nodes)
	{
		if (UFlowEventEdGraphNode* FlowNode = Cast<UFlowEventEdGraphNode>(Node))
		{
			OrderedNodes.Add(FlowNode);
		}
	}

	return OrderedNodes;
}

UFlowEventEdGraphNode* FFlowEventSequenceAssetEditor::CreateGraphNodeFromFlowNode(const FFlowEventNode& FlowNode, int32 SourceIndex, const FVector2D& FallbackPosition)
{
	UFlowEventEdGraphNode* NewNode = NewObject<UFlowEventEdGraphNode>(EditorGraph);
	NewNode->SetFlags(RF_Transactional);
	NewNode->CreateNewGuid();
	NewNode->FlowNode = FlowNode;
	NewNode->SourceNodeIndex = SourceIndex;
#if WITH_EDITORONLY_DATA
	const FVector2D NodePosition = FlowNode.EditorNodeGuid.IsValid() ? FlowNode.EditorPosition : FallbackPosition;
#else
	const FVector2D NodePosition = FallbackPosition;
#endif
	NewNode->NodePosX = NodePosition.X;
	NewNode->NodePosY = NodePosition.Y;
	NewNode->EnsureValidNodeIdentity(SourceIndex);
	NewNode->AllocateDefaultPins();
	EditorGraph->AddNode(NewNode, false, false);
	return NewNode;
}

void FFlowEventSequenceAssetEditor::SynchronizeGraphLinksFromNodeProperties()
{
	if (!EditorGraph || bIsSynchronizing)
	{
		return;
	}

	TGuardValue<bool> SynchronizingGuard(bIsSynchronizing, true);
	const TArray<UFlowEventEdGraphNode*> OrderedNodes = GetOrderedGraphNodes();

	for (int32 Index = 0; Index < OrderedNodes.Num(); ++Index)
	{
		UFlowEventEdGraphNode* Node = OrderedNodes[Index];
		if (!Node)
		{
			continue;
		}

		TArray<int32> DesiredNextIndices;
		if (Node->FlowNode.NextMode == EFlowEventNextMode::Parallel && Node->FlowNode.ParallelNextNodeIndices.Num() > 0)
		{
			for (int32 NextIndex : Node->FlowNode.ParallelNextNodeIndices)
			{
				DesiredNextIndices.AddUnique(NextIndex);
			}
		}
		else if (Node->FlowNode.NextNodeIndex == INDEX_NONE)
		{
			DesiredNextIndices.Add(Index + 1);
		}
		else if (Node->FlowNode.NextNodeIndex != StopNextNodeIndex)
		{
			DesiredNextIndices.Add(Node->FlowNode.NextNodeIndex);
		}

		UEdGraphPin* OutputPin = Node->GetOutputPin();
		if (!OutputPin)
		{
			continue;
		}

		OutputPin->Modify();
		OutputPin->BreakAllPinLinks();

		for (int32 NextIndex : DesiredNextIndices)
		{
			if (!OrderedNodes.IsValidIndex(NextIndex))
			{
				continue;
			}

			if (UEdGraphPin* InputPin = OrderedNodes[NextIndex] ? OrderedNodes[NextIndex]->GetInputPin() : nullptr)
			{
				InputPin->Modify();
				InputPin->BreakAllPinLinks();
				OutputPin->MakeLinkTo(InputPin);
			}
		}
	}

	if (EditorGraph)
	{
		EditorGraph->NotifyGraphChanged();
	}
}

void FFlowEventSequenceAssetEditor::AutoArrangeNodes()
{
	if (!EditorGraph)
	{
		return;
	}

	EditorGraph->Modify();
	const TArray<UFlowEventEdGraphNode*> OrderedNodes = GetOrderedGraphNodes();
	TMap<UFlowEventEdGraphNode*, int32> NodeColumns;
	TMap<UFlowEventEdGraphNode*, int32> NodeRows;
	TSet<UFlowEventEdGraphNode*> VisitedNodes;
	TArray<UFlowEventEdGraphNode*> Queue;

	if (OrderedNodes.Num() > 0)
	{
		NodeColumns.Add(OrderedNodes[0], 0);
		NodeRows.Add(OrderedNodes[0], 0);
		Queue.Add(OrderedNodes[0]);
	}

	while (Queue.Num() > 0)
	{
		UFlowEventEdGraphNode* Node = Queue[0];
		Queue.RemoveAt(0);
		if (!Node)
		{
			continue;
		}

		if (VisitedNodes.Contains(Node))
		{
			continue;
		}

		VisitedNodes.Add(Node);

		TArray<UFlowEventEdGraphNode*> LinkedNodes;
		if (UEdGraphPin* OutputPin = Node->GetOutputPin())
		{
			for (UEdGraphPin* LinkedPin : OutputPin->LinkedTo)
			{
				if (UFlowEventEdGraphNode* LinkedNode = LinkedPin ? Cast<UFlowEventEdGraphNode>(LinkedPin->GetOwningNode()) : nullptr)
				{
					LinkedNodes.Add(LinkedNode);
				}
			}
		}

		const int32 ParentColumn = NodeColumns.FindRef(Node);
		const int32 ParentRow = NodeRows.FindRef(Node);
		const int32 FirstChildRow = ParentRow - (LinkedNodes.Num() - 1);
		for (int32 ChildIndex = 0; ChildIndex < LinkedNodes.Num(); ++ChildIndex)
		{
			UFlowEventEdGraphNode* LinkedNode = LinkedNodes[ChildIndex];
			if (!LinkedNode || NodeColumns.Contains(LinkedNode))
			{
				continue;
			}

			NodeColumns.Add(LinkedNode, ParentColumn + 1);
			NodeRows.Add(LinkedNode, FirstChildRow + ChildIndex * 2);
			Queue.Add(LinkedNode);
		}
	}

	for (int32 Index = 0; Index < OrderedNodes.Num(); ++Index)
	{
		UFlowEventEdGraphNode* Node = OrderedNodes[Index];
		if (!Node)
		{
			continue;
		}

		if (!NodeColumns.Contains(Node))
		{
			NodeColumns.Add(Node, Index);
			NodeRows.Add(Node, 0);
		}

		Node->Modify();
		Node->NodePosX = NodeColumns.FindRef(Node) * 340;
		Node->NodePosY = NodeRows.FindRef(Node) * 90;
	}

	EditorGraph->NotifyGraphChanged();
}

void FFlowEventSequenceAssetEditor::FixNodeIds()
{
	const TArray<UFlowEventEdGraphNode*> OrderedNodes = GetOrderedGraphNodes();
	TSet<FName> UsedNodeIds;

	for (int32 Index = 0; Index < OrderedNodes.Num(); ++Index)
	{
		UFlowEventEdGraphNode* Node = OrderedNodes[Index];
		if (!Node)
		{
			continue;
		}

		FName DesiredNodeId = Node->FlowNode.NodeId;
		if (DesiredNodeId.IsNone() || UsedNodeIds.Contains(DesiredNodeId))
		{
			int32 CandidateNumber = Index + 1;
			do
			{
				DesiredNodeId = *FString::Printf(TEXT("Node_%03d"), CandidateNumber++);
			}
			while (UsedNodeIds.Contains(DesiredNodeId));

			Node->Modify();
			Node->FlowNode.NodeId = DesiredNodeId;
		}

		UsedNodeIds.Add(DesiredNodeId);
	}
}

#undef LOCTEXT_NAMESPACE

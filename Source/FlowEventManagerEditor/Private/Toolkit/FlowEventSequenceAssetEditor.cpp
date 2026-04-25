#include "Toolkit/FlowEventSequenceAssetEditor.h"

#include "EdGraph/EdGraph.h"
#include "EdGraphUtilities.h"
#include "Graph/FlowEventEdGraphNode.h"
#include "Graph/FlowEventGraphSchema.h"
#include "GraphEditor.h"
#include "GraphEditorActions.h"
#include "Framework/Commands/GenericCommands.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FlowEventSequenceAssetEditor"

const FName FFlowEventSequenceAssetEditor::GraphTabId(TEXT("FlowEventSequenceAssetEditor_Graph"));
const FName FFlowEventSequenceAssetEditor::DetailsTabId(TEXT("FlowEventSequenceAssetEditor_Details"));

void FFlowEventSequenceAssetEditor::InitFlowEventSequenceAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UFlowEventSequenceAsset* InFlowAsset)
{
	FlowAsset = InFlowAsset;
	CreateEditorGraph();

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
			DetailsView.ToSharedRef()
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
		int32 NextIndex = FlowNode.NextNodeIndex;
		if (NextIndex == INDEX_NONE)
		{
			NextIndex = Index + 1;
		}

		if (GraphNodes.IsValidIndex(Index) && GraphNodes.IsValidIndex(NextIndex))
		{
			UEdGraphPin* OutputPin = GraphNodes[Index]->GetOutputPin();
			UEdGraphPin* InputPin = GraphNodes[NextIndex]->GetInputPin();
			if (OutputPin && InputPin)
			{
				OutputPin->MakeLinkTo(InputPin);
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

		if (UEdGraphPin* OutputPin = GraphNode->GetOutputPin())
		{
			for (UEdGraphPin* LinkedPin : OutputPin->LinkedTo)
			{
				if (UFlowEventEdGraphNode* LinkedNode = LinkedPin ? Cast<UFlowEventEdGraphNode>(LinkedPin->GetOwningNode()) : nullptr)
				{
					if (const int32* LinkedIndex = NodeToIndex.Find(LinkedNode))
					{
						FlowNode.NextNodeIndex = *LinkedIndex;
						break;
					}
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
	if (GraphEditor.IsValid())
	{
		GraphEditor->NotifyGraphChanged();
	}

	ExportGraphToAsset();
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
	FlowGraphNode->FlowNode.bUseTimelineCurve = true;
	DetailsView->SetObject(FlowGraphNode);
	ExportGraphToAsset();
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

#undef LOCTEXT_NAMESPACE

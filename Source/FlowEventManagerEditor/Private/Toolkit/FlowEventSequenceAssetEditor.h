#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "FlowEventSequenceAsset.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "UObject/GCObject.h"

class IDetailsView;
class SGraphEditor;
class UFlowEventEdGraphNode;

class FFlowEventSequenceAssetEditor : public FAssetEditorToolkit, public FGCObject
{
public:
	void InitFlowEventSequenceAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UFlowEventSequenceAsset* InFlowAsset);

	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

	virtual FString GetReferencerName() const override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

private:
	static const FName GraphTabId;
	static const FName DetailsTabId;
	static constexpr int32 StopNextNodeIndex = MAX_int32;

	TSharedRef<SDockTab> SpawnGraphTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnDetailsTab(const FSpawnTabArgs& Args);

	void CreateEditorGraph();
	void RebuildGraphFromAsset();
	void ExportGraphToAsset();
	void OnGraphChanged(const FEdGraphEditAction& Action);
	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);
	void OnSelectedNodesChanged(const TSet<UObject*>& NewSelection);
	void OnNodeDoubleClicked(UEdGraphNode* Node);
	FReply OnValidateFlowClicked();
	FReply OnAutoArrangeClicked();
	FReply OnFixNodeIdsClicked();
	void RefreshValidationResults();
	FText GetValidationSummaryText() const;
	FText GetValidationDetailsText() const;
	void DeleteSelectedNodes();
	bool CanDeleteSelectedNodes() const;

	TArray<UFlowEventEdGraphNode*> GetOrderedGraphNodes() const;
	UFlowEventEdGraphNode* CreateGraphNodeFromFlowNode(const FFlowEventNode& FlowNode, int32 SourceIndex, const FVector2D& FallbackPosition);
	void SynchronizeGraphLinksFromNodeProperties();
	void AutoArrangeNodes();
	void FixNodeIds();

	TObjectPtr<UFlowEventSequenceAsset> FlowAsset = nullptr;
	TObjectPtr<UEdGraph> EditorGraph = nullptr;
	FDelegateHandle OnGraphChangedHandle;

	TSharedPtr<SGraphEditor> GraphEditor;
	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<FUICommandList> GraphEditorCommands;
	FText ValidationSummaryText;
	FText ValidationDetailsText;
	bool bIsSynchronizing = false;
};

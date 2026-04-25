#include "Factories/FlowEventSequenceAssetFactory.h"

#include "FlowEventSequenceAsset.h"

UFlowEventSequenceAssetFactory::UFlowEventSequenceAssetFactory()
{
	SupportedClass = UFlowEventSequenceAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UFlowEventSequenceAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UFlowEventSequenceAsset* NewAsset = NewObject<UFlowEventSequenceAsset>(InParent, Class, Name, Flags);
	if (NewAsset)
	{
		FFlowEventNode StartNode;
		StartNode.NodeId = TEXT("Start");
		StartNode.EventDuration = 0.0f;
#if WITH_EDITORONLY_DATA
		StartNode.EditorNodeGuid = FGuid::NewGuid();
		StartNode.EditorPosition = FVector2D::ZeroVector;
#endif
		NewAsset->Nodes.Add(StartNode);
	}

	return NewAsset;
}

bool UFlowEventSequenceAssetFactory::ShouldShowInNewMenu() const
{
	return true;
}

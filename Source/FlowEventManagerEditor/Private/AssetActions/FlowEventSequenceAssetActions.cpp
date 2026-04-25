#include "AssetActions/FlowEventSequenceAssetActions.h"

#include "Toolkit/FlowEventSequenceAssetEditor.h"

#define LOCTEXT_NAMESPACE "FlowEventSequenceAssetActions"

FText FFlowEventSequenceAssetActions::GetName() const
{
	return LOCTEXT("FlowEventSequenceAssetName", "Flow Event Sequence");
}

FColor FFlowEventSequenceAssetActions::GetTypeColor() const
{
	return FColor(68, 160, 210);
}

UClass* FFlowEventSequenceAssetActions::GetSupportedClass() const
{
	return UFlowEventSequenceAsset::StaticClass();
}

uint32 FFlowEventSequenceAssetActions::GetCategories()
{
	return AssetCategory;
}

void FFlowEventSequenceAssetActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (UObject* Object : InObjects)
	{
		if (UFlowEventSequenceAsset* FlowAsset = Cast<UFlowEventSequenceAsset>(Object))
		{
			TSharedRef<FFlowEventSequenceAssetEditor> NewEditor = MakeShared<FFlowEventSequenceAssetEditor>();
			NewEditor->InitFlowEventSequenceAssetEditor(Mode, EditWithinLevelEditor, FlowAsset);
		}
	}
}

#undef LOCTEXT_NAMESPACE

#pragma once

#include "AssetTypeActions_Base.h"
#include "FlowEventSequenceAsset.h"

class FFlowEventSequenceAssetActions : public FAssetTypeActions_Base
{
public:
	explicit FFlowEventSequenceAssetActions(EAssetTypeCategories::Type InAssetCategory)
		: AssetCategory(InAssetCategory)
	{
	}

	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor) override;

private:
	EAssetTypeCategories::Type AssetCategory;
};

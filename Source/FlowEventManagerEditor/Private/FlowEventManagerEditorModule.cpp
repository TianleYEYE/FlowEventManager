#include "FlowEventManagerEditorModule.h"

#include "AssetActions/FlowEventSequenceAssetActions.h"
#include "AssetToolsModule.h"
#include "GraphEditorActions.h"
#include "IAssetTools.h"

#define LOCTEXT_NAMESPACE "FlowEventManagerEditor"

class FFlowEventManagerEditorModule : public IFlowEventManagerEditorModule
{
public:
	virtual void StartupModule() override
	{
		FGraphEditorCommands::Register();

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		FlowEventAssetCategory = AssetTools.RegisterAdvancedAssetCategory(TEXT("FlowEventManager"), LOCTEXT("FlowEventManagerCategory", "Flow Event Manager"));
		RegisterAssetTypeAction(AssetTools, MakeShared<FFlowEventSequenceAssetActions>(FlowEventAssetCategory));
	}

	virtual void ShutdownModule() override
	{
		if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
		{
			IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
			for (const TSharedPtr<IAssetTypeActions>& Action : RegisteredAssetTypeActions)
			{
				if (Action.IsValid())
				{
					AssetTools.UnregisterAssetTypeActions(Action.ToSharedRef());
				}
			}
		}

		RegisteredAssetTypeActions.Reset();
	}

private:
	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
	{
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}

	EAssetTypeCategories::Type FlowEventAssetCategory = EAssetTypeCategories::Misc;
	TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetTypeActions;
};

DEFINE_LOG_CATEGORY(LogFlowEventManagerEditor);

IMPLEMENT_MODULE(FFlowEventManagerEditorModule, FlowEventManagerEditor)

#undef LOCTEXT_NAMESPACE

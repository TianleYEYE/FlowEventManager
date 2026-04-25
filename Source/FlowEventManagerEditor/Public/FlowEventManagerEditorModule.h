#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFlowEventManagerEditor, Log, All);

class IFlowEventManagerEditorModule : public IModuleInterface
{
public:
	static inline IFlowEventManagerEditorModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IFlowEventManagerEditorModule>("FlowEventManagerEditor");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("FlowEventManagerEditor");
	}
};

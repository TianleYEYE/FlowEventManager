#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFlowEventManager, Log, All);

class IFlowEventManagerModule : public IModuleInterface
{
public:
	static inline IFlowEventManagerModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IFlowEventManagerModule>("FlowEventManager");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("FlowEventManager");
	}
};

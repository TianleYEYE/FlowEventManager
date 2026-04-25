#include "FlowEventManagerModule.h"

#define LOCTEXT_NAMESPACE "FlowEventManager"

class FFlowEventManagerModule : public IFlowEventManagerModule
{
public:
	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}
};

DEFINE_LOG_CATEGORY(LogFlowEventManager);

IMPLEMENT_MODULE(FFlowEventManagerModule, FlowEventManager)

#undef LOCTEXT_NAMESPACE

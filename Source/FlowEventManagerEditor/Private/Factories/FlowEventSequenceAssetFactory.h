#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "FlowEventSequenceAssetFactory.generated.h"

UCLASS()
class UFlowEventSequenceAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UFlowEventSequenceAssetFactory();

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override;
};

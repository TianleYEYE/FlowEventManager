#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "FlowEventGraphSchema.generated.h"

USTRUCT()
struct FFlowEventGraphSchemaAction_NewNode : public FEdGraphSchemaAction
{
	GENERATED_BODY()

	FFlowEventGraphSchemaAction_NewNode()
		: FEdGraphSchemaAction()
	{
	}

	FFlowEventGraphSchemaAction_NewNode(const FText& InNodeCategory, const FText& InMenuDesc, const FText& InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping)
	{
	}

	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
};

UCLASS()
class FLOWEVENTMANAGEREDITOR_API UFlowEventGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()

public:
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
	virtual bool ShouldHidePinDefaultValue(UEdGraphPin* Pin) const override;
};

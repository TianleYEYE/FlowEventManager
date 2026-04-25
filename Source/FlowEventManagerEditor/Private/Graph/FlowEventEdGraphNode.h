#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "FlowEventSequenceAsset.h"
#include "FlowEventEdGraphNode.generated.h"

UCLASS()
class FLOWEVENTMANAGEREDITOR_API UFlowEventEdGraphNode : public UEdGraphNode
{
	GENERATED_BODY()

public:
	static const FName InputPinName;
	static const FName OutputPinName;

	UPROPERTY(EditAnywhere, Category = "Flow", meta = (ShowOnlyInnerProperties))
	FFlowEventNode FlowNode;

	int32 SourceNodeIndex = INDEX_NONE;

	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual bool CanUserDeleteNode() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	UEdGraphPin* GetInputPin() const;
	UEdGraphPin* GetOutputPin() const;
	void EnsureValidNodeIdentity(int32 FallbackIndex);
};

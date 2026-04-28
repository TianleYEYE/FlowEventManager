#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FlowEventSequenceAsset.h"
#include "TimerManager.h"
#include "FlowEventManagerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFlowEventFlowStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFlowEventFlowFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnFlowEventNodeStarted, FName, NodeId, int32, NodeIndex, AActor*, TargetActor, float, EventDuration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFlowEventNodeFinished, FName, NodeId, int32, NodeIndex);

USTRUCT()
struct FFlowEventRuntimeNode
{
	GENERATED_BODY()

	UPROPERTY()
	FName NodeId = NAME_None;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> Targets;

	float ElapsedTime = 0.0f;
	FTimerHandle FinishTimerHandle;
};

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FLOWEVENTMANAGER_API UFlowEventManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlowEventManagerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Flow Event Manager")
	void StartFlow();

	UFUNCTION(BlueprintCallable, Category = "Flow Event Manager")
	void StartFlowAtIndex(int32 NodeIndex);

	UFUNCTION(BlueprintCallable, Category = "Flow Event Manager")
	void StopFlow(bool bBroadcastFinished = false);

	UFUNCTION(BlueprintCallable, Category = "Flow Event Manager")
	bool IsFlowRunning() const { return bFlowRunning; }

	UFUNCTION(BlueprintCallable, Category = "Flow Event Manager|Runtime State")
	int32 GetActiveNodeCount() const { return ActiveNodes.Num(); }

	UFUNCTION(BlueprintCallable, Category = "Flow Event Manager|Runtime State")
	TArray<int32> GetActiveNodeIndices() const;

	UFUNCTION(BlueprintCallable, Category = "Flow Event Manager|Runtime State")
	TArray<FName> GetActiveNodeIds() const;

	UFUNCTION(BlueprintCallable, Category = "Flow Event Manager|Runtime State")
	bool GetActiveNodeElapsedTime(FName NodeId, float& OutElapsedTime) const;

	UFUNCTION(BlueprintCallable, Category = "Flow Event Manager|Runtime State")
	bool GetActiveNodeProgress(FName NodeId, float& OutProgress) const;

	UFUNCTION(BlueprintCallable, Category = "Flow Event Manager|Runtime State")
	int32 GetPendingStartCount() const { return PendingStartCount; }

	UFUNCTION(BlueprintCallable, Category = "Flow Event Manager|Validation")
	void ValidateConfiguredFlow(TArray<FFlowEventValidationIssue>& OutIssues) const;

	UFUNCTION(BlueprintCallable, Category = "Flow Event Manager")
	void FinishNodeEarly(FName NodeId);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flow Event Manager")
	TObjectPtr<UFlowEventSequenceAsset> FlowAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flow Event Manager")
	bool bUseInlineNodes = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flow Event Manager", meta = (EditCondition = "bUseInlineNodes", EditConditionHides))
	TArray<FFlowEventNode> InlineNodes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flow Event Manager")
	bool bAutoStart = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flow Event Manager", meta = (ClampMin = "0"))
	int32 StartNodeIndex = 0;

	UPROPERTY(BlueprintAssignable, Category = "Flow Event Manager")
	FOnFlowEventFlowStarted OnFlowStarted;

	UPROPERTY(BlueprintAssignable, Category = "Flow Event Manager")
	FOnFlowEventFlowFinished OnFlowFinished;

	UPROPERTY(BlueprintAssignable, Category = "Flow Event Manager")
	FOnFlowEventNodeStarted OnNodeStarted;

	UPROPERTY(BlueprintAssignable, Category = "Flow Event Manager")
	FOnFlowEventNodeFinished OnNodeFinished;

private:
	const TArray<FFlowEventNode>* GetConfiguredNodes() const;
	void StartNodeInternal(int32 NodeIndex);
	void FinishNodeInternal(int32 NodeIndex);
	void ScheduleNextNode(const FFlowEventNode& Node, int32 CurrentNodeIndex);
	void StartNextNode(const FFlowEventNode& Node, int32 CurrentNodeIndex);
	void CheckFlowFinished();

	int32 ResolveNextNodeIndex(const FFlowEventNode& Node, int32 CurrentNodeIndex) const;
	TArray<int32> ResolveNextNodeIndices(const FFlowEventNode& Node, int32 CurrentNodeIndex) const;
	void ResolveTargets(const FFlowEventNode& Node, TArray<AActor*>& OutTargets) const;
	bool ExecuteEventOnTarget(AActor* Target, const FFlowEventNode& Node, float OutputValue, float ElapsedTime) const;
	float EvaluateTimelineValue(const FFlowEventNode& Node, float ElapsedTime) const;

	UPROPERTY()
	TMap<int32, FFlowEventRuntimeNode> ActiveNodes;

	TArray<FTimerHandle> PendingStartTimerHandles;

	bool bFlowRunning = false;
	int32 ActiveRunId = 0;
	int32 PendingStartCount = 0;
};

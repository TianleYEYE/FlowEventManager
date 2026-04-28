#include "FlowEventManagerComponent.h"

#include "FlowEventManagerModule.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

UFlowEventManagerComponent::UFlowEventManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UFlowEventManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStart)
	{
		StartFlowAtIndex(StartNodeIndex);
	}
}

void UFlowEventManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopFlow(false);
	Super::EndPlay(EndPlayReason);
}

void UFlowEventManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bFlowRunning)
	{
		SetComponentTickEnabled(false);
		return;
	}

	const TArray<FFlowEventNode>* Nodes = GetConfiguredNodes();
	if (!Nodes)
	{
		return;
	}

	for (TPair<int32, FFlowEventRuntimeNode>& ActiveNodePair : ActiveNodes)
	{
		if (!Nodes->IsValidIndex(ActiveNodePair.Key))
		{
			continue;
		}

		const FFlowEventNode& Node = (*Nodes)[ActiveNodePair.Key];
		if (Node.NodeType == EFlowEventNodeType::Delay || !Node.bUseTimelineCurve)
		{
			continue;
		}

		FFlowEventRuntimeNode& RuntimeNode = ActiveNodePair.Value;
		RuntimeNode.ElapsedTime = FMath::Min(RuntimeNode.ElapsedTime + DeltaTime, FMath::Max(0.0f, Node.EventDuration));
		const float OutputValue = EvaluateTimelineValue(Node, RuntimeNode.ElapsedTime);

		for (TObjectPtr<AActor>& Target : RuntimeNode.Targets)
		{
			ExecuteEventOnTarget(Target.Get(), Node, OutputValue, RuntimeNode.ElapsedTime);
		}
	}
}

void UFlowEventManagerComponent::StartFlow()
{
	StartFlowAtIndex(StartNodeIndex);
}

void UFlowEventManagerComponent::StartFlowAtIndex(int32 NodeIndex)
{
	StopFlow(false);

	const TArray<FFlowEventNode>* Nodes = GetConfiguredNodes();
	if (!Nodes || !Nodes->IsValidIndex(NodeIndex))
	{
		UE_LOG(LogFlowEventManager, Warning, TEXT("StartFlowAtIndex failed: node index %d is invalid."), NodeIndex);
		return;
	}

	bFlowRunning = true;
	++ActiveRunId;
	PendingStartCount = 0;
	SetComponentTickEnabled(true);
	OnFlowStarted.Broadcast();
	StartNodeInternal(NodeIndex);
	CheckFlowFinished();
}

void UFlowEventManagerComponent::StopFlow(bool bBroadcastFinished)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		for (TPair<int32, FFlowEventRuntimeNode>& ActiveNode : ActiveNodes)
		{
			TimerManager.ClearTimer(ActiveNode.Value.FinishTimerHandle);
		}

		for (FTimerHandle& TimerHandle : PendingStartTimerHandles)
		{
			TimerManager.ClearTimer(TimerHandle);
		}
	}

	ActiveNodes.Reset();
	PendingStartTimerHandles.Reset();
	PendingStartCount = 0;

	const bool bWasRunning = bFlowRunning;
	bFlowRunning = false;
	++ActiveRunId;
	SetComponentTickEnabled(false);

	if (bWasRunning && bBroadcastFinished)
	{
		OnFlowFinished.Broadcast();
	}
}

TArray<int32> UFlowEventManagerComponent::GetActiveNodeIndices() const
{
	TArray<int32> NodeIndices;
	ActiveNodes.GetKeys(NodeIndices);
	NodeIndices.Sort();
	return NodeIndices;
}

TArray<FName> UFlowEventManagerComponent::GetActiveNodeIds() const
{
	TArray<FName> NodeIds;
	NodeIds.Reserve(ActiveNodes.Num());

	for (const TPair<int32, FFlowEventRuntimeNode>& ActiveNode : ActiveNodes)
	{
		NodeIds.Add(ActiveNode.Value.NodeId);
	}

	return NodeIds;
}

bool UFlowEventManagerComponent::GetActiveNodeElapsedTime(FName NodeId, float& OutElapsedTime) const
{
	for (const TPair<int32, FFlowEventRuntimeNode>& ActiveNode : ActiveNodes)
	{
		if (ActiveNode.Value.NodeId == NodeId)
		{
			OutElapsedTime = ActiveNode.Value.ElapsedTime;
			return true;
		}
	}

	OutElapsedTime = 0.0f;
	return false;
}

bool UFlowEventManagerComponent::GetActiveNodeProgress(FName NodeId, float& OutProgress) const
{
	const TArray<FFlowEventNode>* Nodes = GetConfiguredNodes();
	if (!Nodes)
	{
		OutProgress = 0.0f;
		return false;
	}

	for (const TPair<int32, FFlowEventRuntimeNode>& ActiveNode : ActiveNodes)
	{
		if (ActiveNode.Value.NodeId != NodeId || !Nodes->IsValidIndex(ActiveNode.Key))
		{
			continue;
		}

		const float Duration = FMath::Max(0.0f, (*Nodes)[ActiveNode.Key].EventDuration);
		OutProgress = Duration > 0.0f ? FMath::Clamp(ActiveNode.Value.ElapsedTime / Duration, 0.0f, 1.0f) : 1.0f;
		return true;
	}

	OutProgress = 0.0f;
	return false;
}

void UFlowEventManagerComponent::ValidateConfiguredFlow(TArray<FFlowEventValidationIssue>& OutIssues) const
{
	const TArray<FFlowEventNode>* Nodes = GetConfiguredNodes();
	if (!Nodes)
	{
		OutIssues.Reset();
		FFlowEventValidationIssue Issue;
		Issue.Severity = EFlowEventValidationSeverity::Error;
		Issue.NodeIndex = INDEX_NONE;
		Issue.Message = NSLOCTEXT("FlowEventManagerComponent", "NoConfiguredNodes", "No flow nodes are configured. Enable InlineNodes or assign a FlowAsset.");
		OutIssues.Add(Issue);
		return;
	}

	UFlowEventSequenceAsset::ValidateNodes(*Nodes, OutIssues);
}

void UFlowEventManagerComponent::FinishNodeEarly(FName NodeId)
{
	int32 NodeIndexToFinish = INDEX_NONE;
	for (const TPair<int32, FFlowEventRuntimeNode>& ActiveNode : ActiveNodes)
	{
		if (ActiveNode.Value.NodeId == NodeId)
		{
			NodeIndexToFinish = ActiveNode.Key;
			break;
		}
	}

	if (NodeIndexToFinish != INDEX_NONE)
	{
		FinishNodeInternal(NodeIndexToFinish);
	}
}

const TArray<FFlowEventNode>* UFlowEventManagerComponent::GetConfiguredNodes() const
{
	if (bUseInlineNodes)
	{
		return &InlineNodes;
	}

	return FlowAsset ? &FlowAsset->Nodes : nullptr;
}

void UFlowEventManagerComponent::StartNodeInternal(int32 NodeIndex)
{
	if (!bFlowRunning)
	{
		return;
	}

	const TArray<FFlowEventNode>* Nodes = GetConfiguredNodes();
	if (!Nodes || !Nodes->IsValidIndex(NodeIndex))
	{
		UE_LOG(LogFlowEventManager, Warning, TEXT("StartNodeInternal failed: node index %d is invalid."), NodeIndex);
		return;
	}

	if (ActiveNodes.Contains(NodeIndex))
	{
		UE_LOG(LogFlowEventManager, Warning, TEXT("Node index %d is already active. The duplicate start was skipped."), NodeIndex);
		return;
	}

	const FFlowEventNode& Node = (*Nodes)[NodeIndex];

	FFlowEventRuntimeNode RuntimeNode;
	RuntimeNode.NodeId = Node.NodeId;
	ActiveNodes.Add(NodeIndex, RuntimeNode);

	if (Node.NodeType == EFlowEventNodeType::Delay)
	{
		OnNodeStarted.Broadcast(Node.NodeId, NodeIndex, nullptr, Node.EventDuration);
		ScheduleNextNode(Node, NodeIndex);
	}
	else
	{
		TArray<AActor*> Targets;
		ResolveTargets(Node, Targets);

		if (Targets.Num() == 0)
		{
			UE_LOG(LogFlowEventManager, Warning, TEXT("Node '%s' has no resolved target actors."), *Node.NodeId.ToString());
			OnNodeStarted.Broadcast(Node.NodeId, NodeIndex, nullptr, Node.EventDuration);
		}

		for (AActor* Target : Targets)
		{
			OnNodeStarted.Broadcast(Node.NodeId, NodeIndex, Target, Node.EventDuration);
			ExecuteEventOnTarget(Target, Node, Node.bUseTimelineCurve ? EvaluateTimelineValue(Node, 0.0f) : Node.EventDuration, 0.0f);
			RuntimeNode.Targets.Add(Target);
		}

		if (FFlowEventRuntimeNode* RuntimeNodePtr = ActiveNodes.Find(NodeIndex))
		{
			RuntimeNodePtr->Targets = RuntimeNode.Targets;
			RuntimeNodePtr->ElapsedTime = 0.0f;
		}

		ScheduleNextNode(Node, NodeIndex);
	}

	if (Node.EventDuration <= 0.0f)
	{
		FinishNodeInternal(NodeIndex);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		FinishNodeInternal(NodeIndex);
		return;
	}

	FFlowEventRuntimeNode* RuntimeNodePtr = ActiveNodes.Find(NodeIndex);
	if (!RuntimeNodePtr)
	{
		return;
	}

	const int32 CapturedRunId = ActiveRunId;
	World->GetTimerManager().SetTimer(
		RuntimeNodePtr->FinishTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, CapturedRunId, NodeIndex]()
		{
			if (bFlowRunning && ActiveRunId == CapturedRunId)
			{
				FinishNodeInternal(NodeIndex);
			}
		}),
		Node.EventDuration,
		false);
}

void UFlowEventManagerComponent::FinishNodeInternal(int32 NodeIndex)
{
	FFlowEventRuntimeNode RuntimeNode;
	if (!ActiveNodes.RemoveAndCopyValue(NodeIndex, RuntimeNode))
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RuntimeNode.FinishTimerHandle);
	}

	OnNodeFinished.Broadcast(RuntimeNode.NodeId, NodeIndex);

	const TArray<FFlowEventNode>* Nodes = GetConfiguredNodes();
	if (Nodes && Nodes->IsValidIndex(NodeIndex))
	{
		const FFlowEventNode& Node = (*Nodes)[NodeIndex];
		if (Node.NextMode == EFlowEventNextMode::Serial)
		{
			StartNextNode(Node, NodeIndex);
		}
	}

	CheckFlowFinished();
}

void UFlowEventManagerComponent::ScheduleNextNode(const FFlowEventNode& Node, int32 CurrentNodeIndex)
{
	if (Node.NextMode != EFlowEventNextMode::Parallel)
	{
		return;
	}

	const TArray<FFlowEventNode>* Nodes = GetConfiguredNodes();
	if (!Nodes)
	{
		return;
	}

	const TArray<int32> NextNodeIndices = ResolveNextNodeIndices(Node, CurrentNodeIndex);
	TArray<int32> ValidNextNodeIndices;
	for (int32 NextNodeIndex : NextNodeIndices)
	{
		if (Nodes->IsValidIndex(NextNodeIndex))
		{
			ValidNextNodeIndices.Add(NextNodeIndex);
		}
	}

	if (ValidNextNodeIndices.Num() == 0)
	{
		return;
	}

	const float Delay = FMath::Max(0.0f, Node.ParallelStartDelay);
	PendingStartCount += ValidNextNodeIndices.Num();

	if (Delay <= 0.0f)
	{
		for (int32 NextNodeIndex : ValidNextNodeIndices)
		{
			PendingStartCount = FMath::Max(0, PendingStartCount - 1);
			StartNodeInternal(NextNodeIndex);
		}
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		for (int32 NextNodeIndex : ValidNextNodeIndices)
		{
			PendingStartCount = FMath::Max(0, PendingStartCount - 1);
			StartNodeInternal(NextNodeIndex);
		}
		return;
	}

	for (int32 NextNodeIndex : ValidNextNodeIndices)
	{
		PendingStartTimerHandles.Add(FTimerHandle());
		FTimerHandle& TimerHandle = PendingStartTimerHandles.Last();
		const int32 CapturedRunId = ActiveRunId;

		World->GetTimerManager().SetTimer(
			TimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this, CapturedRunId, NextNodeIndex]()
			{
				if (!bFlowRunning || ActiveRunId != CapturedRunId)
				{
					return;
				}

				PendingStartCount = FMath::Max(0, PendingStartCount - 1);
				StartNodeInternal(NextNodeIndex);
				CheckFlowFinished();
			}),
			Delay,
			false);
	}
}

void UFlowEventManagerComponent::StartNextNode(const FFlowEventNode& Node, int32 CurrentNodeIndex)
{
	const int32 NextNodeIndex = ResolveNextNodeIndex(Node, CurrentNodeIndex);
	const TArray<FFlowEventNode>* Nodes = GetConfiguredNodes();
	if (!Nodes || !Nodes->IsValidIndex(NextNodeIndex))
	{
		return;
	}

	StartNodeInternal(NextNodeIndex);
}

void UFlowEventManagerComponent::CheckFlowFinished()
{
	if (bFlowRunning && ActiveNodes.Num() == 0 && PendingStartCount == 0)
	{
		bFlowRunning = false;
		SetComponentTickEnabled(false);
		OnFlowFinished.Broadcast();
	}
}

int32 UFlowEventManagerComponent::ResolveNextNodeIndex(const FFlowEventNode& Node, int32 CurrentNodeIndex) const
{
	const TArray<int32> NextNodeIndices = ResolveNextNodeIndices(Node, CurrentNodeIndex);
	return NextNodeIndices.Num() > 0 ? NextNodeIndices[0] : INDEX_NONE;
}

TArray<int32> UFlowEventManagerComponent::ResolveNextNodeIndices(const FFlowEventNode& Node, int32 CurrentNodeIndex) const
{
	TArray<int32> NextNodeIndices;
	if (Node.NextMode == EFlowEventNextMode::Parallel && Node.ParallelNextNodeIndices.Num() > 0)
	{
		for (int32 NextNodeIndex : Node.ParallelNextNodeIndices)
		{
			NextNodeIndices.AddUnique(NextNodeIndex);
		}

		return NextNodeIndices;
	}

	if (Node.NextNodeIndex != INDEX_NONE)
	{
		if (Node.NextNodeIndex == MAX_int32)
		{
			return NextNodeIndices;
		}

		NextNodeIndices.Add(Node.NextNodeIndex);
		return NextNodeIndices;
	}

	NextNodeIndices.Add(CurrentNodeIndex + 1);
	return NextNodeIndices;
}

void UFlowEventManagerComponent::ResolveTargets(const FFlowEventNode& Node, TArray<AActor*>& OutTargets) const
{
	OutTargets.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	switch (Node.TargetMode)
	{
	case EFlowEventTargetMode::ExplicitActor:
		if (Node.TargetActor)
		{
			OutTargets.Add(Node.TargetActor);
		}
		break;

	case EFlowEventTargetMode::FirstActorWithTag:
	case EFlowEventTargetMode::AllActorsWithTag:
		UGameplayStatics::GetAllActorsWithTag(World, Node.TargetTag, OutTargets);
		if (Node.TargetMode == EFlowEventTargetMode::FirstActorWithTag && OutTargets.Num() > 1)
		{
			OutTargets.SetNum(1);
		}
		break;

	case EFlowEventTargetMode::FirstActorOfClass:
	case EFlowEventTargetMode::AllActorsOfClass:
		if (Node.TargetClass)
		{
			UGameplayStatics::GetAllActorsOfClass(World, Node.TargetClass, OutTargets);
			if (Node.TargetMode == EFlowEventTargetMode::FirstActorOfClass && OutTargets.Num() > 1)
			{
				OutTargets.SetNum(1);
			}
		}
		break;

	default:
		break;
	}
}

bool UFlowEventManagerComponent::ExecuteEventOnTarget(AActor* Target, const FFlowEventNode& Node, float OutputValue, float ElapsedTime) const
{
	if (!Target || Node.EventName.IsNone())
	{
		return false;
	}

	UFunction* Function = Target->FindFunction(Node.EventName);
	if (!Function)
	{
		UE_LOG(LogFlowEventManager, Warning, TEXT("Actor '%s' does not have event/function '%s'."), *Target->GetName(), *Node.EventName.ToString());
		return false;
	}

	uint8* Params = nullptr;
	if (Function->ParmsSize > 0)
	{
		Params = static_cast<uint8*>(FMemory_Alloca(Function->ParmsSize));
		FMemory::Memzero(Params, Function->ParmsSize);

		int32 NumericParamIndex = 0;
		for (TFieldIterator<FProperty> It(Function); It; ++It)
		{
			FProperty* Property = *It;
			if (!Property->HasAnyPropertyFlags(CPF_Parm) || Property->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				continue;
			}

			void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Params);
			if (NumericParamIndex >= 2)
			{
				UE_LOG(LogFlowEventManager, Warning, TEXT("Event/function '%s' on '%s' has too many parameters. Use no parameters, one float/double output value parameter, or two float/double parameters for output value and elapsed time."), *Node.EventName.ToString(), *Target->GetName());
				return false;
			}

			const float NumericValue = NumericParamIndex == 0 ? OutputValue : ElapsedTime;
			if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
			{
				FloatProperty->SetPropertyValue(ValuePtr, NumericValue);
			}
			else if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
			{
				DoubleProperty->SetPropertyValue(ValuePtr, static_cast<double>(NumericValue));
			}
			else
			{
				UE_LOG(LogFlowEventManager, Warning, TEXT("Event/function '%s' on '%s' has an unsupported parameter '%s'. Use no parameters, one float/double output value parameter, or two float/double parameters for output value and elapsed time."), *Node.EventName.ToString(), *Target->GetName(), *Property->GetName());
				return false;
			}

			++NumericParamIndex;
		}
	}

	Target->ProcessEvent(Function, Params);
	return true;
}

float UFlowEventManagerComponent::EvaluateTimelineValue(const FFlowEventNode& Node, float ElapsedTime) const
{
	const FRichCurve* RichCurve = Node.TimelineCurve.GetRichCurveConst();
	if (!RichCurve || RichCurve->IsEmpty())
	{
		return ElapsedTime;
	}

	return RichCurve->Eval(ElapsedTime);
}

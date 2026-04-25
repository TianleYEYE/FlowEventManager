#include "FlowEventManagerComponent.h"

#include "FlowEventManagerModule.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

UFlowEventManagerComponent::UFlowEventManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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

	if (bWasRunning && bBroadcastFinished)
	{
		OnFlowFinished.Broadcast();
	}
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
		ExecuteEventOnTarget(Target, Node);
	}

	ScheduleNextNode(Node, NodeIndex);

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

	const int32 NextNodeIndex = ResolveNextNodeIndex(Node, CurrentNodeIndex);
	const TArray<FFlowEventNode>* Nodes = GetConfiguredNodes();
	if (!Nodes || !Nodes->IsValidIndex(NextNodeIndex))
	{
		return;
	}

	const float Delay = FMath::Max(0.0f, Node.ParallelStartDelay);
	++PendingStartCount;

	if (Delay <= 0.0f)
	{
		PendingStartCount = FMath::Max(0, PendingStartCount - 1);
		StartNodeInternal(NextNodeIndex);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		PendingStartCount = FMath::Max(0, PendingStartCount - 1);
		StartNodeInternal(NextNodeIndex);
		return;
	}

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
		OnFlowFinished.Broadcast();
	}
}

int32 UFlowEventManagerComponent::ResolveNextNodeIndex(const FFlowEventNode& Node, int32 CurrentNodeIndex) const
{
	if (Node.NextNodeIndex != INDEX_NONE)
	{
		return Node.NextNodeIndex;
	}

	return CurrentNodeIndex + 1;
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

bool UFlowEventManagerComponent::ExecuteEventOnTarget(AActor* Target, const FFlowEventNode& Node) const
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

		for (TFieldIterator<FProperty> It(Function); It; ++It)
		{
			FProperty* Property = *It;
			if (!Property->HasAnyPropertyFlags(CPF_Parm) || Property->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				continue;
			}

			void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Params);
			if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
			{
				FloatProperty->SetPropertyValue(ValuePtr, Node.EventDuration);
			}
			else if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
			{
				DoubleProperty->SetPropertyValue(ValuePtr, static_cast<double>(Node.EventDuration));
			}
			else
			{
				UE_LOG(LogFlowEventManager, Warning, TEXT("Event/function '%s' on '%s' has an unsupported parameter '%s'. Use no parameter or one float/double duration parameter."), *Node.EventName.ToString(), *Target->GetName(), *Property->GetName());
				return false;
			}
		}
	}

	Target->ProcessEvent(Function, Params);
	return true;
}

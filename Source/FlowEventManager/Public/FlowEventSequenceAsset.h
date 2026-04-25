#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "FlowEventSequenceAsset.generated.h"

UENUM(BlueprintType)
enum class EFlowEventTargetMode : uint8
{
	ExplicitActor UMETA(DisplayName = "Explicit Actor"),
	FirstActorWithTag UMETA(DisplayName = "First Actor With Tag"),
	AllActorsWithTag UMETA(DisplayName = "All Actors With Tag"),
	FirstActorOfClass UMETA(DisplayName = "First Actor Of Class"),
	AllActorsOfClass UMETA(DisplayName = "All Actors Of Class")
};

UENUM(BlueprintType)
enum class EFlowEventNextMode : uint8
{
	Serial UMETA(DisplayName = "Serial"),
	Parallel UMETA(DisplayName = "Parallel")
};

UENUM(BlueprintType)
enum class EFlowEventValidationSeverity : uint8
{
	Info UMETA(DisplayName = "Info"),
	Warning UMETA(DisplayName = "Warning"),
	Error UMETA(DisplayName = "Error")
};

USTRUCT(BlueprintType)
struct FLOWEVENTMANAGER_API FFlowEventValidationIssue
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Flow Validation")
	EFlowEventValidationSeverity Severity = EFlowEventValidationSeverity::Info;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Validation")
	int32 NodeIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Validation")
	FName NodeId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Flow Validation")
	FText Message;
};

USTRUCT(BlueprintType)
struct FLOWEVENTMANAGER_API FFlowEventNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
	FName NodeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
	EFlowEventTargetMode TargetMode = EFlowEventTargetMode::ExplicitActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow", meta = (EditCondition = "TargetMode == EFlowEventTargetMode::ExplicitActor", EditConditionHides))
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow", meta = (EditCondition = "TargetMode == EFlowEventTargetMode::FirstActorWithTag || TargetMode == EFlowEventTargetMode::AllActorsWithTag", EditConditionHides))
	FName TargetTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow", meta = (EditCondition = "TargetMode == EFlowEventTargetMode::FirstActorOfClass || TargetMode == EFlowEventTargetMode::AllActorsOfClass", EditConditionHides))
	TSubclassOf<AActor> TargetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
	FName EventName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float EventDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Timeline")
	bool bUseTimelineCurve = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Timeline", meta = (EditCondition = "bUseTimelineCurve", EditConditionHides))
	FRuntimeFloatCurve TimelineCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
	EFlowEventNextMode NextMode = EFlowEventNextMode::Serial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "NextMode == EFlowEventNextMode::Parallel", EditConditionHides))
	float ParallelStartDelay = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow", meta = (ClampMin = "-1"))
	int32 NextNodeIndex = INDEX_NONE;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	FGuid EditorNodeGuid;

	UPROPERTY()
	FVector2D EditorPosition = FVector2D::ZeroVector;
#endif
};

UCLASS(BlueprintType)
class FLOWEVENTMANAGER_API UFlowEventSequenceAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flow")
	TArray<FFlowEventNode> Nodes;

	UFUNCTION(BlueprintCallable, Category = "Flow Validation")
	void ValidateFlow(TArray<FFlowEventValidationIssue>& OutIssues) const;

	UFUNCTION(BlueprintCallable, Category = "Flow Validation")
	static void ValidateNodes(const TArray<FFlowEventNode>& InNodes, TArray<FFlowEventValidationIssue>& OutIssues);

	UFUNCTION(BlueprintCallable, Category = "Flow Validation")
	bool HasValidationErrors() const;
};

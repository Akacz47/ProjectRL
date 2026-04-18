// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_SendGetAnswearPPO.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTRL_API UBTT_SendGetAnswearPPO : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_SendGetAnswearPPO();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// Aby task by³ asynchroniczny
	virtual void OnGameplayTaskActivated(UGameplayTask& Task) override {}

	//void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult);

	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	FTimerHandle TimerHandle_CheckUDP;
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;
};

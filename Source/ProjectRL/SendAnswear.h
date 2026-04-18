// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "SendAnswear.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTRL_API USendAnswear : public UBTTaskNode
{
	GENERATED_BODY()
public:
	USendAnswear();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// Aby task by³ asynchroniczny
	virtual void OnGameplayTaskActivated(UGameplayTask& Task) override {}
private:
	FTimerHandle TimerHandle_CheckUDP;
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;
};

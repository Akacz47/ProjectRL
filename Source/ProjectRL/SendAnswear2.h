// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Bullet.h"
#include "AIC_Turret.h"
#include "SendAnswear2.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTRL_API USendAnswear2 : public UBTTaskNode
{
	GENERATED_BODY()
public:
	USendAnswear2();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// Aby task by³ asynchroniczny
	virtual void OnGameplayTaskActivated(UGameplayTask& Task) override {}

	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Config")
	TSubclassOf<ABullet> BulletClass;
private:
	FTimerHandle TimerHandle_CheckUDP;
	TWeakObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;
};

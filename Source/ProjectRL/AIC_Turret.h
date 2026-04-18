// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AIC_Turret.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTRL_API AAIC_Turret : public AAIController
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Blackboard")
	void SetBulletDistance(float Distance);
	void SetShoudShootCpp(bool val);

};

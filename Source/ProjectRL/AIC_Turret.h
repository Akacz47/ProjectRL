// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "UDPSubsystem.h"
#include "Components/ArrowComponent.h"
#include "AIC_Turret.generated.h"

class ABullet;

UCLASS()
class PROJECTRL_API AAIC_Turret : public AAIController
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Blackboard")
	void SetBulletDistance(float Distance);

	void SendAnswear();
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SI")
	AActor* AttackTarget = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TSubclassOf<ABullet> BulletClass;
	
	float ShootCooldown = 0.0f;
	float bulletDistance = -1.0f;
	
private:
	bool training = false;

	float GetAngleDeltaToTarget(const FVector& TurretLocation, const FVector& TurretForward, const FVector& TargetLocation);
	float GetPitchDelta(const FVector& TurretLocation, const FVector& TurretForward, const FVector& TargetLocation);
};

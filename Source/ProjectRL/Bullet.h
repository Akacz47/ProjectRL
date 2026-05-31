// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AIC_Turret.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Bullet.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBulletDestroyed);

UCLASS()
class PROJECTRL_API ABullet : public APawn
{
	GENERATED_BODY()

	void UpdateUi(bool UpdateSiCounter);

public:
	// Sets default values for this pawn's properties
	ABullet();

	UPROPERTY(BlueprintAssignable)
	FOnBulletDestroyed OnBulletDestroyed;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	virtual void Tick(float DeltaTime) override;

public:	

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY()
	AActor* EnemyActor = nullptr;

	UPROPERTY()
	AAIC_Turret* TowerAIController = nullptr;

	// Setter do ustawiania referencji przy spawn
	UFUNCTION(BlueprintCallable, Category = "Bullet")
	void InitBullet(AActor* InEnemyActor, AAIC_Turret* InAIController = nullptr);


};

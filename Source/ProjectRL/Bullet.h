// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AIC_Turret.h"
#include "Bullet.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBulletDestroyed);

UCLASS()
class PROJECTRL_API ABullet : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ABullet();

	UPROPERTY(BlueprintAssignable)
	FOnBulletDestroyed OnBulletDestroyed;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//UFUNCTION()
	//void OnActorOverlap(AActor* OverlappedActor, AActor* OtherActor);

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	virtual void Tick(float DeltaTime) override;

public:	

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Referencja do przeciwnika, opcjonalna */
	UPROPERTY()
	AActor* EnemyActor = nullptr;

	/** Referencja do AI Controller wie¿yczki, opcjonalna */
	UPROPERTY()
	AAIC_Turret* TowerAIController = nullptr;

	/** Setter do ustawiania referencji przy spawn */
	UFUNCTION(BlueprintCallable, Category = "Bullet")
	void InitBullet(AActor* InEnemyActor, AAIC_Turret* InAIController = nullptr);


};

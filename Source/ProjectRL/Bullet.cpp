// Fill out your copyright notice in the Description page of Project Settings.


#include "Bullet.h"

// Sets default values
ABullet::ABullet()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ABullet::BeginPlay()
{
	Super::BeginPlay();
	//this->OnActorBeginOverlap.AddDynamic(this, &ABullet::OnActorOverlap);
}

void ABullet::InitBullet(AActor* InEnemyActor, AAIC_Turret* InAIController)
{
	EnemyActor = InEnemyActor;
	TowerAIController = InAIController;
}

void ABullet::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	//if (GEngine)
	//{
	//	FString msg = FString::Printf(TEXT("Overlap"));
	//	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, msg);
	//}

    if (EnemyActor && TowerAIController) { // Dla SI
		if (OtherActor == EnemyActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("Bullet hit Enemy."));
			TowerAIController->SetBulletDistance(0.0f);
			EnemyActor->Destroy();
		}
		else {
			float Distance = FVector::Distance(GetActorLocation(), EnemyActor->GetActorLocation());
			TowerAIController->SetBulletDistance(Distance);
		}
    }
	else { // Dla gracza
		if (OtherActor->ActorHasTag("Enemy")) {
			UE_LOG(LogTemp, Warning, TEXT("Bullet hit Enemy."));
			OtherActor->Destroy();
		}
	}

	// Test
	//if (EnemyActor) {
	//	if (OtherActor == EnemyActor)
	//	{
	//		UE_LOG(LogTemp, Warning, TEXT("Bullet hit Enemy"));
	//	}

	//	float Distance = FVector::Distance(GetActorLocation(), EnemyActor->GetActorLocation());
	//	if (GEngine)
	//	{
	//		FString msg = FString::Printf(TEXT("Distance = %f"), Distance);
	//		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, msg);
	//	}

	//}
	OnBulletDestroyed.Broadcast();
    Destroy();
}

// Called to bind functionality to input
void ABullet::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ABullet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsValid(this))
		return;

	FVector Loc = GetActorLocation();

	if (Loc.Z < -150 || Loc.X < -2200 || Loc.X > 2300 || Loc.Y > 2300)
	{
		if (TowerAIController) {
			float Distance = FVector::Distance(GetActorLocation(), EnemyActor->GetActorLocation());
			TowerAIController->SetBulletDistance(Distance);
		}
		Destroy();
	}
}

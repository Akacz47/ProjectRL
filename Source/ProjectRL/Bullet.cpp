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
}

void ABullet::InitBullet(AActor* InEnemyActor, AAIC_Turret* InAIController)
{
	EnemyActor = InEnemyActor;
	TowerAIController = InAIController;
}

void ABullet::UpdateUi(bool UpdateSiCounter)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	UFunction* Func = PC->FindFunction(FName("UpdateWidgetCounter"));
	struct FFunctionParams
	{
		bool bValue;
	};

	FFunctionParams Params;
	Params.bValue = UpdateSiCounter;
	PC->ProcessEvent(Func, &Params);
}

void ABullet::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

    if (EnemyActor && TowerAIController) { // Dla AI
		if (OtherActor == EnemyActor)
		{
			TowerAIController->SetBulletDistance(0.0f);
			EnemyActor->Destroy();
			UpdateUi(true);
		}
		else {
			float Distance = FVector::Distance(GetActorLocation(), EnemyActor->GetActorLocation());
			TowerAIController->SetBulletDistance(Distance);
		}
    }
	else { // Dla gracza
		if (OtherActor->ActorHasTag("Enemy")) {
			OtherActor->Destroy();
			UpdateUi(false);
		}
	}

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

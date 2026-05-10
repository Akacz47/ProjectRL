// Fill out your copyright notice in the Description page of Project Settings.


#include "SendAnswear.h"
#include "UDPSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

USendAnswear::USendAnswear()
{
	bNotifyTick = true; // Wymagane aby dzialal TickTask
	bCreateNodeInstance = true; // Bo przechowujemy stan (flagi, dane)
}

float GetAngleDeltaToTarget(const FVector& TurretLocation, const FVector& TurretForward, const FVector& TargetLocation)
{
    // wektor do celu
    FVector ToTarget = (TargetLocation - TurretLocation).GetSafeNormal();

    // k¹t miêdzy forward a celem
    float Dot = FVector::DotProduct(TurretForward, ToTarget);
    Dot = FMath::Clamp(Dot, -1.0f, 1.0f);

    // k¹t w stopniach
    float Angle = FMath::Acos(Dot);

    // znak kierunku (left/right)
    FVector Cross = FVector::CrossProduct(TurretForward, ToTarget);
    float Sign = FMath::Sign(Cross.Z);   // zak³adamy obrót w osi Z

    float DeltaTheta = Angle * Sign; // podpisany k¹t
    return DeltaTheta; // radiany
}

float GetPitchDelta(const FVector& TurretLocation, const FVector& TurretForward, const FVector& TargetLocation)
{
    FVector ToTarget = (TargetLocation - TurretLocation).GetSafeNormal();

    // Projekcja forward na p³aszczyznê YZ (pitch)
    FVector ForwardPitch = FVector(0, TurretForward.Y, TurretForward.Z).GetSafeNormal();
    FVector TargetPitch = FVector(0, ToTarget.Y, ToTarget.Z).GetSafeNormal();

    float Dot = FVector::DotProduct(ForwardPitch, TargetPitch);
    Dot = FMath::Clamp(Dot, -1.f, 1.f);

    float Angle = FMath::Acos(Dot);

    float Sign = FMath::Sign(ToTarget.Z - TurretForward.Z);
    return Angle * Sign; // radiany (+ w górê, – w dó³)
}



EBTNodeResult::Type USendAnswear::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    CachedOwnerComp = &OwnerComp;

    UWorld* World = GetWorld();

    UGameInstance* GameInstance = World->GetGameInstance();
    UUDPSubsystem* UDPSubsystem = GameInstance ? GameInstance->GetSubsystem<UUDPSubsystem>() : nullptr;
    FTimerDelegate TimerDelegate;
    TimerDelegate.BindLambda([this, World, UDPSubsystem]()
        {
            UBlackboardComponent* BB = CachedOwnerComp->GetBlackboardComponent();
            const FName KeyName = TEXT("ShouldShoot");
            bool shouldShoot = BB->GetValueAsBool(KeyName);
            if (shouldShoot) {
                World->GetTimerManager().ClearTimer(TimerHandle_CheckUDP);
                //UBlackboardComponent* BB = CachedOwnerComp.GetBlackboardComponent();

                AAIController* AI = CachedOwnerComp->GetAIOwner();
                if (!AI) FinishLatentTask(*CachedOwnerComp.Get(), EBTNodeResult::Failed);

                APawn* Pawn = AI->GetPawn();
                if (!Pawn) FinishLatentTask(*CachedOwnerComp.Get(), EBTNodeResult::Failed);

                FRotator TurretRotation = Pawn->GetActorRotation();

                AActor* AttackTarget = Cast<AActor>(BB->GetValueAsObject("AttackTarget"));

                FVector TargetVelocity = FVector::ZeroVector;
                float TargetSpeed = 0.0f;
                float TargetX = 0.f;
                float TargetY = 0.f;

                if (AttackTarget)
                {
                    FVector Pos = AttackTarget->GetActorLocation();
                    TargetX = Pos.X;
                    TargetY = Pos.Y;
                    if (UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(AttackTarget->GetRootComponent()))
                    {
                        TargetVelocity = Root->GetComponentVelocity();
                        TargetSpeed = TargetVelocity.Size(); // Skalar
                    }
                }

                float BulletDistance = BB->GetValueAsFloat("BulletDistance");
                float PrevBulletDistance = BB->GetValueAsFloat("PrevBulletDistance");

                float Reward = 0.0f;
                if (BulletDistance == 0.0f) {
                    Reward += 50.0f;
                }
                //else if (BulletDistance > 0.0f) {
                    //Reward += (PrevBulletDistance - BulletDistance) * 0.001f;
                    
                    //Reward += 1.0f / (BulletDistance * 0.001f);
                    
                    //Reward -= BulletDistance * 0.001f;
                    //Reward -= BulletDistance / 100000;

                    //const float Tolerance = 1.0f;
                    //if (FMath::IsNearlyEqual(PrevBulletDistance, BulletDistance, Tolerance))
                    //    Reward -= 50.0f;

                    else if (BulletDistance < 200.f)
                        Reward += 5.0f;
                    else if (BulletDistance < 400.f)
                        Reward += 2.0f;
                    else if (BulletDistance < 800.f)
                        Reward += 1.0f;
                //}
                //if (GEngine)
                //{
                //    FString msg = FString::Printf(TEXT("%f"), Reward);
                //    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, msg);
                //}

                //tr.x - turret rotation yaw
                //tr.y - turret rotation pitch
                //float PitchRange = 0.0f - (-85.0f); // 85.0f
                //float NormalizedPitch = -1.0f + (TurretRotation.Pitch - (-85.0f)) * (2.0f / PitchRange);
                //NormalizedPitch = FMath::Clamp(NormalizedPitch, -1.0f, 1.0f);
                //float YawRange = 175.0f - 5.0f; // 170.0f
                //float NormalizedYaw = -1.0f + (TurretRotation.Yaw - 5.0f) * (2.0f / YawRange);
                //NormalizedYaw = FMath::Clamp(NormalizedYaw, -1.0f, 1.0f);
                //FString OutMessage = FString::Printf(
                //    TEXT("{\"x\":%.2f, \"y\":%.2f, \"bullet_distance\":%.2f, \"velocity\":%.2f, \"tr.y\":%.2f, \"tr.x\":%.2f, \"reward\":%.2f}"),
                //    TargetX,
                //    TargetY,
                //    BulletDistance,
                //    TargetSpeed,
                //    NormalizedPitch,
                //    NormalizedYaw,
                //    Reward
                //);

                
                FString OutMessage;
                if (BulletDistance > 0.1f) {
                    FVector ForwardVector = BB->GetValueAsVector("ForwardVector");
                    FVector TurretLocation = Pawn->GetActorLocation();
                    FVector AttackTargetLocation = AttackTarget->GetActorLocation();
                    float DeltaTheta = GetAngleDeltaToTarget(TurretLocation, ForwardVector, AttackTargetLocation);
                    float SinDelta = FMath::Sin(DeltaTheta);

                    float PitchDelta = GetPitchDelta(TurretLocation, ForwardVector, AttackTargetLocation);
                    float NormalizedPitch = PitchDelta / (PI / 2.0f);

                    float hitTarget = 0.0f;
                    if (BulletDistance == 0.0f) {
                        hitTarget = 1.0f;
                    }
                    Reward += -abs(DeltaTheta) * 0.05;
                    Reward += -abs(PitchDelta) * 0.05;

                    OutMessage = FString::Printf(
                        TEXT("{\"x\":%.2f, \"y\":%.2f, \"hit\":%.1f, \"reward\":%.2f}"),
                        SinDelta,
                        NormalizedPitch,
                        //BulletDistance,
                        hitTarget,
                        Reward
                    );
                }
                else{
                    //hit target
                    OutMessage = FString::Printf(
                        TEXT("{\"x\":%.2f, \"y\":%.2f, \"hit\":%.1f, \"reward\":%.2f}"),
                        0.0f,
                        0.0f,
                        //BulletDistance,
                        1.0f,
                        Reward
                    );
                }
                

                //UDPSubsystem->SendData(OutMessage);
                FinishLatentTask(*CachedOwnerComp.Get(), EBTNodeResult::Succeeded);
            }
        });

    World->GetTimerManager().SetTimer(
        TimerHandle_CheckUDP,
        TimerDelegate,
        0.01f, // 10ms
        true
    );

    return EBTNodeResult::InProgress;
}

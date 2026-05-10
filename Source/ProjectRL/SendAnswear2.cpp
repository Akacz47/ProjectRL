// Fill out your copyright notice in the Description page of Project Settings.

#include "SendAnswear2.h"
#include "UDPSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/ArrowComponent.h"

USendAnswear2::USendAnswear2()
{
    bNotifyTick = true; // Wymagane aby dzialal TickTask
    bCreateNodeInstance = true; // Bo przechowujemy stan (flagi, dane)
}

float GetAngleDeltaToTarget3(const FVector& TurretLocation, const FVector& TurretForward, const FVector& TargetLocation)
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

float GetPitchDelta3(const FVector& TurretLocation, const FVector& TurretForward, const FVector& TargetLocation)
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

EBTNodeResult::Type USendAnswear2::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // Czyœcimy flagi/stan jeœli to konieczne
    return EBTNodeResult::InProgress;
}

void USendAnswear2::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    UWorld* World = GetWorld();
    UUDPSubsystem* UDPSubsystem = World->GetGameInstance()->GetSubsystem<UUDPSubsystem>();
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    AAIController* AI = OwnerComp.GetAIOwner();

    // 1. Walidacja podstawowych obiektów
    if (!UDPSubsystem || !BB || !AI)
    {
        //FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    APawn* Pawn = AI->GetPawn();
    AActor* AttackTarget = Cast<AActor>(BB->GetValueAsObject("AttackTarget"));

    if (!Pawn || !AttackTarget)
    {
        // Jeœli nie ma celu, mo¿emy albo czekaæ, albo zwróciæ b³¹d. 
        // Tutaj zwracamy Succeeded, ¿eby BT mog³o przejœæ do innych stanów (np. szukania).
        // FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }

    // 2. Pobieranie danych ze œwiata
    FVector TurretLocation = Pawn->GetActorLocation();

    UArrowComponent* Arrow = Pawn->FindComponentByClass<UArrowComponent>();
    FVector ForwardVector = Arrow->GetForwardVector();

    //FVector ForwardVector = BB->GetValueAsVector("ForwardVector");
    FVector AttackTargetLocation = AttackTarget->GetActorLocation();
    float BulletDistance = BB->GetValueAsFloat("BulletDistance");

    // 3. Obliczenia (Twoja logika matematyczna)
    float DeltaTheta = GetAngleDeltaToTarget3(TurretLocation, ForwardVector, AttackTargetLocation);
    float SinDelta = FMath::Sin(DeltaTheta);

    float PitchDelta = GetPitchDelta3(TurretLocation, ForwardVector, AttackTargetLocation);
    float NormalizedPitch = PitchDelta / (PI / 2.0f);

    FVector RawVel = AttackTarget->GetVelocity();
    float NormVelX = RawVel.X * 0.001f;
    float NormVelY = RawVel.Y * 0.001f;

    // 4. Budowanie JSONa i wysy³ka
    //float ShootCooldown = BB->GetValueAsFloat("ShootCooldown");
    //FString OutMessage = FString::Printf(
    //    TEXT("{\"error_x\":%.2f, \"error_y\":%.2f, \"v_x\":%.2f, \"v_y\":%.2f, \"bullet_distance\":%.2f, \"target_distance\":%.2f, \"shoot_cooldown\":%.2f}"),
    //    SinDelta,
    //    NormalizedPitch,
    //    NormVelX, //    /2.f
    //    NormVelY, //    /2.f
    //    BulletDistance,
    //    float(FVector::Dist(TurretLocation, AttackTargetLocation)),
    //    FMath::Clamp(ShootCooldown - World->GetTimeSeconds(), 0.0f, 1.0f)
    //);
    //UDPSubsystem->SendData(OutMessage);


    float MaxCD = 0.25f; // 1.0f
    float ShootCooldown = BB->GetValueAsFloat("ShootCooldown");
    float CurrentTime = World->GetTimeSeconds();
    float ReadyStatus = FMath::Clamp(1.0f - ((ShootCooldown - CurrentTime) / MaxCD), 0.0f, 1.0f);

    FUDPStatePacket Packet;
    Packet.ErrorX = SinDelta;
    Packet.ErrorY = NormalizedPitch;
    Packet.VX = NormVelX;
    Packet.VY = NormVelY;
    Packet.BulletDistance = BulletDistance;
    Packet.TargetDistance = FVector::Dist(TurretLocation, AttackTargetLocation);
    //Packet.ShootCooldown = ReadyStatus; // Wysy³amy status 0-1

    UDPSubsystem->SendBinaryData(Packet);

    // 5. Koñczymy zadanie natychmiast po wys³aniu
    //FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    

    if (ShootCooldown <= CurrentTime) {
        BB->SetValueAsBool(TEXT("ShouldShoot"), false);
        BB->SetValueAsFloat(TEXT("ShootCooldown"), CurrentTime + MaxCD);
        //UFunction* ShootFunc = Pawn->FindFunction(FName("Shoot"));
        //Pawn->ProcessEvent(ShootFunc, nullptr);

        FVector SpawnLoc = Arrow->GetComponentLocation();
        FRotator SpawnRot = Arrow->GetComponentRotation();
        ABullet* bullet = GetWorld()->SpawnActor<ABullet>(BulletClass, SpawnLoc, SpawnRot);
        if (!Pawn->IsPlayerControlled() && IsValid(bullet)) {
            AAIC_Turret* MyController = Cast<AAIC_Turret>(Pawn->GetController());
            bullet->InitBullet(AttackTarget, MyController);
        }
    }
    FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
}
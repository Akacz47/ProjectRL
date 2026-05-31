// Fill out your copyright notice in the Description page of Project Settings.

#include "AIC_Turret.h"
#include "Bullet.h"
#include "BehaviorTree/BlackboardComponent.h"

float AAIC_Turret::GetAngleDeltaToTarget(const FVector& TurretLocation, const FVector& TurretForward, const FVector& TargetLocation)
{
    // Wektor do celu
    FVector ToTarget = (TargetLocation - TurretLocation).GetSafeNormal();

    // K¹t miêdzy forward a celem
    float Dot = FVector::DotProduct(TurretForward, ToTarget);
    Dot = FMath::Clamp(Dot, -1.0f, 1.0f);

    // K¹t w stopniach
    float Angle = FMath::Acos(Dot);

    // Znak kierunku (left/right)
    FVector Cross = FVector::CrossProduct(TurretForward, ToTarget);
    float Sign = FMath::Sign(Cross.Z); // Obrót w osi Z

    float DeltaTheta = Angle * Sign;
    return DeltaTheta; // radiany (+ w prawo, – w lewo)
}

float AAIC_Turret::GetPitchDelta(const FVector& TurretLocation, const FVector& TurretForward, const FVector& TargetLocation)
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

void AAIC_Turret::SetBulletDistance(float Distance)
{
    UBlackboardComponent* BlackboardComp = GetBlackboardComponent();

    if (BlackboardComp)
    {
        const FName KeyName = TEXT("BulletDistance");
        BlackboardComp->SetValueAsFloat(KeyName, Distance);
    }
    bulletDistance = Distance;
}

void AAIC_Turret::Tick(float DeltaTime)
{
    if (training) {
        UGameInstance* GameInstance = GetWorld()->GetGameInstance();
        UUDPSubsystem* UDPSubsystem = GameInstance ? GameInstance->GetSubsystem<UUDPSubsystem>() : nullptr;
        FUDPActionPacket Action;
        if (IsValid(AttackTarget) && UDPSubsystem->ReceiveBinaryAction(Action))
        {
            float RotateLR = Action.RotateLR;
            float RotateUD = Action.RotateUD;
            if (RotateLR == 0.f && RotateUD == 0.f)
                GetPawn()->SetActorRelativeRotation(FRotator(0.f, 90.f, 0.f));
            else {
                #pragma pack(push, 1) // Wymusza brak paddingu
                struct FFunctionParams
                {
                    double XAxis;
                    double YAxis;
                };
                #pragma pack(pop)
                FFunctionParams Params;
                Params.XAxis = RotateLR;
                Params.YAxis = RotateUD;
                UFunction* Func = GetPawn()->FindFunction(FName("AimPlayer"));
                if (Func)
                {
                    GetPawn()->ProcessEvent(Func, &Params);
                }
            }
            SendAnswear();
        }
    }
}

void AAIC_Turret::SendAnswear()
{
    UWorld* World = GetWorld();
    UUDPSubsystem* UDPSubsystem = World->GetGameInstance()->GetSubsystem<UUDPSubsystem>();

    APawn* MyPawn = GetPawn();
    if (!MyPawn || !AttackTarget)
        return;

    FVector TurretLocation = MyPawn->GetActorLocation();

    UArrowComponent* Arrow = MyPawn->FindComponentByClass<UArrowComponent>();
    FVector ForwardVector = Arrow->GetForwardVector();

    FVector AttackTargetLocation = AttackTarget->GetActorLocation();

    float DeltaTheta = GetAngleDeltaToTarget(TurretLocation, ForwardVector, AttackTargetLocation);
    float SinDelta = FMath::Sin(DeltaTheta);

    float PitchDelta = GetPitchDelta(TurretLocation, ForwardVector, AttackTargetLocation);
    float NormalizedPitch = PitchDelta / (PI / 2.0f);

    FVector RawVel = AttackTarget->GetVelocity();
    // Prêdkoœæ œwiata na prêdkoœæ "lokaln¹" wie¿yczki
    FVector LocalVel = MyPawn->GetActorTransform().InverseTransformVectorNoScale(RawVel);
    float NormVelX = LocalVel.X * 0.001f; // Prêdkoœæ przód-ty³
    float NormVelY = LocalVel.Y * 0.001f; // Prêdkoœæ lewo-prawo

    FUDPStatePacket Packet;
    Packet.ErrorX = SinDelta;
    Packet.ErrorY = NormalizedPitch;
    Packet.VX = NormVelX;
    Packet.VY = NormVelY;
    Packet.BulletDistance = bulletDistance;
    Packet.TargetDistance = float(FVector::Dist(TurretLocation, AttackTargetLocation) / 5000.0f);

    UDPSubsystem->SendBinaryData(Packet);
	
	float MaxCD = 0.25f;
    float CurrentTime = World->GetTimeSeconds();

    if (ShootCooldown <= CurrentTime) {
        ShootCooldown = CurrentTime + MaxCD;
        FVector SpawnLoc = Arrow->GetComponentLocation();
        FRotator SpawnRot = Arrow->GetComponentRotation();
        ABullet* bullet = GetWorld()->SpawnActor<ABullet>(BulletClass, SpawnLoc, SpawnRot);
        if (IsValid(bullet)) {

            bullet->InitBullet(AttackTarget, this);
        }
    }
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "AIC_Turret.h"
#include "Bullet.h"
#include "BehaviorTree/BlackboardComponent.h"

float GetAngleDeltaToTarget4(const FVector& TurretLocation, const FVector& TurretForward, const FVector& TargetLocation)
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

float GetPitchDelta4(const FVector& TurretLocation, const FVector& TurretForward, const FVector& TargetLocation)
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
        const FName KeyName2 = TEXT("PrevBulletDistance");
        float BulletDistance = BlackboardComp->GetValueAsFloat(KeyName);
        if (BulletDistance == -1.0)
            BlackboardComp->SetValueAsFloat(KeyName2, Distance);
        else
            BlackboardComp->SetValueAsFloat(KeyName2, BulletDistance);

        BlackboardComp->SetValueAsFloat(KeyName, Distance);

        SetShoudShootCpp(true);
    }
    bulletDistance = Distance;
}

void AAIC_Turret::SetShoudShootCpp(bool val)
{
    UBlackboardComponent* BlackboardComp = GetBlackboardComponent();

    if (BlackboardComp)
    {
        BlackboardComp->SetValueAsBool(TEXT("ShouldShoot"), val);
    }
}


void AAIC_Turret::Tick(float DeltaTime)
{
    if (training) {
        UGameInstance* GameInstance = GetWorld()->GetGameInstance();
        UUDPSubsystem* UDPSubsystem = GameInstance ? GameInstance->GetSubsystem<UUDPSubsystem>() : nullptr;
        FUDPActionPacket Action;
        if (IsValid(AttackTarget2) && UDPSubsystem->ReceiveBinaryAction(Action))
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
    if (!MyPawn || !AttackTarget2)
        return;

    FVector TurretLocation = MyPawn->GetActorLocation();

    UArrowComponent* Arrow = MyPawn->FindComponentByClass<UArrowComponent>();
    FVector ForwardVector = Arrow->GetForwardVector();

    FVector AttackTargetLocation = AttackTarget2->GetActorLocation();

    // 3. Obliczenia (Twoja logika matematyczna)
    float DeltaTheta = GetAngleDeltaToTarget4(TurretLocation, ForwardVector, AttackTargetLocation);
    float SinDelta = FMath::Sin(DeltaTheta);

    float PitchDelta = GetPitchDelta4(TurretLocation, ForwardVector, AttackTargetLocation);
    float NormalizedPitch = PitchDelta / (PI / 2.0f);

    FVector RawVel = AttackTarget2->GetVelocity();
    //float NormVelX = RawVel.X * 0.001f;
    //float NormVelY = RawVel.Y * 0.001f;
    // Przelicz prêdkoœæ œwiata na prêdkoœæ "lokaln¹" wie¿yczki
    FVector LocalVel = MyPawn->GetActorTransform().InverseTransformVectorNoScale(RawVel);
    float NormVelX = LocalVel.X * 0.001f; // Prêdkoœæ przód-ty³
    float NormVelY = LocalVel.Y * 0.001f; // Prêdkoœæ lewo-prawo

    float MaxCD = 0.25f; // 1.0f
    float CurrentTime = World->GetTimeSeconds();
    float ReadyStatus = FMath::Clamp(1.0f - ((ShootCooldown - CurrentTime) / MaxCD), 0.0f, 1.0f);

    FUDPStatePacket Packet;
    Packet.ErrorX = SinDelta;
    Packet.ErrorY = NormalizedPitch;
    Packet.VX = NormVelX;
    Packet.VY = NormVelY;
    Packet.BulletDistance = bulletDistance;
    Packet.TargetDistance = float(FVector::Dist(TurretLocation, AttackTargetLocation) / 5000.0f);
    //Packet.ShootCooldown = ReadyStatus; // Wysy³amy status 0-1

    UDPSubsystem->SendBinaryData(Packet);

    if (ShootCooldown <= CurrentTime) {
        //BB->SetValueAsBool(TEXT("ShouldShoot"), false);
        ShootCooldown = CurrentTime + MaxCD;
        FVector SpawnLoc = Arrow->GetComponentLocation();
        FRotator SpawnRot = Arrow->GetComponentRotation();
        ABullet* bullet = GetWorld()->SpawnActor<ABullet>(BulletClass, SpawnLoc, SpawnRot);
        if (IsValid(bullet)) {
            bullet->InitBullet(AttackTarget2, this);
        }
    }
}

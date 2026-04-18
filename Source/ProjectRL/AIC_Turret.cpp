// Fill out your copyright notice in the Description page of Project Settings.


#include "AIC_Turret.h"
#include "BehaviorTree/BlackboardComponent.h"

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
}

void AAIC_Turret::SetShoudShootCpp(bool val)
{
    UBlackboardComponent* BlackboardComp = GetBlackboardComponent();

    if (BlackboardComp)
    {
        BlackboardComp->SetValueAsBool(TEXT("ShouldShoot"), val);
    }
}

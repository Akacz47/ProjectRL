// Fill out your copyright notice in the Description page of Project Settings.


#include "BTT_SendGetAnswearPPO.h"
#include "UDPSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"


UBTT_SendGetAnswearPPO::UBTT_SendGetAnswearPPO()
{
	bNotifyTick = true; // Wymagane aby dzialal TickTask
	bCreateNodeInstance = true; // Bo przechowujemy stan (flagi, dane)

    bNotifyTaskFinished = true;
}

EBTNodeResult::Type UBTT_SendGetAnswearPPO::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // Czyœcimy bufor przed nowym krokiem, ¿eby nie czytaæ starych œmieci
    UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    UUDPSubsystem* UDPSubsystem = GameInstance ? GameInstance->GetSubsystem<UUDPSubsystem>() : nullptr;

    // Opcjonalnie: wyœlij zapytanie o stan tutaj, jeœli tego nie robisz w innym miejscu

    return EBTNodeResult::InProgress;
}

void UBTT_SendGetAnswearPPO::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    UUDPSubsystem* UDPSubsystem = GameInstance ? GameInstance->GetSubsystem<UUDPSubsystem>() : nullptr;

    if (!UDPSubsystem)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    FUDPActionPacket Action;
    // Sprawdzamy czy przyszed³ pakiet (nie blokujemy w¹tku!)
    if (UDPSubsystem->ReceiveBinaryAction(Action))
    {
        float RotateLR = Action.RotateLR;
        float RotateUD = Action.RotateUD;

        UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
        if (BB)
        {
            FRotator NewRot(RotateUD, RotateLR, 0.f); // Pitch, Yaw, Roll
            AAIController* AI = OwnerComp.GetAIOwner();
            APawn* Pawn = AI->GetPawn();
            if (RotateLR == 0.f && RotateUD == 0.f) {
                NewRot = FRotator(RotateLR, RotateUD, -1.0f);

                Pawn->SetActorRelativeRotation(FRotator(0.f, 90.f, 0.f));
            }
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

                UFunction* Func = Pawn->FindFunction(FName("AimPlayer"));
                if (Func)
                {
                    Pawn->ProcessEvent(Func, &Params);
                }
            }
            //BB->SetValueAsRotator(TEXT("TurretRotation"), NewRot);

            // Koñczymy zadanie - BT przejdzie do nastêpnego wêz³a
            FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        }
        else
        {
            FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        }
    }
    // Jeœli nie ma danych, nic nie robimy - TickTask wróci tu w nastêpnej klatce
}
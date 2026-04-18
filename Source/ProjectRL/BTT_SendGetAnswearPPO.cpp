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

//EBTNodeResult::Type UBTT_SendGetAnswearPPO::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
//{
//    // Trzeba czekac z wykonaniem tego taska, az kula nie zniknie, (chyba tak zlikwiduje crashe)
//    //CachedOwnerComp = &OwnerComp;
//    CachedOwnerComp = TWeakObjectPtr<UBehaviorTreeComponent>(&OwnerComp);
//
//    UWorld* World = GetWorld();
//
//    UGameInstance* GameInstance = World->GetGameInstance();
//    UUDPSubsystem* UDPSubsystem = GameInstance ? GameInstance->GetSubsystem<UUDPSubsystem>() : nullptr;
//    FTimerDelegate TimerDelegate;
//    TimerDelegate.BindLambda([this, World, UDPSubsystem]()
//        {
//            FString ReceivedMessage;
//            if (UDPSubsystem->ReceiveData(ReceivedMessage))
//            {
//                World->GetTimerManager().ClearTimer(TimerHandle_CheckUDP);
//
//                //UE_LOG(LogTemp, Log, TEXT("%s"), *ReceivedMessage);
//
//                TSharedPtr<FJsonObject> JsonObject;
//                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ReceivedMessage);
//
//                if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
//                {
//                    // Pobranie wartoœci
//                    float RotateLR = JsonObject->GetNumberField(TEXT("rotateLeftRight"));
//                    float RotateUD = JsonObject->GetNumberField(TEXT("rotateUpDown"));
//                    //bool ShouldShoot = JsonObject->GetBoolField("shoot");
//                    //RotateLR *= 12.0f;
//                    //RotateUD *= 6.0f;
//
//                    //UBlackboardComponent* BB = CachedOwnerComp->GetBlackboardComponent();
//                    if (!CachedOwnerComp.IsValid()) {
//                        UE_LOG(LogTemp, Log, TEXT("Wywalilo 104"));
//                        FinishLatentTask(*CachedOwnerComp.Get(), EBTNodeResult::Failed);
//                        return;
//                    }
//                    else {
//                        UBlackboardComponent* BB = CachedOwnerComp->GetBlackboardComponent();
//                        if (!BB) {
//                            UE_LOG(LogTemp, Log, TEXT("Wywalilo 110"));
//                            FinishLatentTask(*CachedOwnerComp.Get(), EBTNodeResult::Failed);
//                            return;
//                        }
//                        else {
//                            FName Key_TurretRotation = "TurretRotation";
//                            //FName Key_ShouldShoot = "ShouldShoot";
//
//                            FRotator NewRot(RotateLR, RotateUD, 0.f);
//                            if (RotateLR == 0.f && RotateUD == 0.f) {
//                                NewRot = FRotator(RotateLR, RotateUD, -1.0f);
//                            }
//
//                            BB->SetValueAsRotator(Key_TurretRotation, NewRot);
//                            //BB->SetValueAsBool(Key_ShouldShoot, ShouldShoot);
//
//                            //UE_LOG(LogTemp, Log, TEXT("BB TurretRotation=(P=%.2f,Y=%.2f), Shoot=%d"),
//                            //    RotateUD, RotateLR, ShouldShoot);
//                            //UE_LOG(LogTemp, Log, TEXT("BB TurretRotation=(P=%.2f,Y=%.2f)"), RotateUD, RotateLR);
//                        }
//                    }
//                }
//                else
//                {
//                    UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON from Python: %s"), *ReceivedMessage);
//                }
//
//                //FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Succeeded);
//                if (CachedOwnerComp.IsValid())
//                    FinishLatentTask(*CachedOwnerComp.Get(), EBTNodeResult::Succeeded);
//            }
//        });
//
//
//    World->GetTimerManager().SetTimer(
//        TimerHandle_CheckUDP,
//        TimerDelegate,
//        0.0001f, // 10ms - 0.01
//        true
//    );
//
//	return EBTNodeResult::InProgress;
//}
//
//void UBTT_SendGetAnswearPPO::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
//{
//    if (UWorld* World = GetWorld())
//    {
//        World->GetTimerManager().ClearTimer(TimerHandle_CheckUDP);
//    }
//
//    CachedOwnerComp = nullptr;
//    if (GEngine)
//    {
//    	FString msg = FString::Printf(TEXT("Wykonuje"));
//    	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, msg);
//    }
//
//    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
//}



// W pliku (.cpp)
EBTNodeResult::Type UBTT_SendGetAnswearPPO::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // Czyœcimy bufor przed nowym krokiem, ¿eby nie czytaæ starych œmieci
    UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    UUDPSubsystem* UDPSubsystem = GameInstance ? GameInstance->GetSubsystem<UUDPSubsystem>() : nullptr;

    // Opcjonalnie: wyœlij zapytanie o stan tutaj, jeœli tego nie robisz w innym miejscu

    return EBTNodeResult::InProgress;
}

//void UBTT_SendGetAnswearPPO::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
//{
//    UGameInstance* GameInstance = GetWorld()->GetGameInstance();
//    UUDPSubsystem* UDPSubsystem = GameInstance ? GameInstance->GetSubsystem<UUDPSubsystem>() : nullptr;
//
//    if (!UDPSubsystem)
//    {
//        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
//        return;
//    }
//
//    FString ReceivedMessage;
//    // Sprawdzamy czy przyszed³ pakiet (nie blokujemy w¹tku!)
//    if (UDPSubsystem->ReceiveData(ReceivedMessage))
//    {
//        TSharedPtr<FJsonObject> JsonObject;
//        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ReceivedMessage);
//
//        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
//        {
//            float RotateLR = JsonObject->GetNumberField(TEXT("rotateLeftRight"));
//            float RotateUD = JsonObject->GetNumberField(TEXT("rotateUpDown"));
//
//            UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
//            if (BB)
//            {
//                FRotator NewRot(RotateUD, RotateLR, 0.f); // Pitch, Yaw, Roll
//                if (RotateLR == 0.f && RotateUD == 0.f) {
//                    NewRot = FRotator(RotateLR, RotateUD, -1.0f);
//                }
//                BB->SetValueAsRotator(TEXT("TurretRotation"), NewRot);
//
//                // Koñczymy zadanie - BT przejdzie do nastêpnego wêz³a
//                FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
//            }
//            else
//            {
//                FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
//            }
//        }
//    }
//    // Jeœli nie ma danych, nic nie robimy - TickTask wróci tu w nastêpnej klatce
//}

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
            BB->SetValueAsRotator(TEXT("TurretRotation"), NewRot);

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
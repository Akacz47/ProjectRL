// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"

#include "../../../../UE_5.7/Engine/Plugins/NNE/NNERuntimeORT/Source/ThirdParty/Onnxruntime/Internal/onnxruntime_cxx_api.h"
//#include "onnxruntime_cxx_api.h"
#include <vector>
#include <iostream>

#include "BTT_Predict.generated.h"



/**
 * 
 */
UCLASS()
class PROJECTRL_API UBTT_Predict : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_Predict();
	virtual ~UBTT_Predict();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
private:
	// Przechowujemy te obiekty jako wskaüniki, aby øy≥y d≥uøej niø jedna klatka
	Ort::Env* OrtEnv;
	Ort::Session* OrtSession;
	Ort::SessionOptions* SessionOptions;

	FVector LastTargetLocation;
	float LastTimestamp;

	// Funkcja pomocnicza do inicjalizacji
	void InitializeONNX();
};

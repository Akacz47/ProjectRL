// Fill out your copyright notice in the Description page of Project Settings.


#include "BTT_Predict.h"
#include "UDPSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/ArrowComponent.h"

UBTT_Predict::UBTT_Predict()
{
    OrtEnv = nullptr;
    OrtSession = nullptr;
    SessionOptions = nullptr;
}

UBTT_Predict::~UBTT_Predict()
{
    // Czyszczona sesja
    if (OrtSession) {
        delete OrtSession;
        OrtSession = nullptr;
    }

    // Czyszczone œrodowisko
    if (OrtEnv) {
        delete OrtEnv;
        OrtEnv = nullptr;
    }

    if (SessionOptions) {
        delete SessionOptions;
        SessionOptions = nullptr;
    }
}

float UBTT_Predict::GetAngleDeltaToTarget(const FVector& TurretLocation, const FVector& TurretForward, const FVector& TargetLocation)
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

float UBTT_Predict::GetPitchDelta(const FVector& TurretLocation, const FVector& TurretForward, const FVector& TargetLocation)
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

void UBTT_Predict::InitializeONNX()
{
    if (OrtSession) return; // Ju¿ zainicjalizowano

    OrtEnv = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "SAC-Turret-Session");

    SessionOptions = new Ort::SessionOptions();
    SessionOptions->SetIntraOpNumThreads(1);
    SessionOptions->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

    const wchar_t* ModelPath;
    if (loaded_model == "SIMPLE") {
        ModelPath = L"E:/UE/Projekty/ProjectRL/Content/Models/sac_actor_simple.onnx";
    }
    else {
        ModelPath = L"E:/UE/Projekty/ProjectRL/Content/Models/sac_actor_udp.onnx";
    }
    OrtSession = new Ort::Session(*OrtEnv, ModelPath, *SessionOptions);

    UE_LOG(LogTemp, Log, TEXT("ONNX Session successfully initialized."));
}

EBTNodeResult::Type UBTT_Predict::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UWorld* World = GetWorld();
    UGameInstance* GameInstance = World->GetGameInstance();
    UUDPSubsystem* UDPSubsystem = GameInstance ? GameInstance->GetSubsystem<UUDPSubsystem>() : nullptr;
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    AAIController* AI = OwnerComp.GetAIOwner();
    if (!AI) return EBTNodeResult::Failed;

    APawn* Pawn = AI->GetPawn();
    if (!Pawn) return EBTNodeResult::Failed;

    AActor* AttackTarget = Cast<AActor>(BB->GetValueAsObject("AttackTarget"));
    UArrowComponent* Arrow = Pawn->FindComponentByClass<UArrowComponent>();
    FVector ForwardVector = Arrow->GetForwardVector();
    FVector TurretLocation = Pawn->GetActorLocation();
    if (!AttackTarget) {
        return EBTNodeResult::Succeeded;
    }
    FVector AttackTargetLocation = AttackTarget->GetActorLocation();
    float DeltaTheta = GetAngleDeltaToTarget(TurretLocation, ForwardVector, AttackTargetLocation);
    float SinDelta = FMath::Sin(DeltaTheta);

    float PitchDelta = GetPitchDelta(TurretLocation, ForwardVector, AttackTargetLocation);
    float NormalizedPitch = PitchDelta / (PI / 2.0f);

    FVector RawVel = AttackTarget->GetVelocity();

    InitializeONNX();
    if (!OrtSession) return EBTNodeResult::Failed;

    float NormVelX, NormVelY;
    std::vector<float> input_tensor_values;
    if (loaded_model == "UDP") {
        FVector LocalVel = Pawn->GetActorTransform().InverseTransformVectorNoScale(RawVel);
        NormVelX = LocalVel.X * 0.001f;
        NormVelY = LocalVel.Y * 0.001f;
        input_tensor_values = {
            SinDelta,
            NormalizedPitch,
            NormVelX,
            NormVelY,
            float(FVector::Dist(TurretLocation, AttackTargetLocation) / 5000.0f)
        };
    }
    else if (loaded_model == "SIMPLE") {
        NormVelX = RawVel.X * 0.001f;
        NormVelY = RawVel.Y * 0.001f;
        input_tensor_values = {
            SinDelta,
            NormalizedPitch,
            NormVelX / 2.f,
            NormVelY / 2.f,
            float(FVector::Dist(TurretLocation, AttackTargetLocation) / 5000.0f)
        };
    }

    std::vector<int64_t> input_shape = { 1, 5 };  // batch=1, dim=5

    Ort::MemoryInfo memory_info =
        Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        input_tensor_values.data(),
        input_tensor_values.size(),
        input_shape.data(),
        input_shape.size()
    );

    const char* input_names[] = { "obs" };
    const char* output_names[] = { "action" };

    auto output_tensors = OrtSession->Run(Ort::RunOptions{ nullptr }, input_names, &input_tensor, 1, output_names, 1);
    float* action = output_tensors.front().GetTensorMutableData<float>();

    float rotateLR = action[0];
    float rotateUD = action[1];
    
    FName Key_TurretRotation = "TurretRotation";
    FRotator NewRot(rotateUD, rotateLR, 0.f); //pitch, yaw, roll
    BB->SetValueAsRotator(Key_TurretRotation, NewRot);

    return EBTNodeResult::Succeeded;
}

//UE_LOG(LogTemp, Warning, TEXT("-----------------------------------------------------"));
//UE_LOG(LogTemp, Warning, TEXT("Pitch i Yaw : % f   % f"), NormalizedPitch, SinDelta);
//UE_LOG(LogTemp, Warning, TEXT("Predykcja SAC : rotateUD = % f   rotateLR = % f"), rotateUD, rotateLR);
//UE_LOG(LogTemp, Warning, TEXT("TargetVel.X = % f  TargetVel.Y = % f"), NormVelX, NormVelY);
//UE_LOG(LogTemp, Warning, TEXT("BulletDistance = % f"), BulletDistance);
//UE_LOG(LogTemp, Warning, TEXT("TargetDistance = % f"), FVector::Dist(TurretLocation, AttackTargetLocation));

//UE_LOG(LogTemp, Warning, TEXT("% f, % f, % f, % f, % f, % f"), SinDelta, NormalizedPitch, NormVelX, NormVelY, 1.0f - exp(float(-FVector::Dist(TurretLocation, AttackTargetLocation) / 4500.0)), ReadyStatus);

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
    // Czyœcimy sesjê
    if (OrtSession) {
        delete OrtSession;
        OrtSession = nullptr;
    }

    // Czyœcimy œrodowisko
    if (OrtEnv) {
        delete OrtEnv;
        OrtEnv = nullptr;
    }

    if (SessionOptions) {
        delete SessionOptions;
        SessionOptions = nullptr;
    }
}

float GetAngleDeltaToTarget2(const FVector& TurretLocation, const FVector& TurretForward, const FVector& TargetLocation)
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

float GetPitchDelta2(const FVector& TurretLocation, const FVector& TurretForward, const FVector& TargetLocation)
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

//UBTT_Predict::UBTT_Predict()
//{
//    Env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "SAC-Test");
//    // Ustawienia sesji
//    Ort::SessionOptions session_options;
//    session_options.SetIntraOpNumThreads(1);
//    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
//
//    // Œcie¿ka do modelu ONNX
//    const wchar_t* model_path_w = L"E:/UE/Projekty/ProjectRL/Content/Models/sac_actor.onnx";
//
//    // Sprawdzenie czy plik istnieje
//    FString model_path(model_path_w);
//    if (!FPaths::FileExists(model_path))
//    {
//        UE_LOG(LogTemp, Error, TEXT("Model nie istnieje: %s"), *model_path);
//        return;
//    }
//
//    // Tworzymy sesjê tylko raz
//    Session = new Ort::Session(*Env, model_path_w, session_options);
//
//    UE_LOG(LogTemp, Warning, TEXT("SAC Session wczytana w konstruktorze."));
//
//}

void UBTT_Predict::InitializeONNX()
{
    if (OrtSession) return; // Ju¿ zainicjalizowano

    OrtEnv = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "SAC-Turret-Session");

    SessionOptions = new Ort::SessionOptions();
    SessionOptions->SetIntraOpNumThreads(1);
    SessionOptions->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

    const wchar_t* ModelPath = L"E:/UE/Projekty/ProjectRL/Content/Models/sac_actor_simple.onnx";

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

    UArrowComponent* Arrow = Pawn->FindComponentByClass<UArrowComponent>();
    FVector ForwardVector = Arrow->GetForwardVector();
    FVector TurretLocation = Pawn->GetActorLocation();
    if (!AttackTarget) {
        return EBTNodeResult::Succeeded;
    }
    FVector AttackTargetLocation = AttackTarget->GetActorLocation();
    float DeltaTheta = GetAngleDeltaToTarget2(TurretLocation, ForwardVector, AttackTargetLocation);
    float SinDelta = FMath::Sin(DeltaTheta);

    float PitchDelta = GetPitchDelta2(TurretLocation, ForwardVector, AttackTargetLocation);
    float NormalizedPitch = PitchDelta / (PI / 2.0f);

    FVector RawVel = AttackTarget->GetVelocity();

    //////important!!!
    float NormVelX = RawVel.X * 0.001f;
    float NormVelY = RawVel.Y * 0.001f;

    //FVector LocalVel = Pawn->GetActorTransform().InverseTransformVectorNoScale(RawVel);
    //NormVelX = LocalVel.X * 0.001f;
    //NormVelY = LocalVel.Y * 0.001f;

    InitializeONNX();
    if (!OrtSession) return EBTNodeResult::Failed;

    // Normalizacja do zakresu -1, 1 (przyk³adowo dla Yaw)
    float CurrentYawNorm = (TurretRotation.Yaw - 90.0f) / 85.0f;
    float CurrentPitchNorm = (TurretRotation.Pitch + 42.5f) / 42.5f;

    //float MaxCD = 1.0f; // 1.0f
    //float ShootCooldown = BB->GetValueAsFloat("ShootCooldown");
    //float ReadyStatus = FMath::Clamp(1.0f - ((ShootCooldown - World->GetTimeSeconds()) / MaxCD), 0.0f, 1.0f);

    std::vector<float> input_tensor_values = {
        SinDelta,
        NormalizedPitch,
        NormVelX /2.f,
        NormVelY /2.f,
        //CurrentYawNorm,
        //CurrentPitchNorm
 
        float(FVector::Dist(TurretLocation, AttackTargetLocation) / 5000.0f)
        //1.0f - exp(float( - FVector::Dist(TurretLocation, AttackTargetLocation) / 4500.0)), // 4500
        //ReadyStatus
    };

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

    UE_LOG(LogTemp, Warning, TEXT("-----------------------------------------------------"));
    UE_LOG(LogTemp, Warning, TEXT("Pitch i Yaw : % f   % f"), NormalizedPitch, SinDelta);
    UE_LOG(LogTemp, Warning, TEXT("Predykcja SAC : rotateUD = % f   rotateLR = % f"), rotateUD, rotateLR);
    UE_LOG(LogTemp, Warning, TEXT("TargetVel.X = % f  TargetVel.Y = % f"), NormVelX, NormVelY);
    UE_LOG(LogTemp, Warning, TEXT("BulletDistance = % f"), BulletDistance);
    UE_LOG(LogTemp, Warning, TEXT("TargetDistance = % f"), FVector::Dist(TurretLocation, AttackTargetLocation));

    //UE_LOG(LogTemp, Warning, TEXT("% f, % f, % f, % f, % f, % f"), SinDelta, NormalizedPitch, NormVelX, NormVelY, 1.0f - exp(float(-FVector::Dist(TurretLocation, AttackTargetLocation) / 4500.0)), ReadyStatus);
    
    FName Key_TurretRotation = "TurretRotation";
    FRotator NewRot(rotateUD, rotateLR, 0.f); //pitch, yaw, roll
    BB->SetValueAsRotator(Key_TurretRotation, NewRot);

    //#pragma pack(push, 1) // Wymusza brak paddingu
    //struct FFunctionParams
    //{
    //    double XAxis;
    //    double YAxis;
    //};
    //#pragma pack(pop)
    //FFunctionParams Params;
    //Params.XAxis = rotateLR;
    //Params.YAxis = rotateUD;
    //UFunction* Func = Pawn->FindFunction(FName("AimPlayer"));
    //if (Func)
    //{
    //    Pawn->ProcessEvent(Func, &Params);
    //}

    return EBTNodeResult::Succeeded;
}

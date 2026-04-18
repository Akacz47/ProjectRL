#include "UDPComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/RunnableThread.h"
#include "Misc/Paths.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "JsonObjectConverter.h"


//#include "CoreMinimal.h"
//#include "Engine/AssetManager.h"

#include "../../../../UE_5.7/Engine/Plugins/NNE/NNERuntimeORT/Source/ThirdParty/Onnxruntime/Internal/onnxruntime_cxx_api.h"
//#include "onnxruntime_cxx_api.h"
#include <vector>
#include <iostream>


UUDPComponent::UUDPComponent()
//UUDPComponent::UUDPComponent(const FObjectInitializer& ObjectInitializer)
//    : UActorComponent(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UUDPComponent::BeginPlay()
{
    Super::BeginPlay();
    SetupSocket();
    UE_LOG(LogTemp, Warning, TEXT("Aktywowano"));

    // CHAT
    //FString ModelPath = FPaths::ProjectContentDir() / TEXT("AI/linear_model.onnx");

    //FOrtSessionOptions Options;
    //UOnnxModel* Model = UOnnxModel::Load(ModelPath, Options);

    //if (Model)
    //{
    //    UE_LOG(LogTemp, Warning, TEXT("Model za³adowany poprawnie!"));

    //     Przyk³ad: predykcja dla x = 5
    //    TArray<float> Input = { 5.0f };
    //    TArray<int64> Shape = { 1, 1 };

    //    FOrtValue InputTensor = FOrtValue::CreateTensor<float>(Input, Shape);

    //    TArray<FOrtValue> Outputs = Model->Run({ InputTensor });
    //    TArray<float> Result = Outputs[0].GetTensorMutableData<float>();

    //    UE_LOG(LogTemp, Warning, TEXT("Wynik predykcji: %f"), Result[0]);
    //}
    //else
    //{
    //    UE_LOG(LogTemp, Error, TEXT("Nie uda³o siê wczytaæ modelu ONNX!"));
    //}


    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "test");
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(1);

    const wchar_t* model_path_w = L"E:/UE/Projekty/ProjectRL/Content/Models/linear_model2.onnx";
    // 1. Convert the wide string literal to an FString
    const FString model_path(model_path_w);

    // 2. Use FString::FileExists (preferred) or FPaths::FileExists
    if (!FPaths::FileExists(model_path)) {
        // 3. Log the FString using the '*' operator
        // The * operator on an FString returns the TCHAR* which %s expects.
        UE_LOG(LogTemp, Error, TEXT("Nie znaleziono modelu: %s"), *model_path);
        return;
    }

    Ort::Session session(env, model_path_w, session_options);

    std::vector<float> input_tensor_values = { 3.0f };
    std::vector<int64_t> input_shape = { 1, 1 };

    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_tensor_values.data(), input_tensor_values.size(), input_shape.data(), input_shape.size());

    const char* input_names[] = { "input" };
    const char* output_names[] = { "output" };

    auto output_tensors = session.Run(Ort::RunOptions{ nullptr }, input_names, &input_tensor, 1, output_names, 1);
    float* result = output_tensors.front().GetTensorMutableData<float>();

    UE_LOG(LogTemp, Warning, TEXT("Wynik predykcji: %f"), result[0]);

    if (GEngine)
    {
        FString msg = FString::Printf(TEXT("Wynik predykcji: %f"), result[0]);
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, msg);
    }
}

void UUDPComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ListenSocket)
    {
        ListenSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenSocket);
    }
    if (SendSocket)
    {
        SendSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(SendSocket);
    }
    Super::EndPlay(EndPlayReason);
}

void UUDPComponent::SetupSocket()
{
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

    // Tworzenie socketu nas³uchuj¹cego
    ListenSocket = FUdpSocketBuilder(TEXT("UDP_Listener"))
        .AsNonBlocking()
        .AsReusable()
        .BoundToPort(9000)
        .WithReceiveBufferSize(2 * 1024 * 1024);

    // Tworzenie socketu do wysy³ania (na port Pythona)
    SendSocket = FUdpSocketBuilder(TEXT("UDP_Sender"))
        .AsReusable()
        .WithSendBufferSize(2 * 1024 * 1024);

    FIPv4Address Addr;
    FIPv4Address::Parse(TEXT("127.0.0.1"), Addr);
    RemoteEndpoint = FIPv4Endpoint(Addr, 8000); // port Pythona
}

void UUDPComponent::ReceiveData()
{
    if (!ListenSocket) return;

    uint32 Size;
    while (ListenSocket->HasPendingData(Size))
    {
        TArray<uint8> ReceivedData;
        ReceivedData.SetNumUninitialized(FMath::Min(Size, 65507u));

        int32 BytesRead = 0;
        ListenSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), BytesRead);

        FString ReceivedStr = FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(ReceivedData.GetData())));
        LastReceivedAction = ReceivedStr;
        bReceivedNewMessage = true;

        UE_LOG(LogTemp, Warning, TEXT("Received: %s"), *ReceivedStr);
    }
}

void UUDPComponent::SendData(FString& Message)
{
    if (!SendSocket) return;

    TCHAR* SerializedChar = Message.GetCharArray().GetData();
    int32 Size = FCString::Strlen(SerializedChar);
    int32 Sent = 0;

    SendSocket->SendTo(
        (uint8*)TCHAR_TO_UTF8(SerializedChar),
        Size,
        Sent,
        *RemoteEndpoint.ToInternetAddr()
    );
}

void UUDPComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ReceiveData();

    if (bReceivedNewMessage)
    {

        //// Przyk³ad: przetwórz akcjê (np. "rotate_left", "shoot")
        //if (LastReceivedAction.Contains("rotate_left"))
        //{
        //    GetOwner()->AddActorWorldRotation(FRotator(0, -1, 0));
        //}
        //else if (LastReceivedAction.Contains("rotate_right"))
        //{
        //    GetOwner()->AddActorWorldRotation(FRotator(0, 1, 0));
        //}

        // Przyk³ad: stan do wys³ania
        FVector Pos = GetOwner()->GetActorLocation();
        FString OutMessage = FString::Printf(TEXT("{\"x\":%.2f, \"y\":%.2f, \"reward\":0.5}"), Pos.X, Pos.Y);
        SendData(OutMessage);
        bReceivedNewMessage = false;
        UE_LOG(LogTemp, Warning, TEXT("Wyslano"));
    }
}


// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Networking.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "UDPComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTRL_API UUDPComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UUDPComponent();
    //UUDPComponent(const FObjectInitializer& ObjectInitializer);
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void SetupSocket();

private:
    FSocket* ListenSocket;
    FSocket* SendSocket;
    FIPv4Endpoint RemoteEndpoint;

    void ReceiveData();
    void SendData(FString& Message);

    bool bReceivedNewMessage = true;
    FString LastReceivedAction;
};

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Networking.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Containers/Queue.h"
#include "UDPSubsystem.generated.h"

//DECLARE_MULTICAST_DELEGATE_OneParam(FUDPMessageReceivedSignature, const FString&);

/**
 * Globalny Subsystem zarz¹dzaj¹cy komunikacj¹ UDP
 */

 // --- STRUKTURY BINARNE (Czysty C++) ---
#pragma pack(push, 1)
struct FUDPStatePacket
{
    float ErrorX;
    float ErrorY;
    float VX;
    float VY;
    float BulletDistance;
    float TargetDistance;
    //float ShootCooldown;

    FUDPStatePacket() : ErrorX(0), ErrorY(0), VX(0), VY(0), BulletDistance(0), TargetDistance(0) {}//, ShootCooldown(0) {}
};

struct FUDPActionPacket
{
    float RotateLR;
    float RotateUD;

    FUDPActionPacket() : RotateLR(0), RotateUD(0) {}
};
#pragma pack(pop)

UCLASS()
class PROJECTRL_API UUDPSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    //bool SendData(FString& Message);
    //bool ReceiveData(FString& OutMessage);

    bool SendBinaryData(const FUDPStatePacket& Packet);
    bool ReceiveBinaryAction(FUDPActionPacket& OutAction);

protected:
    // Powi¹zany z portem UE
    FSocket* ListenSocket;

    // celuj¹cy w port Pythona
    FSocket* SendSocket;

    // Docelowy adres i port Pythona - 127.0.0.1:8000
    FIPv4Endpoint RemoteEndpoint;

private:

    void SetupSockets();
    void CleanUpSockets();
};
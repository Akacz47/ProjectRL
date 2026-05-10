#include "UDPSubsystem.h"
#include "Common/UdpSocketBuilder.h"
#include "Common/UdpSocketReceiver.h"

namespace UDPConfig
{
    const int32 ListenPort = 9000;
    const int32 RemotePort = 8000;
    const FString RemoteIP = TEXT("127.0.0.1");
}

void UUDPSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    SetupSockets();
    UE_LOG(LogTemp, Warning, TEXT("UUDPSubsystem zainicjalizowany. Sockets gotowe."));
}

void UUDPSubsystem::Deinitialize()
{
    CleanUpSockets();
    Super::Deinitialize();
}

void UUDPSubsystem::SetupSockets()
{
    FIPv4Address Addr;
    FIPv4Address::Parse(UDPConfig::RemoteIP, Addr);
    RemoteEndpoint = FIPv4Endpoint(Addr, UDPConfig::RemotePort);

    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

    ListenSocket = FUdpSocketBuilder(TEXT("PROJECTRL_UDP_Listener"))
        .AsNonBlocking()
        .AsReusable()
        .BoundToPort(UDPConfig::ListenPort)
        .WithReceiveBufferSize(2 * 1024 * 1024)
        .Build();

    SendSocket = FUdpSocketBuilder(TEXT("PROJECTRL_UDP_Sender"))
        .AsReusable()
        .WithSendBufferSize(2 * 1024 * 1024)
        .Build();
}

void UUDPSubsystem::CleanUpSockets()
{
    // Zamkniêcie i zniszczenie Sockeów
    if (ListenSocket)
    {
        ListenSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenSocket);
        ListenSocket = nullptr;
    }
    if (SendSocket)
    {
        SendSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(SendSocket);
        SendSocket = nullptr;
    }
}

 //Wysy³anie binarne
bool UUDPSubsystem::SendBinaryData(const FUDPStatePacket& Packet)
{
    if (!SendSocket) return false;

    int32 Sent = 0;
    return SendSocket->SendTo(
        (uint8*)&Packet,        // WskaŸnik na pocz¹tek struktury
        sizeof(FUDPStatePacket), // Rozmiar w bajtach
        Sent,
        *RemoteEndpoint.ToInternetAddr()
    );
}

// Odbieranie binarne
bool UUDPSubsystem::ReceiveBinaryAction(FUDPActionPacket& OutAction)
{
    if (!ListenSocket) return false;

    uint32 Size = 0;
    if (ListenSocket->HasPendingData(Size))
    {
        if (Size >= sizeof(FUDPActionPacket)) // Sprawdzamy czy przysz³o doœæ danych
        {
            int32 BytesRead = 0;
            return ListenSocket->Recv((uint8*)&OutAction, sizeof(FUDPActionPacket), BytesRead);
        }
    }
    return false;
}
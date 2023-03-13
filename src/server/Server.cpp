#include "Server.h"

#include <chrono>

#include "schemas/chatpacket_generated.h"
#include "common/Random.h"


stc::Server::Server():
    ServerHost {nullptr}
    , bRun{ true }
    , ServerName{ "ExampleServerName" }
{
    /* Create packet type dispatcher */
    PacketTypeCallbacks = {
        {   0001,   std::bind(&stc::Server::OnClientServerInfoRequest, this, std::placeholders::_1, std::placeholders::_2) }
        , { 0002,   std::bind(&stc::Server::OnClientJoinServer, this, std::placeholders::_1, std::placeholders::_2) }
        , { 0003,   std::bind(&stc::Server::OnClientChatMessage, this, std::placeholders::_1, std::placeholders::_2) }
        , { 0004,   std::bind(&stc::Server::OnClientRequestNickChange, this, std::placeholders::_1, std::placeholders::_2) }
    };
}

stc::Server::~Server()
{
    enet_host_destroy(ServerHost);
}

bool stc::Server::InitConnection()
{
    ENetAddress address;
    /* Bind the server to the default localhost.     */
    /* A specific host address can be specified by   */
    /* enet_address_set_host (& address, "x.x.x.x"); */
    address.host = ENET_HOST_ANY;
    /* Bind the server to port 1234. */
    address.port = net::SERVER_DEFAULT_PORT;
    
    ServerHost = enet_host_create(&address /* the address to bind the server host to */,
        32      /* allow up to 32 clients and/or outgoing connections */,
        2      /* allow up to 2 channels to be used, 0 and 1 */,
        0      /* assume any amount of incoming bandwidth */,
        0      /* assume any amount of outgoing bandwidth */);
    
    if (ServerHost == NULL)
    {
        fprintf(stderr, "An error occurred while trying to create an ENet server host.\n");
        return false;
    }

    return true;
}

void stc::Server::Run()
{
    ENetEvent Event;

    std::chrono::steady_clock::time_point Now;
    std::chrono::steady_clock::time_point Start;
    std::chrono::duration<float> ProcessingTime;

    // Packets loop.
    while (bRun)
    {
        Start = std::chrono::high_resolution_clock::now();
        while (enet_host_service(ServerHost, &Event, 0) > 0)
        {
            HandleEvent(Event);
        }

        // Throttle.
        Now = std::chrono::high_resolution_clock::now();
        ProcessingTime = (Now - Start);
        const auto SleepTime = std::max(
            std::chrono::duration<float>(0.f),
            std::chrono::duration<float>(SERVER_TICK) - ProcessingTime
        );
        std::this_thread::sleep_for(SleepTime);
    }
}

stc::ChatUser* stc::Server::CreateNewTempUser(ENetPeer* Peer)
{
    static uint32_t UserCounter = 0;
    UserCounter++;

    ChatUser* NewUser = new ChatUser{
        0,
        std::format("User{}", UserCounter),
        Peer,
        net::Key()
    };
    TempUsers.push_back(std::shared_ptr<ChatUser>(NewUser));

    return NewUser;
}

bool stc::Server::IsNickValid(const std::string& Nick)
{
    // Size
    if (!(Nick.size() > 2 && Nick.size() < 24))
    {
        return false;
    }

    // Only printable characters
    const bool bNonePrintable = !std::all_of(Nick.begin(), Nick.end(), [](char c) { return std::isgraph(c); });
    if (bNonePrintable)
    {
        return false;
    }

    return true;
}

void stc::Server::HandleEvent(const ENetEvent& Event)
{
    char Name[] = "Client001";

    switch (Event.type)
    {
    case ENET_EVENT_TYPE_CONNECT:
        printf("A new client connected from %x:%u.\n",
            Event.peer->address.host,
            Event.peer->address.port);

        OnEventConnect(Event);
        break;
    
    case ENET_EVENT_TYPE_RECEIVE:
        printf("A packet of length %zu containing %s was received from %s on channel %u.\n",
            Event.packet->dataLength,
            (char*)Event.packet->data,
            "Test",
            Event.channelID);
        
        /* Dispatch net event */
        OnEventReceive(Event);

        /* Clean up the packet now that we're done using it. */
        enet_packet_destroy(Event.packet);
        break;

    case ENET_EVENT_TYPE_DISCONNECT:
        printf("%s disconnected.\n", (char*)Event.peer->data);
    
        OnEventDisconnect(Event);

        break;
    }
}

void stc::Server::OnEventReceive(const ENetEvent& Event)
{
    ChatUser* User = (ChatUser*)Event.peer->data;

    if (!User)
    {
        printf("User pointer for peer is set to null.\n");
        return;
    }

    net::PacketWrapperDecoder Packet(Event.packet->data, Event.packet->dataLength);
    uint32_t PacketTypeID = Packet.GetType();

    auto CallbackIt = PacketTypeCallbacks.find(PacketTypeID);
    if (CallbackIt == PacketTypeCallbacks.end())
    {
        printf("Unknown packet type %d.\n", PacketTypeID);
        return;
    }
    else 
    {
        auto& CallbackFn = CallbackIt->second;
        CallbackFn(Packet, User);
    }
}

void stc::Server::OnEventConnect(const ENetEvent& Event)
{
    /* Create new temporary User. */
    ChatUser* NewUser = CreateNewTempUser(Event.peer);
    Event.peer->data = (void*)NewUser;
}

void stc::Server::OnEventDisconnect(const ENetEvent& Event)
{
    ChatUser* User = (ChatUser*)Event.peer->data;
    User->Peer = nullptr;
    Event.peer->data = nullptr;
}

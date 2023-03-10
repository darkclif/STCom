#include "Server.h"

#include <chrono>
#include "enet_wrapper.h"

stc::Server::Server():
    ServerHost {nullptr}
    , bRun{ true }
{
    /* Create packet type dispatcher */
    static std::map<uint32_t, std::function<void(stc::Server&)(net::PacketWrapper&)>> PacketTypeMap = {
        { (uint32_t)0001, stc::Server::OnClientJoinServer }
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
    address.port = SERVER_PORT;
    
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
        std::this_thread::sleep_for(
            std::max(
                std::chrono::duration<float>(0.f),
                std::chrono::duration<float>(SERVER_TICK) - ProcessingTime
            )
        );
    }
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
            Event.packet->data,
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
        
        /* Reset the peer's client information. */
        Event.peer->data = NULL;
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

    net::PacketWrapper Packet(Event.packet->data, Event.packet->dataLength);
    
}

void stc::Server::OnEventConnect(const ENetEvent& Event)
{
    /* Create new temporary User. */
    ChatUser* NewUser = CreateNewTempUser(Event.peer);
    Event.peer->data = (void*)NewUser;


}

void stc::Server::OnEventDisconnect(const ENetEvent& Event)
{

}

void stc::Server::OnClientJoinServer(net::PacketWrapper& Packet)
{
    auto* Req = Packet.GetInternalPacket<NetPackets::ClientJoinServer>();

    Req->nick();
    Req->pub_key();
}

void stc::Server::OnClientChatMessage(net::PacketWrapper& Packet)
{
    auto* Req = Packet.GetInternalPacket<NetPackets::ClientJoinServer>();

    Req->nick();
    Req->pub_key();
}

void stc::Server::BroadcastAll(const ChatUser* ExceptUser)
{
    for (auto& User : ChannelUsers)
    {
        if (User.get() != ExceptUser) 
        {
            // Send packet to this user.
        }
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
        0,
        0
    };
    TempUsers.push_back(std::shared_ptr<ChatUser>(NewUser));

    return NewUser;
}

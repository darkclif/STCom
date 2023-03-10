#include "Client.h"
#include "ftxui/component/loop.hpp"
#include "schemas/chatpacket_generated.h"

#include <format>


ftxui::Component Window(std::string title, ftxui::Component component) {
    return ftxui::Renderer(component, [component, title] {
        return ftxui::window(ftxui::text(title), component->Render()) | ftxui::flex;
    });
}

stc::Client::Client()
{
    bRun = true;
    ClientHost = nullptr;

    ChatLog = std::shared_ptr<stc::ChatLogUI>(new stc::ChatLogUI(Messages));
    UserList = std::shared_ptr<stc::UsersListUI>(new stc::UsersListUI(Users));
}

stc::Client::~Client()
{
    ShutdownNetworkClient();
    enet_host_destroy(ClientHost);
}

int stc::Client::Run()
{
    // Connect to server
    SetupNetworkClient();

    // Debug data
    AddMessage(Message{ 0, "User", "Test!" });
    AddUser(User{ "User1" });
    AddUser(User{ "Useffffr2" });
    AddUser(User{ "User3" });

    // Create interface
    std::function<bool(ftxui::Event)> OnMessageInput = std::bind(&stc::Client::OnMessageInput, this, std::placeholders::_1);
    std::function<bool(ftxui::Event)> OnGlobalInput = std::bind(&stc::Client::OnGlobalInput, this, std::placeholders::_1);

    ftxui::Component InputLine = ftxui::Input(&InputMessage, "Input...") | ftxui::CatchEvent(OnMessageInput);

    ftxui::Component RootComponent = ftxui::Container::Horizontal({
        ftxui::Container::Vertical({
            ftxui::Renderer([&] { return ChatLog->Render() | ftxui::flex; }),
            ftxui::Renderer(InputLine, [&] {
                    return ftxui::hbox({ftxui::text("Message| "), InputLine->Render()}) | ftxui::border;
            }),
        }) | ftxui::flex,
        ftxui::Renderer([&] { return UserList->Render(); })
    }) | ftxui::CatchEvent(OnGlobalInput);

    ftxui::ScreenInteractive Screen = ftxui::ScreenInteractive::Fullscreen();

    // Main event loop
    ftxui::Loop MainLoop(&Screen, RootComponent);
    while (!ShutdownTimer.Done() && !MainLoop.HasQuitted())
    {
        // Process network.
        ProcessNetworkEvents();

        // Render.
        MainLoop.RunOnce();
        
        // Throttle.
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

	return 0;
}

bool stc::Client::OnMessageInput(ftxui::Event Event)
{
    if (Event == ftxui::Event::Return) 
    {
        Net_SendMessage(InputMessage);
        AddMessage(Message{ 0, "Me", InputMessage});
        InputMessage.clear();
        return true;
    }

    return false;
}

bool stc::Client::OnGlobalInput(ftxui::Event Event)
{
    if (Event == ftxui::Event::Escape)
    {
        AddSystemMessage("Quiting...");
        ShutdownNetworkClient();
        ShutdownTimer.Start(1.f);
        return true;
    }

    return false;
}

void stc::Client::SetupNetworkClient()
{
    // Create ENet host.
    ClientHost = enet_host_create(
        NULL /* create a client host */,
        1 /* only allow 1 outgoing connection */,
        2 /* allow up 2 channels to be used, 0 and 1 */,
        0 /* assume any amount of incoming bandwidth */,
        0 /* assume any amount of outgoing bandwidth */
    );

    if (ClientHost == NULL)
    {
        AddSystemMessage("An error occurred while trying to create an ENet client host.\n");
    }

    // Connect to server.
    ENetAddress Address;
    ENetEvent Event;

    /* Connect to some.server.net:1234. */
    enet_address_set_host(&Address, "localhost");
    Address.port = 1234;
    
    /* Initiate the connection, allocating the two channels 0 and 1. */
    PeerConnection = enet_host_connect(ClientHost, &Address, 2, 0);
    if (PeerConnection == NULL)
    {
        AddSystemMessage("No available peers for initiating an ENet connection.");
    }

    /* Wait up to 5 seconds for the connection attempt to succeed. */
    if (enet_host_service(ClientHost, &Event, 500) > 0 && Event.type == ENET_EVENT_TYPE_CONNECT)
    {
        AddSystemMessage("Connection to localhost:1234 succeeded.");
    }
    else
    {
        /* Either the 5 seconds are up or a disconnect event was */
        /* received. Reset the peer in the event the 5 seconds   */
        /* had run out without any significant event.            */
        enet_peer_reset(PeerConnection);

        AddSystemMessage("Connection to localhost:1234 failed.");
    }
}

void stc::Client::ShutdownNetworkClient()
{
    ENetEvent event;
    enet_peer_disconnect(PeerConnection, 0);

    /* Allow up to 0.5 seconds for the disconnect to succeed
     * and drop any packets received packets.
     */
    while (enet_host_service(ClientHost, &event, 500) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_RECEIVE:
            enet_packet_destroy(event.packet);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            puts("Disconnection succeeded.");
            return;
        }
    }

    /* We've arrived here, so the disconnect attempt didn't */
    /* succeed yet.  Force the connection down.             */
    enet_peer_reset(PeerConnection);
}

void stc::Client::ProcessNetworkEvents()
{
    ENetEvent Event;
    char Name[] = "Aaa";

    while (enet_host_service(ClientHost, &Event, 0) > 0)
    {
        switch (Event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            AddSystemMessage(std::format("A new client connected from %x:%u.\n",
                Event.peer->address.host,
                Event.peer->address.port
            ));

            /* Store any relevant client information here. */
            Event.peer->data = Name;
            break;

        case ENET_EVENT_TYPE_RECEIVE:
            AddSystemMessage(std::format("A packet of length %zu containing %s was received from %s on channel %u.",
                Event.packet->dataLength,
                (char*)Event.packet->data,
                (char*)Event.peer->data,
                Event.channelID
            ));

            /* Clean up the packet now that we're done using it. */
            enet_packet_destroy(Event.packet);
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            AddSystemMessage(std::format("%s disconnected.\n", 
                (char*)Event.peer->data
            ));

            /* Reset the peer's client information. */
            Event.peer->data = NULL;
            break;
        }
    }
}

void stc::Client::Net_SendMessage(const std::string& Content)
{
    // Create flatbuffer packet.
    flatbuffers::FlatBufferBuilder NetBuilder(256);

    // Message
    auto Msg = NetBuilder.CreateString(Content);

    NetPackets::ClientChatMessageBuilder MessageBuilder(NetBuilder);
    MessageBuilder.add_content(Msg);
    auto MessagePacket = MessageBuilder.Finish();
    
    NetBuilder.Finish(MessagePacket);
    uint8_t* ChatMessageBuf = NetBuilder.GetBufferPointer();
    uint32_t ChatMessageSize = NetBuilder.GetSize();

    // Packet
    auto MsgPacket = NetBuilder.CreateVector(ChatMessageBuf, ChatMessageSize);

    NetPackets::STCPacketBuilder PacketBuilder(NetBuilder);
    PacketBuilder.add_type(2);
    PacketBuilder.add_content(MsgPacket);
    auto MainPacket = PacketBuilder.Finish();
    
    NetBuilder.Finish(MainPacket);
    uint8_t* PacketBuf = NetBuilder.GetBufferPointer();
    uint32_t PacketSize = NetBuilder.GetSize();
    
    // Create enet reliable packet
    ENetPacket* EnetPacket = enet_packet_create(
        PacketBuf,
        PacketSize,
        ENET_PACKET_FLAG_RELIABLE
    );

    // DEBUG: Try to decode message.
    auto OutPacket = NetPackets::GetSTCPacket(PacketBuf);
    auto PacketType = OutPacket->type();
    auto PacketContent = OutPacket->content();

    auto Message = ::flatbuffers::GetRoot<NetPackets::ClientChatMessage>(PacketContent->data());
    auto MessageContent = Message->content()->str();

    AddSystemMessage(std::format("DECODED MESSAGE: {} {}", PacketType, MessageContent));

    /* Extend the packet so and append the string "foo", so it now */
    /* contains "packetfoo\0"                                      */
    //enet_packet_resize(packet, strlen("packetfoo") + 1);
    //strcpy(&packet->data[strlen("packet")], "foo");

    /* Send the packet to the peer over channel id 0. */
    /* One could also broadcast the packet by         */
    /* enet_host_broadcast (host, 0, packet);         */
    enet_peer_send(PeerConnection, 0, EnetPacket);

    /* One could just use enet_host_service() instead. */
    enet_host_flush(ClientHost);
}

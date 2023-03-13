#include "Client.h"
#include <format>

#include "ftxui/component/loop.hpp"
#include "schemas/chatpacket_generated.h"
#include "common/Strings.h"


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

    /* Create packet type dispatcher */
    PacketTypeCallbacks = {
        {   1001,   std::bind(&stc::Client::OnServerAnnounce, this, std::placeholders::_1) }
        , { 1002,   std::bind(&stc::Client::OnServerUserAccepted, this, std::placeholders::_1) }
        , { 1003,   std::bind(&stc::Client::OnServerChatMessage, this, std::placeholders::_1) }
        , { 1004,   std::bind(&stc::Client::OnServerUserNickChange, this, std::placeholders::_1) }
        , { 1005,   std::bind(&stc::Client::OnServerUserJoined, this, std::placeholders::_1) }
    };
}

stc::Client::~Client()
{
    ShutdownNetworkClient();
    enet_host_destroy(ClientHost);
}

int stc::Client::Run()
{
    // Connect to server
    LocalUser = AddUser(User{ "You", 0 }); // And local user.
    SetupNetworkClient();

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
        ENetEvent Event;
        bool bHadEvent = false;
        while (enet_host_service(ClientHost, &Event, 0) > 0)
        {
            HandleEvent(Event);
            bHadEvent = true;
        }

        // Render.
        if (bHadEvent)
        {
            Screen.RequestAnimationFrame();
        }
        MainLoop.RunOnce();
        
        // Throttle.
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

	return 0;
}

bool stc::Client::OnMessageInput(ftxui::Event Event)
{
    if (Event == ftxui::Event::Return && InputMessage.size() > 0)
    {
        std::string InMessage = InputMessage;
        InputMessage.clear();

        const bool bCommand = (InMessage[0] == '/');
        if (bCommand)
        {
            auto CommandStr = InMessage.substr(1);

            auto Tokens = tools::SplitString(CommandStr);
            if (Tokens.size() < 1)
            {
                AddSystemMessage("Empty command.");
                return true;
            }

            if(Tokens[0] == "announce")
            {
                Net_SendServerInfoRequest();
                AddSystemMessage("Announce packet sent.");
            }
            else if (Tokens[0] == "join")
            {
                auto TmpKey = net::Key();
                Net_SendServerJoin(LocalUser->Nick, TmpKey);
                AddSystemMessage("Join packet sent.");
            }
            else if (Tokens[0] == "nick")
            {
                if (Tokens.size() < 2)
                {
                    AddSystemMessage("You passed empty nick. This is bad.");
                    return true;
                }

                auto Nick = Tokens[1];
                Net_SendServerChangeNick(Nick);
            }
            else
            {
                AddSystemMessage("Unknown command.");
                return true;
            }
        }
        else
        {
            Net_SendMessage(InMessage);
            AddMyMessage(InMessage);
        }

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

void stc::Client::HandleEvent(const ENetEvent& Event)
{
    static char DummyData[] = "Server";
    
    switch (Event.type)
    {
    case ENET_EVENT_TYPE_CONNECT:
        AddSystemDebugMessage(std::format("A new client connected from {}:{}.\n",
            Event.peer->address.host,
            Event.peer->address.port
        ));

        OnEventConnect(Event);

        /* Store any relevant client information here. */
        Event.peer->data = DummyData;
        break;

    case ENET_EVENT_TYPE_RECEIVE:
        //AddSystemDebugMessage(std::format("A packet of length {} received on channel {}.",
        //    Event.packet->dataLength,
        //    Event.channelID
        //));

        OnEventReceive(Event);

        /* Clean up the packet now that we're done using it. */
        enet_packet_destroy(Event.packet);
        break;

    case ENET_EVENT_TYPE_DISCONNECT:
        AddSystemDebugMessage(std::format("{} disconnected.\n",
            (char*)Event.peer->data
        ));

        OnEventDisconnect(Event);

        /* Reset the peer's client information. */
        Event.peer->data = nullptr;
        break;
    }
}

void stc::Client::OnEventReceive(const ENetEvent& Event)
{
    void* Data = Event.peer->data;

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
        CallbackFn(Packet);
    }
}

void stc::Client::OnEventConnect(const ENetEvent& Event)
{
    // We shouldn't receive connection request on client. Ignore.
}

void stc::Client::OnEventDisconnect(const ENetEvent& Event)
{
    void* Data = Event.peer->data;

    AddSystemMessage("Server disconnected.");
}

stc::User* stc::Client::AddUser(const User& user)
{
    Users.push_back(std::shared_ptr<User>(new User{ user }));
    return Users.back().get();
}

stc::User* stc::Client::AddUserIfNotExist(const User& user)
{
    User* User = GetUserByUID(user.UID);
    if (!User)
    {
        User = AddUser(user);
    }
    return User;
}

stc::User* stc::Client::GetUserByUID(const uint64_t& _UserUID)
{
    auto UserIt = std::find_if(Users.begin(), Users.end(), [&](const std::shared_ptr<User>& _UserPtr) { return _UserPtr->UID == _UserUID; });
    if (UserIt != Users.end())
    {
        return UserIt->get();
    }
    else
    {
        return nullptr;
    }
}

void stc::Client::ChangeUserNick(const uint64_t& _UserUID, const std::string& _Nick)
{
    // Check if this is info about our nick.
    if (_UserUID == LocalUser->UID)
    {
        LocalUser->Nick = _Nick;
        AddSystemMessage(std::format("Your nick was changed to '{}'", _Nick));
        return;
    }

    auto UserPtr = GetUserByUID(_UserUID);
    if (UserPtr)
    {
        // Change nick for user.
        AddSystemMessage(std::format("Change nick for user '{}' to '{}'.", UserPtr->Nick, _Nick));
        UserPtr->Nick = _Nick;
    }
    else
    {
        // Cannot find user.
        AddSystemDebugMessage(std::format("Cannot find user for nick change. UID={}; NICK={}", _UserUID, _Nick));
    }
}

void stc::Client::AddMessage(const Message::ShPtr& msg)
{
    Messages.push_back(msg);
}

void stc::Client::AddUserMessage(const User& _ChatUser, const std::string& _Content)
{
    const std::string& UserNickRef = _ChatUser.Nick;
    AddMessage(Message::ShPtr(new MessageUser(UserNickRef, _Content)));
}

void stc::Client::AddMyMessage(const std::string& _Content)
{
    AddMessage(Message::ShPtr(new MessageUser(LocalUser->Nick, _Content)));
}

void stc::Client::AddSystemMessage(const std::string& Content)
{
    AddMessage(Message::ShPtr(new MessageStatic("SYSTEM", Content)));
}

inline void stc::Client::AddSystemDebugMessage(const std::string& Content)
{
#ifdef _DEBUG
    AddMessage(Message::ShPtr(new MessageStatic("SYSTEM DEBUG", Content)));
#endif
}

#pragma once
#include <map>
#include <vector>
#include <string>
#include <memory>

#include "enet_wrapper.h"
#include "common/NetworkHelpers.h"

namespace stc
{
    struct ChatMessage
    {
        uint32_t Timestamp;
        uint32_t UserUID;
        std::string Message;
    };


    struct ChatLog
    {
        std::vector<ChatMessage> Messages;
    };


    struct ChatUser
    {
        uint64_t    UID;
        std::string Name;

        ENetPeer*   Peer;
        net::Key    NetKey;
    };


    struct Server
    {
    public:
        static constexpr float SERVER_TICK = 0.1f;

    public:
        Server();
        virtual ~Server();

        bool InitConnection();

        void Run();
        void Stop() { bRun = false; }

    private:
        void HandleEvent(const ENetEvent& Event);

        // Incoming events
        void OnEventReceive(const ENetEvent& Event);
        void OnEventConnect(const ENetEvent& Event);
        void OnEventDisconnect(const ENetEvent& Event);

        // Incoming packets
        void OnClientServerInfoRequest(net::PacketWrapperDecoder& Packet, ChatUser* User);
        void OnClientJoinServer(net::PacketWrapperDecoder& Packet, ChatUser* User);
        void OnClientChatMessage(net::PacketWrapperDecoder& Packet, ChatUser* User);
        void OnClientRequestNickChange(net::PacketWrapperDecoder& Packet, ChatUser* User);

        // Outgoing packets
        //void Net_SendAnnounce();
        void Net_SendUserAccepted(ChatUser* User);

        void Net_BroadcastUserJoined(ChatUser* User);
        void Net_BroadcastChatMessage(const std::string& Message, const ChatUser* User);
        void Net_BroadcastUserChangedNick(const std::string& Nick, ChatUser* User);

        // Outgoing packets helpers
        void Net_BroadcastAll(const net::PacketWrapper& Packet, const ChatUser* ExceptUser = nullptr);

    private:
        ChatUser* CreateNewTempUser(ENetPeer* Peer);
        bool IsNickValid(const std::string& Nick);

    private:
        std::map<uint32_t, std::function<void(net::PacketWrapperDecoder&,ChatUser*)>> PacketTypeCallbacks;
        std::map<std::string, ChatLog>  Channels;

        std::vector<std::shared_ptr<ChatUser>> TempUsers;
        std::vector<std::shared_ptr<ChatUser>> ChannelUsers;

        ENetHost* ServerHost;
        bool bRun;

        // Server info
        std::string ServerName;
        net::Key    PublicKey;
    };
}

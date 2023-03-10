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
        uint32_t UID;
        std::string Name;

        ENetPeer* Peer;
        uint64_t Secret;
        uint64_t LastActive;
    };



    struct Server
    {
    public:
        static constexpr int SERVER_PORT = 1234;
        static constexpr float SERVER_TICK = 0.1f;

    public:
        Server();
        virtual ~Server();

        bool InitConnection();

        void Run();
        void Stop() { bRun = false; }

    private:
        void HandleEvent(const ENetEvent& Event);

        // Incoming packets
        void OnEventReceive(const ENetEvent& Event);
        void OnEventConnect(const ENetEvent& Event);
        void OnEventDisconnect(const ENetEvent& Event);

        void OnClientJoinServer(net::PacketWrapper& Packet);
        void OnClientChatMessage(net::PacketWrapper& Packet);

        // Outgoing packets
        void BroadcastAll(const ChatUser* ExceptUser = nullptr);

    private:
        ChatUser* CreateNewTempUser(ENetPeer* Peer);

    private:
        std::map<std::string, ChatLog>  Channels;

        std::vector<std::shared_ptr<ChatUser>> TempUsers;
        std::vector<std::shared_ptr<ChatUser>> ChannelUsers;

        ENetHost* ServerHost;
        bool bRun;
    };
}

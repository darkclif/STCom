#pragma once
#include <stdlib.h>  // for EXIT_SUCCESS
#include <memory>    // for allocator, __shared_ptr_access
#include <string>  // for string, operator+, basic_string, to_string, char_traits
#include <vector>  // for vector, __alloc_traits<>::value_type
#include <map>  // for vector, __alloc_traits<>::value_type

#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component.hpp"  // for Menu, Renderer, Horizontal, Vertical
#include "ftxui/component/component_base.hpp"  // for ComponentBase
#include "ftxui/component/screen_interactive.hpp"  // for Component, ScreenInteractive
#include "ftxui/dom/elements.hpp"  // for text, Element, operator|, window, flex, vbox

#include "enet_wrapper.h"
#include "client/Messages.h"
#include "common/Timer.h"
#include "common/NetworkHelpers.h"

namespace stc
{
    struct ChatLogUI
    {
    public:
        ChatLogUI(const std::vector<Message::ShPtr>& _Messages):
            Messages{_Messages}
        {
        }

    public:
        ftxui::Element Render() {
            ftxui::Elements elements;

            for (const Message::ShPtr& Msg: Messages) {
                //elements.push_back(ftxui::hbox({ ftxui::text(Msg.Nick), ftxui::text(Msg.Content) }));
                elements.push_back(ftxui::text(Msg->Print()));
            }

            return ftxui::window(ftxui::text("Chat"), ftxui::vbox(std::move(elements))) | ftxui::flex;
        }

    private:
        const std::vector<Message::ShPtr>& Messages;
    };

    struct User
    {
        std::string Nick;
        uint64_t UID = 0;
        bool bConnected = true;
    };

    class UsersListUI
    {
    public:
        UsersListUI(const std::vector<std::shared_ptr<User>>& _Users):
            Users{_Users}
        {
        
        }

    public:
        ftxui::Element Render() {
            ftxui::Elements elements;

            for (const std::shared_ptr<User>& User: Users) {
                elements.push_back(ftxui::text(User->Nick));
            }

            return ftxui::window(ftxui::text("Users"), ftxui::vbox(std::move(elements)));
        }

    private:
        const std::vector<std::shared_ptr<User>>& Users;
    };

    class Client
    {
    public:
        Client();
        virtual ~Client();

        int Run();
        void Stop() { bRun = false; }

    private:
        bool OnMessageInput(ftxui::Event Event);
        bool OnGlobalInput(ftxui::Event Event);

    private:
        /*
        *   Network.
        */ 
        void SetupNetworkClient();
        void ShutdownNetworkClient();

        void HandleEvent(const ENetEvent& Event);

        // Incoming events
        void OnEventReceive(const ENetEvent& Event);
        void OnEventConnect(const ENetEvent& Event);
        void OnEventDisconnect(const ENetEvent& Event);

        // Incoming packets
        void OnServerAnnounce(net::PacketWrapperDecoder& Packet);
        void OnServerUserAccepted(net::PacketWrapperDecoder& Packet);
        void OnServerChatMessage(net::PacketWrapperDecoder& Packet);
        void OnServerUserNickChange(net::PacketWrapperDecoder& Packet);
        void OnServerUserJoined(net::PacketWrapperDecoder& Packet);

        // Outgoing packets
        void Net_SendServerInfoRequest();
        void Net_SendServerJoin(const std::string& Content, const net::Key& Key);
        void Net_SendMessage(const std::string& Content);
        void Net_SendServerChangeNick(const std::string& Nick);

    private:
        User* AddUser(const User& user);
        User* AddUserIfNotExist(const User& user);
        User* GetUserByUID(const uint64_t& _UserUID);
        
        void ChangeUserNick(const uint64_t& _UserUID, const std::string& _Nick);

        void AddMessage(const Message::ShPtr& _Msg);
        void AddUserMessage(const User& _ChatUser, const std::string& _Content);
        void AddMyMessage(const std::string& _Content);
        void AddSystemMessage(const std::string& Content);
        inline void AddSystemDebugMessage(const std::string& Content);

    private:
        std::map<uint32_t, std::function<void(net::PacketWrapperDecoder&)>> PacketTypeCallbacks;
        
        std::vector<std::shared_ptr<User>> Users;
        std::vector<std::shared_ptr<Message>> Messages;

        std::shared_ptr<stc::ChatLogUI> ChatLog;
        std::shared_ptr<stc::UsersListUI> UserList;
        std::string InputMessage;

        tools::Timer ShutdownTimer;
        bool bRun;

        /*
        *   Network.
        */
        ENetHost* ClientHost;
        ENetPeer* PeerConnection;
        
        User* LocalUser = nullptr;
    };
}


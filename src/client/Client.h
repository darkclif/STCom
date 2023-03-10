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
#include "common/Timer.h"

namespace stc
{
    struct Message
    {
        uint64_t Timestamp;
        std::string Nick;
        std::string Content;
    };

    struct ChatLogUI
    {
    public:
        ChatLogUI(const std::vector<Message>& _Messages):
            Messages{_Messages}
        {
        }

    public:
        ftxui::Element Render() {
            ftxui::Elements elements;

            for (const Message& Msg: Messages) {
                //elements.push_back(ftxui::hbox({ ftxui::text(Msg.Nick), ftxui::text(Msg.Content) }));
                elements.push_back(ftxui::text(Msg.Nick + ": " + Msg.Content));
            }

            return ftxui::window(ftxui::text("Chat"), ftxui::vbox(std::move(elements))) | ftxui::flex;
        }

    private:
        const std::vector<Message>& Messages;
    };

    struct User
    {
        std::string Nick;
        bool bConnected = true;
        uint64_t UID = 0;
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

        void ProcessNetworkEvents();

        // In
        void OnNetUserJoined() {};

        // Out
        void Net_SendMessage(const std::string& Content);

    private:
        void AddUser(const User& user) {
            Users.push_back(std::shared_ptr<User>(new User{user}));
        }

        void AddMessage(const Message& msg) {
            Messages.push_back(msg);
        }

        void AddSystemMessage(const std::string& msg) 
        {
            Messages.push_back(Message{0, "SYSTEM", msg});
        }

    private:
        std::vector<std::shared_ptr<User>> Users;
        std::vector<Message> Messages;

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

        uint64_t SesionToken = 0;
        uint64_t UserUID = 0;
    };
}


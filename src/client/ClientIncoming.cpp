#include "Client.h"
#include "schemas/chatpacket_generated.h"


void stc::Client::OnServerAnnounce(net::PacketWrapperDecoder& Packet)
{
    auto* Req = Packet.GetInternalPacket<NetPackets::ServerAnnounce>();

    auto Name = Req->name()->str();
    auto PKey = net::Key(std::vector<uint8_t>(Req->pub_key()->begin(), Req->pub_key()->end()));

    AddSystemMessage(std::format("Server announce: {} / {}", Name, PKey.GetString()));
}

void stc::Client::OnServerUserAccepted(net::PacketWrapperDecoder& Packet)
{
    auto* Req = Packet.GetInternalPacket<NetPackets::ServerUserAccepted>();

    // Accept local UID
    const uint64_t LocalUserUID = Req->user_uid();
    LocalUser->UID = LocalUserUID;
    
    // Add channel users
    uint32_t UsersInfoSize = 0;
    if (Req->users())
    {
        UsersInfoSize = Req->users()->size();
        for (const auto UserIt : *Req->users())
        {
            auto NewUser = User{ UserIt->nick()->str(), UserIt->user_uid() };
            AddUserIfNotExist(NewUser);
        }
    }
    
    AddSystemDebugMessage(std::format("User accepted by a server. (UID={}). Received info about users. (Count={})", LocalUserUID, UsersInfoSize));

    AddSystemMessage("You joined General channel.");
    AddSystemMessage(std::format("Active users: {}.", UsersInfoSize));
}

void stc::Client::OnServerChatMessage(net::PacketWrapperDecoder& Packet)
{
    auto* Req = Packet.GetInternalPacket<NetPackets::ServerChatMessage>();

    auto MessageContent = Req->content()->str();
    auto MessageUserUID = Req->user_uid();

    // Ignore message when it's our message.
    if (LocalUser->UID != 0 && MessageUserUID == LocalUser->UID)
    {
        return;
    }

    auto UserPtr = GetUserByUID(MessageUserUID);
    if (!UserPtr)
    {
        UserPtr = AddUser(User{ "???", MessageUserUID });
        AddSystemDebugMessage(std::format("Received message from unknown user. Adding temporary user. (Nick={}, UID={})", UserPtr->Nick, UserPtr->UID));
    }
    AddUserMessage(*UserPtr, MessageContent);
}

void stc::Client::OnServerUserNickChange(net::PacketWrapperDecoder& Packet)
{
    auto* Req = Packet.GetInternalPacket<NetPackets::ServerUserChangedNick>();

    auto UserUID = Req->user_uid();
    auto UserNick = Req->nick()->str();

    ChangeUserNick(UserUID, UserNick);
}

void stc::Client::OnServerUserJoined(net::PacketWrapperDecoder& Packet)
{
    auto* Req = Packet.GetInternalPacket<NetPackets::ServerUserJoined>();

    auto UserUID = Req->user_uid();
    auto UserNick = Req->nick()->str();
    
    auto UserPtr = GetUserByUID(UserUID);
    if (!UserPtr)
    {
        AddUser(User{ UserNick, UserUID });
        AddSystemMessage(std::format("{} joined.", UserNick));
    }
    else
    {
        AddSystemDebugMessage(std::format("User (Nick={}, UID={}) already joined. Skipping.", UserNick, UserUID));
    }
}

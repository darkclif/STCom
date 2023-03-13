#include "Server.h"
#include "schemas/chatpacket_generated.h"

void stc::Server::Net_BroadcastAll(const net::PacketWrapper& Packet, const ChatUser* ExceptUser)
{
    for (auto& User : ChannelUsers)
    {
        if (User.get() == nullptr || User.get() == ExceptUser)
        {
            continue;
        }

        net::Net_Send(Packet, ServerHost, User->Peer);
    }
}

void stc::Server::Net_SendUserAccepted(ChatUser* User)
{
    // Send info
    flatbuffers::FlatBufferBuilder NetBuilder(256);

    // Populate current users info
    std::vector<flatbuffers::Offset<NetPackets::UserInfo>> UserInfoVector;
    for (auto& UserPtr : ChannelUsers)
    {
        auto FlatNick = NetBuilder.CreateString(UserPtr->Name);

        NetPackets::UserInfoBuilder OutUser(NetBuilder);
        OutUser.add_nick(FlatNick);
        OutUser.add_user_uid(UserPtr->UID);

        auto FlatUser = OutUser.Finish();
        UserInfoVector.push_back(FlatUser);
    }
    auto FlatUsers = NetBuilder.CreateVector(UserInfoVector);

    // Assemble nested packet
    NetPackets::ServerUserAcceptedBuilder OutPacket(NetBuilder);
    OutPacket.add_user_uid(User->UID);
    OutPacket.add_users(FlatUsers);
    
    auto PacketFinal = net::CreateNestedPacket(NetBuilder, 1002, OutPacket);
    net::Net_Send(PacketFinal, ServerHost, User->Peer);
}

void stc::Server::Net_BroadcastUserJoined(ChatUser* User)
{
    flatbuffers::FlatBufferBuilder NetBuilder(256);

    auto FlatNick = NetBuilder.CreateString(User->Name);

    NetPackets::ServerUserJoinedBuilder OutPacket(NetBuilder);
    OutPacket.add_user_uid(User->UID);
    OutPacket.add_nick(FlatNick);

    // Finnalize
    auto PacketFinal = net::CreateNestedPacket(NetBuilder, 1005, OutPacket);
    Net_BroadcastAll(PacketFinal, User /*Exclude sender*/);
}

void stc::Server::Net_BroadcastChatMessage(const std::string& Message, const ChatUser* User)
{
    flatbuffers::FlatBufferBuilder NetBuilder(256);

    auto MessageContent = NetBuilder.CreateString(Message);

    NetPackets::ServerChatMessageBuilder OutPacket(NetBuilder);
    OutPacket.add_content(MessageContent);
    OutPacket.add_user_uid(User->UID);

    // Finnalize
    auto PacketFinal = net::CreateNestedPacket(NetBuilder, 1003, OutPacket);
    Net_BroadcastAll(PacketFinal, User /*Exclude sender*/);
}

void stc::Server::Net_BroadcastUserChangedNick(const std::string& Nick, ChatUser* User)
{
    flatbuffers::FlatBufferBuilder NetBuilder(256);

    auto FlatNick = NetBuilder.CreateString(Nick);

    NetPackets::ServerUserChangedNickBuilder OutPacket(NetBuilder);
    OutPacket.add_nick(FlatNick);
    OutPacket.add_user_uid(User->UID);

    // Finnalize
    auto PacketFinal = net::CreateNestedPacket(NetBuilder, 1004, OutPacket);
    Net_BroadcastAll(PacketFinal);
}

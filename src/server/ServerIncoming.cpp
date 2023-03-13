#include "Server.h"
#include "schemas/chatpacket_generated.h"
#include "common/Random.h"


void stc::Server::OnClientServerInfoRequest(net::PacketWrapperDecoder& Packet, ChatUser* User)
{
    flatbuffers::FlatBufferBuilder NetBuilder(256);

    auto Name = NetBuilder.CreateString(ServerName);
    auto PubKey = NetBuilder.CreateVector(std::vector<uint8_t>({ 1,2,3,4 }));

    NetPackets::ServerAnnounceBuilder OutPacket(NetBuilder);
    OutPacket.add_name(Name);
    OutPacket.add_pub_key(PubKey);

    // Finnalize
    auto PacketFinal = net::CreateNestedPacket(NetBuilder, 1001, OutPacket);
    net::Net_Send(PacketFinal, ServerHost, User->Peer);
    printf("Sent announce.\n");
}

void stc::Server::OnClientJoinServer(net::PacketWrapperDecoder& Packet, ChatUser* User)
{
    auto* Req = Packet.GetInternalPacket<NetPackets::ClientJoinServer>();

    // Move user from temp users to channel users.
    auto TmpUserIt = std::find_if(TempUsers.begin(), TempUsers.end(), [User](std::shared_ptr<ChatUser> const& _Ptr) { return _Ptr.get() == User; });
    if (TmpUserIt == TempUsers.end())
    {
        printf("Cannot find temporary user.\n");
        return;
    }
    else
    {
        // Move user to channel.
        ChannelUsers.push_back(*TmpUserIt);
        TempUsers.erase(TmpUserIt);

        // Fill data
        User->Name = Req->nick()->str();
        User->NetKey = net::Key(std::vector<uint8_t>(Req->pub_key()->begin(), Req->pub_key()->end()));
        User->UID = stc::GetRandomUID64();

        // Inform
        Net_BroadcastUserJoined(User);
        Net_SendUserAccepted(User);
    }
}

void stc::Server::OnClientChatMessage(net::PacketWrapperDecoder& Packet, ChatUser* User)
{
    auto* Req = Packet.GetInternalPacket<NetPackets::ClientChatMessage>();

    auto TmpUserIt = std::find_if(ChannelUsers.begin(), ChannelUsers.end(), [User](std::shared_ptr<ChatUser> const& _Ptr) { return _Ptr.get() == User; });
    if (TmpUserIt == ChannelUsers.end())
    {
        printf("User not joined a channel.\n");
        return;
    }

    // Broadcast message to other users except sender.
    Net_BroadcastChatMessage(Req->content()->str(), User);
}

void stc::Server::OnClientRequestNickChange(net::PacketWrapperDecoder& Packet, ChatUser* User)
{
    auto* Req = Packet.GetInternalPacket<NetPackets::ClientRequestNickChange>();

    auto Nick = Req->nick()->str();
    const bool bNickValid = IsNickValid(Nick);
    if (!bNickValid)
    {
        // TODO: Send error packet to this user.
        return;
    }

    Net_BroadcastUserChangedNick(Nick, User);
}
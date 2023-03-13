#include "Client.h"
#include "schemas/chatpacket_generated.h"

void stc::Client::Net_SendServerInfoRequest()
{
    flatbuffers::FlatBufferBuilder NetBuilder(256);

    // Finnalize
    auto PacketFinal = net::CreateTypePacket(NetBuilder, 0001);
    net::Net_Send(PacketFinal, ClientHost, PeerConnection);
}

void stc::Client::Net_SendServerJoin(const std::string& Nick, const net::Key& Key)
{
    flatbuffers::FlatBufferBuilder NetBuilder(256);
    auto FlatNick = NetBuilder.CreateString(Nick);
    auto FlatKey = NetBuilder.CreateVector(Key.GetData());

    NetPackets::ClientJoinServerBuilder NestedBuilder(NetBuilder);
    NestedBuilder.add_nick(FlatNick);
    NestedBuilder.add_pub_key(FlatKey);

    // Finnalize
    auto PacketFinal = net::CreateNestedPacket(NetBuilder, 0002, NestedBuilder);
    net::Net_Send(PacketFinal, ClientHost, PeerConnection);
}

void stc::Client::Net_SendMessage(const std::string& Content)
{
    flatbuffers::FlatBufferBuilder NetBuilder(256);
    auto Msg = NetBuilder.CreateString(Content);

    NetPackets::ClientChatMessageBuilder NestedBuilder(NetBuilder);
    NestedBuilder.add_content(Msg);

    // Finnalize
    auto PacketFinal = net::CreateNestedPacket(NetBuilder, 0003, NestedBuilder);
    net::Net_Send(PacketFinal, ClientHost, PeerConnection);
}

void stc::Client::Net_SendServerChangeNick(const std::string& Nick)
{
    flatbuffers::FlatBufferBuilder NetBuilder(256);
    auto FlatNick = NetBuilder.CreateString(Nick);

    NetPackets::ClientRequestNickChangeBuilder NestedBuilder(NetBuilder);
    NestedBuilder.add_nick(FlatNick);

    // Finnalize
    auto PacketFinal = net::CreateNestedPacket(NetBuilder, 0004, NestedBuilder);
    net::Net_Send(PacketFinal, ClientHost, PeerConnection);
}

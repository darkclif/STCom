#include "NetworkHelpers.h"

void stc::net::PacketWrapperDecoder::DecodePacket()
{
    auto OutPacket = NetPackets::GetSTCPacket(Buffer);

    PacketType = OutPacket->type();
    PacketContent = OutPacket->content();
}

stc::net::PacketWrapper stc::net::CreateTypePacket(flatbuffers::FlatBufferBuilder& Builder, uint32_t Type)
{
	// Wrap nested packet.
	NetPackets::STCPacketBuilder STCPacketBuilder(Builder);
	STCPacketBuilder.add_type(Type);
	auto MainPacket = STCPacketBuilder.Finish();

	Builder.Finish(MainPacket);
	uint8_t* PacketBuf = Builder.GetBufferPointer();
	uint32_t PacketSize = Builder.GetSize();

	return stc::net::PacketWrapper(PacketBuf, PacketSize);
}

void stc::net::Net_Send(const net::PacketWrapper& Packet, ENetHost* Host, ENetPeer* Peer)
{
	ENetPacket* EnetPacket = enet_packet_create(
		Packet.GetBuffer(),
		Packet.GetSize(),
		ENET_PACKET_FLAG_RELIABLE
	);

	enet_peer_send(Peer, 0, EnetPacket);
	enet_host_flush(Host);
}

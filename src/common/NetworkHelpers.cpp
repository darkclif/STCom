#include "NetworkHelpers.h"

void stc::net::PacketWrapper::DecodePacket()
{
    auto OutPacket = NetPackets::GetSTCPacket(Buffer);

    PacketType = OutPacket->type();
    PacketContent = OutPacket->content();
}



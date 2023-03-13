#pragma once
#include <cstdint>
#include <format>
#include <sstream>

#include <enet/enet.h>
#include "schemas/chatpacket_generated.h"

namespace stc::net 
{
	/*
	*	Raw packet wrapper.
	*/
	class PacketWrapper
	{
	public:
		PacketWrapper(uint8_t* _Buffer = nullptr, size_t _Size = 0) :
			Buffer{ _Buffer }
			, Size{ _Size }
		{
		};

	public:
		inline const uint8_t* GetBuffer() const { return Buffer; };
		inline size_t GetSize() const { return Size; };

	protected:
		/* Raw enet packet */
		uint8_t* Buffer = nullptr;
		size_t Size = 0;
	};

	/*
	*	Helper class for extracting packet type and nested internal packet.
	*/
	class PacketWrapperDecoder : public PacketWrapper 
	{
	public:
		PacketWrapperDecoder(uint8_t* _Buffer, size_t _Size) :
			PacketWrapper(_Buffer, _Size)
		{
			DecodePacket();
		};

	public:
		inline uint16_t GetType() { return PacketType; };

		template<typename T>
		const T* GetInternalPacket()
		{
			// Check if PacketType = T::PacketId, otherwise it will fail.
			return ::flatbuffers::GetRoot<T>(PacketContent->data());
		}

	private:
		void DecodePacket();

	private:
		/* Decoded structures */
		uint16_t PacketType = 0;
		const ::flatbuffers::Vector<uint8_t>* PacketContent = nullptr;
	};

	class Key
	{
	public:
		Key() {};
		Key(std::vector<uint8_t> _KeyData) : KeyData{ _KeyData } {};
		Key(uint8_t* _Buff, size_t _Size) : KeyData{ _Buff, _Buff + _Size } {};

	public:
		std::string GetString()
		{
			std::stringstream ss;
			for (auto& Byte : KeyData)
			{
				ss << std::format("{:x}{:x}", (Byte & 0xf0) >> 4, Byte & 0x0f);
			}
			return ss.str();
		}

		const std::vector<uint8_t>& GetData() const { return KeyData; };

	private:
		std::vector<uint8_t> KeyData;
	};

	template<typename T>
	stc::net::PacketWrapper CreateNestedPacket(flatbuffers::FlatBufferBuilder& Builder, uint32_t Type, T& NestedPacketBuilder)
	{
		// Finnalize nested packet.
		auto NestedPacket = NestedPacketBuilder.Finish();
		Builder.Finish(NestedPacket);

		uint8_t* NestedPacketBuf = Builder.GetBufferPointer();
		uint32_t NestedPacketSize = Builder.GetSize();

		// Wrap nested packet.
		auto MsgPacket = Builder.CreateVector(NestedPacketBuf, NestedPacketSize);

		NetPackets::STCPacketBuilder STCPacketBuilder(Builder);
		STCPacketBuilder.add_type(Type);
		STCPacketBuilder.add_content(MsgPacket);
		auto MainPacket = STCPacketBuilder.Finish();

		Builder.Finish(MainPacket);
		uint8_t* PacketBuf = Builder.GetBufferPointer();
		uint32_t PacketSize = Builder.GetSize();

		return stc::net::PacketWrapper(PacketBuf, PacketSize);
	}

	stc::net::PacketWrapper CreateTypePacket(flatbuffers::FlatBufferBuilder& Builder, uint32_t Type);
	void Net_Send(const net::PacketWrapper& Packet, ENetHost* Host, ENetPeer* Peer);

}
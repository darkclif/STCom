#pragma once
#include <cstdint>
#include "schemas/chatpacket_generated.h"

namespace stc::net 
{
	class PacketWrapper
	{
	public:
		PacketWrapper(uint8_t* _Buffer, size_t _Size) :
			Buffer{ _Buffer }
			, Size{ _Size }
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

	private:
		/* Raw enet packet */
		uint8_t* Buffer = nullptr;
		size_t Size = 0;
	};

}
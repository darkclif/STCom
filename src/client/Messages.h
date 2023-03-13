#pragma once
#include <cstdint>
#include <string>
#include <memory>
#include <format>

namespace stc
{
    /*
    *   Base class for sotring chat message.
    */
    class Message
    {
    public:
        typedef std::shared_ptr<Message> ShPtr;

    public:
        Message(const std::string& _Content, uint64_t _Timestamp = 0) :
            Content{ _Content }
            , Timestamp{ _Timestamp }
        {
        };

        virtual std::string Print() = 0;

    protected:
        const uint64_t Timestamp;
        std::string Content;
    };

    /*
    *   Static class with nick and content that does not change in time.
    */
    class MessageStatic : public Message
    {
    public:
        MessageStatic(const std::string& _Nick, std::string _Content) :
            Message(_Content)
            , Nick{ _Nick }
        {
        };

        std::string Print() override
        {
            return std::format("{}: {}", Nick, Content);
        }

    private:
        std::string Nick;
    };

    class MessageUser : public Message
    {
    public:
        MessageUser(const std::string& _NickRef, std::string _Content) :
            Message(_Content)
            , NickRef{ _NickRef }
        {
        };

        std::string Print() override
        {
            return std::format("{}: {}", NickRef, Content);
        }

    private:
        //uint64_t UserUID;
        const std::string& NickRef;
    };
}

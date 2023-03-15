#ifndef HW2_MESSAGE_H
#define HW2_MESSAGE_H

#include <enet/enet.h>
#include <cstring>
#include <string_view>

enum class MessageType {
    TEXT,
    COMMAND,
    SERVER_INFO,
    SYSTEM_TIME
};

struct CookedMessage {
    uint8_t* data;
    size_t len;

    ~CookedMessage() {
        delete[] data;
    }
};

struct CommandMessage {
    std::string_view cmd;
    explicit CommandMessage(const ENetPacket* a_packet): cmd(reinterpret_cast<const char*>(a_packet->data + 1), a_packet->dataLength - 1) { }

    static CookedMessage Create(const std::string_view a_str) {
        auto* data = new uint8_t[a_str.size() + 1];
        data[0] = static_cast<uint8_t>(MessageType::COMMAND);
        memcpy(data+1, a_str.data(), a_str.size());
        return {data, a_str.size()+1};
    }
};

struct TextMessage {
    std::string_view txt;
    explicit TextMessage(const ENetPacket* a_packet): txt(reinterpret_cast<const char*>(a_packet->data + 1), a_packet->dataLength - 1) { }

    static CookedMessage Create(const std::string_view a_str) {
        auto* data = new uint8_t[a_str.size() + 1];
        data[0] = static_cast<uint8_t>(MessageType::TEXT);
        memcpy(data+1, a_str.data(), a_str.size());
        return {data, a_str.size()+1};
    }
};

struct ServerInfoMessage {
    std::string_view address;
    uint16_t port;

    explicit ServerInfoMessage(const ENetPacket* a_packet):
        port(*reinterpret_cast<uint16_t *>(a_packet->data+1)),
        address(reinterpret_cast<const char *>(a_packet->data+1+sizeof (uint16_t)), a_packet->dataLength-1-sizeof (uint16_t))
        { }

    static CookedMessage Create(const std::string_view a_addr, uint16_t a_port) {
        auto* data = new uint8_t[1 + sizeof(a_port) + a_addr.size()];
        data[0] = static_cast<uint8_t>(MessageType::SERVER_INFO);
        memcpy(data+1, &a_port, sizeof(a_port));
        memcpy(data+1+sizeof(a_port), a_addr.data(),  a_addr.size());
        return {data, 1 + sizeof(a_port) + a_addr.size()};
    }

};

struct SystemTimeMessage {
    uint32_t unixTime;
    
    explicit SystemTimeMessage(const ENetPacket* a_packet): unixTime(*reinterpret_cast<uint32_t*>(a_packet->data + 1)) {}

    static CookedMessage Create(const uint32_t a_unixTime) {
        auto* data = new uint8_t[sizeof(a_unixTime) + 1];
        data[0] = static_cast<uint8_t>(MessageType::SYSTEM_TIME);
        memcpy(data+1, reinterpret_cast<const char*>(&a_unixTime), sizeof(a_unixTime));
        return {data, sizeof(a_unixTime)+1};
    }
};


MessageType getType(const uint8_t * data) {
    return static_cast<MessageType>(data[0]);
}

#endif //HW2_MESSAGE_H

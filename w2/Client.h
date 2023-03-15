#ifndef HW2_CLIENT_H
#define HW2_CLIENT_H

#include <enet/enet.h>
#include <string>

enum class RemoteType {
    None,
    Lobby,
    Server
};

class Client {
public:
    Client(const std::string& a_address, enet_uint16 port);

    void Serve();
private:

    bool ConnectToRemote();
    void ServeRemote();

    void HandleReceive(ENetEvent& a_event);

    bool m_clientRunning = true;
    RemoteType m_currentRemote = RemoteType::None;
    ENetHost* m_host;
    ENetPeer* m_remotePeer{};
    ENetAddress m_remoteAddress{};

    std::thread m_spammer;
};


#endif //HW2_CLIENT_H

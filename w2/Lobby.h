#ifndef HW2_LOBBY_H
#define HW2_LOBBY_H

#include <enet/enet.h>

#include <unordered_set>
#include <string>

class Lobby {
public:
    Lobby(enet_uint16 a_port, std::string  a_serverAddr, uint16_t a_serverPort);

    void Serve();

private:
    void HandleConnect(ENetPeer* a_newPeer);
    void HandleReceive(ENetEvent& a_event);
    void SendServerInfo(ENetPeer* a_peer);

    ENetHost* m_host;
    std::unordered_set<ENetPeer*> m_awaitingPlayers;
    bool m_serverRunning = false;
    std::string m_serverAddr;
    uint16_t m_serverPort;

    void StartServer();
};


#endif //HW2_LOBBY_H

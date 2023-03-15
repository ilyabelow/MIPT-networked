#include "Lobby.h"
#include "Message.h"
#include <iostream>
#include <thread>
#include <utility>

Lobby::Lobby(enet_uint16 a_port, std::string a_serverAddr, uint16_t a_serverPort):
        m_serverPort(a_serverPort), m_serverAddr(std::move(a_serverAddr)) {
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = a_port;

    m_host = enet_host_create(&address, 32, 2, 0, 0);
    if (m_host == nullptr)
    {
        std::cerr << "An error occurred while initializing host, exiting\n";
        exit(EXIT_FAILURE);
    }
}

void Lobby::Serve() {
    bool lobbyRunning = true;
    std::cout << "lobby commands:\nstop - disconnect all clients and stop lobby\nreset - start queuing players again\nstart - start server prematurely\n===\n";
    std::thread([&lobbyRunning, this]() {
        while (true) {
            std::string cmd;
            std::cin >> cmd;
            if (cmd == "stop") {
                for (auto* peer : m_awaitingPlayers) {
                    enet_peer_disconnect_now(peer, 0);
                }
                lobbyRunning = false;
                std::cout << "lobby stopped\n";
                break;
            } else if (cmd == "reset"){
                if (m_serverRunning) {
                    m_serverRunning = false;
                    std::cout << "lobby is waiting for players again\n";
                } else {
                    std::cout << "server is not yet running\n";
                }
            }  else if (cmd == "start"){
                if (m_serverRunning) {
                    std::cout << "server is already started\n";
                } else {
                    StartServer();
                    std::cout << "server started\n";
                }
            }else {
                std::cout << "unknown command\n";
            }
        }
    }).detach();

    ENetEvent event;
    while (lobbyRunning)
    {
        while (enet_host_service(m_host, &event, 0) > 0)
        {
            switch (event.type)
            {
                case ENET_EVENT_TYPE_CONNECT:
                    HandleConnect(event.peer);
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    HandleReceive(event);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    m_awaitingPlayers.erase(event.peer);
                    break;
                default:
                    break;
            }
        }
    }
}

void Lobby::HandleConnect(ENetPeer *a_newPeer) {
    if (m_serverRunning) {
        SendServerInfo(a_newPeer);
    } else {
        CookedMessage msg = TextMessage::Create("===\nWelcome to our super-duper lobby! Available commands:\nplayers - print current awaiting player count\nstart - start a game\n===");
        ENetPacket * packet = enet_packet_create (msg.data, msg.len, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(a_newPeer, 0, packet);
        m_awaitingPlayers.insert(a_newPeer);
    }
}

void Lobby::HandleReceive(ENetEvent &a_event) {
    auto* incomingPacket = a_event.packet;
    if (getType(incomingPacket->data) == MessageType::COMMAND) {
        CommandMessage incomingMsg(incomingPacket);
        if (incomingMsg.cmd == "players") {
            CookedMessage msg = TextMessage::Create(std::to_string(m_awaitingPlayers.size()));
            ENetPacket * packet = enet_packet_create (msg.data, msg.len, ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(a_event.peer, 0, packet);
        } else if (incomingMsg.cmd == "start") {
            StartServer();
        }
    }
    enet_packet_destroy(a_event.packet);
}

void Lobby::SendServerInfo(ENetPeer *a_peer) {
    CookedMessage msg = ServerInfoMessage::Create(m_serverAddr, m_serverPort);
    ENetPacket * packet = enet_packet_create (msg.data, msg.len, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(a_peer, 0, packet);
}

void Lobby::StartServer() {
    for (auto* peer : m_awaitingPlayers) {
        SendServerInfo(peer);
    }
    m_serverRunning = true;
}


int main() {
    enet_initialize();
    Lobby lobby(12345, "localhost", 12346);
    lobby.Serve();
    enet_deinitialize();
    return 0;
}



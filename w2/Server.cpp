#include <iostream>
#include <thread>
#include "Server.h"
#include "Message.h"
#include <cstdlib>

Server::Server(uint16_t a_port) {
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

void Server::Serve() {
    bool serverRunning = true;
    std::cout << "server commands:\nstop - disconnect all clients and stop server\n===\n";
    m_spammer = std::thread([this, &serverRunning](){
        {
            while(serverRunning) {
                {
                    {   // PINGS
                        std::lock_guard guard(m_lock);
                        std::string pings = "Pings: ";
                        for (auto &peer_id: m_players) {
                            pings += std::to_string(peer_id.second.id) + ": "
                                     + std::to_string(peer_id.first->roundTripTime) + ", ";
                        }
                        CookedMessage msg = TextMessage::Create(pings);
                        ENetPacket *packet = enet_packet_create(msg.data, msg.len, 0);
                        for (auto &peer_id: m_players) {
                            enet_peer_send(peer_id.first, 1, packet);
                        }
                    }
                    {// SYSTEM TIME
                        std::chrono::time_point now = std::chrono::system_clock::now();
                        CookedMessage msg = SystemTimeMessage::Create(
                                std::chrono::time_point_cast<std::chrono::milliseconds>(
                                        now).time_since_epoch().count());
                        ENetPacket *packet = enet_packet_create(msg.data, msg.len, ENET_PACKET_FLAG_RELIABLE);
                        for (auto &peer_id: m_players) {
                            enet_peer_send(peer_id.first, 0, packet);
                        }
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }

    });
    std::thread([&serverRunning, this]() {
        while (true) {
            std::string cmd;
            std::cin >> cmd;
            if (cmd == "stop") {
                std::lock_guard guard(m_lock);
                for (auto& peer_id : m_players) {
                    enet_peer_disconnect_now(peer_id.first, 0);
                }
                serverRunning = false;
                break;
            } else {
                std::cout << "unknown command\n";
            }
        }
    }).detach();

    ENetEvent event;
    while (serverRunning)
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
                    m_players.erase(event.peer);
                    break;
                default:
                    break;
            }
        }
    }
    m_spammer.join();
}

void Server::HandleConnect(ENetPeer *a_newPeer) {
    Player player(m_curId, s_names[rand() % s_namesCount]);
    std::lock_guard guard(m_lock);

    {   // send full players list  to the new player
        std::string playerList = "You have " + player.ToString();
        if (!m_players.empty()) {
            playerList += "\nOther players:";
            for (auto& peer_id : m_players) {
                playerList += "\n" + peer_id.second.ToString();
            }
        }

        CookedMessage msg = TextMessage::Create(playerList);
        ENetPacket * packet = enet_packet_create (msg.data, msg.len, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(a_newPeer, 0, packet);
    }

    {   // send new player to all the old players
        CookedMessage msg = TextMessage::Create("New player connected: " + player.ToString());
        ENetPacket * packet = enet_packet_create (msg.data, msg.len, ENET_PACKET_FLAG_RELIABLE);
        for (auto& peer_id : m_players) {
            enet_peer_send(peer_id.first, 0, packet);
        }
    }
    // register new player
    m_players.insert(std::make_pair(a_newPeer, std::move(player)));
    m_curId++;
}

void Server::HandleReceive(ENetEvent &a_event) {
    auto* incomingPacket = a_event.packet;
    if (getType(incomingPacket->data) == MessageType::SYSTEM_TIME) {
        SystemTimeMessage incomingMsg(incomingPacket);
        std::cout << "id: " << std::to_string(m_players[a_event.peer].id) << " -> " << incomingMsg.unixTime<< "\n";
    }
    enet_packet_destroy(a_event.packet);
}

int main() {
    enet_initialize();
    Server server(12346);
    server.Serve();
    enet_deinitialize();
    return 0;
}



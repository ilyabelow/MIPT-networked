#include <iostream>
#include <thread>
#include "Client.h"
#include "Message.h"
#include <chrono>
Client::Client(const std::string &a_address, enet_uint16 port) {
    m_host = enet_host_create(nullptr, 1, 2, 0, 0);
    if (m_host == nullptr)
    {
        std::cerr << "An error occurred while initializing host, exiting\n";
        exit(EXIT_FAILURE);
    }
    enet_address_set_host(&m_remoteAddress, a_address.c_str());
    m_remoteAddress.port = port;
}

void Client::Serve() {
    if (!ConnectToRemote()) {
        return;
    }
    m_currentRemote = RemoteType::Lobby;
    ServeRemote();
}

void Client::HandleReceive(ENetEvent &a_event) {
    auto* incomingPacket = a_event.packet;
    if (getType(incomingPacket->data) == MessageType::TEXT) {
        TextMessage incomingMsg(incomingPacket);
        std::cout << incomingMsg.txt << std::endl;
    }
    if (getType(incomingPacket->data) == MessageType::SERVER_INFO) {
        ServerInfoMessage incomingMsg(incomingPacket);
        enet_address_set_host(&m_remoteAddress, incomingMsg.address.data());
        m_remoteAddress.port = incomingMsg.port;
        enet_peer_reset (m_remotePeer);
        m_clientRunning &= ConnectToRemote();
        m_currentRemote = RemoteType::Server;
        m_spammer = std::thread([this](){
            while (m_clientRunning) {
                std::chrono::time_point now = std::chrono::system_clock::now();
                CookedMessage msg = SystemTimeMessage::Create(std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count());
                ENetPacket * packet = enet_packet_create (msg.data, msg.len, ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(m_remotePeer, 0, packet);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });
    }
    if (getType(incomingPacket->data) == MessageType::SYSTEM_TIME) {
        SystemTimeMessage incomingMsg(incomingPacket);
        std::cout << "server's system time: "  << incomingMsg.unixTime  << '\n';
    }
    enet_packet_destroy(a_event.packet);
}

bool Client::ConnectToRemote() {
    m_remotePeer = enet_host_connect (m_host, &m_remoteAddress, 2, 0);
    if (m_remotePeer == nullptr) {
        std::cerr << "connection to remote failed\n";
        return false;
    }
    ENetEvent event;
    if (enet_host_service (m_host, & event, 1000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        std::cout << "connected to remote\n";
        return true;
    }
    std::cerr << "connection to remote failed\n";
    enet_peer_reset (m_remotePeer);
    return false;
}

void Client::ServeRemote() {
    std::cout << "client commands:\nstop - stop client\nsend <command> - send specific command to remote host\n";

    std::thread([this]() {
        while (true) {
            std::string cmd;
            std::cin >> cmd;
            if (cmd == "stop") {
                enet_peer_disconnect(m_remotePeer, 0);
            } else if (cmd == "send") {
                std::cin >> cmd;
                CookedMessage msg = CommandMessage::Create(cmd.c_str());
                ENetPacket * packet = enet_packet_create (msg.data, msg.len, ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(m_remotePeer, 0, packet);
            } else {
                std::cout << "unknown command\n";
            }
        }
    }).detach();
    // wait for lobby to reply with server info
    ENetEvent event;
    while (m_clientRunning)
    {
        while (enet_host_service(m_host, &event, 0) > 0)
        {
            switch (event.type)
            {
                case ENET_EVENT_TYPE_RECEIVE:
                    HandleReceive(event);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << "Remote disconnected\n";
                    m_clientRunning = false;
                    break;
                default:
                    break;
            }
        }
    }
    if (m_currentRemote == RemoteType::Server) {
        m_spammer.join();
    }
}


int main() {
    enet_initialize();
    Client client("localhost", 12345);
    client.Serve();
    enet_deinitialize();
    return 0;
}


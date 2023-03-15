#ifndef HW2_SERVER_H
#define HW2_SERVER_H

#include <enet/enet.h>
#include <unordered_set>
#include <unordered_map>
#include <utility>


static const std::string s_names[] = {"Michael",
                                      "Dwight",
                                      "Jim",
                                      "Pam",
                                      "Andy",
                                      "Darryl",
                                      "Kevin",
                                      "Angela",
                                      "Oscar",
                                      "Phyllis",
                                      "Stanley",
                                      "Toby",
                                      "Ryan",
                                      "Kelly",
                                      "Creed",
                                      "Meredith",
                                      "Erin",
                                      "Gabe",
                                      "Holly",
                                      "Jan"};
static const size_t s_namesCount = 20;

struct Player {
    uint32_t id;
    std::string name;

    std::string ToString() {
        return "id: " + std::to_string(id) + ", name: " + name;
    }
    Player() {}
    Player(uint32_t a_id, std::string a_name): id(a_id), name(std::move(a_name)) { }
};

class Server {
public:
    Server(uint16_t a_port);

    void Serve();

private:
    void HandleConnect(ENetPeer* a_newPeer);
    void HandleReceive(ENetEvent &a_event);

    ENetHost* m_host;
    std::unordered_map<ENetPeer*, Player> m_players;
    std::mutex m_lock;
    uint32_t m_curId = 0;
    std::thread m_spammer;
};


#endif //HW2_SERVER_H

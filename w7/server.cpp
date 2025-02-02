#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include "mathUtils.h"
#include <stdlib.h>
#include <vector>
#include <map>
#include <set>

static std::vector<Entity> entities;
static std::map<uint32_t, ENetPeer*> controlledMap;



static int type = 3;

uint32_t genEid() {
  type = (type + 1) % 3;
  if (type == 0) {
    return rand() % (1 << 7);
  } else if (type == 1) {
    return (1 << 7) + rand() % ((1 << 14) - (1 << 7));
  }
  return (1 << 14) + rand() % ((1 << 30) - (1 << 14));
}

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
  // send all entities
  for (const Entity &ent : entities)
    send_new_entity(peer, ent);

  // find max eid
//  uint32_t maxEid = entities.empty() ? 0 : entities[0].eid;
//  for (const Entity &e : entities)
//    maxEid = std::max(maxEid, e.eid);
//  uint32_t newEid = maxEid + 1;

// for int packing demo
  uint32_t newEid = genEid();
  std::set<uint32_t > takenEids;
  for (auto& e: entities) takenEids.insert(e.eid);
  while (takenEids.contains(newEid)) newEid = genEid();
  std::cout << newEid << std::endl;

  // Alpha is last byte!!!
  uint32_t color = 0x44000000 * (rand() % 5) +
                   0x00440000 * (rand() % 5) +
                   0x00004400 * (rand() % 5) +
                   0x000000ff;

  float x = (rand() % 4) * 5.f;
  float y = (rand() % 4) * 5.f;
  Entity ent = {color, x, y, 0.f, (rand() / RAND_MAX) * 3.141592654f, 0.f, 0.f, newEid};
  entities.push_back(ent);

  controlledMap[newEid] = peer;


  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, newEid);
}

void on_input(ENetPacket *packet)
{
  uint32_t eid = invalid_entity;
  float thr = 0.f; float steer = 0.f;
  deserialize_entity_input(packet, eid, thr, steer);
  for (Entity &e : entities)
    if (e.eid == eid)
    {
      e.thr = thr;
      e.steer = steer;
    }
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10131;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  uint32_t lastTime = enet_time_get();
  while (true)
  {
    uint32_t curTime = enet_time_get();
    float dt = (curTime - lastTime) * 0.001f;
    lastTime = curTime;
    ENetEvent event;
    while (enet_host_service(server, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
          case E_CLIENT_TO_SERVER_JOIN:
            on_join(event.packet, event.peer, server);
            break;
          case E_CLIENT_TO_SERVER_INPUT:
            on_input(event.packet);
            break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    static int t = 0;
    for (Entity &e : entities)
    {
      // simulate
      simulate_entity(e, dt);
      // send
      for (size_t i = 0; i < server->peerCount; ++i)
      {
        ENetPeer *peer = &server->peers[i];
        // skip this here in this implementation
        //if (controlledMap[e.eid] != peer)
        send_snapshot(peer, e.eid, e.x, e.y, e.ori);
      }
    }
    usleep(10000);
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}



#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include "ai.h"
#include "screen.h"
#include <cstdlib>
#include <vector>
#include <map>
#include <chrono>

static std::vector<Entity> entities;
static std::map<uint16_t, ENetPeer*> playerControlled;
static std::map<uint16_t, ai> aiControlled;


Entity& create_entity(float speed) {
  // find max eid
  uint16_t maxEid = entities.empty() ? invalid_entity : entities[0].eid;
  for (const Entity &e : entities)
    maxEid = std::max(maxEid, e.eid);
  uint16_t newEid = maxEid + 1;
  uint32_t color = 0x44000000 * (rand() % 5) +
                   0x00440000 * (rand() % 5) +
                   0x00004400 * (rand() % 5) +
                   0x000000a0;
  float x = rand() % 200 - 100;
  float y = rand() % 200 - 100;
  Entity ent = {color, x, y, newEid, (((float) rand() / RAND_MAX) * 4 + 1) * 10, speed};
  entities.push_back(ent);
  return entities[entities.size()-1];
}

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
  // send all entities
  for (const Entity &ent : entities)
    send_new_entity(peer, ent);

  // create entity
  Entity& ent = create_entity(110); // advantage to the player for easier tests
  playerControlled[ent.eid] = peer;

  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, ent.eid);
}

void on_state(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f;
  deserialize_entity_state(packet, eid, x, y);
  for (Entity &e : entities)
    if (e.eid == eid)
    {
      e.x = x;
      e.y = y;
    }
}

void add_ai(ENetHost* host) {
  Entity& ent = create_entity(100.f);
  aiControlled.insert(std::make_pair(ent.eid, ai(-SCREEN_WIDTH/2, SCREEN_WIDTH/2, -SCREEN_HEIGHT/2, SCREEN_HEIGHT/2)));
  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
}

void consume_entity(Entity& consumer, Entity& consumed) {
  consumer.size += consumed.size / 2;
  consumed.size /= 2;
  // dirty copypaste from ai
  consumed.x =  -SCREEN_WIDTH / 2.f + (rand() / (float) RAND_MAX) * SCREEN_WIDTH;
  consumed.y = -SCREEN_HEIGHT / 2.f + (rand() / (float) RAND_MAX) * SCREEN_HEIGHT;

  consumed.changed_size = true;
  consumer.changed_size = true;

  if (aiControlled.contains(consumed.eid)) {
    aiControlled.find(consumed.eid)->second.was_teleported();
  }
}

void collide_entity(Entity& ent) {
  for (Entity &other : entities) {
    if (other.eid == ent.eid) continue;
    float dist = ent.size / 2 + other.size / 2;
    if (abs(other.x - ent.x) <= dist && abs(other.y - ent.y) <= dist) {
      if (ent.size < other.size) {
        consume_entity(other, ent);
      } else {
        consume_entity(ent, other);
      }
      break;
    }
  }
}


float get_time() {
  // I hate std::chrono
  return (float) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() / 1000.f;
}

int main(int argc, const char **argv)
{
  srand(clock());
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
  for (int i = 0; i < 3; ++i) {
    add_ai(server);
  }
  float time = get_time();
  const float ai_countdown = 5;
  float ai_timer = ai_countdown;
  while (true)
  {
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
          case E_CLIENT_TO_SERVER_STATE:
            on_state(event.packet);
            break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    float dt = get_time() - time;

    for (Entity &e : entities) {
      if (aiControlled.contains(e.eid)) {
        aiControlled.find(e.eid)->second.move(e, dt);
      }
      collide_entity(e);
      for (size_t i = 0; i < server->peerCount; ++i)
      {
        ENetPeer *peer = &server->peers[i];
        bool controlled_by_current_peer = playerControlled.contains(e.eid) && playerControlled.find(e.eid)->second == peer;
        if (!controlled_by_current_peer || e.changed_size)
          send_snapshot(peer, e.eid, e.x, e.y, e.size);
      }
    }
    for (Entity &e : entities) {
      e.changed_size = false;
    }
    // spawn new ais
    ai_timer -= dt;
    if (ai_timer <= 0) {
      ai_timer = ai_countdown;
      add_ai(server);
    }

    time = get_time();
    usleep(10000);
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}


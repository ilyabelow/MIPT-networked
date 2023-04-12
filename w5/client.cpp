// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <functional>
#include "raylib.h"
#include <enet/enet.h>

#include <vector>
#include <cstdio>
#include <iostream>
#include "entity.h"
#include "protocol.h"


static std::vector<EntityWithSnapshots> entities;
static uint16_t my_entity = invalid_entity;
static duration shift;
static std::chrono::microseconds tick;
static time_point start_time;

void on_clock(ENetPacket *packet) {
  auto client_time = steady_now();
  time_point server_time;
  deserialize_clock(packet, server_time, tick);
  shift = client_time - server_time;
}

void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntityBase;
  deserialize_new_entity(packet, newEntityBase);
  EntityWithSnapshots newEntity(newEntityBase);
  // TODO: Direct adressing, of course!
  for (const Entity &e : entities)
    if (e.eid == newEntity.eid)
      return; // don't need to do anything, we already have entity
  entities.push_back(newEntity);
}

void on_set_controlled_entity(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_entity);
}

void on_snapshot(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  EntitySnapshot snapshot;
  deserialize_snapshot(packet, eid, snapshot);
  snapshot.time += shift;
  // TODO: Direct adressing, of course!
  for (EntityWithSnapshots &e : entities)
    if (e.eid == eid)
    {
      e.push_new_snapshot(snapshot);
    }
}

int main(int argc, const char **argv)
{
  start_time = steady_now();
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "localhost");
  address.port = 10131;

  ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
  if (!serverPeer)
  {
    printf("Cannot connect to server");
    return 1;
  }

  int width = 600;
  int height = 600;

  InitWindow(width, height, "w5 networked MIPT");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight - 150, height);
    SetWindowSize(width, height);
  }

  Camera2D camera = { {0, 0}, {0, 0}, 0.f, 1.f };
  camera.target = Vector2{ 0.f, 0.f };
  camera.offset = Vector2{ width * 0.5f, height * 0.5f };
  camera.rotation = 0.f;
  camera.zoom = 10.f;


  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

  bool connected = false;
  while (!WindowShouldClose())
  {
    float dt = GetFrameTime();
    ENetEvent event;
    while (enet_host_service(client, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        send_join(serverPeer);
        connected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
        case E_SERVER_TO_CLIENT_NEW_ENTITY:
          on_new_entity_packet(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
          on_set_controlled_entity(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SNAPSHOT:
          on_snapshot(event.packet);
          break;
        case E_SERVER_TO_CLIENT_CLOCK:
          on_clock(event.packet);
          break;
        };
        break;
      default:
        break;
      };
    }
    if (my_entity != invalid_entity)
    {
      bool left = IsKeyDown(KEY_LEFT);
      bool right = IsKeyDown(KEY_RIGHT);
      bool up = IsKeyDown(KEY_UP);
      bool down = IsKeyDown(KEY_DOWN);
      // TODO: Direct adressing, of course!
      for (Entity &e : entities)
        if (e.eid == my_entity)
        {
          // Update
          float thr = (up ? 1.f : 0.f) + (down ? -1.f : 0.f);
          float steer = (left ? -1.f : 0.f) + (right ? 1.f : 0.f);

          // Send
          send_entity_input(serverPeer, my_entity, thr, steer);
        }
    }

    BeginDrawing();
      ClearBackground(GRAY);
      BeginMode2D(camera);
        auto now = steady_now() - tick; // HERE! we shift time in the past
        for (EntityWithSnapshots &e : entities)
        {
          {
            e.actualize(now);
            const Rectangle rect = {e.x, e.y, 3.f, 1.f};
            DrawRectanglePro(rect, {0.f, 0.5f}, e.ori * 180.f / PI, GetColor(e.color));
          }
          {
            e.just_use_latest();
            const Rectangle rect = {e.x, e.y, 3.f, 1.f};
            uint32_t alpha = e.color & 0x000000ff;
            DrawRectanglePro(rect, {0.f, 0.5f}, e.ori * 180.f / PI, GetColor(e.color - alpha + alpha/2));
          }
        }

      EndMode2D();
    EndDrawing();
  }

  CloseWindow();
  return 0;
}

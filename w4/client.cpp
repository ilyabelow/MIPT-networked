#include <functional>
#include <algorithm> // min/max
#include "raylib.h"
#include <enet/enet.h>
#include <cmath>

#include <vector>
#include <cstdio>
#include "entity.h"
#include "protocol.h"
#include "screen.h"

static std::vector<Entity> entities;
static uint16_t my_entity = invalid_entity;

void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntity;
  deserialize_new_entity(packet, newEntity);
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
  float x = 0.f; float y = 0.f; float size = 0.f;
  deserialize_snapshot(packet, eid, x, y, size);
  // TODO: Direct adressing, of course!
  for (Entity &e : entities)
    if (e.eid == eid)
    {
      e.x = x;
      e.y = y;
      e.size = size;
      break;
    }
}

int main(int argc, const char **argv)
{
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
  enet_address_set_host(&address, "127.0.0.1");
  address.port = 10131;

  ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
  if (!serverPeer)
  {
    printf("Cannot connect to server");
    return 1;
  }

  int width = SCREEN_WIDTH;
  int height = SCREEN_HEIGHT;
  InitWindow(width, height, "w6 AI MIPT");

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
  camera.zoom = 1.f;

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
        };
        enet_packet_destroy(event.packet);
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
          // COPYPASTE FROM AI!
          float dx = ((left ? -1 : 0.f) + (right ? +1 : 0.f));
          float dy = ((up ? -1 : 0.f) + (down ? +1 : 0.f));
          float mag = sqrt(dx*dx + dy*dy);
          if (mag < 0.01) { // no movement
            break;
          }
          e.x += dx / mag * e.speed * dt;
          e.y += dy / mag * e.speed * dt;

          // Send
          send_entity_state(serverPeer, my_entity, e.x, e.y);
          break;
        }
    }


    BeginDrawing();
      ClearBackground(GRAY);
      BeginMode2D(camera);
        for (const Entity &e : entities)
        {
          const Rectangle rect = {e.x-e.size/2, e.y-e.size/2, e.size, e.size};
          DrawRectangleRec(rect, GetColor(e.color));
        }

      EndMode2D();
    EndDrawing();
  }

  CloseWindow();

  return 0;
}

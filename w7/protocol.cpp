#include "protocol.h"
#include "quantisation.h"
#include <cstring> // memcpy
#include <iostream>

void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
  *packet->data = E_CLIENT_TO_SERVER_JOIN;

  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(Entity),
                                                   ENET_PACKET_FLAG_RELIABLE);
  uint8_t *ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_NEW_ENTITY; ptr += sizeof(uint8_t);
  memcpy(ptr, &ent, sizeof(Entity)); ptr += sizeof(Entity);

  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t),
                                                   ENET_PACKET_FLAG_RELIABLE);
  uint8_t *ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY; ptr += sizeof(uint8_t);
  memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);

  enet_peer_send(peer, 0, packet);
}

typedef PackedVec2<uint8_t, 4, 4> ControlsQuantized;

void send_entity_input(ENetPeer *peer, uint16_t eid, float thr, float ori)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   sizeof(ControlsQuantized),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  uint8_t *ptr = packet->data;
  *ptr = E_CLIENT_TO_SERVER_INPUT; ptr += sizeof(uint8_t);
  memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);

  ControlsQuantized controlsPacked({thr, ori}, {-1., 1.}, {-1., 1.});
  memcpy(ptr, &controlsPacked.packedVal, sizeof(ControlsQuantized)); ptr += sizeof(ControlsQuantized);
  /*
  memcpy(ptr, &thrPacked, sizeof(uint8_t)); ptr += sizeof(uint8_t);
  memcpy(ptr, &oriPacked, sizeof(uint8_t)); ptr += sizeof(uint8_t);
  */

  enet_peer_send(peer, 1, packet);
}

typedef PackedVec3<uint32_t, 12, 11, 9> TransformQuantized;

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float ori)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   sizeof(TransformQuantized),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  uint8_t *ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_SNAPSHOT; ptr += sizeof(uint8_t);
  memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);

  TransformQuantized tmPacked({x, y, ori}, {-16., 16.}, {-8., 8.},  {-PI, PI});
  //printf("xPacked/unpacked %d %f\n", xPacked, x);
  memcpy(ptr, &tmPacked.packedVal, sizeof(TransformQuantized)); ptr += sizeof(TransformQuantized);
  enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  ent = *(Entity*)(ptr); ptr += sizeof(Entity);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
}

void deserialize_entity_input(ENetPacket *packet, uint16_t &eid, float &thr, float &steer)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
  ControlsQuantized ctrlPacked(*(uint8_t*)(ptr)); ptr += sizeof(ControlsQuantized);
  /*
  uint8_t thrPacked = *(uint8_t*)(ptr); ptr += sizeof(uint8_t);
  uint8_t oriPacked = *(uint8_t*)(ptr); ptr += sizeof(uint8_t);
  */
  static auto neutralPackedValue = pack_float<ControlsQuantized::PackedType>(0.f, -1.f, 1.f, 4);
  Vec2 ctrl = ctrlPacked.unpack({-1., 1.}, {-1., 1.});

  thr = ctrlPacked.getXPacked() == neutralPackedValue ? 0.f : ctrl.x;
  steer = ctrlPacked.getYPacked() == neutralPackedValue ? 0.f : ctrl.y;
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &ori)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
  TransformQuantized tmPacked(*(TransformQuantized *)(ptr)); ptr += sizeof(TransformQuantized);
  Vec3 tm = tmPacked.unpack({-16., 16.}, {-8., 8.},  {-PI, PI});
  x = tm.x; y = tm.y; ori = tm.z;
}


#include "protocol.h"
#include "quantisation.h"
#include <cstring> // memcpy
#include <iostream>
#include "bitstream.h"

void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
  *packet->data = E_CLIENT_TO_SERVER_JOIN;

  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  size_t size = ruler() << E_SERVER_TO_CLIENT_NEW_ENTITY << ent;
  ENetPacket *packet = enet_packet_create(nullptr, size, ENET_PACKET_FLAG_RELIABLE);
  bitstream(packet->data) << E_SERVER_TO_CLIENT_NEW_ENTITY << ent;

  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint32_t eid)
{
  size_t size = ruler() << E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY << eid;
  ENetPacket *packet = enet_packet_create(nullptr, size, ENET_PACKET_FLAG_RELIABLE);
  bitstream(packet->data) << E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY << eid;

  enet_peer_send(peer, 0, packet);
}

typedef PackedVec2<uint8_t, 4, 4> ControlsQuantized;

void send_entity_input(ENetPeer *peer, uint32_t eid, float thr, float ori)
{
  ControlsQuantized controlsPacked({thr, ori}, {-1., 1.}, {-1., 1.});

  size_t size = ruler() << E_CLIENT_TO_SERVER_INPUT << eid << controlsPacked;
  ENetPacket *packet = enet_packet_create(nullptr, size, ENET_PACKET_FLAG_UNSEQUENCED);
  bitstream(packet->data) << E_CLIENT_TO_SERVER_INPUT << eid << controlsPacked;

  enet_peer_send(peer, 1, packet);
}

typedef PackedVec3<uint32_t, 12, 11, 9> TransformQuantized;

void send_snapshot(ENetPeer *peer, uint32_t eid, float x, float y, float ori)
{
  TransformQuantized tmPacked({x, y, ori}, {-16., 16.}, {-8., 8.},  {-PI, PI});

  size_t size = ruler() << E_SERVER_TO_CLIENT_SNAPSHOT << eid << tmPacked;
  ENetPacket *packet = enet_packet_create(nullptr, size, ENET_PACKET_FLAG_UNSEQUENCED);
  bitstream(packet->data) << E_SERVER_TO_CLIENT_SNAPSHOT << eid << tmPacked;

  enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

 void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  bitstream(packet->data) >> Skip<MessageType>{} >> ent;
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint32_t &eid)
{
  bitstream(packet->data) >> Skip<MessageType>{} >> eid;
}

void deserialize_entity_input(ENetPacket *packet, uint32_t &eid, float &thr, float &steer)
{
  ControlsQuantized ctrlPacked(0);
  bitstream(packet->data) >> Skip<MessageType>{} >> eid >> ctrlPacked;

  static auto neutralPackedValue = pack_float<ControlsQuantized::PackedType>(0.f, -1.f, 1.f, 4);
  Vec2 ctrl = ctrlPacked.unpack({-1., 1.}, {-1., 1.});

  thr = ctrlPacked.getXPacked() == neutralPackedValue ? 0.f : ctrl.x;
  steer = ctrlPacked.getYPacked() == neutralPackedValue ? 0.f : ctrl.y;
}

void deserialize_snapshot(ENetPacket *packet, uint32_t &eid, float &x, float &y, float &ori)
{
  TransformQuantized tmPacked(0);
  bitstream(packet->data) >> Skip<MessageType>{} >> eid >> tmPacked;

  Vec3 tm = tmPacked.unpack({-16., 16.}, {-8., 8.},  {-PI, PI});
  x = tm.x; y = tm.y; ori = tm.z;
}

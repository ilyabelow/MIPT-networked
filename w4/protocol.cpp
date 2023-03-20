#include "protocol.h"
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

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  size_t size = ruler() << E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY << eid;
  ENetPacket *packet = enet_packet_create(nullptr, size, ENET_PACKET_FLAG_RELIABLE);
  bitstream(packet->data) << E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY << eid;
  enet_peer_send(peer, 0, packet);
}

void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y)
{
  size_t size = ruler() << E_CLIENT_TO_SERVER_STATE << eid << x << y;
  ENetPacket *packet = enet_packet_create(nullptr, size, ENET_PACKET_FLAG_UNSEQUENCED);
  bitstream(packet->data) << E_CLIENT_TO_SERVER_STATE << eid << x << y;
  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float s)
{
  size_t size = ruler() << E_SERVER_TO_CLIENT_SNAPSHOT << eid << x << y << s;
  ENetPacket *packet = enet_packet_create(nullptr, size, ENET_PACKET_FLAG_UNSEQUENCED);
  bitstream(packet->data) << E_SERVER_TO_CLIENT_SNAPSHOT << eid << x << y << s;
  enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  bitstream(packet->data) >> MessageType() >> ent;
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  bitstream(packet->data) >> MessageType() >> eid;
}

void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y)
{
  bitstream(packet->data) >> MessageType() >> eid >> x >> y;
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &s)
{
  bitstream(packet->data) >> MessageType() >> eid >> x >> y >> s;
}


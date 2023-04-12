#include "protocol.h"
#include "bitstream.h"


void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
  *packet->data = E_CLIENT_TO_SERVER_JOIN;

  enet_peer_send(peer, 0, packet);
}

void send_clock(ENetPeer *peer, time_point join_time, std::chrono::microseconds tick) {
  size_t size = ruler() << E_SERVER_TO_CLIENT_CLOCK << join_time << tick;
  ENetPacket *packet = enet_packet_create(nullptr, size, ENET_PACKET_FLAG_RELIABLE);
  bitstream(packet->data) << E_SERVER_TO_CLIENT_CLOCK << join_time << tick;
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

void send_entity_input(ENetPeer *peer, uint16_t eid, float thr, float steer)
{
  size_t size = ruler() << E_CLIENT_TO_SERVER_INPUT << eid << thr << steer;
  ENetPacket *packet = enet_packet_create(nullptr, size, ENET_PACKET_FLAG_UNSEQUENCED);
  bitstream(packet->data) << E_CLIENT_TO_SERVER_INPUT << eid << thr << steer;
  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, EntitySnapshot snapshot)
{
  size_t size = ruler() << E_SERVER_TO_CLIENT_SNAPSHOT << eid << snapshot;
  ENetPacket *packet = enet_packet_create(nullptr, size, ENET_PACKET_FLAG_UNSEQUENCED);
  bitstream(packet->data) << E_SERVER_TO_CLIENT_SNAPSHOT << eid << snapshot;
  enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_clock(ENetPacket *packet, time_point &join_time, std::chrono::microseconds& tick) {
  bitstream(packet->data) >> MessageType() >> join_time >> tick;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  bitstream(packet->data) >> MessageType() >> ent;
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  bitstream(packet->data) >> MessageType() >> eid;
}

void deserialize_entity_input(ENetPacket *packet, uint16_t &eid, float &thr, float &steer)
{
  bitstream(packet->data) >> MessageType() >> eid >> thr >> steer;
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, EntitySnapshot &snapshot)
{
  bitstream(packet->data) >> MessageType() >> eid >> snapshot;
}


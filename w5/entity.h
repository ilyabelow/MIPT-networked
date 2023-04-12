#pragma once
#include <cstdint>
#include <array>
#include "time.h"

struct EntitySnapshot {
  EntitySnapshot();

  EntitySnapshot(time_point point, float x, float y, float ori);

  time_point time;
  float x = 0.f;
  float y = 0.f;
  float ori = 0.f;
};

constexpr uint16_t invalid_entity = -1;
struct Entity
{
  uint32_t color = 0xff00ffff;
  float x = 0.f;
  float y = 0.f;
  float speed = 0.f;
  float ori = 0.f;

  float thr = 0.f;
  float steer = 0.f;

  uint16_t eid = invalid_entity;

  EntitySnapshot get_snapshot(time_point now) const;
};


struct EntityWithSnapshots: Entity {
  explicit EntityWithSnapshots(Entity base);

  void actualize(time_point time);

  void push_new_snapshot(EntitySnapshot snapshot);

  // for debug
  void just_use_latest();
//private:
  std::array<EntitySnapshot, 2> snapshots;
};

void simulate_entity(Entity &e, float dt);


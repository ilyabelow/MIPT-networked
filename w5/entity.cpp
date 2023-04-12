#include <iostream>
#include "entity.h"
#include "mathUtils.h"

void simulate_entity(Entity &e, float dt)
{
  bool isBraking = sign(e.thr) != 0.f && sign(e.thr) != sign(e.speed);
  float accel = isBraking ? 12.f : 3.f;
  e.speed = move_to(e.speed, clamp(e.thr, -0.3, 1.f) * 10.f, dt, accel);
  e.ori += e.steer * dt * clamp(e.speed, -2.f, 2.f) * 0.3f;
  e.x += cosf(e.ori) * e.speed * dt;
  e.y += sinf(e.ori) * e.speed * dt;
}

void EntityWithSnapshots::push_new_snapshot(EntitySnapshot snapshot) {
  snapshots[0] = snapshots[1]; // Snapshots are cheap to copy
  snapshots[1] = snapshot;
}

void EntityWithSnapshots::actualize(time_point time) {
  if (time < snapshots[0].time) {
    x = snapshots[0].x;
    y = snapshots[0].y;
    ori = snapshots[0].ori;
  }
  if (time > snapshots[1].time) {
    x = snapshots[1].x;
    y = snapshots[1].y;
    ori = snapshots[1].ori;
  }
  duration_float full = snapshots[1].time - snapshots[0].time;
  duration_float passed = time - snapshots[0].time;
  float t = passed / full;
  //std::cout << t << std::endl;
  x = interpolate(snapshots[0].x, snapshots[1].x, t);
  y = interpolate(snapshots[0].y, snapshots[1].y, t);
  ori = interpolate(snapshots[0].ori, snapshots[1].ori, t);
}

EntityWithSnapshots::EntityWithSnapshots(Entity base): Entity(base) { }

void EntityWithSnapshots::just_use_latest() {
  x = snapshots[1].x;
  y = snapshots[1].y;
  ori = snapshots[1].ori;
}


EntitySnapshot Entity::get_snapshot(time_point now) const {
  return {now, x, y, ori};
}

EntitySnapshot::EntitySnapshot(time_point point, float x, float y, float ori): time(point), x(x), y(y), ori(ori) { }

EntitySnapshot::EntitySnapshot() = default;

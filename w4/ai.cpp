#include <cstdlib>
#include <cmath>
#include <cstdio>

#include "ai.h"

ai::ai(int minX, int maxX, int minY, int maxY): m_maxX(maxX), m_maxY(maxY), m_minX(minX), m_minY(minY) {
  add_new_target();
}

void ai::add_new_target() {
  m_targetX =  m_minX + (rand() / (float) RAND_MAX) * (m_maxX-m_minX);
  m_targetY = m_minY + (rand() / (float) RAND_MAX) * (m_maxY-m_minY);
}

void ai::move(Entity &host, float dt) {
  float dx = m_targetX - host.x;
  float dy = m_targetY - host.y;
  if (dx < host.speed*dt && dy < host.speed*dt) {
    add_new_target();
    return;
  }
  float mag = sqrt(dx*dx + dy*dy);
  host.x += dx / mag * host.speed * dt;
  host.y += dy / mag * host.speed * dt;
}

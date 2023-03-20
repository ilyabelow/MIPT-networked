#pragma once

#include "entity.h"

class ai {
public:
  ai(int minX, int maxX, int minY, int maxY);

  void move(Entity &host, float dt);
private:
  void add_new_target();

  float m_targetX;
  float m_targetY;

  float m_maxX;
  float m_maxY;
  float m_minX;
  float m_minY;
};
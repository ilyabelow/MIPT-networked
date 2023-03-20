#pragma once
#include <cstdint>

constexpr uint16_t invalid_entity = -1;
struct Entity
{
  uint32_t color = 0xff00ffff;
  float x = 0.f;
  float y = 0.f;
  uint16_t eid = invalid_entity;
  float size = 10.f;
  float speed = 100.f;
  bool changed_size = false;
};


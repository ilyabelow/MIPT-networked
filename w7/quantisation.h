#pragma once
#include "mathUtils.h"
#include <limits>

template<typename T>
T pack_float(float v, float lo, float hi, int num_bits)
{
  T range = (1 << num_bits) - 1;//std::numeric_limits<T>::max();
  return range * ((clamp(v, lo, hi) - lo) / (hi - lo));
}

template<typename T>
float unpack_float(T c, float lo, float hi, int num_bits)
{
  T range = (1 << num_bits) - 1;//std::numeric_limits<T>::max();
  return float(c) / range * (hi - lo) + lo;
}

template<typename T, int num_bits>
struct PackedFloat
{
  T packedVal;

  PackedFloat(float v, float lo, float hi) { pack(v, lo, hi); }
  PackedFloat(T compressed_val) : packedVal(compressed_val) {}

  void pack(float v, float lo, float hi) { packedVal = pack_float<T>(v, lo, hi, num_bits); }
  float unpack(float lo, float hi) { return unpack_float<T>(packedVal, lo, hi, num_bits); }
};


struct Vec2 {
  float x, y;
};

template<typename T, int bits_x, int bits_y>
struct PackedVec2 {
  T packedVal;
  typedef T PackedType;

  PackedVec2(Vec2 v, Vec2 xlohi, Vec2 ylohi) { pack(v, xlohi, ylohi); }
  explicit PackedVec2(T compressed_val) : packedVal(compressed_val) {}

  void pack(Vec2 v, Vec2 xlohi, Vec2 ylohi) {
    PackedFloat<T, bits_x> packed_x(v.x, xlohi.x, xlohi.y);
    PackedFloat<T, bits_y> packed_y(v.y, ylohi.x, ylohi.y);
    packedVal = packed_x.packedVal << bits_y | packed_y.packedVal ;
  }
  Vec2 unpack(Vec2 xlohi, Vec2 ylohi) {
    PackedFloat<T, bits_x> packed_x(getXPacked());
    PackedFloat<T, bits_y> packed_y(getYPacked());
    return {packed_x.unpack(xlohi.x, xlohi.y),
            packed_y.unpack(ylohi.x, ylohi.y)};
  }
  T getXPacked() {
    return packedVal >> bits_y;
  }

  T getYPacked() {
    return packedVal & ((1 << bits_y) - 1);
  }
};


struct Vec3 {
  float x, y, z;
};

template<typename T, int bits_x, int bits_y, int bits_z>
struct PackedVec3 {
  T packedVal;
  typedef T PackedType;

  PackedVec3(Vec3 v, Vec2 xlohi, Vec2 ylohi, Vec2 zlohi) { pack(v, xlohi, ylohi, zlohi); }
  explicit PackedVec3(T compressed_val) : packedVal(compressed_val) {}

  void pack(Vec3 v, Vec2 xlohi, Vec2 ylohi, Vec2 zlohi) {
    PackedFloat<T, bits_x> packed_x(v.x, xlohi.x, xlohi.y);
    PackedFloat<T, bits_y> packed_y(v.y, ylohi.x, ylohi.y);
    PackedFloat<T, bits_z> packed_z(v.z, zlohi.x, zlohi.y);
    packedVal = (packed_x.packedVal << bits_y | packed_y.packedVal) << bits_z | packed_z.packedVal;
  }
  Vec3 unpack(Vec2 xlohi, Vec2 ylohi, Vec2 zlohi) {
    PackedFloat<T, bits_x> packed_x(getXPacked());
    PackedFloat<T, bits_y> packed_y(getYPacked());
    PackedFloat<T, bits_z> packed_z(getZPacked());
    return {packed_x.unpack(xlohi.x, xlohi.y),
            packed_y.unpack(ylohi.x, ylohi.y),
            packed_z.unpack(zlohi.x, zlohi.y)};
  }

  T getXPacked() {
    return packedVal >> bits_y + bits_z;
  }

  T getYPacked() {
    return packedVal >> bits_z & ((1 << bits_y) - 1);
  }

  T getZPacked() {
    return packedVal & ((1 << bits_z) - 1);
  }
};
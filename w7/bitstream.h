#pragma once
#include <cstring>

// ===== data packing

struct PackUint {
  const uint32_t *value;

  static const uint32_t byte_1 = (1 << (8   - 1));
  static const uint32_t byte_2 = (1 << (8*2 - 2));
  static const uint32_t byte_4 = (1 << (8*4 - 2));

  [[nodiscard]] size_t size() const {
    if (*value < byte_1) {
      return 1;
    } else if (*value < byte_2) {
      return 2;
    } else if (*value < byte_4) {
      return 4;
    }
    return 0;
  }
};

struct UnpackUint {
  uint32_t *value;
};

// ===== data streaming

template<class T>
struct Skip{ };

struct bitstream {
  explicit bitstream(uint8_t *data): m_data(data) { }

  template<class T>
  friend bitstream& operator<<(bitstream&, const T&);

  template<class T>
  friend bitstream& operator>>(bitstream&, T&);
  template<class T>
  friend bitstream& operator>>(bitstream&, Skip<T>&&);
  friend bitstream& operator>>(bitstream&, UnpackUint&&);

  bitstream& start() {
    return *this;
  }

private:
  uint8_t *m_data;
  size_t m_offset = 0;
};

// write

template<class T>
bitstream& operator<<(bitstream& s, const T& t) {
  memcpy(s.m_data + s.m_offset, &t, sizeof(T));
  s.m_offset += sizeof(T);
  return s;
}



template<>
bitstream& operator<<(bitstream& s, const PackUint& t) {
  auto size = t.size();
  // In comments: 1 - lowest order byte, 4 - highest order byte
  // LE: 1 2 3 4 (inverse of arab numerals), BE: 4 3 2 1 (natural)
  // We need to use BE so we change bits only of highest order byte
  // 0 means zero byte and x means garbage
  if (size == 1) {                             // 1st byte already starts with 0
                                               // 1 2 3 4 -> 1
    memcpy(s.m_data + s.m_offset, reinterpret_cast<const uint8_t *>(t.value), 1);
    s.m_offset += 1;
  } else if (size == 2) {                      // 3 = 4 = 0!
    uint32_t be = __builtin_bswap32(*t.value); // 1 2 3 4 -> 4 3 2 1
    be >>= 16;                                 // 4 3 2 1 -> 2 1 0 0
    be |= 0b10000000;                          // making 2nd byte start with 10
                                               // 2 1 0 0 -> 2 1
    memcpy(s.m_data + s.m_offset, reinterpret_cast<uint16_t*>(&be), 2);
    s.m_offset += 2;
  } else if (size == 4) {
    uint32_t be = __builtin_bswap32(*t.value);  // 1 2 3 4 -> 4 3 2 1
    be |= 0b11000000;                           // making 4th byte start with 11
    memcpy(s.m_data + s.m_offset, &be, 4);
    s.m_offset += 4;
  } else {
    throw std::invalid_argument("Value is too big to be packed");
  }
  return s;
}

// read

template<class T>
bitstream& operator>>(bitstream& s, T& t) {
  memcpy(&t, s.m_data + s.m_offset, sizeof(T));
  s.m_offset += sizeof(T);
  return s;
}

template<class T>
bitstream& operator>>(bitstream& s, Skip<T>&&) {
  s.m_offset += sizeof(T);
  return s;
}

bitstream& operator>>(bitstream& s, UnpackUint&& t) {
  auto *current_byte = s.m_data+s.m_offset;
  if ((*current_byte & 0b10000000) == 0) {                      // were reading 1st byte
    *t.value = static_cast<uint32_t>(*current_byte);            //       1 -> 1 0 0 0
    s.m_offset += 1;
  } else {
    if ((*current_byte & 0b01000000) == 0) {                    // were reading 2nd byte
      uint32_t be = *reinterpret_cast<uint32_t*>(current_byte); //       2 -> 2 1 x x
      *reinterpret_cast<uint8_t*>(&be) &= 0b00111111;           // removing extra 10 in 2nd byte
      be <<= 16;                                                // 2 1 x x -> 0 0 2 1
      *t.value = __builtin_bswap32(be);                         // 0 0 2 1 -> 1 2 0 0 = 1 2 3 4
      s.m_offset += 2;
    } else {                                                    // were reading 4th byte
      uint32_t be = *reinterpret_cast<uint32_t*>(current_byte); //       4 -> 4 3 2 1
      *reinterpret_cast<uint8_t*>(&be) &= 0b00111111;           // removing extra 11 in 4th byte
      *t.value = __builtin_bswap32(be);                         // 4 3 2 1 -> 1 2 3 4
      s.m_offset += 4;
    }
  }
  return s;
}

// === size measurement

struct ruler {
  operator size_t() {
    return m_size;
  }

  template<class T>
  friend ruler& operator<<(ruler&, const T&);

  ruler& start() {
    return *this;
  }
private:
  size_t m_size;
};

template<class T>
ruler& operator<<(ruler& r, const T&) {
  r.m_size += sizeof(T);
  return r;
}

template<>
ruler& operator<<(ruler& r, const PackUint& t) {
  r.m_size += t.size();
  return r;
}

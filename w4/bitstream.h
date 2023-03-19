#pragma once
#include <cstring>

// ===== data streaming

struct bitstream {
  explicit bitstream(uint8_t *data): m_data(data) { }

  template<class T>
  friend bitstream& operator<<(bitstream&, const T&);
  template<class T>
  friend bitstream& operator<<(bitstream&&, const T&);

  template<class T>
  friend bitstream& operator>>(bitstream&, T&);
  template<class T>
  friend bitstream& operator>>(bitstream&&, T&);
  template<class T>
  friend bitstream& operator>>(bitstream&&, T&&);

private:
  uint8_t *m_data;
  size_t m_offset = 0;
};

// write

template<class T>
bitstream& operator<<(bitstream&& s, const T& t) { // r-value variant for in-place/implicit construction
  memcpy(s.m_data + s.m_offset, &t, sizeof(T));
  s.m_offset += sizeof(T);
  return s;
}

template<class T>
bitstream& operator<<(bitstream& s, const T& t) {
  memcpy(s.m_data + s.m_offset, &t, sizeof(T));
  s.m_offset += sizeof(T);
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
bitstream& operator>>(bitstream&& s, T& t) { // r-value variant for in-place/implicit construction
  memcpy(&t, s.m_data + s.m_offset, sizeof(T));
  s.m_offset += sizeof(T);
  return s;
}

template<class T>
bitstream& operator>>(bitstream&& s, T&&) { // for byte skipping
  s.m_offset += sizeof(T);
  return s;
}

// === size measurement

struct ruler {
  operator size_t() {
    return m_size;
  }

  template<class T>
  friend ruler& operator<<(ruler&, const T&);
  template<class T>
  friend ruler& operator<<(ruler&&, const T&);
private:
  size_t m_size;
};

template<class T>
ruler& operator<<(ruler& r, const T&) {
  r.m_size += sizeof(T);
  return r;
}

template<class T>
ruler& operator<<(ruler&& r, const T&) { // r-value variant for in-place/implicit construction
  r.m_size += sizeof(T);
  return r;
}
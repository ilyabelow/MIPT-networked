#include "time.h"

time_point steady_now() {
  return std::chrono::steady_clock::now();
}
#include <chrono>


typedef std::chrono::time_point<std::chrono::steady_clock> time_point;
typedef std::chrono::steady_clock::duration duration;
typedef std::chrono::duration<float> duration_float;

time_point steady_now();
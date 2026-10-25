/* SPDX-License-Identifier: MIT */
#ifndef TS_TEST_ARDUINO_H
#define TS_TEST_ARDUINO_H
#include <stddef.h>
#include <stdint.h>
struct MockSerial {
  void begin(int) {}
  template <typename T> void print(T) {}
  template <typename T> void print(T, int) {}
  template <typename T> void println(T) {}
  template <typename T> void println(T, int) {}
  void println() {}
  size_t write(uint8_t) { return 1u; }
  int available() { return 0; }
  int read() { return -1; }
};
extern MockSerial Serial;
inline uint32_t millis() { return 0; }
#endif
